#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "ui/BeatRuler.h"
#include "ui/AbletonLnF.h"

class DeVerbProcessor;

// UI horizontal 1280×620 estilo Ableton minimal: topbar + 5 colunas com
// knobs rotativos compactos. Nomes dos params pintados no paint().
// Lógica (attachments, Timer, flipStep) igual à versão vertical.
class DeVerbEditor : public juce::AudioProcessorEditor,
                     private juce::Timer
{
    // Harness de testes de sessão (tests/test_ui.cpp): conduz a UI como
    // um heavy user (flips FWD|REV, tweaks, presets, resizes).
    friend struct UiSessionDriver;
public:
    explicit DeVerbEditor(DeVerbProcessor&);
    ~DeVerbEditor() override;
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;
    void flipStep(int step);
    // (Re)liga os attachments de cada coluna aos params FWD ou REV.
    void bindGateColumn();
    void bindDelayColumn();
    void bindVerbColumn();
    void bindGranColumn();
    const char* gatePatId() const   { return showRev ? "rev_gate_pattern" : "fwd_gate_pattern"; }
    const char* gateStepsId() const { return showRev ? "rev_gate_steps" : "fwd_gate_steps"; }

    DeVerbProcessor& proc;
    AbletonLnF lnf;
    juce::Label gateReadout; // "GATE 07/16" na topbar
    // Modo global de edição FWD|REV (um interruptor na topbar religa tudo).
    juce::ToggleButton fwdModeBtn { "FWD" }, revModeBtn { "REV" };
    bool showRev = false;
    // Faixa REV na 2ª linha da topbar: engine + links + trims.
    juce::ComboBox revModeBox, revSourceBox, revCaptureBox;
    // Fase 6: preset global (topbar) + routing (faixa REV).
    juce::ComboBox globalPresetBox, orderBox, xfadeBox;
    // Botão RANDOM: gera um preset totalmente aleatório (topbar).
    juce::TextButton randomButton { "RANDOM" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
        orderAttach, xmodeAttach;    juce::ToggleButton revThrowButton { "THROW" };
    juce::ToggleButton linkMasterBtn { "ALL" }, linkGateBtn { "GATE" },
                       linkDelayBtn { "DLY" }, linkVerbBtn { "VRB" },
                       linkGranBtn { "GRN" };
    juce::Slider revRateSlider, revLfoSlider, revDuckSlider,
                 trimDelaySlider, trimDecaySlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
        revModeAttach, revSourceAttach, revCaptureAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
        revThrowAttach, linkMasterAttach, linkGateAttach, linkDelayAttach,
        linkVerbAttach, linkGranAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        revRateAttach, revLfoAttach, revDuckAttach, trimDelayAttach, trimDecayAttach;
    juce::Rectangle<int> revKnobs[5];
    // Fila 1 (fase 0)
    juce::Slider inputSlider, fwdMixSlider, revMixSlider, morphSlider, masterSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        inputAttach, fwdMixAttach, revMixAttach, morphAttach, masterAttach;
    // Fila 2 (fase 1: delay + tempo) + fase 7 (algos: combo + 4 minis).
    juce::Slider bpmSlider, timeSlider, fbSlider, dampSlider, dmixSlider;
    juce::ComboBox noteBox, delayAlgoBox;
    juce::Slider driveSlider, wowRateSlider, wowDepthSlider, spreadSlider;
    juce::ToggleButton freezeButton { "Freeze" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        bpmAttach, timeAttach, fbAttach, dampAttach, dmixAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
        noteAttach, delayAlgoAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        driveAttach, wowRateAttach, wowDepthAttach, spreadAttach;
    juce::Rectangle<int> delayMinis[4];
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> freezeAttach;
    // Fila 3 (fase 2: gate)
    BeatRuler ruler;
    juce::ToggleButton stepButtons[16];
    juce::ComboBox presetBox, rateBox, stepsBox, trigBox;
    juce::Slider smoothSlider, depthSlider, gmixSlider, panSlider, thrSlider;
    juce::Label bpmReadout;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
        rateAttach, stepsAttach, trigAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        smoothAttach, depthAttach, gmixAttach, panAttach, thrAttach;
    // Fila 4 (fase 3: reverb)
    juce::ComboBox algoBox, preNoteBox;
    juce::ToggleButton verbFreezeButton { "Freeze" };
    juce::Slider sizeSlider, decaySlider, dampVSlider, widthSlider, predelaySlider,
                 locutSlider, hicutSlider, vmixSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
        algoAttach, preNoteAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> verbFreezeAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        sizeAttach, decayAttach, dampVAttach, widthAttach, predelayAttach,
        locutAttach, hicutAttach, vmixAttach;
    // Fila 5 (fase 4: granular)
    juce::ComboBox grModeBox, grTrigBox, grLenBox, grTimeNoteBox;
    juce::ToggleButton grManualButton { "GRAB" }, grInterruptButton { "Interrupt" };
    juce::Label granLed;
    juce::Slider chanceSlider, thrGSlider, repeatsSlider, decayGSlider, timeMsSlider,
                 pitchSlider, fluxSlider, xfadeSlider, granMixSlider;
    // Readouts calculados (delay ms, verb decay) + bounds dos knobs.
    juce::Label delayMsReadout, verbDecayReadout;
    // Bounds dos knobs (preenchidos em resized(), lidos em paint() p/ labels).
    juce::Rectangle<int> motorKnobs[5], gateKnobs[5], delayKnobs[5],
                         verbKnobs[8], granKnobs[9];    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
        grModeAttach, grTrigAttach, grLenAttach, grTimeNoteAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
        grManualAttach, grInterruptAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        chanceAttach, thrGAttach, repeatsAttach, decayGAttach, timeMsAttach,
        pitchAttach, fluxAttach, xfadeAttach, granMixAttach;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DeVerbEditor)
};
