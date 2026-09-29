#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "core/TempoInfo.h"
#include "core/FactoryPresets.h"
#include "dsp/Gater.h"
#include <iterator>

// UI v2 horizontal 1280×624: topbar (título+preset+monitores / faixa REV) +
// 5 panels (MOTOR perform / GATE / DELAY / VERB / GRAN) com knobs BIG/STD/MINI.
// Grelha 8px, labels pintados, acento por módulo. Lógica igual à v1.

namespace {
constexpr int kW = 1280, kH = 624, kTopH = 100, kPad = 8;
constexpr int kColY = kTopH + kPad;          // 108
constexpr int kColBottom = kH - kPad;        // 616
constexpr int kHeadH = 24;

const char* kMotorLabels[5] = { "INPUT", "FWD MIX", "REV MIX", "MORPH", "MASTER" };
const char* kGateLabels[5]  = { "SMOOTH", "DEPTH", "MIX", "PAN", "THR" };
const char* kDelayLabels[5] = { "BPM", "TIME", "FB", "DAMP", "MIX" };
const char* kVerbLabels[8]  = { "SIZE", "DECAY", "DAMP", "WIDTH", "PRE-DLY", "LO-CUT", "HI-CUT", "MIX" };
const char* kGranLabels[9]  = { "CHANCE", "THR", "REPEATS", "DECAY", "TIME", "PITCH", "FLUX", "XFADE", "MIX" };
// (Nomes aplicados aos sliders via setName; o LnF desenha-os colados ao knob.
//  paintKnobLabels foi removido: duas fontes de labels = desalinho garantido.)

// Panels e headers das 5 colunas (x, w, nome, acento, tint).
struct ColDef { int x, w; const char* name; juce::Colour accent, panel; };
inline const ColDef* colDefs()
{
    using C = AbletonLnF;
    static const ColDef defs[5] = {
        { 8, 184, "MOTOR - PERFORM", C::motorAccent, C::motorPanel },
        { 200, 296, "GATE - TRANCE", C::gateAccent, C::gatePanel },
        { 504, 232, "DELAY - ECHO", C::delayAccent, C::delayPanel },
        { 744, 256, "VERB - SPACE", C::verbAccent, C::verbPanel },
        { 1008, 264, "GRAN - GLITCH", C::granAccent, C::granPanel },
    };
    return defs;
}
} // namespace

