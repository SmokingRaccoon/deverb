// Testes de sessão heavy-user v4 (Espelho de Agua): conduz o DeVerbEditor REAL.
// 8 ModulePanels com attachments ctor-once (sem bind*Column): FWD e REV
// visíveis em simultâneo por módulo selecionado; sem cross-talk.
// Corre sob Xvfb: xvfb-run -a ./tests/build/test_ui
#include <cstdio>
#include <cmath>
#include <vector>
#include "../Source/PluginEditor.h"
#include "../Source/PluginProcessor.h"
#include "../Source/core/FactoryPresets.h"
#include "../Source/dsp/Gater.h"

static int failures = 0;
#define CHECK(cond, ...) do { \
    if (!(cond)) { ++failures; std::printf("FAIL: " __VA_ARGS__); std::printf("\n"); } \
    else { std::printf("ok: " __VA_ARGS__); std::printf("\n"); } \
} while (0)

struct UiSessionDriver
{
    DeVerbEditor& ed;
    DeVerbProcessor& proc;
    explicit UiSessionDriver(DeVerbEditor& e, DeVerbProcessor& p) : ed(e), proc(p) {}

    void pump(int ms = 120)
    {
        if (auto* mm = juce::MessageManager::getInstanceWithoutCreating())
            mm->runDispatchLoopUntil(ms);
    }

    void selectMod(const juce::String& m) { ed.setSelMod(m, false); pump(); }

    float stored(const char* id)
    {
        if (auto* p = proc.apvts.getParameter(id))
            return proc.apvts.getRawParameterValue(id)->load();
        return -1.f;
    }
    void setParam(const char* id, float norm01)
    {
        if (auto* p = proc.apvts.getParameter(id))
            p->setValueNotifyingHost(norm01);
        pump(30);
    }
    float shownDial(const char* id)
    {
        if (auto* d = ed.findDial(id)) return d->getValue();
        return -999.f;
    }
    void dragDial(const char* id, float realVal)
    {
        if (auto* d = ed.findDial(id)) d->setValue(realVal, juce::sendNotificationSync);
        pump(30);
    }
    void clickRandom()
    {
        if (auto* k = ed.findKey("link_master")) { juce::ignoreUnused(k); }
        ed.randomize();
        pump();
    }
    void flipStepFwd(int i) { ed.panels[0][0]->flipStep(i); pump(); }
    void flipStepRev(int i) { ed.panels[1][0]->flipStep(i); pump(); }
    bool panelVisible(int e, int m) { return ed.panels[e][m]->isVisible(); }
};

static bool noisyFinite(DeVerbProcessor& p)
{
    juce::MidiBuffer midi;
    juce::AudioBuffer<float> buf(2, 512);
    for (int b = 0; b < 30; ++b)
    {
        for (int i = 0; i < 512; ++i)
        {
            float v = std::sin((b * 512 + i) * 0.03f) * 0.5f;
            buf.setSample(0, i, v);
            buf.setSample(1, i, v);
        }
        p.processBlock(buf, midi);
        for (int i = 0; i < 512; ++i)
            if (! std::isfinite(buf.getSample(0, i)) || std::abs(buf.getSample(0, i)) > 10.f)
                return false;
    }
    return true;
}

