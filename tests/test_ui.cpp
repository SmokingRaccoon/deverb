// Testes de sessão heavy-user do deVerb (Fase 7): conduz o DeVerbEditor REAL
// como um utilizador numa sessão de trabalho a sério — FWD>REV>FWD>REV>FWD,
// tweaks com propósito, presets, resizes — e verifica coerência total:
// valores mostrados == valores guardados, sem cross-talk, DSP finito.
//
// Corre sob Xvfb (componentes precisam de MessageManager com display):
//   xvfb-run -a ./tests/build/test_ui
// Sai 0 = passa.
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

// Driver com acesso total ao editor (friend). Replica EXATAMENTE o que os
// handlers da UI fazem (mesmas chamadas, sem atalhos).
struct UiSessionDriver
{
    DeVerbEditor& ed;
    DeVerbProcessor& proc;
    // Referências públicas p/ os widgets (ligadas no ctor, onde há acesso).
    juce::Slider &fb, &decay, &master;
    juce::ComboBox &order, &xfade, &note, &algo;
    juce::ToggleButton &gatePwr;
    explicit UiSessionDriver(DeVerbEditor& e, DeVerbProcessor& p)
        : ed(e), proc(p), fb(e.fbSlider), decay(e.decaySlider), master(e.masterSlider),
          order(e.orderBox), xfade(e.xfadeBox), note(e.noteBox), algo(e.delayAlgoBox),
          gatePwr(e.gatePwrBtn) {}

    bool stepState(int i) { return ed.stepButtons[i].getToggleState(); }
    void clickRandom() { ed.randomButton.triggerClick(); pump(); }

    void pump(int ms = 120) // esvazia a message queue (sync param→widget)
    {
        if (auto* mm = juce::MessageManager::getInstanceWithoutCreating())
            mm->runDispatchLoopUntil(ms);
    }

    void flipToRev()
    {
        ed.showRev = true;
        ed.fwdModeBtn.setToggleState(false, juce::dontSendNotification);
        ed.revModeBtn.setToggleState(true, juce::dontSendNotification);
        ed.bindGateColumn();
        ed.bindDelayColumn();
        ed.bindVerbColumn();
        ed.bindGranColumn();
        pump();
    }
    void flipToFwd()
    {
        ed.showRev = false;
        ed.fwdModeBtn.setToggleState(true, juce::dontSendNotification);
        ed.revModeBtn.setToggleState(false, juce::dontSendNotification);
        ed.bindGateColumn();
        ed.bindDelayColumn();
        ed.bindVerbColumn();
        ed.bindGranColumn();
        pump();
    }

    // Valor que o knob MOSTRA (unidades reais; attachments espelham o range).
    float shown(juce::Slider& s) { return s.getValue(); }
    // Valor guardado no parâmetro (unidades reais).
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
    // Arrasta um knob (widget→param, síncrono via attachment).
    void dragKnob(juce::Slider& s, float norm01)
    {
        s.setValue(norm01, juce::sendNotificationSync);
    }
    void pickCombo(juce::ComboBox& c, int itemId)
    {
        c.setSelectedId(itemId, juce::sendNotificationSync);
    }
    void flipStep(int i) { ed.flipStep(i); pump(); } // pump longo: Timer é 30 Hz

    int comboItems(juce::ComboBox& c) { return c.getNumItems(); }
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
    juce::ScopedJuceInitialiser_GUI gui; // MessageManager p/ widgets e Timer
    DeVerbProcessor proc;
    proc.prepareToPlay(48000.0, 512);
    DeVerbEditor ed(proc);
    ed.setSize(1280, 624);
    UiSessionDriver d(ed, proc);

    // --- U1. Resize repetido não duplica items nem parte binds ---
    for (int k = 0; k < 5; ++k)
    {
        ed.setSize(800, 600);
        ed.setSize(1280, 624);
    }
    d.pump();
    CHECK(d.comboItems(d.order) == 4, "orderBox tem 4 items após resizes (%d)",
          d.comboItems(d.order));
    CHECK(d.comboItems(d.xfade) == 2, "xfadeBox tem 2 items (%d)",
          d.comboItems(d.xfade));
    CHECK(d.comboItems(d.note) == 12, "noteBox tem 12 items (%d)",
          d.comboItems(d.note));
    CHECK(d.comboItems(d.algo) == 5, "delayAlgoBox tem 5 items (%d)",
          d.comboItems(d.algo));