DeVerbEditor::DeVerbEditor(DeVerbProcessor& p)
    : AudioProcessorEditor(p), proc(p)
{
    setLookAndFeel(&lnf);
    setSize(kW, kH);

    for (auto* s : { &inputSlider, &fwdMixSlider, &revMixSlider, &morphSlider, &masterSlider,
                     &bpmSlider, &timeSlider, &fbSlider, &dampSlider, &dmixSlider,
                     &smoothSlider, &depthSlider, &gmixSlider, &panSlider, &thrSlider,
                     &sizeSlider, &decaySlider, &dampVSlider, &widthSlider, &predelaySlider,
                     &locutSlider, &hicutSlider, &vmixSlider,
                     &chanceSlider, &thrGSlider, &repeatsSlider, &decayGSlider, &timeMsSlider,
                     &pitchSlider, &fluxSlider, &xfadeSlider, &granMixSlider })
    {
        s->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 56, 18);
        addAndMakeVisible(s);
    }
    // Acento por módulo nos knobs.
    for (auto* s : { &inputSlider, &fwdMixSlider, &revMixSlider, &morphSlider, &masterSlider })
        s->setColour(juce::Slider::rotarySliderFillColourId, AbletonLnF::motorAccent);
    for (auto* s : { &smoothSlider, &depthSlider, &gmixSlider, &panSlider, &thrSlider })
        s->setColour(juce::Slider::rotarySliderFillColourId, AbletonLnF::gateAccent);
    for (auto* s : { &bpmSlider, &timeSlider, &fbSlider, &dampSlider, &dmixSlider })
        s->setColour(juce::Slider::rotarySliderFillColourId, AbletonLnF::delayAccent);
    for (auto* s : { &sizeSlider, &decaySlider, &dampVSlider, &widthSlider, &predelaySlider,
                     &locutSlider, &hicutSlider, &vmixSlider })
        s->setColour(juce::Slider::rotarySliderFillColourId, AbletonLnF::verbAccent);
    for (auto* s : { &chanceSlider, &thrGSlider, &repeatsSlider, &decayGSlider, &timeMsSlider,
                     &pitchSlider, &fluxSlider, &xfadeSlider, &granMixSlider })
        s->setColour(juce::Slider::rotarySliderFillColourId, AbletonLnF::granAccent);
    for (auto* s : { &revRateSlider, &revLfoSlider, &revDuckSlider,
                     &trimDelaySlider, &trimDecaySlider })
    {
        s->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 48, 16);
        s->setColour(juce::Slider::rotarySliderFillColourId, AbletonLnF::revAccent);
        addAndMakeVisible(s);
    }

    inputAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, "input_gain", inputSlider);
    fwdMixAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, "fwd_mix", fwdMixSlider);
    revMixAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, "rev_mix", revMixSlider);
    morphAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, "morph", morphSlider);
    masterAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, "master", masterSlider);

    bpmAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, "tempo_bpm", bpmSlider);
    // time/fb/damp/dmix/note/freeze vivem em bindDelayColumn() (FWD|REV).
    noteBox.addItemList(TempoInfo::noteNames(), 1);
    noteBox.setTooltip("Delay note (Free = ms livres)");
    addAndMakeVisible(noteBox);
    delayAlgoBox.addItemList({ "Digital", "Tape", "PingPong", "MultiTap", "Reverse" }, 1);
    delayAlgoBox.setTooltip("Algoritmo do delay");
    addAndMakeVisible(delayAlgoBox);
    for (auto* s : { &driveSlider, &wowRateSlider, &wowDepthSlider, &spreadSlider })
    {
        s->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 56, 16);
        s->setColour(juce::Slider::rotarySliderFillColourId, AbletonLnF::delayAccent);
        addAndMakeVisible(s);
    }
    driveSlider.setTooltip("Saturação tape");
    wowRateSlider.setTooltip("Wow rate (tape)");
    wowDepthSlider.setTooltip("Wow depth em ms (tape)");
    spreadSlider.setTooltip("Cruzamento stereo (pingpong)");
    freezeButton.setTooltip("Congela o buffer do delay");
    addAndMakeVisible(freezeButton);
    bindDelayColumn();

    // --- Gate ---
    addAndMakeVisible(ruler);
    for (int i = 0; i < 16; ++i)
    {
        stepButtons[i].setButtonText(juce::String(i + 1));
        stepButtons[i].setClickingTogglesState(true);
        stepButtons[i].onClick = [this, i] { flipStep(i); };
        addAndMakeVisible(stepButtons[i]);
    }

    for (auto& fp : Gater::factoryPatterns)
        presetBox.addItem(fp.name, (int) (&fp - Gater::factoryPatterns) + 1);
    presetBox.setTextWhenNothingSelected("Pattern...");
    presetBox.setTooltip("Pattern de fábrica → escreve no pattern");
    presetBox.onChange = [this]
    {
        int id = presetBox.getSelectedId();
        if (id >= 1 && id <= (int) std::size(Gater::factoryPatterns))
        {
            uint16_t bits = Gater::factoryPatterns[id - 1].bits;
            if (auto* par = proc.apvts.getParameter(gatePatId()))
                par->setValueNotifyingHost(par->convertTo0to1((float) bits));
        }
    };
    addAndMakeVisible(presetBox);

    rateBox.addItemList(TempoInfo::noteNames(), 1);
    rateBox.setTooltip("Gate rate");
    addAndMakeVisible(rateBox);
    stepsBox.addItemList({ "8", "16" }, 1);
    stepsBox.setTooltip("Passos do pattern");
    addAndMakeVisible(stepsBox);
    trigBox.addItemList({ "Host", "Midi", "Transient", "Free" }, 1);
    trigBox.setTooltip("Trigger do restart do pattern");
    addAndMakeVisible(trigBox);
    // rate/steps/trig/smooth/depth/mix/pan/thr vivem em bindGateColumn().

    bpmReadout.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(bpmReadout);
    gateReadout.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(gateReadout);
    delayMsReadout.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(delayMsReadout);
    verbDecayReadout.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(verbDecayReadout);

    // --- Fase 6: preset global na topbar (só UI: escreve nos params) ---
    {
        int id = 1;
        for (auto& pr : deVerbFactoryPresets())
            globalPresetBox.addItem(pr.name, id++);
    }
    globalPresetBox.setTextWhenNothingSelected("Preset...");
    globalPresetBox.setTooltip("Preset de fábrica (escreve nos parâmetros)");
    globalPresetBox.onChange = [this]
    {
        int id = globalPresetBox.getSelectedId();
        if (id >= 1)
            applyFactoryPreset(proc.apvts, id - 1);
    };
    addAndMakeVisible(globalPresetBox);

    // --- Botão RANDOM: preset totalmente aleatório, sempre divertido ---
    // Randomiza TUDO menos o gain staging (master/input fixos para nunca
    // dar silêncio por acidente). Usa o RNG do sistema (não determinístico).
    randomButton.setTooltip("Gera um preset totalmente aleatório");
    randomButton.onClick = [this]
    {
        auto* masterPar = proc.apvts.getParameter("master");
        auto* inputPar = proc.apvts.getParameter("input_gain");
        juce::Random rng;
        for (auto* par : proc.getParameters())
        {
            if (par == masterPar)
                par->setValueNotifyingHost(0.8f);
            else if (par == inputPar)
                par->setValueNotifyingHost(0.5f); // range 0..2 → 1.0
            else
                par->setValueNotifyingHost(rng.nextFloat());
        }
    };
    addAndMakeVisible(randomButton);

    // --- Reverb ---
    algoBox.addItemList({ "Room", "Hall", "Plate", "Shimmer" }, 1);
    algoBox.setTooltip("Algoritmo de reverb");
    addAndMakeVisible(algoBox);
    preNoteBox.addItemList(TempoInfo::noteNames(), 1);
    preNoteBox.setTooltip("Pre-delay em nota (Free = ms)");
    addAndMakeVisible(preNoteBox);
    verbFreezeButton.setTooltip("Congela a cauda do reverb");
    addAndMakeVisible(verbFreezeButton);
    bindVerbColumn(); // algo/prenote/freeze + 8 sliders (FWD|REV).

    // --- Granular ---
    grModeBox.addItemList({ "Off", "BeatRepeat", "Slice", "Reverse", "Pitch", "Stutter" }, 1);
    grModeBox.setTooltip("Modo do granular");
    addAndMakeVisible(grModeBox);
    grTrigBox.addItemList({ "Chance", "Envelope", "Manual" }, 1);
    grTrigBox.setTooltip("Trigger do grab");
    addAndMakeVisible(grTrigBox);
    grManualButton.setTooltip("Disparo manual (flanco)");
    addAndMakeVisible(grManualButton);
    grLenBox.addItemList(TempoInfo::noteNames(), 1);
    grLenBox.setTooltip("Tamanho do fragmento");
    addAndMakeVisible(grLenBox);
    grTimeNoteBox.addItemList(TempoInfo::noteNames(), 1);
    grTimeNoteBox.setTooltip("Duração em nota (Free = ms)");
    addAndMakeVisible(grTimeNoteBox);
    grInterruptButton.setTooltip("Glitch pausa o dry");
    addAndMakeVisible(grInterruptButton);
    bindGranColumn(); // mode/trig/manual/len/timenote/interrupt + 9 sliders.

    granLed.setJustificationType(juce::Justification::centred);
    granLed.setText("IDLE", juce::dontSendNotification);
    addAndMakeVisible(granLed);

    // --- Modo global FWD|REV (segmentado na topbar, religa as 4 colunas) ---
    fwdModeBtn.setTooltip("Editar cadeia FWD");
    revModeBtn.setTooltip("Editar cadeia REV (completa, não por módulo)");
    fwdModeBtn.setClickingTogglesState(true);
    revModeBtn.setClickingTogglesState(true);
    fwdModeBtn.setToggleState(true, juce::dontSendNotification);
    revModeBtn.setColour(juce::TextButton::buttonOnColourId, AbletonLnF::revAccent);
    addAndMakeVisible(fwdModeBtn);
    addAndMakeVisible(revModeBtn);
    auto setMode = [this](bool rev)
    {
        showRev = rev;
        fwdModeBtn.setToggleState(! rev, juce::dontSendNotification);
        revModeBtn.setToggleState(rev, juce::dontSendNotification);
        bindGateColumn();
        bindDelayColumn();
        bindVerbColumn();
        bindGranColumn();
    };
    fwdModeBtn.onClick = [setMode] { setMode(false); };
    revModeBtn.onClick = [setMode] { setMode(true); };

    // --- Faixa REV na 2ª linha da topbar: engine + links + trims + routing ---
    revModeBox.addItemList({ "Off", "Loop", "Throw" }, 1);
    revModeBox.setTooltip("Motor REV: desligado / loop / throw único");
    addAndMakeVisible(revModeBox);
    revModeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        proc.apvts, "rev_mode", revModeBox);
    revSourceBox.addItemList({ "Dry", "PostFWD" }, 1);
    revSourceBox.setTooltip("Fonte da captura REV");
    addAndMakeVisible(revSourceBox);
    revSourceAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        proc.apvts, "rev_source", revSourceBox);
    revCaptureBox.addItemList({ "2 beats", "3 beats", "4 beats" }, 1);
    revCaptureBox.setTooltip("Janela de captura");
    addAndMakeVisible(revCaptureBox);
    revCaptureAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        proc.apvts, "rev_capture", revCaptureBox);
    revRateAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, "rev_rate", revRateSlider);
    revLfoAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, "rev_lfo", revLfoSlider);
    revDuckAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, "rev_duck", revDuckSlider);
    trimDelayAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, "trim_delay", trimDelaySlider);
    trimDecayAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, "trim_decay", trimDecaySlider);
    revThrowButton.setTooltip("Dispara uma cauda (modo Throw; também por nota MIDI)");
    addAndMakeVisible(revThrowButton);
    revThrowAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        proc.apvts, "rev_throw", revThrowButton);
    linkMasterBtn.setTooltip("Tudo o que tem link segue o FWD");
    linkGateBtn.setTooltip("Gate REV segue FWD");
    linkDelayBtn.setTooltip("Delay REV segue FWD");
    linkVerbBtn.setTooltip("Verb REV segue FWD");
    linkGranBtn.setTooltip("Gran REV segue FWD");
    for (auto* b : { &linkMasterBtn, &linkGateBtn, &linkDelayBtn, &linkVerbBtn, &linkGranBtn })
    {
        b->setColour(juce::TextButton::buttonOnColourId, AbletonLnF::revAccent);
        addAndMakeVisible(b);
    }
    linkMasterAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        proc.apvts, "link_master", linkMasterBtn);
    linkGateAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        proc.apvts, "link_gate", linkGateBtn);
    linkDelayAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        proc.apvts, "link_delay", linkDelayBtn);
    linkVerbAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        proc.apvts, "link_verb", linkVerbBtn);
    linkGranAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        proc.apvts, "link_gran", linkGranBtn);

    // Routing (era criado no resized: bug quando re-resize duplicava items).
    orderBox.addItemList({ "G-D-V-G", "G-V-D-G", "D-G-V-G", "V-D-G-G" }, 1);
    orderBox.setTooltip("Ordem da cadeia: Gate Delay Verb Granular (FWD e REV)");
    addAndMakeVisible(orderBox);
    orderAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        proc.apvts, "chain_order", orderBox);
    xfadeBox.addItemList({ "Add", "XFade" }, 1);
    xfadeBox.setTooltip("Add = soma; XFade = beats pares FWD, ímpares REV");
    addAndMakeVisible(xfadeBox);
    xmodeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        proc.apvts, "x_mode", xfadeBox);

    bindGateColumn(); // bindDelay/Verb/Gran chamados nos seus blocos; gate aqui.

    // Nomes colados aos knobs pelo LnF (nunca via paint — anti-desalinho).
    {
        juce::Slider* all[] = { &inputSlider, &fwdMixSlider, &revMixSlider, &morphSlider, &masterSlider };
        for (int i = 0; i < 5; ++i) all[i]->setName(kMotorLabels[i]);
    }
    {
        juce::Slider* all[] = { &smoothSlider, &depthSlider, &gmixSlider, &panSlider, &thrSlider };
        for (int i = 0; i < 5; ++i) all[i]->setName(kGateLabels[i]);
    }
    {
        juce::Slider* all[] = { &bpmSlider, &timeSlider, &fbSlider, &dampSlider, &dmixSlider };
        for (int i = 0; i < 5; ++i) all[i]->setName(kDelayLabels[i]);
    }
    {
        juce::Slider* all[] = { &sizeSlider, &decaySlider, &dampVSlider, &widthSlider,
                                &predelaySlider, &locutSlider, &hicutSlider, &vmixSlider };
        for (int i = 0; i < 8; ++i) all[i]->setName(kVerbLabels[i]);
    }
    {
        juce::Slider* all[] = { &chanceSlider, &thrGSlider, &repeatsSlider, &decayGSlider,
                                &timeMsSlider, &pitchSlider, &fluxSlider, &xfadeSlider,
                                &granMixSlider };
        for (int i = 0; i < 9; ++i) all[i]->setName(kGranLabels[i]);
    }
    {
        juce::Slider* all[] = { &driveSlider, &wowRateSlider, &wowDepthSlider, &spreadSlider };
        const char* names[] = { "DRIVE", "WOW RT", "WOW DP", "SPREAD" };
        for (int i = 0; i < 4; ++i) all[i]->setName(names[i]);
    }

    startTimerHz(30);
}