int main()
{
    juce::ScopedJuceInitialiser_GUI gui;
    DeVerbProcessor proc;
    proc.prepareToPlay(48000.0, 512);
    DeVerbEditor ed(proc);
    ed.setSize(1280, 624);
    UiSessionDriver d(ed, proc);

    // U1. Janela fixa + resize idempotente + 8 painéis + selMod persistido
    CHECK(ed.getWidth() == 1280 && ed.getHeight() == 624, "janela 1280x624");
    for (int k = 0; k < 3; ++k) { ed.setSize(800, 600); ed.setSize(1280, 624); }
    d.pump();
    CHECK(ed.getWidth() == 1280 && ed.getHeight() == 624, "resize volta a 1280x624");
    d.selectMod("verb");
    CHECK(ed.getSelMod() == "verb", "selMod verb");
    CHECK(proc.getSelMod() == "verb", "selMod persiste no ValueTree");
    d.selectMod("gate");
    CHECK(d.panelVisible(0, 0) && d.panelVisible(1, 0), "gate FWD+REV visíveis");
    CHECK(! d.panelVisible(0, 1), "delay escondido com gate selecionado");

    // U2. FWD/REV independentes com attachments ctor-once (sem flips de bind)
    d.dragDial("fwd_delay_fb", 0.5f);
    CHECK(std::abs(d.stored("fwd_delay_fb") - 0.5f) < 0.02f, "dial FWD escreve fwd_delay_fb");
    CHECK(std::abs(d.stored("rev_delay_fb") - 0.35f) < 0.02f, "REV intacto (%f)", d.stored("rev_delay_fb"));
    d.dragDial("rev_delay_fb", 0.9f);
    CHECK(std::abs(d.stored("rev_delay_fb") - 0.9f) < 0.02f, "dial REV escreve");
    CHECK(std::abs(d.stored("fwd_delay_fb") - 0.5f) < 0.02f, "FWD intacto após REV");
    CHECK(std::abs(d.shownDial("fwd_delay_fb") - 0.5f) < 0.02f, "FWD mostra FWD");
    CHECK(std::abs(d.shownDial("rev_delay_fb") - 0.9f) < 0.02f, "REV mostra REV");
    CHECK(noisyFinite(proc), "DSP finito após tweaks");

    // U3. Steps nos dois motores, sem cross-talk (tether: REV ligado segue FWD)
    {
        int beforeRev = (int)d.stored("rev_gate_pattern");
        d.flipStepFwd(0);
        CHECK((int)d.stored("fwd_gate_pattern") == (0x1111 ^ 1), "flipStep FWD");
        CHECK((int)d.stored("rev_gate_pattern") == beforeRev, "REV intacto");
        // Com LINK GATE ON (default), o passo REV está amarrado: não escreve
        d.flipStepRev(1);
        CHECK((int)d.stored("rev_gate_pattern") == beforeRev, "REV tethered não escreve com link");
        // Desliga o link e aí escreve
        d.setParam("link_gate", 0.f);
        d.flipStepRev(1);
        CHECK((int)d.stored("rev_gate_pattern") == (beforeRev ^ 2), "flipStep REV após UNLINK");
        d.setParam("link_gate", 1.f);
    }

    // U4. Presets + master segue
    {
        bool allOk = true;
        for (size_t i = 0; i < deVerbFactoryPresets().size(); ++i)
        {
            applyFactoryPreset(proc.apvts, (int)i);
            d.pump(20);
            if (! noisyFinite(proc)) { allOk = false; break; }
        }
        CHECK(allOk, "presets aplicam sem partir DSP");
        applyFactoryPreset(proc.apvts, 0);
        d.pump();
        CHECK(std::abs(d.shownDial("master") - 0.8f) < 0.02f, "master segue Init");
    }

    // U5. Round-trip com UI aberta
    {
        d.dragDial("fwd_verb_decay", 5.0f);
        d.dragDial("rev_verb_decay", 6.0f);
        juce::MemoryBlock mb;
        proc.getStateInformation(mb);
        d.dragDial("fwd_verb_decay", 10.0f);
        d.dragDial("rev_verb_decay", 10.0f);
        proc.setStateInformation(mb.getData(), (int)mb.getSize());
        d.pump();
        CHECK(std::abs(d.stored("fwd_verb_decay") - 5.0f) < 0.05f, "restore FWD");
        CHECK(std::abs(d.stored("rev_verb_decay") - 6.0f) < 0.05f, "restore REV");
        CHECK(noisyFinite(proc), "DSP finito após restore");
    }

    // U6. RANDOM
    {
        std::vector<float> before;
        for (auto* p : proc.getParameters()) before.push_back(p->getValue());
        d.clickRandom();
        int changed = 0;
        for (size_t i = 0; i < before.size(); ++i)
            if (std::abs(proc.getParameters()[(int)i]->getValue() - before[i]) > 0.001f) ++changed;
        CHECK(changed > (int)before.size() / 2, "RANDOM muda >50%% (%d)", changed);
        CHECK(noisyFinite(proc), "DSP finito após RANDOM");
    }

    // U7. PWR por motor (fonte única nas pedras)
    {
        if (auto* k = ed.findKey("fwd_gate_on")) k->setState(false);
        d.pump();
        CHECK(d.stored("fwd_gate_on") == 0.f, "PWR desliga FWD");
        CHECK(d.stored("rev_gate_on") == 1.f, "REV intacto");
        if (auto* k = ed.findKey("rev_gate_on")) k->setState(false);
        d.pump();
        CHECK(d.stored("rev_gate_on") == 0.f, "PWR desliga REV");
    }

    if (failures == 0) std::printf("\nALL UI SESSION TESTS PASSED\n");
    else std::printf("\n%d FAILURES\n", failures);
    return failures == 0 ? 0 : 1;
}
