#include "PluginProcessor.h"
#include "PluginEditor.h"

DeVerbProcessor::DeVerbProcessor()
    : AudioProcessor(BusesProperties()
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Params", createParams())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout DeVerbProcessor::createParams()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "input_gain", "Input Gain", juce::NormalisableRange<float>(0.f, 2.f, 0.01f), 1.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "fwd_mix", "FWD Mix", juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 1.f));
    // NOTA: o "rev_mix" da fase 0 foi removido — o "REV Mix" real (engine,
    // default 0.4) vive na secção da fase 5. IDs duplicados partiam o restore.
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "morph", "Morph", juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "master", "Master", juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.8f));
    // --- Fase 1: tempo + delay digital FWD (IDs congelados a partir do v1) ---
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "tempo_bpm", "Tempo BPM", juce::NormalisableRange<float>(40.f, 240.f, 0.1f), 120.f));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "fwd_delay_note", "FWD Delay Note", TempoInfo::noteNames(), 7)); // 7 = "1/8D"
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "fwd_delay_time", "FWD Delay Ms", juce::NormalisableRange<float>(1.f, 2000.f, 0.1f), 375.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "fwd_delay_fb", "FWD Delay FB", juce::NormalisableRange<float>(0.f, 0.95f, 0.01f), 0.35f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "fwd_delay_damp", "FWD Delay Damp",
        juce::NormalisableRange<float>(200.f, 18000.f, 1.f, 0.35f), 6000.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "fwd_delay_mix", "FWD Delay Mix", juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.25f));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        "fwd_delay_freeze", "FWD Delay Freeze", false));
    // --- Fase 7: algoritmos do delay (IDs congelados a partir do v1) ---
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "fwd_delay_algo", "FWD Delay Algo",
        juce::StringArray { "Digital", "Tape", "PingPong", "MultiTap", "Reverse" }, 0));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "fwd_delay_drive", "FWD Delay Drive", juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.3f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "fwd_delay_wow_rate", "FWD Delay WowRate",
        juce::NormalisableRange<float>(0.1f, 5.f, 0.01f, 0.5f), 1.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "fwd_delay_wow_depth", "FWD Delay WowDepth",
        juce::NormalisableRange<float>(0.f, 20.f, 0.1f), 4.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "fwd_delay_spread", "FWD Delay Spread", juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 1.f));
    // --- Fase 2: gater FWD (IDs congelados a partir do v1) ---
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "fwd_gate_rate", "FWD Gate Rate", TempoInfo::noteNames(), 3)); // 3 = "1/16"
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "fwd_gate_steps", "FWD Gate Steps", juce::StringArray { "8", "16" }, 1));
    layout.add(std::make_unique<juce::AudioParameterInt>(
        "fwd_gate_pattern", "FWD Gate Pattern", 0, 65535, 0x1111));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "fwd_gate_smooth", "FWD Gate Smooth", juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.15f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "fwd_gate_depth", "FWD Gate Depth", juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 1.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "fwd_gate_mix", "FWD Gate Mix", juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 1.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "fwd_gate_pan", "FWD Gate PanAlt", juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.f));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "fwd_gate_trig", "FWD Gate Trig",
        juce::StringArray { "Host", "Midi", "Transient", "Free" }, 0));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "fwd_gate_env_thr", "FWD Gate EnvThr",
        juce::NormalisableRange<float>(-60.f, 0.f, 0.5f), -18.f));
    // --- Fase 3: reverb FWD (IDs congelados a partir do v1) ---
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "fwd_verb_algo", "FWD Verb Algo",
        juce::StringArray { "Room", "Hall", "Plate", "Shimmer" }, 1));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "fwd_verb_size", "FWD Verb Size", juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "fwd_verb_decay", "FWD Verb Decay",
        juce::NormalisableRange<float>(0.2f, 20.f, 0.01f, 0.4f), 2.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "fwd_verb_damp", "FWD Verb Damp", juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.3f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "fwd_verb_width", "FWD Verb Width", juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 1.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "fwd_verb_predelay", "FWD Verb PreDelay",
        juce::NormalisableRange<float>(0.f, 250.f, 0.1f), 20.f));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "fwd_verb_predelay_note", "FWD Verb PreDelay Note", TempoInfo::noteNames(), 0));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        "fwd_verb_freeze", "FWD Verb Freeze", false));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "fwd_verb_mix", "FWD Verb Mix", juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.3f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "fwd_verb_locut", "FWD Verb LoCut",
        juce::NormalisableRange<float>(20.f, 500.f, 1.f, 0.5f), 80.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "fwd_verb_hicut", "FWD Verb HiCut",
        juce::NormalisableRange<float>(2000.f, 20000.f, 1.f, 0.5f), 12000.f));
    // --- Fase 4: granular/glitch FWD (IDs congelados a partir do v1) ---
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "gr_mode", "Gran Mode",
        juce::StringArray { "Off", "BeatRepeat", "Slice", "Reverse", "Pitch", "Stutter" }, 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "gr_trigger", "Gran Trig",
        juce::StringArray { "Chance", "Envelope", "Manual" }, 0));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        "gr_manual", "Gran Manual", false));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "gr_chance", "Gran Chance", juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.2f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "gr_env_thr", "Gran EnvThr",
        juce::NormalisableRange<float>(-60.f, 0.f, 0.5f), -18.f));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "gr_len_note", "Gran Len Note", TempoInfo::noteNames(), 3)); // 3 = "1/16"
    layout.add(std::make_unique<juce::AudioParameterInt>(
        "gr_repeats", "Gran Repeats", 1, 16, 4));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "gr_decay", "Gran Decay", juce::NormalisableRange<float>(0.5f, 0.99f, 0.01f), 0.85f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "gr_time", "Gran Time Ms", juce::NormalisableRange<float>(60.f, 8000.f, 1.f, 0.4f), 250.f));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "gr_time_note", "Gran Time Note", TempoInfo::noteNames(), 0));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "gr_pitch", "Gran Pitch", juce::NormalisableRange<float>(-12.f, 12.f, 0.5f), 12.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "gr_flux", "Gran Flux", juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.3f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "gr_xfade", "Gran Xfade", juce::NormalisableRange<float>(1.f, 50.f, 0.5f), 8.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "gr_mix", "Gran Mix", juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.5f));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        "gr_interrupt", "Gran Interrupt", false));
    // --- Fase 5: motor REV + links (IDs congelados a partir do v1) ---
    // Engine:
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "rev_mode", "REV Mode", juce::StringArray { "Off", "Loop", "Throw" }, 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "rev_source", "REV Source", juce::StringArray { "Dry", "PostFWD" }, 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "rev_capture", "REV Capture", juce::StringArray { "2 beats", "3 beats", "4 beats" }, 0));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "rev_rate", "REV Rate", juce::NormalisableRange<float>(0.25f, 2.f, 0.01f), 1.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "rev_lfo", "REV LFO", juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "rev_mix", "REV Mix", juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.4f));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        "rev_throw", "REV Throw", false));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "rev_duck", "REV Duck", juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.3f));
    // Links + trims:
    layout.add(std::make_unique<juce::AudioParameterBool>("link_master", "Link Master", true));
    layout.add(std::make_unique<juce::AudioParameterBool>("link_gate", "Link Gate", true));
    layout.add(std::make_unique<juce::AudioParameterBool>("link_delay", "Link Delay", true));
    layout.add(std::make_unique<juce::AudioParameterBool>("link_verb", "Link Verb", true));
    layout.add(std::make_unique<juce::AudioParameterBool>("link_gran", "Link Gran", true));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "trim_delay", "Trim Delay", juce::NormalisableRange<float>(0.25f, 4.f, 0.01f), 1.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "trim_decay", "Trim Decay", juce::NormalisableRange<float>(0.25f, 2.f, 0.01f), 1.f));
    // --- Fase 6: routing (IDs congelados a partir do v1) ---
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "chain_order", "Chain Order",
        juce::StringArray { "G-D-V-Gr", "G-V-D-Gr", "D-G-V-Gr", "V-D-G-Gr" }, 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "x_mode", "X Mode", juce::StringArray { "Add", "XFade" }, 0));
    // Gate REV (defaults = FWD para arranque coerente):
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "rev_gate_rate", "REV Gate Rate", TempoInfo::noteNames(), 3));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "rev_gate_steps", "REV Gate Steps", juce::StringArray { "8", "16" }, 1));
    layout.add(std::make_unique<juce::AudioParameterInt>(
        "rev_gate_pattern", "REV Gate Pattern", 0, 65535, 0x1111));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "rev_gate_smooth", "REV Gate Smooth", juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.15f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "rev_gate_depth", "REV Gate Depth", juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 1.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "rev_gate_mix", "REV Gate Mix", juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 1.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "rev_gate_pan", "REV Gate PanAlt", juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.f));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "rev_gate_trig", "REV Gate Trig",
        juce::StringArray { "Host", "Midi", "Transient", "Free" }, 0));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "rev_gate_env_thr", "REV Gate EnvThr",
        juce::NormalisableRange<float>(-60.f, 0.f, 0.5f), -18.f));
    // Delay REV:
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "rev_delay_note", "REV Delay Note", TempoInfo::noteNames(), 7));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "rev_delay_time", "REV Delay Ms", juce::NormalisableRange<float>(1.f, 2000.f, 0.1f), 375.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "rev_delay_fb", "REV Delay FB", juce::NormalisableRange<float>(0.f, 0.95f, 0.01f), 0.35f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "rev_delay_damp", "REV Delay Damp",
        juce::NormalisableRange<float>(200.f, 18000.f, 1.f, 0.35f), 6000.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "rev_delay_mix", "REV Delay Mix", juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.25f));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        "rev_delay_freeze", "REV Delay Freeze", false));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "rev_delay_algo", "REV Delay Algo",
        juce::StringArray { "Digital", "Tape", "PingPong", "MultiTap", "Reverse" }, 0));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "rev_delay_drive", "REV Delay Drive", juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.3f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "rev_delay_wow_rate", "REV Delay WowRate",
        juce::NormalisableRange<float>(0.1f, 5.f, 0.01f, 0.5f), 1.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "rev_delay_wow_depth", "REV Delay WowDepth",
        juce::NormalisableRange<float>(0.f, 20.f, 0.1f), 4.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "rev_delay_spread", "REV Delay Spread", juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 1.f));
    // Verb REV:
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "rev_verb_algo", "REV Verb Algo",
        juce::StringArray { "Room", "Hall", "Plate", "Shimmer" }, 1));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "rev_verb_size", "REV Verb Size", juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "rev_verb_decay", "REV Verb Decay",
        juce::NormalisableRange<float>(0.2f, 20.f, 0.01f, 0.4f), 2.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "rev_verb_damp", "REV Verb Damp", juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.3f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "rev_verb_width", "REV Verb Width", juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 1.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "rev_verb_predelay", "REV Verb PreDelay",
        juce::NormalisableRange<float>(0.f, 250.f, 0.1f), 20.f));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "rev_verb_predelay_note", "REV Verb PreDelay Note", TempoInfo::noteNames(), 0));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        "rev_verb_freeze", "REV Verb Freeze", false));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "rev_verb_mix", "REV Verb Mix", juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.3f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "rev_verb_locut", "REV Verb LoCut",
        juce::NormalisableRange<float>(20.f, 500.f, 1.f, 0.5f), 80.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "rev_verb_hicut", "REV Verb HiCut",
        juce::NormalisableRange<float>(2000.f, 20000.f, 1.f, 0.5f), 12000.f));
    // Granular REV:
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "rev_gr_mode", "REV Gran Mode",
        juce::StringArray { "Off", "BeatRepeat", "Slice", "Reverse", "Pitch", "Stutter" }, 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "rev_gr_trigger", "REV Gran Trig",
        juce::StringArray { "Chance", "Envelope", "Manual" }, 0));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        "rev_gr_manual", "REV Gran Manual", false));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "rev_gr_chance", "REV Gran Chance", juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.2f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "rev_gr_env_thr", "REV Gran EnvThr",
        juce::NormalisableRange<float>(-60.f, 0.f, 0.5f), -18.f));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "rev_gr_len_note", "REV Gran Len Note", TempoInfo::noteNames(), 3));
    layout.add(std::make_unique<juce::AudioParameterInt>(
        "rev_gr_repeats", "REV Gran Repeats", 1, 16, 4));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "rev_gr_decay", "REV Gran Decay", juce::NormalisableRange<float>(0.5f, 0.99f, 0.01f), 0.85f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "rev_gr_time", "REV Gran Time Ms", juce::NormalisableRange<float>(60.f, 8000.f, 1.f, 0.4f), 250.f));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "rev_gr_time_note", "REV Gran Time Note", TempoInfo::noteNames(), 0));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "rev_gr_pitch", "REV Gran Pitch", juce::NormalisableRange<float>(-12.f, 12.f, 0.5f), 12.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "rev_gr_flux", "REV Gran Flux", juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.3f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "rev_gr_xfade", "REV Gran Xfade", juce::NormalisableRange<float>(1.f, 50.f, 0.5f), 8.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "rev_gr_mix", "REV Gran Mix", juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.5f));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        "rev_gr_interrupt", "REV Gran Interrupt", false));
    return layout;
}

void DeVerbProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    fwdDelay.prepare(sampleRate, samplesPerBlock);
    fwdGate.prepare(sampleRate);
    fwdVerb.prepare(sampleRate, samplesPerBlock);
    fwdGran.prepare(sampleRate);
    revEngine.prepare(sampleRate);
    revGate.prepare(sampleRate);
    revDelay.prepare(sampleRate, samplesPerBlock);
    revVerb.prepare(sampleRate, samplesPerBlock);
    revGran.prepare(sampleRate);
    revBuf.setSize(2, samplesPerBlock, false, false, true);
    fwdBuf.setSize(2, samplesPerBlock, false, false, true);
    smoothInput.reset(sampleRate, 0.03);
    smoothMaster.reset(sampleRate, 0.03);
    xfFwd.reset(sampleRate, 0.01); // 10 ms: crossfade do XFADE
    xfRev.reset(sampleRate, 0.01);
    smoothRevMix.reset(sampleRate, 0.03);
    smoothInput.setCurrentAndTargetValue(apvts.getRawParameterValue("input_gain")->load());
    smoothMaster.setCurrentAndTargetValue(apvts.getRawParameterValue("master")->load());
    xfFwd.setCurrentAndTargetValue(1.f);
    xfRev.setCurrentAndTargetValue(1.f);
    smoothRevMix.setCurrentAndTargetValue(apvts.getRawParameterValue("rev_mix")->load());
}

void DeVerbProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    // Efeito com mesma config in/out: não tocar no áudio (passthrough).
    // Limpar canais de saída extra sem input correspondente (regra JUCE).
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Fase 1: ganhos com smoothing + delay FWD.
    smoothInput.setTargetValue(apvts.getRawParameterValue("input_gain")->load());
    smoothMaster.setTargetValue(apvts.getRawParameterValue("master")->load());

    tempo.update(*this, (double) apvts.getRawParameterValue("tempo_bpm")->load());

    // Resolver tempo do delay: nota musical (via BPM) ou ms livres.
    auto noteIdx = (int) apvts.getRawParameterValue("fwd_delay_note")->load();
    float delayMs;
    if (noteIdx == (int) TempoInfo::Note::Free)
        delayMs = apvts.getRawParameterValue("fwd_delay_time")->load();
    else
        delayMs = (float) (TempoInfo::beatsToSeconds(
            TempoInfo::noteToBeats((TempoInfo::Note) noteIdx), tempo.bpm) * 1000.0);

    fwdDelay.setTimeMs(delayMs);
    fwdDelay.setFeedback(apvts.getRawParameterValue("fwd_delay_fb")->load());
    fwdDelay.setDampingHz(apvts.getRawParameterValue("fwd_delay_damp")->load());
    fwdDelay.setMix(apvts.getRawParameterValue("fwd_delay_mix")->load() *
                    apvts.getRawParameterValue("fwd_mix")->load());
    fwdDelay.setFrozen(apvts.getRawParameterValue("fwd_delay_freeze")->load() > 0.5f);
    fwdDelay.setAlgo((Delay::Algo) (int) apvts.getRawParameterValue("fwd_delay_algo")->load());
    fwdDelay.setDrive(apvts.getRawParameterValue("fwd_delay_drive")->load());
    fwdDelay.setWowRate(apvts.getRawParameterValue("fwd_delay_wow_rate")->load());
    fwdDelay.setWowDepthMs(apvts.getRawParameterValue("fwd_delay_wow_depth")->load());
    fwdDelay.setSpread(apvts.getRawParameterValue("fwd_delay_spread")->load());
    fwdDelay.setTempoBpm(tempo.bpm);

    // input_gain → gate → delay (com o seu mix interno) → master.
    // O smoothing avança 1× por amostra (mesmo ganho em todos os canais).
    const int nCh = buffer.getNumChannels();
    const int n = buffer.getNumSamples();
    for (int i = 0; i < n; ++i)
    {
        const float g = smoothInput.getNextValue();
        for (int ch = 0; ch < nCh; ++ch)
            buffer.getWritePointer(ch)[i] *= g;
    }

    // --- Fase 5 (captura Dry): lê-se aqui porque o buffer muda a seguir.
    // Off = custo zero (nem grava); Dry grava pós-gain, PostFWD grava depois.
    const int revMode = (int) apvts.getRawParameterValue("rev_mode")->load();
    const int revSource = (int) apvts.getRawParameterValue("rev_source")->load();
    if (revMode != 0 && revSource == 0)
        revEngine.recordBlock(buffer);

    // --- Fase 6: a cadeia FWD corre num scratch (ordem configurável). ---
    const int chainOrder = juce::jlimit(0, 3,
        (int) apvts.getRawParameterValue("chain_order")->load());
    if (fwdBuf.getNumChannels() != 2 || fwdBuf.getNumSamples() < n)
        fwdBuf.setSize(2, n, false, false, true);
    for (int ch = 0; ch < nCh; ++ch)
        fwdBuf.copyFrom(ch, 0, buffer, ch, 0, n);

    // --- Fase 2: gater ANTES do delay (tails não são cortados). ---
    fwdGate.setRate((TempoInfo::Note) (int) apvts.getRawParameterValue("fwd_gate_rate")->load());
    fwdGate.setSteps(apvts.getRawParameterValue("fwd_gate_steps")->load() > 0.5f ? 16 : 8);
    fwdGate.setPattern((int) apvts.getRawParameterValue("fwd_gate_pattern")->load());
    fwdGate.setSmooth(apvts.getRawParameterValue("fwd_gate_smooth")->load());
    fwdGate.setDepth(apvts.getRawParameterValue("fwd_gate_depth")->load());
    fwdGate.setMix(apvts.getRawParameterValue("fwd_gate_mix")->load());
    fwdGate.setPanAlt(apvts.getRawParameterValue("fwd_gate_pan")->load());
    fwdGate.setTrigMode((Gater::TrigMode) (int) apvts.getRawParameterValue("fwd_gate_trig")->load());
    fwdGate.setEnvThrDb(apvts.getRawParameterValue("fwd_gate_env_thr")->load());
    fwdGate.setInternalBpm(tempo.bpm);
    fwdGate.scanMidi(midi);
    // (process() corre no runner da fase 6, pela ordem configurada)

    gateStepUi.store(fwdGate.getCurrentStep());
    bpmUi.store((float) tempo.bpm);

    // --- Fase 3: reverb DEPOIS do delay. Mudanças estruturais (algo/size)
    // fazem fade + reset dentro do wrapper (sem cliques).
    {
        int algo = (int) apvts.getRawParameterValue("fwd_verb_algo")->load();
        float size01 = apvts.getRawParameterValue("fwd_verb_size")->load();
        int bucket = (int) (size01 * 20.f + 0.5f);
        if (algo != lastVerbAlgo)
        {
            fwdVerb.setAlgo((reverb::Reverb::Algo) algo);
            fwdVerb.noteStructuralChange(true);
            lastVerbAlgo = algo;
            lastVerbSizeBucket = bucket;
        }
        else if (bucket != lastVerbSizeBucket)
        {
            fwdVerb.noteStructuralChange(false);
            lastVerbSizeBucket = bucket;
        }
        fwdVerb.setSize01(size01);
        fwdVerb.setT60(apvts.getRawParameterValue("fwd_verb_decay")->load());
        fwdVerb.setDamp01(apvts.getRawParameterValue("fwd_verb_damp")->load());
        fwdVerb.setWidth01(apvts.getRawParameterValue("fwd_verb_width")->load());

        auto preNote = (int) apvts.getRawParameterValue("fwd_verb_predelay_note")->load();
        double preSec;
        if (preNote == (int) TempoInfo::Note::Free)
            preSec = apvts.getRawParameterValue("fwd_verb_predelay")->load() * 0.001;
        else
            preSec = TempoInfo::beatsToSeconds(
                TempoInfo::noteToBeats((TempoInfo::Note) preNote), tempo.bpm);
        double sr = getSampleRate();
        fwdVerb.setPredelaySamples((int) juce::jlimit(0.0, 2.0 * sr - 1.0, preSec * sr));

        fwdVerb.setFrozen(apvts.getRawParameterValue("fwd_verb_freeze")->load() > 0.5f);
        fwdVerb.setMix(apvts.getRawParameterValue("fwd_verb_mix")->load() *
                       apvts.getRawParameterValue("fwd_mix")->load());
        fwdVerb.setTone(apvts.getRawParameterValue("fwd_verb_locut")->load(),
                        apvts.getRawParameterValue("fwd_verb_hicut")->load());
        // (process() no runner)
    }

    // --- Fase 4: granular/glitch POR ÚLTIMO (trabalha sobre o som com espaço).
    fwdGran.setMode((Granular::Mode) (int) apvts.getRawParameterValue("gr_mode")->load());
    fwdGran.setTrig((Granular::Trig) (int) apvts.getRawParameterValue("gr_trigger")->load());
    fwdGran.setManual(apvts.getRawParameterValue("gr_manual")->load() > 0.5f);
    fwdGran.setChance(apvts.getRawParameterValue("gr_chance")->load());
    fwdGran.setEnvThrDb(apvts.getRawParameterValue("gr_env_thr")->load());
    fwdGran.setLenNote((TempoInfo::Note) (int) apvts.getRawParameterValue("gr_len_note")->load());
    fwdGran.setRepeats((int) apvts.getRawParameterValue("gr_repeats")->load());
    fwdGran.setDecay(apvts.getRawParameterValue("gr_decay")->load());
    fwdGran.setTimeMs(apvts.getRawParameterValue("gr_time")->load());
    fwdGran.setTimeNote((TempoInfo::Note) (int) apvts.getRawParameterValue("gr_time_note")->load());
    fwdGran.setPitchSt(apvts.getRawParameterValue("gr_pitch")->load());
    fwdGran.setFlux(apvts.getRawParameterValue("gr_flux")->load());
    fwdGran.setXfadeMs(apvts.getRawParameterValue("gr_xfade")->load());
    fwdGran.setMix(apvts.getRawParameterValue("gr_mix")->load());
    fwdGran.setInterrupt(apvts.getRawParameterValue("gr_interrupt")->load() > 0.5f);
    fwdGran.setInternalBpm(tempo.bpm);

    // --- Fase 6: corre a cadeia FWD no scratch, pela ordem configurada.
    // 0 = Gate·Delay·Verb·Gran | 1 = G·V·D·Gr | 2 = D·G·V·Gr | 3 = V·D·G·Gr.
    switch (chainOrder)
    {
        case 1:
            fwdGate.process(fwdBuf, tempo);
            fwdVerb.process(fwdBuf);
            fwdDelay.process(fwdBuf);
            fwdGran.process(fwdBuf, tempo);
            break;
        case 2:
            fwdDelay.process(fwdBuf);
            fwdGate.process(fwdBuf, tempo);
            fwdVerb.process(fwdBuf);
            fwdGran.process(fwdBuf, tempo);
            break;
        case 3:
            fwdVerb.process(fwdBuf);
            fwdDelay.process(fwdBuf);
            fwdGate.process(fwdBuf, tempo);
            fwdGran.process(fwdBuf, tempo);
            break;
        default:
            fwdGate.process(fwdBuf, tempo);
            fwdDelay.process(fwdBuf);
            fwdVerb.process(fwdBuf);
            fwdGran.process(fwdBuf, tempo);
            break;
    }
    granActiveUi.store(fwdGran.isActive());

    // --- Fase 5: motor REV paralelo (cadeia gémea sobre sinal reverso). ---
    if (revSource == 1 && revMode != 0)
        revEngine.recordBlock(fwdBuf); // captura PostFWD (= saída da cadeia)

    if (revMode != 0)
    {
        auto RV = [&](const char* id) -> float
        { return apvts.getRawParameterValue(id)->load(); };
        auto RB = [&](const char* id) -> bool { return RV(id) > 0.5f; };
        using RL = RevLinker;
        using N = TempoInfo::Note;

        bool linkM = RB("link_master");
        bool lGate = linkM && RB("link_gate");
        bool lDelay = linkM && RB("link_delay");
        bool lVerb = linkM && RB("link_verb");
        bool lGran = linkM && RB("link_gran");
        float morph = RV("morph");
        float trimDly = RV("trim_delay");
        float trimDec = RV("trim_decay");

        revEngine.setMode((ReverseEngine::Mode) revMode);
        revEngine.setRate(RV("rev_rate"));
        revEngine.setLfoDepth(RV("rev_lfo"));
        const double capBeats[3] = { 2.0, 3.0, 4.0 };
        revEngine.setCaptureBeats(capBeats[juce::jlimit(0, 2, (int) RV("rev_capture"))]);
        revEngine.setDuckDepth(RV("rev_duck"));
        revEngine.setThrowButton(RB("rev_throw"));

        if (revBuf.getNumChannels() != 2 || revBuf.getNumSamples() < n)
            revBuf.setSize(2, n, false, false, true);
        juce::AudioBuffer<float> revView(revBuf.getArrayOfWritePointers(), 2, 0, n);

        bool midiThrow = false;
        for (auto meta : midi)
            if (meta.getMessage().isNoteOn()) { midiThrow = true; break; }
        revEngine.renderBlock(revView, tempo, tempo.bpm, midiThrow);

        // Gate REV (mesma ordem da FWD).
        revGate.setRate((N) RL::resolveDiscrete(lGate, (int) RV("fwd_gate_rate"), (int) RV("rev_gate_rate")));
        revGate.setSteps(RL::resolveDiscrete(lGate, (int) RV("fwd_gate_steps"), (int) RV("rev_gate_steps")) > 0 ? 16 : 8);
        revGate.setPattern(RL::resolveDiscrete(lGate, (int) RV("fwd_gate_pattern"), (int) RV("rev_gate_pattern")));
        revGate.setSmooth(RL::resolveContinuous(lGate, morph, RV("fwd_gate_smooth"), RV("rev_gate_smooth"), 0.f, 1.f));
        revGate.setDepth(RL::resolveContinuous(lGate, morph, RV("fwd_gate_depth"), RV("rev_gate_depth"), 0.f, 1.f));
        revGate.setMix(RL::resolveContinuous(lGate, morph, RV("fwd_gate_mix"), RV("rev_gate_mix"), 0.f, 1.f));
        revGate.setPanAlt(RL::resolveContinuous(lGate, morph, RV("fwd_gate_pan"), RV("rev_gate_pan"), 0.f, 1.f));
        revGate.setTrigMode((Gater::TrigMode) RL::resolveDiscrete(lGate, (int) RV("fwd_gate_trig"), (int) RV("rev_gate_trig")));
        revGate.setEnvThrDb(RL::resolveContinuous(lGate, morph, RV("fwd_gate_env_thr"), RV("rev_gate_env_thr"), -60.f, 0.f));
        revGate.setInternalBpm(tempo.bpm);
        revGate.scanMidi(midi);
        // (process() no switch em baixo, mesma ordem da FWD)

        // Delay REV (tempo com trim só em link).
        {
            int dNote = RL::resolveDiscrete(lDelay, (int) RV("fwd_delay_note"), (int) RV("rev_delay_note"));
            float dMs;
            if (dNote == (int) N::Free)
                dMs = lDelay ? RV("fwd_delay_time") : RV("rev_delay_time");
            else
                dMs = (float) (TempoInfo::beatsToSeconds(
                    TempoInfo::noteToBeats((N) dNote), tempo.bpm) * 1000.0);
            if (lDelay)
                dMs *= trimDly;
            revDelay.setTimeMs(juce::jlimit(1.f, 2200.f, dMs));
            revDelay.setFeedback(RL::resolveContinuous(lDelay, morph, RV("fwd_delay_fb"), RV("rev_delay_fb"), 0.f, 0.95f));
            revDelay.setDampingHz(RL::resolveContinuous(lDelay, morph, RV("fwd_delay_damp"), RV("rev_delay_damp"), 200.f, 18000.f));
            revDelay.setMix(RL::resolveContinuous(lDelay, morph, RV("fwd_delay_mix"), RV("rev_delay_mix"), 0.f, 1.f));
            revDelay.setFrozen(RL::resolveBool(lDelay, RB("fwd_delay_freeze"), RB("rev_delay_freeze")));
            revDelay.setAlgo((Delay::Algo) RL::resolveDiscrete(lDelay, (int) RV("fwd_delay_algo"), (int) RV("rev_delay_algo")));
            revDelay.setDrive(RL::resolveContinuous(lDelay, morph, RV("fwd_delay_drive"), RV("rev_delay_drive"), 0.f, 1.f));
            revDelay.setWowRate(RL::resolveContinuous(lDelay, morph, RV("fwd_delay_wow_rate"), RV("rev_delay_wow_rate"), 0.1f, 5.f));
            revDelay.setWowDepthMs(RL::resolveContinuous(lDelay, morph, RV("fwd_delay_wow_depth"), RV("rev_delay_wow_depth"), 0.f, 20.f));
            revDelay.setSpread(RL::resolveContinuous(lDelay, morph, RV("fwd_delay_spread"), RV("rev_delay_spread"), 0.f, 1.f));
            revDelay.setTempoBpm(tempo.bpm);
            // (process() no switch em baixo)
        }

        // Verb REV (estrutural próprio: algo/size com fade+reset).
        {
            int algo = RL::resolveDiscrete(lVerb, (int) RV("fwd_verb_algo"), (int) RV("rev_verb_algo"));
            float size01 = RL::resolveContinuous(lVerb, morph, RV("fwd_verb_size"), RV("rev_verb_size"), 0.f, 1.f);
            int bucket = (int) (size01 * 20.f + 0.5f);
            if (algo != lastRevAlgo)
            {
                revVerb.setAlgo((reverb::Reverb::Algo) algo);
                revVerb.noteStructuralChange(true);
                lastRevAlgo = algo;
                lastRevSizeBucket = bucket;
            }
            else if (bucket != lastRevSizeBucket)
            {
                revVerb.noteStructuralChange(false);
                lastRevSizeBucket = bucket;
            }
            revVerb.setSize01(size01);
            float dec = lVerb ? RV("fwd_verb_decay") * trimDec : RV("rev_verb_decay");
            revVerb.setT60(juce::jlimit(0.05, 30.0, (double) dec));
            revVerb.setDamp01(RL::resolveContinuous(lVerb, morph, RV("fwd_verb_damp"), RV("rev_verb_damp"), 0.f, 1.f));
            revVerb.setWidth01(RL::resolveContinuous(lVerb, morph, RV("fwd_verb_width"), RV("rev_verb_width"), 0.f, 1.f));
            int preNote = RL::resolveDiscrete(lVerb, (int) RV("fwd_verb_predelay_note"), (int) RV("rev_verb_predelay_note"));
            double preSec;
            if (preNote == (int) N::Free)
                preSec = (lVerb ? RV("fwd_verb_predelay") : RV("rev_verb_predelay")) * 0.001;
            else
                preSec = TempoInfo::beatsToSeconds(TempoInfo::noteToBeats((N) preNote), tempo.bpm);
            revVerb.setPredelaySamples((int) juce::jlimit(0.0, 2.0 * getSampleRate() - 1.0, preSec * getSampleRate()));
            revVerb.setFrozen(RL::resolveBool(lVerb, RB("fwd_verb_freeze"), RB("rev_verb_freeze")));
            revVerb.setMix(RL::resolveContinuous(lVerb, morph, RV("fwd_verb_mix"), RV("rev_verb_mix"), 0.f, 1.f));
            revVerb.setTone(RL::resolveContinuous(lVerb, morph, RV("fwd_verb_locut"), RV("rev_verb_locut"), 20.f, 500.f),
                            RL::resolveContinuous(lVerb, morph, RV("fwd_verb_hicut"), RV("rev_verb_hicut"), 2000.f, 20000.f));
            // (process() no switch em baixo)
        }

        // Granular REV.
        revGran.setMode((Granular::Mode) RL::resolveDiscrete(lGran, (int) RV("gr_mode"), (int) RV("rev_gr_mode")));
        revGran.setTrig((Granular::Trig) RL::resolveDiscrete(lGran, (int) RV("gr_trigger"), (int) RV("rev_gr_trigger")));
        revGran.setManual(RL::resolveBool(lGran, RB("gr_manual"), RB("rev_gr_manual")));
        revGran.setChance(RL::resolveContinuous(lGran, morph, RV("gr_chance"), RV("rev_gr_chance"), 0.f, 1.f));
        revGran.setEnvThrDb(RL::resolveContinuous(lGran, morph, RV("gr_env_thr"), RV("rev_gr_env_thr"), -60.f, 0.f));
        revGran.setLenNote((N) RL::resolveDiscrete(lGran, (int) RV("gr_len_note"), (int) RV("rev_gr_len_note")));
        revGran.setRepeats(RL::resolveDiscrete(lGran, (int) RV("gr_repeats"), (int) RV("rev_gr_repeats")));
        revGran.setDecay(RL::resolveContinuous(lGran, morph, RV("gr_decay"), RV("rev_gr_decay"), 0.5f, 0.99f));
        revGran.setTimeMs(RL::resolveContinuous(lGran, morph, RV("gr_time"), RV("rev_gr_time"), 60.f, 8000.f));
        revGran.setTimeNote((N) RL::resolveDiscrete(lGran, (int) RV("gr_time_note"), (int) RV("rev_gr_time_note")));
        revGran.setPitchSt(RL::resolveContinuous(lGran, morph, RV("gr_pitch"), RV("rev_gr_pitch"), -12.f, 12.f));
        revGran.setFlux(RL::resolveContinuous(lGran, morph, RV("gr_flux"), RV("rev_gr_flux"), 0.f, 1.f));
        revGran.setXfadeMs(RL::resolveContinuous(lGran, morph, RV("gr_xfade"), RV("rev_gr_xfade"), 1.f, 50.f));
        revGran.setMix(RL::resolveContinuous(lGran, morph, RV("gr_mix"), RV("rev_gr_mix"), 0.f, 1.f));
        revGran.setInterrupt(RL::resolveBool(lGran, RB("gr_interrupt"), RB("rev_gr_interrupt")));
        revGran.setInternalBpm(tempo.bpm);

        // Cadeia gémea na mesma ordem da FWD.
        switch (chainOrder)
        {
            case 1:
                revGate.process(revView, tempo);
                revVerb.process(revView);
                revDelay.process(revView);
                revGran.process(revView, tempo);
                break;
            case 2:
                revDelay.process(revView);
                revGate.process(revView, tempo);
                revVerb.process(revView);
                revGran.process(revView, tempo);
                break;
            case 3:
                revVerb.process(revView);
                revDelay.process(revView);
                revGate.process(revView, tempo);
                revGran.process(revView, tempo);
                break;
            default:
                revGate.process(revView, tempo);
                revDelay.process(revView);
                revVerb.process(revView);
                revGran.process(revView, tempo);
                break;
        }
    }

    // --- Fase 6: mistura final. ADD: dry + FWD + REV. XFADE: beats pares
    // levam FWD, ímpares levam REV (crossfade 10 ms); o dry fica sempre.
    // Sem REV ativo, XFADE comporta-se como ADD.
    int xMode = (int) apvts.getRawParameterValue("x_mode")->load();
    long beatIdx;
    if (tempo.fromHost && tempo.isPlaying)
        beatIdx = (long) std::floor(tempo.ppqPosition);
    else
        beatIdx = (long) std::floor(procBeats);
    procBeats += (double) n * tempo.bpm / 60.0 / getSampleRate();
    bool xfade = (xMode == 1) && (revMode != 0);
    bool even = (((beatIdx % 2) + 2) % 2) == 0;
    xfFwd.setTargetValue(! xfade || even ? 1.f : 0.f);
    xfRev.setTargetValue(! xfade || ! even ? 1.f : 0.f);
    smoothRevMix.setTargetValue(apvts.getRawParameterValue("rev_mix")->load());

    for (int i = 0; i < n; ++i)
    {
        const float gM = smoothMaster.getNextValue();
        const float gF = xfFwd.getNextValue();
        const float gR = xfRev.getNextValue() * smoothRevMix.getNextValue();
        for (int ch = 0; ch < nCh; ++ch)
        {
            float dry = buffer.getWritePointer(ch)[i];
            float fw = fwdBuf.getWritePointer(ch)[i];
            float rv = (revMode != 0) ? revBuf.getWritePointer(ch)[i] : 0.f;
            buffer.getWritePointer(ch)[i] = (dry * (1.f - gF) + fw * gF + rv * gR) * gM;
        }
    }
}

juce::AudioProcessorEditor* DeVerbProcessor::createEditor()
{
    return new DeVerbEditor(*this);
}

void DeVerbProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary(*xml, destData);
}

void DeVerbProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        if (xml->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DeVerbProcessor();
}