    // --- U2. Sessão "tail reverso do trance gate": FWD>REV>FWD>REV>FWD ---
    // FWD: pattern porta + delay 1/8D.
    d.flipToFwd();
    d.dragKnob(d.fb, 0.5f); // mexe no knob FWD...
    CHECK(std::abs(d.stored("fwd_delay_fb") - 0.5f) < 0.02f,
          "knob FWD escreve fwd_delay_fb");
    CHECK(std::abs(d.stored("rev_delay_fb") - 0.35f) < 0.02f,
          "REV intacto enquanto edita FWD (%f)", d.stored("rev_delay_fb"));
    // Vira REV: knobs mostram REV, FWD preservado.
    d.flipToRev();
    CHECK(std::abs(d.shown(d.fb) - d.stored("rev_delay_fb")) < 0.02f,
          "ao virar REV o knob mostra rev_delay_fb");
    d.dragKnob(d.fb, 0.9f);
    d.pickCombo(d.algo, 3); // PingPong no REV
    CHECK(std::abs(d.stored("rev_delay_fb") - 0.9f) < 0.02f, "knob REV escreve");
    CHECK(d.proc.apvts.getRawParameterValue("rev_delay_algo")->load() == 2.f,
          "combo REV escreve rev_delay_algo=PingPong");
    // Volta FWD: tudo intacto, sem cross-talk.
    d.flipToFwd();
    CHECK(std::abs(d.stored("fwd_delay_fb") - 0.5f) < 0.02f, "FWD intacto após ida a REV");
    CHECK(std::abs(d.shown(d.fb) - 0.5f) < 0.02f, "knob FWD mostra valor FWD");
    CHECK(d.proc.apvts.getRawParameterValue("fwd_delay_algo")->load() == 0.f,
          "algo FWD continua Digital");
    // Outra vez REV e volta (2º ciclo FWD>REV>FWD).
    d.flipToRev();
    CHECK(std::abs(d.shown(d.fb) - 0.9f) < 0.02f, "REV preservado no 2º ciclo");
    d.flipToFwd();
    d.flipToRev();
    d.flipToFwd();
    CHECK(std::abs(d.stored("fwd_delay_fb") - 0.5f) < 0.02f, "FWD intacto após 3 ciclos");
    CHECK(std::abs(d.stored("rev_delay_fb") - 0.9f) < 0.02f, "REV intacto após 3 ciclos");
    CHECK(noisyFinite(proc), "DSP finito após flips");

    // --- U3. Pattern editor nos dois alvos, sem cross-talk ---
    d.flipToFwd();
    {
        int before = (int) proc.apvts.getRawParameterValue("rev_gate_pattern")->load();
        d.flipStep(0); // mexe passo 1 do FWD
        int fwd = (int) proc.apvts.getRawParameterValue("fwd_gate_pattern")->load();
        int rev = (int) proc.apvts.getRawParameterValue("rev_gate_pattern")->load();
        CHECK(fwd == (0x1111 ^ 1), "flipStep FWD (got 0x%x)", fwd);
        CHECK(rev == before, "REV pattern intacto (0x%x)", rev);
        // Toggles refletem o alvo após pump.
        CHECK(d.stepState(0) == ((fwd & 1) != 0),
              "toggle 1 espelha FWD");
    }
    d.flipToRev();
    d.flipStep(1); // passo 2 do REV
    {
        int rev = (int) proc.apvts.getRawParameterValue("rev_gate_pattern")->load();
        int fwd = (int) proc.apvts.getRawParameterValue("fwd_gate_pattern")->load();
        CHECK(rev == (0x1111 ^ 2), "flipStep REV (got 0x%x)", rev);
        CHECK(fwd == (0x1111 ^ 1), "FWD intacto (0x%x)", fwd);
        CHECK(d.stepState(1) == ((rev & 2) != 0),
              "toggle 2 espelha REV");
    }
    d.flipToFwd();

