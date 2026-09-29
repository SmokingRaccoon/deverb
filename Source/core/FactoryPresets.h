#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <utility>

// Presets de fábrica do deVerb (Fase 6). Cada preset é uma lista de
// (paramID, valor REAL); a normalização usa o próprio range do parâmetro
// (respeita skews). Só guarda deltas — o resto fica nos defaults.
// Aplicar = setValueNotifyingHost (automatizável e visível na UI).
struct FactoryPreset
{
    const char* name;
    std::vector<std::pair<const char*, float>> values;
};

inline const std::vector<FactoryPreset>& deVerbFactoryPresets()
{
    // Índices: notas Free0 1/32:1 1/16T:2 1/16:3 1/16D:4 1/8T:5 1/8:6 1/8D:7
    //   1/4:8 1/4D:9 1/2:10 1/1:11 | verb Room0 Hall1 Plate2 Shimmer3
    //   gran Off0 BR1 Slice2 Rev3 Pitch4 Stut5 | trig Chance0 Env1 Manual2
    //   rev Off0 Loop1 Throw2 | source Dry0 Post1 | capture 2/3/4 beats = 0/1/2
    static const std::vector<FactoryPreset> presets = {
        { "Init: Clean Slate",
          { { "fwd_gate_mix", 0.f }, { "fwd_delay_mix", 0.f },
            { "fwd_verb_mix", 0.f }, { "master", 0.8f } } },
        { "Trance Gate 16",
          { { "fwd_gate_rate", 3.f }, { "fwd_gate_steps", 1.f },
            { "fwd_gate_pattern", 0x1111 }, { "fwd_gate_mix", 1.f },
            { "fwd_gate_depth", 1.f }, { "fwd_delay_note", 7.f },
            { "fwd_delay_mix", 0.25f }, { "fwd_verb_algo", 1.f },
            { "fwd_verb_decay", 1.8f }, { "fwd_verb_mix", 0.2f } } },
        { "Offbeat Chop",
          { { "fwd_gate_rate", 3.f }, { "fwd_gate_pattern", 0x4444 },
            { "fwd_gate_mix", 1.f }, { "fwd_delay_note", 5.f },
            { "fwd_delay_fb", 0.45f }, { "fwd_delay_mix", 0.3f },
            { "fwd_verb_algo", 2.f }, { "fwd_verb_mix", 0.25f } } },
        { "Big Hall Space",
          { { "fwd_gate_mix", 0.f }, { "fwd_delay_note", 8.f },
            { "fwd_delay_mix", 0.2f }, { "fwd_verb_algo", 1.f },
            { "fwd_verb_size", 0.7f }, { "fwd_verb_decay", 6.f },
            { "fwd_verb_mix", 0.45f }, { "fwd_verb_predelay", 40.f } } },
        { "Reverse Tail",
          { { "rev_mode", 1.f }, { "rev_mix", 0.5f }, { "rev_capture", 0.f },
            { "rev_rate", 1.f }, { "rev_duck", 0.4f },
            { "fwd_verb_mix", 0.2f } } },
        { "Reverse Throw",
          { { "rev_mode", 2.f }, { "rev_throw", 1.f }, { "rev_mix", 0.6f },
            { "rev_capture", 1.f }, { "rev_verb_decay", 4.f } } },
        { "Beat Repeat",
          { { "gr_mode", 1.f }, { "gr_trigger", 0.f }, { "gr_chance", 0.6f },
            { "gr_len_note", 3.f }, { "gr_repeats", 4.f }, { "gr_decay", 0.85f },
            { "gr_mix", 0.6f } } },
        { "Stutter Brk",
          { { "gr_mode", 5.f }, { "gr_trigger", 0.f }, { "gr_chance", 0.8f },
            { "gr_interrupt", 1.f }, { "gr_mix", 0.7f }, { "gr_len_note", 6.f } } },
        { "Dub Echo",
          { { "fwd_gate_mix", 0.f }, { "fwd_delay_note", 7.f },
            { "fwd_delay_fb", 0.7f }, { "fwd_delay_damp", 2500.f },
            { "fwd_delay_mix", 0.4f }, { "fwd_verb_algo", 0.f },
            { "fwd_verb_mix", 0.15f } } },
        { "Shimmer Pad",
          { { "fwd_gate_mix", 0.f }, { "fwd_delay_note", 9.f },
            { "fwd_delay_mix", 0.15f }, { "fwd_verb_algo", 3.f },
            { "fwd_verb_decay", 8.f }, { "fwd_verb_size", 0.8f },
            { "fwd_verb_damp", 0.2f }, { "fwd_verb_mix", 0.5f } } },
        { "Build Up",
          { { "fwd_gate_rate", 1.f }, { "fwd_gate_pattern", 0x844B },
            { "fwd_gate_smooth", 0.05f }, { "fwd_gate_mix", 1.f },
            { "fwd_verb_decay", 4.f }, { "fwd_verb_mix", 0.4f },
            { "gr_chance", 0.4f }, { "gr_len_note", 1.f } } },
    };
    return presets;
}

inline void applyFactoryPreset(juce::AudioProcessorValueTreeState& apvts, int index)
{
    const auto& presets = deVerbFactoryPresets();
    if (index < 0 || index >= (int) presets.size())
        return;
    for (auto& [id, real] : presets[(size_t) index].values)
    {
        auto* par = apvts.getParameter(id);
        if (par == nullptr)
            continue;
        if (auto* f = dynamic_cast<juce::AudioParameterFloat*>(par))
            par->setValueNotifyingHost(f->getNormalisableRange().convertTo0to1(real));
        else if (auto* c = dynamic_cast<juce::AudioParameterChoice*>(par))
            par->setValueNotifyingHost(c->convertTo0to1((int) real));
        else if (auto* b = dynamic_cast<juce::AudioParameterBool*>(par))
            par->setValueNotifyingHost(real > 0.5f ? 1.f : 0.f);
        else if (auto* ni = dynamic_cast<juce::AudioParameterInt*>(par))
            par->setValueNotifyingHost(ni->convertTo0to1((int) real));
    }
}