void DeVerbEditor::bindGateColumn()
{
    // Ordem crítica: destruir os velhos ANTES de criar os novos — o initial
    // update do novo attachment faz slider.setValue (notifica!) e o velho,
    // ainda vivo, escreveria o valor no parâmetro errado (cross-talk FWD|REV).
    rateAttach.reset(); stepsAttach.reset(); trigAttach.reset();
    smoothAttach.reset(); depthAttach.reset(); gmixAttach.reset();
    panAttach.reset(); thrAttach.reset();
    juce::String p = showRev ? "rev_" : "fwd_";
    rateAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        proc.apvts, p + "gate_rate", rateBox);
    stepsAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        proc.apvts, p + "gate_steps", stepsBox);
    trigAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        proc.apvts, p + "gate_trig", trigBox);
    smoothAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, p + "gate_smooth", smoothSlider);
    depthAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, p + "gate_depth", depthSlider);
    gmixAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, p + "gate_mix", gmixSlider);
    panAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, p + "gate_pan", panSlider);
    thrAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, p + "gate_env_thr", thrSlider);
}

void DeVerbEditor::bindDelayColumn()
{
    timeAttach.reset(); fbAttach.reset(); dampAttach.reset(); dmixAttach.reset();
    noteAttach.reset(); freezeAttach.reset();
    delayAlgoAttach.reset(); driveAttach.reset(); wowRateAttach.reset();
    wowDepthAttach.reset(); spreadAttach.reset();
    juce::String p = showRev ? "rev_" : "fwd_";
    timeAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, p + "delay_time", timeSlider);
    fbAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, p + "delay_fb", fbSlider);
    dampAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, p + "delay_damp", dampSlider);
    dmixAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, p + "delay_mix", dmixSlider);
    noteAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        proc.apvts, p + "delay_note", noteBox);
    freezeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        proc.apvts, p + "delay_freeze", freezeButton);
    delayAlgoAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        proc.apvts, p + "delay_algo", delayAlgoBox);
    driveAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, p + "delay_drive", driveSlider);
    wowRateAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, p + "delay_wow_rate", wowRateSlider);
    wowDepthAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, p + "delay_wow_depth", wowDepthSlider);
    spreadAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, p + "delay_spread", spreadSlider);
}