    // --- U4. Todos os 11 presets aplicam e o DSP fica finito ---
    {
        bool allOk = true;
        for (size_t i = 0; i < deVerbFactoryPresets().size(); ++i)
        {
            applyFactoryPreset(proc.apvts, (int) i);
            d.pump(20);
            if (! noisyFinite(proc)) { allOk = false; break; }
        }
        CHECK(allOk, "11 presets aplicam sem partir o DSP");
        // UI segue o preset: master do preset Init = 0.8.
        applyFactoryPreset(proc.apvts, 0);
        d.pump();
        CHECK(std::abs(d.shown(d.master) - 0.8f) < 0.02f,
              "knob master segue preset Init (%f)", d.shown(d.master));
    }

    // --- U5. State round-trip com UI aberta e flips no meio ---
    {
        d.flipToFwd();
        d.dragKnob(d.decay, 0.7f); // verb decay FWD
        d.flipToRev();
        d.dragKnob(d.decay, 0.2f); // verb decay REV
        juce::MemoryBlock mb;
        proc.getStateInformation(mb);
        d.dragKnob(d.decay, 0.9f); // suja o REV
        d.flipToFwd();
        d.dragKnob(d.decay, 5.0f); // suja o FWD
        proc.setStateInformation(mb.getData(), (int) mb.getSize());
        d.pump();
        // Compara em unidades REAIS (raw) dos dois lados (ranges com skew!).
        float fwd = proc.apvts.getRawParameterValue("fwd_verb_decay")->load();
        float rev = proc.apvts.getRawParameterValue("rev_verb_decay")->load();
        d.flipToFwd();
        CHECK(std::abs(d.shown(d.decay) - fwd) < 0.05f,
              "restore repõe FWD e UI segue (%f vs %f)", d.shown(d.decay), fwd);
        d.flipToRev();
        CHECK(std::abs(d.shown(d.decay) - rev) < 0.05f,
              "restore repõe REV e UI segue (%f vs %f)", d.shown(d.decay), rev);
        CHECK(noisyFinite(proc), "DSP finito após restore");
    }

    // --- U6. Botão RANDOM: muda quase tudo, poupa gains, DSP finito ---
    {
        std::vector<float> before;
        for (auto* p : proc.getParameters()) before.push_back(p->getValue());
        d.clickRandom();
        int changed = 0, total = 0;
        for (size_t i = 0; i < before.size(); ++i)
        {
            ++total;
            if (std::abs(proc.getParameters()[(int) i]->getValue() - before[i]) > 0.001f)
                ++changed;
        }
        // RANDOM civilizado muda ~2/3 dos params (curated: alguns coincidem).
        CHECK(changed > total / 2, "RANDOM muda >50%% dos params (%d/%d)",
              changed, total);
        CHECK(std::abs(proc.apvts.getRawParameterValue("master")->load() - 0.8f) < 1e-6f,
              "RANDOM poupa o master");
        CHECK(std::abs(proc.apvts.getRawParameterValue("input_gain")->load() - 1.f) < 1e-6f,
              "RANDOM poupa o input");
        bool inRange = true;
        for (auto* p : proc.getParameters())
        {
            float v = p->getValue();
            if (v < 0.f || v > 1.f) { inRange = false; break; }
        }
        CHECK(inRange, "RANDOM dentro de [0,1]");
        CHECK(noisyFinite(proc), "DSP finito após RANDOM");
    }

    // --- U7. PWR segue o alvo FWD|REV sem cross-talk ---
    {
        d.flipToFwd();
        d.gatePwr.setToggleState(false, juce::sendNotificationSync);
        CHECK(d.stored("fwd_gate_on") == 0.f, "PWR desliga fwd_gate_on");
        CHECK(d.stored("rev_gate_on") == 1.f, "rev_gate_on intacto");
        d.flipToRev();
        CHECK(d.gatePwr.getToggleState() == true, "em REV o PWR mostra rev (ON)");
        d.gatePwr.setToggleState(false, juce::sendNotificationSync);
        CHECK(d.stored("rev_gate_on") == 0.f, "PWR desliga rev_gate_on");
        d.flipToFwd();
        CHECK(d.gatePwr.getToggleState() == false, "de volta a FWD mostra OFF");
        CHECK(d.stored("fwd_gate_on") == 0.f, "fwd_gate_on intacto");
    }

    if (failures == 0) std::printf("\nALL UI SESSION TESTS PASSED\n");
    else std::printf("\n%d FAILURES\n", failures);
    return failures == 0 ? 0 : 1;
}
