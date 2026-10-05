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
    void doubleClickDial(const char* id)
    {
        if (auto* d = ed.findDial(id))
        {
            juce::MouseEvent e(juce::Desktop::getInstance().getMainMouseSource(),
                               juce::Point<float>(32.f, 32.f), juce::ModifierKeys(),
                               0.f, 0.f, 0.f, 0.f, 0.f, d, d,
                               juce::Time::getCurrentTime(), juce::Point<float>(32.f, 32.f),
                               juce::Time::getCurrentTime(), 2, false);
            d->mouseDoubleClick(e);
            pump(30);
        }
    }
    void singleClickDialBox(const char* id)
    {
        // Um clique só: após ~250 ms abre o editor (sem mudar o valor).
        if (auto* vd = dynamic_cast<V4Dial*>(ed.findDial(id)))
            for (int c = 0; c < vd->getNumChildComponents(); ++c)
                if (auto* l = dynamic_cast<juce::Label*>(vd->getChildComponent(c)))
                {
                    auto mm = juce::Desktop::getInstance().getMainMouseSource();
                    auto now = juce::Time::getCurrentTime();
                    juce::MouseEvent e1(mm, juce::Point<float>(28.f, 7.f), juce::ModifierKeys(),
                                        0.f, 0.f, 0.f, 0.f, 0.f, l, l, now,
                                        juce::Point<float>(28.f, 7.f), now, 1, false);
                    vd->mouseDown(e1);
                    vd->mouseUp(e1);
                    pump(400);
                    dblBoxEditor_ = (l->getCurrentTextEditor() != nullptr);
                    l->hideEditor(true);
                    break;
                }
    }
    bool dblBoxEditor_ = false;
    void doubleClickDialBox(const char* id)
    {
        // Sequência real na caixa: down, up, double-click (listener do dial,
        // chamado via V4Dial onde é público).
        if (auto* vd = dynamic_cast<V4Dial*>(ed.findDial(id)))
            for (int c = 0; c < vd->getNumChildComponents(); ++c)
                if (auto* l = dynamic_cast<juce::Label*>(vd->getChildComponent(c)))
                {
                    auto mm = juce::Desktop::getInstance().getMainMouseSource();
                    auto now = juce::Time::getCurrentTime();
                    juce::MouseEvent e1(mm, juce::Point<float>(28.f, 7.f), juce::ModifierKeys(),
                                        0.f, 0.f, 0.f, 0.f, 0.f, l, l, now,
                                        juce::Point<float>(28.f, 7.f), now, 1, false);
                    vd->mouseDown(e1);
                    vd->mouseUp(e1);
                    vd->mouseDoubleClick(e1);
                    pump(30);
                    dblBoxNoEditor_ = (l->getCurrentTextEditor() == nullptr);
                    break;
                }
    }
    bool dblBoxNoEditor_ = false;
    void clickRandom()
    {
        if (auto* k = ed.findKey("link_master")) { juce::ignoreUnused(k); }
        ed.randomize();
        pump();
    }
    void pressRandom()
    {
        // Via tecla Action (caminho real do utilizador, não randomize() direto).
        // randomKey é privado mas o driver é friend: acesso direto.
        if (ed.randomKey != nullptr) ed.randomKey->press();
        pump();
    }
    float bpmAlpha() { return ed.bpmNum != nullptr ? ed.bpmNum->getAlpha() : -1.f; }
    void clickSyncSeg(int i) // 0=HOST, 1=MANUAL, via clique real no segmento
    {
        if (ed.syncSeg == nullptr) return;
        juce::TextButton* pick = nullptr;
        for (int c = 0; c < ed.syncSeg->getNumChildComponents(); ++c)
            if (auto* b = dynamic_cast<juce::TextButton*>(ed.syncSeg->getChildComponent(c)))
                if (pick == nullptr || (i == 0) == (b->getX() < pick->getX())) pick = b;
        if (pick != nullptr) pick->triggerClick();
        pump();
    }
    void runAudio(int blocks = 4)
    {
        juce::MidiBuffer midi;
        juce::AudioBuffer<float> buf(2, 512);
        for (int b = 0; b < blocks; ++b)
        {
            buf.clear();
            proc.processBlock(buf, midi);
        }
        pump(60);
    }
    juce::String grText()
    {
        for (int c = 0; c < ed.getNumChildComponents(); ++c)
            if (auto* v = dynamic_cast<V4Viz*>(ed.getChildComponent(c)))
                if (v->getText().startsWith("GR ")) return v->getText();
        return {};
    }
    void flipStepFwd(int i) { ed.panels[0][0]->flipStep(i); pump(); }
    void flipStepRev(int i) { ed.panels[1][0]->flipStep(i); pump(); }
    V4Viz* granLedFwd() { return ed.panels[0][3]->findViz(V4Viz::GranLed); }
    void pressThrow()
    {
        if (auto* k = ed.findKey("rev_throw")) k->press();
    }
    // PROBE temporário: aplica presets via stepper e lê valores reais
    void probePreset(int i)
    {
        if (ed.presetStepper != nullptr) ed.presetStepper->applyIndex(i);
        pump(60);
    }
    void clickPresetArrow(bool next)
    {
        if (ed.presetStepper == nullptr) return;
        // setas são os 2 filhos TextButton (0=prev, 1=next)
        for (int c = 0; c < ed.presetStepper->getNumChildComponents(); ++c)
            if (auto* b = dynamic_cast<juce::TextButton*>(
                    ed.presetStepper->getChildComponent(c)))
            {
                bool isNext = b->getX() > 100;
                if (isNext == next) { b->triggerClick(); break; }
            }
        pump(60);
    }
    int presetIndex()
    {
        return ed.presetStepper != nullptr ? ed.presetStepper->getIndex() : -9;
    }
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
    // Sem buffer: se houver segfault, o log mostra exatamente onde parou.
    setvbuf(stdout, nullptr, _IONBF, 0);
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

    // U5b. selMod sobrevive ao save/load (fora dos 124 IDs)
    {
        d.selectMod("delay");
        juce::MemoryBlock mb;
        proc.getStateInformation(mb);
        d.selectMod("gran");
        CHECK(ed.getSelMod() == "gran", "selMod muda para gran");
        proc.setStateInformation(mb.getData(), (int) mb.getSize());
        d.pump();
        CHECK(proc.getSelMod() == "delay", "restore repõe selMod delay");
        CHECK(ed.getSelMod() == "delay", "editor segue selMod do restore");
    }

    // U5c. syncMode persiste e o seletor segue (HOST/MAN, fora dos params)
    {
        proc.setSyncMode("man");
        d.pump();
        CHECK(proc.getSyncMode() == "man", "syncMode man guardado");
        juce::MemoryBlock mb;
        proc.getStateInformation(mb);
        proc.setSyncMode("host");
        proc.setStateInformation(mb.getData(), (int) mb.getSize());
        d.pump();
        CHECK(proc.getSyncMode() == "man", "restore repõe syncMode man");
        proc.setSyncMode("host");
    }

    // U6. RANDOM via tecla Action (caminho do utilizador)
    {
        std::vector<float> before;
        for (auto* p : proc.getParameters()) before.push_back(p->getValue());
        d.pressRandom();
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

    // U8. THROW: press() segura o botão 60 ms (o flanco do DSP lê por bloco;
    // o 1->0 imediato antigo morria antes do próximo bloco e nunca disparava)
    {
        d.pressThrow();
        CHECK(d.stored("rev_throw") == 1.f, "THROW press segura a 1");
        d.pump(150);
        CHECK(d.stored("rev_throw") == 0.f, "THROW larga após o hold");
    }

    // C2. Sync: HOST com dados esbate o BPM; MANUAL (clicado) usa o knob.
    {
        struct FakePh : juce::AudioPlayHead
        {
            juce::Optional<juce::AudioPlayHead::PositionInfo> getPosition() const override
            {
                juce::AudioPlayHead::PositionInfo p;
                p.setBpm(128.0);
                p.setIsPlaying(true);
                p.setPpqPosition(4.0);
                return p;
            }
        } ph;
        CHECK(std::abs(d.bpmAlpha() - 1.f) < 0.01f, "BPM aceso sem host");
        proc.setPlayHead(&ph);
        d.runAudio();
        CHECK(proc.isTempoFromHost(), "host fornece tempo");
        CHECK(std::abs(proc.getUiBpm() - 128.f) < 0.5f, "BPM segue a DAW (%f)", proc.getUiBpm());
        d.pump(120);
        CHECK(std::abs(d.bpmAlpha() - 0.5f) < 0.01f, "BPM esbate com host");
        d.clickSyncSeg(1); // MANUAL clicado a sério
        CHECK(proc.getSyncMode() == "man", "seletor põe MAN");
        d.runAudio();
        CHECK(! proc.isTempoFromHost(), "MAN ignora o host");
        d.pump(120);
        CHECK(std::abs(d.bpmAlpha() - 1.f) < 0.01f, "BPM acende em MAN");
        d.clickSyncSeg(0); // de volta a HOST
        d.runAudio();
        d.pump(120);
        CHECK(std::abs(d.bpmAlpha() - 0.5f) < 0.01f, "BPM volta a esbater em HOST");
        proc.setPlayHead(nullptr);
    }

    // C3. Granular: TIME mostra o locked + dims por modo + LED com hold.
    {
        // T-NOTE 1/4 (idx 8): TIME esbate e a caixa mostra 500 ms @120
        // (o knob de ms continua nos 250: é ele que não conta).
        d.setParam("gr_time_note", 8.f / 11.f);
        d.pump(120);
        auto* timeDial = ed.findDial("gr_time");
        CHECK(timeDial != nullptr, "dial TIME existe");
        if (timeDial != nullptr)
        {
            CHECK(std::abs(timeDial->getAlpha() - 0.38f) < 0.02f, "TIME esbate com nota");
            // 1/4 = 1 beat no BPM efetivo (o RANDOM anterior pode o ter mudado).
            juce::String want = juce::String((int) std::round(60000.0 / proc.getUiBpm())) + " ms";
            juce::String got = timeDial->getTextFromValue(timeDial->getValue());
            CHECK(got == want, "TIME mostra locked (got %s, want %s)",
                  got.toRawUTF8(), want.toRawUTF8());
        }
        // DECAY só conta em BeatRepeat/Stutter: em Slice esbate.
        d.setParam("gr_mode", 2.f / 5.f); // Slice
        d.pump(120);
        auto* decDial = ed.findDial("gr_decay");
        CHECK(decDial != nullptr && std::abs(decDial->getAlpha() - 0.38f) < 0.02f,
              "DECAY esbate em Slice");
        // LED: grab manual -> GRAB!, depois apaga com o hold.
        // (O DSP tem de correr: o flag vem do processBlock.)
        d.setParam("gr_trigger", 1.f); // Manual (idx 2/2)
        d.setParam("gr_manual", 1.f);
        noisyFinite(proc); // o flanco dispara no 1º bloco; ainda a tocar
        d.setParam("gr_manual", 0.f);
        d.pump(120);
        auto* led = d.granLedFwd();
        CHECK(led != nullptr && led->getText() == "GRAB!" && led->isHot(), "LED acende no grab");
        d.setParam("gr_mode", 0.f); // Off: larga e o hold expira
        noisyFinite(proc);
        d.pump(600);
        CHECK(led != nullptr && led->getText() == "IDLE" && ! led->isHot(), "LED apaga após hold");
        // Repor Free: TIME volta a contar.
        d.setParam("gr_time_note", 0.f);
        d.pump(120);
        CHECK(timeDial != nullptr && std::abs(timeDial->getAlpha() - 1.f) < 0.02f,
              "TIME acende em Free");
    }

    // U9. Preset stepper: setas aplicam mesmo (nomes + valores + wrap).
    // Os presets são deltas sobre base neutra: navegar nunca deixa motores
    // presos (era o "seletor não escolhia" — o nome mudava, o som ficava).
    {
        d.probePreset(9); // Shimmer Pad: algo 3, mix 0.5
        CHECK(d.presetIndex() == 9, "preset idx 9");
        CHECK(d.stored("fwd_verb_algo") == 3.f, "Shimmer Pad aplica algo");
        CHECK(std::abs(d.stored("fwd_verb_mix") - 0.5f) < 0.01f, "Shimmer Pad aplica mix");
        d.clickPresetArrow(true); // 9 -> 10 Build Up (decay 4)
        CHECK(d.presetIndex() == 10, "seta next avança");
        CHECK(std::abs(d.stored("fwd_verb_decay") - 4.f) < 0.05f, "Build Up aplica decay");
        d.clickPresetArrow(true); // 10 -> 0 wrap (Init: mixes 0)
        CHECK(d.presetIndex() == 0, "seta next faz wrap");
        CHECK(d.stored("fwd_gate_mix") == 0.f, "Init aplica gate_mix 0");
        d.clickPresetArrow(false); // 0 -> 10
        CHECK(d.presetIndex() == 10, "seta prev recua com wrap");
        CHECK(noisyFinite(proc), "DSP finito após navegar presets");
    }

    // F2. Nome do preset sobrevive ao restore (lastPreset em v4ui).
    {
        d.probePreset(3);
        juce::MemoryBlock mb;
        proc.getStateInformation(mb);
        d.probePreset(9);
        CHECK(d.presetIndex() == 9, "navega para 9");
        proc.setStateInformation(mb.getData(), (int) mb.getSize());
        d.pump(120);
        CHECK(d.presetIndex() == 3, "restore repõe nome do preset");
        d.pressRandom();
        int lp = (int) proc.apvts.state.getChildWithName("v4ui").getProperty("lastPreset", -2);
        CHECK(lp == -1, "RANDOM marca lastPreset -1 (Custom)");
    }

    // U10. OUT: knobs SIZE/WIDTH escrevem e mostram.
    {
        d.dragDial("dim_size", 0.7f);
        CHECK(std::abs(d.stored("dim_size") - 0.7f) < 0.02f, "SIZE escreve dim_size");
        CHECK(std::abs(d.shownDial("dim_size") - 0.7f) < 0.02f, "SIZE mostra SIZE");
        d.dragDial("dim_mix", 0.5f);
        CHECK(std::abs(d.stored("dim_mix") - 0.5f) < 0.02f, "WIDTH escreve dim_mix");
        CHECK(noisyFinite(proc), "DSP finito com widener");
    }

    // F5. Espaço passa ao host; só Enter ativa a tecla focada.
    {
        auto* k = ed.findKey("link_master");
        CHECK(k != nullptr, "tecla link_master existe");
        if (k != nullptr)
        {
            bool before = k->getState();
            CHECK(! k->keyPressed(juce::KeyPress(juce::KeyPress::spaceKey)),
                  "espaço não é consumido");
            CHECK(k->getState() == before, "espaço não alterna");
            CHECK(k->keyPressed(juce::KeyPress(juce::KeyPress::returnKey)),
                  "enter ativa");
            CHECK(k->getState() != before, "enter alterna");
            k->keyPressed(juce::KeyPress(juce::KeyPress::returnKey)); // repõe
            CHECK(k->getState() == before, "enter repõe");
        }
    }

    // U11. GR mostra a redução do limiter (não fica preso em 0).
    {
        d.setParam("input_gain", 1.f); // 2.0, quente
        d.setParam("fwd_verb_algo", 1.f / 3.f); // Hall
        d.setParam("fwd_verb_decay", 8.f);
        d.setParam("fwd_verb_size", 0.8f);
        d.setParam("fwd_verb_mix", 1.f); // cauda por cima: limita a sério
        d.setParam("fwd_delay_mix", 0.5f);
        juce::MidiBuffer midi;
        juce::AudioBuffer<float> buf(2, 512);
        for (int b = 0; b < 60; ++b)
        {
            for (int i = 0; i < 512; ++i)
            {
                float v = std::sin((b * 512 + i) * 0.05f) * 0.9f;
                buf.setSample(0, i, v);
                buf.setSample(1, i, v);
            }
            proc.processBlock(buf, midi);
        }
        d.pump(120);
        juce::String gr = d.grText();
        CHECK(gr.startsWith("GR ") && gr != "GR 0.0", "GR mostra redução (%s)",
              gr.toRawUTF8());
        // Calmo: cala módulos (as caudas quentes limitariam para sempre)
        // e baixa o input; o hold esgota e o GR volta a 0.
        d.setParam("input_gain", 0.025f); // ~0.05
        d.setParam("fwd_verb_mix", 0.f);
        d.setParam("fwd_delay_mix", 0.f);
        d.setParam("gr_mode", 0.f);
        d.setParam("rev_mode", 0.f);
        d.runAudio(500); // ~5 s: o hold do GR esgota e volta a 0
        juce::String gr2 = d.grText();
        CHECK(gr2 == "GR 0.0", "GR volta a 0 (%s)", gr2.toRawUTF8());
    }

    // U12. Duplo clique repõe o default (todos os knobs).
    {
        d.dragDial("fwd_verb_decay", 10.f);
        d.doubleClickDial("fwd_verb_decay");
        CHECK(std::abs(d.stored("fwd_verb_decay") - 2.5f) < 0.05f, "dblclick repõe decay (%f)",
              d.stored("fwd_verb_decay"));
        d.dragDial("master", 0.2f);
        d.doubleClickDial("master");
        CHECK(std::abs(d.stored("master") - 0.8f) < 0.02f, "dblclick repõe master (%f)",
              d.stored("master"));
        d.dragDial("rev_delay_fb", 0.9f);
        d.doubleClickDial("rev_delay_fb");
        CHECK(std::abs(d.stored("rev_delay_fb") - 0.35f) < 0.02f, "dblclick repõe REV (%f)",
              d.stored("rev_delay_fb"));
        d.dragDial("fwd_gate_smooth", 0.9f);
        d.singleClickDialBox("fwd_gate_smooth");
        CHECK(d.dblBoxEditor_, "1 clique na caixa abre o editor");
        d.doubleClickDialBox("fwd_gate_smooth");
        CHECK(d.dblBoxNoEditor_, "caixa: sem editor após duplo clique");
        CHECK(std::abs(d.stored("fwd_gate_smooth") - 0.15f) < 0.02f, "dblclick na caixa repõe (%f)",
              d.stored("fwd_gate_smooth"));
    }

    if (failures == 0) std::printf("\nALL UI SESSION TESTS PASSED\n");
    else std::printf("\n%d FAILURES\n", failures);
    return failures == 0 ? 0 : 1;
}