void DeVerbEditor::bindVerbColumn()
{
    algoAttach.reset(); preNoteAttach.reset(); verbFreezeAttach.reset();
    sizeAttach.reset(); decayAttach.reset(); dampVAttach.reset(); widthAttach.reset();
    predelayAttach.reset(); locutAttach.reset(); hicutAttach.reset(); vmixAttach.reset();
    juce::String p = showRev ? "rev_" : "fwd_";
    algoAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        proc.apvts, p + "verb_algo", algoBox);
    preNoteAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        proc.apvts, p + "verb_predelay_note", preNoteBox);
    verbFreezeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        proc.apvts, p + "verb_freeze", verbFreezeButton);
    sizeAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, p + "verb_size", sizeSlider);
    decayAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, p + "verb_decay", decaySlider);
    dampVAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, p + "verb_damp", dampVSlider);
    widthAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, p + "verb_width", widthSlider);
    predelayAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, p + "verb_predelay", predelaySlider);
    locutAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, p + "verb_locut", locutSlider);
    hicutAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, p + "verb_hicut", hicutSlider);
    vmixAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, p + "verb_mix", vmixSlider);
}

void DeVerbEditor::bindGranColumn()
{
    grModeAttach.reset(); grTrigAttach.reset(); grManualAttach.reset();
    grLenAttach.reset(); grTimeNoteAttach.reset(); grInterruptAttach.reset();
    chanceAttach.reset(); thrGAttach.reset(); repeatsAttach.reset(); decayGAttach.reset();
    timeMsAttach.reset(); pitchAttach.reset(); fluxAttach.reset(); xfadeAttach.reset();
    granMixAttach.reset();
    juce::String p = showRev ? "rev_gr_" : "gr_";
    grModeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        proc.apvts, p + "mode", grModeBox);
    grTrigAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        proc.apvts, p + "trigger", grTrigBox);
    grManualAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        proc.apvts, p + "manual", grManualButton);
    grLenAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        proc.apvts, p + "len_note", grLenBox);
    grTimeNoteAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        proc.apvts, p + "time_note", grTimeNoteBox);
    grInterruptAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        proc.apvts, p + "interrupt", grInterruptButton);
    chanceAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, p + "chance", chanceSlider);
    thrGAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, p + "env_thr", thrGSlider);
    repeatsAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, p + "repeats", repeatsSlider);
    decayGAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, p + "decay", decayGSlider);
    timeMsAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, p + "time", timeMsSlider);
    pitchAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, p + "pitch", pitchSlider);
    fluxAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, p + "flux", fluxSlider);
    xfadeAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, p + "xfade", xfadeSlider);
    granMixAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, p + "mix", granMixSlider);
}

DeVerbEditor::~DeVerbEditor()
{
    setLookAndFeel(nullptr); // antes de `lnf` ser destruído
}

void DeVerbEditor::flipStep(int step)
{
    auto* par = proc.apvts.getParameter(gatePatId());
    if (par == nullptr)
        return;
    int bits = (int) proc.apvts.getRawParameterValue(gatePatId())->load();
    bits ^= (1 << step);
    par->setValueNotifyingHost(par->convertTo0to1((float) bits));
}

void DeVerbEditor::timerCallback()
{
    int bits = (int) proc.apvts.getRawParameterValue(gatePatId())->load();
    int steps = proc.apvts.getRawParameterValue(gateStepsId())->load() > 0.5f ? 16 : 8;
    for (int i = 0; i < 16; ++i)
    {
        bool on = (bits >> i) & 1;
        stepButtons[i].setToggleState(on, juce::dontSendNotification);
        stepButtons[i].setEnabled(i < steps);
    }
    ruler.setNumSteps(steps);
    ruler.setPattern(bits);
    int step = proc.getGateStep();
    ruler.setActiveStep(step);
    float bpm = proc.getUiBpm();
    bpmReadout.setText("BPM " + juce::String(bpm, 1), juce::dontSendNotification);
    gateReadout.setText("GATE " + juce::String::formatted("%02d", step + 1)
                        + "/" + juce::String(steps), juce::dontSendNotification);
    // Readouts calculados (alvo da coluna: FWD ou REV).
    {
        const char* tp = showRev ? "rev_" : "fwd_";
        juce::String nid = juce::String(tp) + "delay_note";
        juce::String mid = juce::String(tp) + "delay_time";
        int ni = (int) proc.apvts.getRawParameterValue(nid)->load();
        float ms = (ni == (int) TempoInfo::Note::Free)
                 ? proc.apvts.getRawParameterValue(mid)->load()
                 : (float) (TempoInfo::beatsToSeconds(
                       TempoInfo::noteToBeats((TempoInfo::Note) ni), (double) bpm) * 1000.0);
        juce::String txt = juce::String(ms, 0) + " ms";
        if (ni != (int) TempoInfo::Note::Free)
            txt += " - " + TempoInfo::noteNames()[ni];
        delayMsReadout.setText(txt, juce::dontSendNotification);
    }
    {
        const char* tp = showRev ? "rev_" : "fwd_";
        float dec = proc.apvts.getRawParameterValue(juce::String(tp) + "verb_decay")->load();
        verbDecayReadout.setText("T60 " + juce::String(dec, 1) + " s",
                                 juce::dontSendNotification);
    }
    bool active = proc.isGranularActive();
    granLed.setText(active ? "GRAB!" : "IDLE", juce::dontSendNotification);
    granLed.setColour(juce::Label::textColourId,
                      active ? AbletonLnF::granAccent : AbletonLnF::dimText);
}

void DeVerbEditor::paint(juce::Graphics& g)
{
    using C = AbletonLnF;
    g.fillAll(C::bg);

    // Topbar.
    g.setColour(C::greyText);
    g.setFont(juce::Font(17.f, juce::Font::bold));
    g.drawText("deVerb", 12, 0, 200, 36, juce::Justification::centredLeft, false);
    g.setColour(C::dimText);
    g.setFont(11.f);
    g.drawText("REV", 12, 40, 52, 56, juce::Justification::centred, false);
    g.drawText("LINK", 656, 40, 40, 56, juce::Justification::centred, false);
    g.setColour(juce::Colour(0xff2a2a2a));
    g.fillRect(0, 35, kW, 1);
    g.fillRect(0, kTopH - 1, kW, 1);

    // Panels + headers com acento por módulo.
    auto defs = colDefs();
    for (int i = 0; i < 5; ++i)
    {
        g.setColour(defs[i].panel);
        g.fillRoundedRectangle((float) defs[i].x, (float) kColY,
                               (float) defs[i].w, (float) (kColBottom - kColY), 8.f);
        g.setColour(defs[i].accent);
        g.fillRect(defs[i].x + 12, kColY + 21, 44, 3);
        g.setColour(C::greyText);
        g.setFont(juce::Font(11.f, juce::Font::bold));
        g.drawText(defs[i].name, defs[i].x + 12, kColY, 220, kHeadH,
                   juce::Justification::centredLeft, false);
    }

    // Placeholder do futuro loader de IRs (ocupa o ar da coluna VERB e
    // comunica o roadmap; sem widgets nem params — só tinta).
    {
        auto ph = juce::Rectangle<float>(752.f, 484.f, 240.f, 120.f);
        g.setColour(juce::Colour(0xff232323));
        g.drawRoundedRectangle(ph, 6.f, 1.f);
        g.setColour(AbletonLnF::faintText);
        g.setFont(11.f);
        g.drawFittedText("CONVOLUTION", ph.withTrimmedBottom(66).toNearestInt(),
                         juce::Justification::centredBottom, 1);
        g.drawFittedText("FASE 8", ph.withTrimmedTop(54).toNearestInt(),
                         juce::Justification::centredTop, 1);
    }

    // (Nomes dos knobs: desenhados pelo LnF dentro de cada slider.)
    // (minis da faixa REV sem labels pintados: tooltips chegam; valores em cima)
}

void DeVerbEditor::resized()
{
    // Topbar linha 1: BPM + gate + preset + LED.
    bpmReadout.setBounds(220, 7, 110, 22);
    gateReadout.setBounds(340, 7, 130, 22);
    globalPresetBox.setBounds(560, 7, 240, 22);
    randomButton.setBounds(806, 7, 68, 22);
    granLed.setBounds(kW - 132, 7, 120, 22);

    // Topbar linha 2: faixa REV (engine + links + trims + routing).
    revModeBox.setBounds(70, 42, 110, 30);
    revSourceBox.setBounds(186, 42, 90, 30);
    revCaptureBox.setBounds(282, 42, 90, 30);
    revKnobs[0] = { 378, 38, 56, 58 };
    revKnobs[1] = { 438, 38, 56, 58 };
    revKnobs[2] = { 498, 38, 56, 58 };
    revRateSlider.setBounds(revKnobs[0]);
    revLfoSlider.setBounds(revKnobs[1]);
    revDuckSlider.setBounds(revKnobs[2]);
    revThrowButton.setBounds(560, 44, 90, 30);
    linkMasterBtn.setBounds(700, 44, 56, 30);
    linkGateBtn.setBounds(758, 44, 56, 30);
    linkDelayBtn.setBounds(816, 44, 56, 30);
    linkVerbBtn.setBounds(874, 44, 56, 30);
    linkGranBtn.setBounds(932, 44, 56, 30);
    revKnobs[3] = { 996, 38, 56, 58 };
    revKnobs[4] = { 1056, 38, 56, 58 };
    trimDelaySlider.setBounds(revKnobs[3]);
    trimDecaySlider.setBounds(revKnobs[4]);
    orderBox.setBounds(1116, 44, 76, 30);
    xfadeBox.setBounds(1196, 44, 72, 30);

    // Segmentado FWD|REV (modo global de edição).
    fwdModeBtn.setBounds(880, 7, 90, 22);
    revModeBtn.setBounds(972, 7, 90, 22);

    int y0 = kColY + kHeadH; // 132

    // MOTOR x[8,192): INPUT + FWD/REV lado a lado + MORPH/MASTER grandes.
    {
        motorKnobs[0] = { 64, y0, 72, 90 };
        inputSlider.setBounds(motorKnobs[0]);
        motorKnobs[1] = { 12, y0 + 94, 84, 90 };
        motorKnobs[2] = { 100, y0 + 94, 84, 90 };
        fwdMixSlider.setBounds(motorKnobs[1]);
        revMixSlider.setBounds(motorKnobs[2]);
        motorKnobs[3] = { 52, y0 + 188, 88, 110 };
        morphSlider.setBounds(motorKnobs[3]);
        motorKnobs[4] = { 52, y0 + 302, 88, 110 };
        masterSlider.setBounds(motorKnobs[4]);
    }

    // GATE x[200,496): régua + steps 2×8 + combos 2×2 + knobs 3+2 BIG.
    {
        ruler.setBounds(204, y0, 288, 44);
        int sy = y0 + 44 + 8;
        for (int i = 0; i < 16; ++i)
        {
            int row = i / 8, col = i % 8;
            stepButtons[i].setBounds(204 + col * 36, sy + row * 48, 34, 46);
        }
        int cy = sy + 2 * 48 + 8;
        presetBox.setBounds(204, cy, 144, 32);
        rateBox.setBounds(352, cy, 144, 32);
        stepsBox.setBounds(204, cy + 36, 144, 32);
        trigBox.setBounds(352, cy + 36, 144, 32);
        int ky = cy + 2 * 36 + 10;
        juce::Slider* ks[5] = { &smoothSlider, &depthSlider, &gmixSlider, &panSlider, &thrSlider };
        for (int i = 0; i < 3; ++i)
        {
            gateKnobs[i] = { 204 + i * 97, ky, 96, 110 };
            ks[i]->setBounds(gateKnobs[i]);
        }
        for (int i = 3; i < 5; ++i)
        {
            gateKnobs[i] = { 204 + (i - 3) * 97, ky + 114, 96, 110 };
            ks[i]->setBounds(gateKnobs[i]);
        }
    }

    // DELAY x[504,736): knobs 3+2 STD + algo + 4 minis + combo + readout + freeze.
    {
        juce::Slider* ks[5] = { &bpmSlider, &timeSlider, &fbSlider, &dampSlider, &dmixSlider };
        for (int i = 0; i < 3; ++i)
        {
            delayKnobs[i] = { 508 + i * 74, y0, 72, 90 };
            ks[i]->setBounds(delayKnobs[i]);
        }
        for (int i = 3; i < 5; ++i)
        {
            delayKnobs[i] = { 508 + (i - 3) * 74, y0 + 94, 72, 90 };
            ks[i]->setBounds(delayKnobs[i]);
        }
        delayAlgoBox.setBounds(508, y0 + 188, 224, 32);
        delayMinis[0] = { 508, y0 + 224, 110, 64 };
        delayMinis[1] = { 622, y0 + 224, 110, 64 };
        delayMinis[2] = { 508, y0 + 292, 110, 64 };
        delayMinis[3] = { 622, y0 + 292, 110, 64 };
        driveSlider.setBounds(delayMinis[0]);
        wowRateSlider.setBounds(delayMinis[1]);
        wowDepthSlider.setBounds(delayMinis[2]);
        spreadSlider.setBounds(delayMinis[3]);
        noteBox.setBounds(508, y0 + 360, 224, 32);
        delayMsReadout.setBounds(508, y0 + 396, 224, 20);
        freezeButton.setBounds(508, y0 + 420, 120, 32);
    }

    // VERB x[744,1000): knobs 4+4 STD + combos + readout + freeze.
    // (espaço livre em baixo reservado ao futuro loader de IRs.)
    {
        juce::Slider* ks[8] = { &sizeSlider, &decaySlider, &dampVSlider, &widthSlider,
                                &predelaySlider, &locutSlider, &hicutSlider, &vmixSlider };
        for (int i = 0; i < 4; ++i)
        {
            verbKnobs[i] = { 744 + i * 64, y0, 64, 90 };
            ks[i]->setBounds(verbKnobs[i]);
        }
        for (int i = 4; i < 8; ++i)
        {
            verbKnobs[i] = { 744 + (i - 4) * 64, y0 + 94, 64, 90 };
            ks[i]->setBounds(verbKnobs[i]);
        }
        algoBox.setBounds(748, y0 + 192, 122, 32);
        preNoteBox.setBounds(874, y0 + 192, 122, 32);
        verbDecayReadout.setBounds(748, y0 + 228, 248, 20);
        verbFreezeButton.setBounds(748, y0 + 252, 124, 32);
    }

    // GRAN x[1008,1272): combos 2×2 + knobs 4+3+2 STD + interrupt/timenote.
    {
        grModeBox.setBounds(1012, y0, 126, 30);
        grTrigBox.setBounds(1142, y0, 126, 30);
        grManualButton.setBounds(1012, y0 + 34, 126, 30);
        grLenBox.setBounds(1142, y0 + 34, 126, 30);
        juce::Slider* ks[9] = { &chanceSlider, &thrGSlider, &repeatsSlider, &decayGSlider,
                                &timeMsSlider, &pitchSlider, &fluxSlider, &xfadeSlider,
                                &granMixSlider };
        int gy = y0 + 72;
        for (int i = 0; i < 4; ++i)
        {
            granKnobs[i] = { 1012 + i * 65, gy, 64, 90 };
            ks[i]->setBounds(granKnobs[i]);
        }
        for (int i = 4; i < 7; ++i)
        {
            granKnobs[i] = { 1012 + (i - 4) * 65, gy + 94, 64, 90 };
            ks[i]->setBounds(granKnobs[i]);
        }
        for (int i = 7; i < 9; ++i)
        {
            granKnobs[i] = { 1012 + (i - 7) * 65, gy + 188, 64, 90 };
            ks[i]->setBounds(granKnobs[i]);
        }
        grInterruptButton.setBounds(1142, gy + 188, 126, 30);
        grTimeNoteBox.setBounds(1142, gy + 222, 126, 30);
    }
}
