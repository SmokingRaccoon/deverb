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
    std::vector<std::pair<std::string, float>> values;
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

inline void applyRealList(juce::AudioProcessorValueTreeState& apvts,
                          const std::vector<std::pair<std::string, float>>& values)
{
    for (auto& [id, real] : values)
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

inline void applyFactoryPreset(juce::AudioProcessorValueTreeState& apvts, int index)
{
    const auto& presets = deVerbFactoryPresets();
    if (index < 0 || index >= (int) presets.size())
        return;
    applyRealList(apvts, presets[(size_t) index].values);
}

// RANDOM civilizado: gamas musicais que garantem som audível (nunca silêncio
// acidental). gain staging fixo; mixes sempre > 0; pattern com 4–12 passos;
// freeze/interrupt/manual sempre OFF; morph contido para o REV seguir o FWD.
// `wildness` 0 = civilizado (hoje); 1 = tudo ao calhas (reservado p/ futuro).
inline std::vector<std::pair<std::string, float>>
civilizedRandom(juce::Random& rng, float wildness = 0.f)
{
    auto fr = [&](float lo, float hi) { return lo + rng.nextFloat() * (hi - lo); };
    auto pick = [&](std::initializer_list<int> xs)
    {
        std::vector<int> v(xs);
        size_t i = (size_t) (rng.nextFloat() * (float) v.size());
        return v[juce::jmin(i, v.size() - 1)];
    };
    auto chance = [&](float p) { return rng.nextFloat() < p; };
    auto popcount16 = [](uint16_t x)
    {
        int c = 0;
        while (x != 0) { c += (x & 1u); x >>= 1; }
        return c;
    };
    juce::ignoreUnused(wildness);

    using P = std::pair<std::string, float>;
    std::vector<P> out;
    out.reserve(115);

    // Gain staging: sempre audível (mixes e PWRs forçados: com tudo a 0 ou
    // desligado o RANDOM anterior podia sair mudo se o utilizador tivesse
    // zerado algo antes).
    out.emplace_back("master", 0.8f);
    out.emplace_back("input_gain", 1.f);
    out.emplace_back("fwd_mix", fr(0.7f, 1.f));
    for (auto id : { "fwd_gate_on", "fwd_delay_on", "fwd_verb_on", "gr_on",
                     "rev_gate_on", "rev_delay_on", "rev_verb_on", "rev_gr_on" })
        out.emplace_back(id, 1.f);
    out.emplace_back("tempo_bpm", fr(90.f, 140.f));
    out.emplace_back("chain_order", (float) pick({ 0, 1, 2, 3 }));
    out.emplace_back("x_mode", chance(0.25f) ? 1.f : 0.f);

    // Gate: pattern com densidade garantida (4–12 passos em 16).
    {
        uint16_t bits = 0;
        int want = 4 + (int) (rng.nextFloat() * 9.f); // 4..12
        while (popcount16(bits) < want)
            bits |= (uint16_t) (1u << (int) (rng.nextFloat() * 16.f));
        out.emplace_back("fwd_gate_pattern", (float) bits);
        out.emplace_back("rev_gate_pattern", (float) bits); // mesmo ritmo base
        out.emplace_back("fwd_gate_rate", (float) pick({ 6, 3, 7, 4 }));
        out.emplace_back("rev_gate_rate", (float) pick({ 6, 3, 7, 4 }));
        out.emplace_back("fwd_gate_steps", 1.f);
        out.emplace_back("rev_gate_steps", 1.f);
        out.emplace_back("fwd_gate_smooth", fr(0.05f, 0.3f));
        out.emplace_back("fwd_gate_depth", fr(0.7f, 1.f));
        out.emplace_back("fwd_gate_mix", fr(0.5f, 1.f));
        out.emplace_back("rev_gate_mix", fr(0.5f, 1.f));
        out.emplace_back("fwd_gate_pan", fr(0.f, 0.5f));
        out.emplace_back("rev_gate_pan", fr(0.f, 0.5f));
        out.emplace_back("fwd_gate_trig", 0.f);
        out.emplace_back("rev_gate_trig", 0.f);
        out.emplace_back("fwd_gate_env_thr", fr(-24.f, -12.f));
        out.emplace_back("rev_gate_env_thr", fr(-24.f, -12.f));
    }
    // Delay: sync musical, feedback domado, freeze OFF.
    for (auto pre : { "fwd_", "rev_" })
    {
        auto id = [&](const char* s) { return std::string(pre) + s; };
        out.emplace_back(id("delay_algo"),
                         (float) pick({ 0, 0, 0, 1, 1, 2, 2, 3, 4 }));
        out.emplace_back(id("delay_note"), (float) pick({ 3, 6, 7, 8, 9 }));
        out.emplace_back(id("delay_time"), 375.f);
        out.emplace_back(id("delay_fb"), fr(0.2f, 0.65f));
        out.emplace_back(id("delay_damp"), fr(2000.f, 12000.f));
        out.emplace_back(id("delay_mix"), fr(0.15f, 0.5f));
        out.emplace_back(id("delay_freeze"), 0.f);
        out.emplace_back(id("delay_drive"), fr(0.1f, 0.6f));
        out.emplace_back(id("delay_wow_rate"), fr(0.3f, 2.f));
        out.emplace_back(id("delay_wow_depth"), fr(1.f, 8.f));
        out.emplace_back(id("delay_spread"), fr(0.6f, 1.f));
    }
    // Verb: caudas médias, freeze OFF.
    for (auto pre : { "fwd_", "rev_" })
    {
        auto id = [&](const char* s) { return std::string(pre) + s; };
        out.emplace_back(id("verb_algo"), (float) pick({ 0, 1, 1, 2, 3 }));
        out.emplace_back(id("verb_size"), fr(0.3f, 0.7f));
        out.emplace_back(id("verb_decay"), fr(0.8f, 5.f));
        out.emplace_back(id("verb_damp"), fr(0.2f, 0.6f));
        out.emplace_back(id("verb_width"), fr(0.7f, 1.f));
        out.emplace_back(id("verb_predelay"), fr(0.f, 60.f));
        out.emplace_back(id("verb_predelay_note"), 0.f);
        out.emplace_back(id("verb_freeze"), 0.f);
        out.emplace_back(id("verb_mix"), fr(0.2f, 0.5f));
        out.emplace_back(id("verb_locut"), fr(40.f, 150.f));
        out.emplace_back(id("verb_hicut"), fr(8000.f, 18000.f));
    }
    // Granular: sem interrupt/manual (fontes de silêncio), chance moderada.
    for (auto pre : { "gr_", "rev_gr_" })
    {
        auto id = [&](const char* s) { return std::string(pre) + s; };
        out.emplace_back(id("mode"), (float) pick({ 0, 0, 0, 1, 1, 1, 2, 3, 4, 5 }));
        out.emplace_back(id("trigger"), 0.f);
        out.emplace_back(id("manual"), 0.f);
        out.emplace_back(id("chance"), fr(0.1f, 0.5f));
        out.emplace_back(id("env_thr"), fr(-24.f, -12.f));
        out.emplace_back(id("len_note"), (float) pick({ 1, 3, 6 }));
        out.emplace_back(id("repeats"), (float) (2 + (int) (rng.nextFloat() * 7.f)));
        out.emplace_back(id("decay"), fr(0.8f, 0.95f));
        out.emplace_back(id("time"), fr(150.f, 600.f));
        out.emplace_back(id("time_note"), 0.f);
        out.emplace_back(id("pitch"), (float) pick({ -12, -5, 0, 7, 12 }));
        out.emplace_back(id("flux"), fr(0.1f, 0.5f));
        out.emplace_back(id("xfade"), fr(4.f, 15.f));
        out.emplace_back(id("mix"), fr(0.2f, 0.6f));
        out.emplace_back(id("interrupt"), 0.f);
    }
    // Motor REV audível mas contido; links quase sempre ON + morph baixo.
    out.emplace_back("rev_mode", (float) pick({ 1, 1, 1, 1, 1, 0, 0, 0, 2, 2 }));
    out.emplace_back("rev_source", chance(0.25f) ? 1.f : 0.f);
    out.emplace_back("rev_capture", (float) pick({ 0, 0, 1 }));
    out.emplace_back("rev_rate", fr(0.7f, 1.4f));
    out.emplace_back("rev_lfo", fr(0.f, 0.3f));
    out.emplace_back("rev_mix", fr(0.2f, 0.5f));
    out.emplace_back("rev_throw", 0.f);
    out.emplace_back("rev_duck", fr(0.1f, 0.4f));
    bool master = ! chance(0.2f);
    out.emplace_back("link_master", master ? 1.f : 0.f);
    for (auto id : { "link_gate", "link_delay", "link_verb", "link_gran" })
        out.emplace_back(id, (! master || chance(0.85f)) ? 1.f : 0.f);
    out.emplace_back("morph", fr(0.f, 0.5f));
    out.emplace_back("trim_delay", fr(0.8f, 1.25f));
    out.emplace_back("trim_decay", fr(0.8f, 1.25f));
    return out;
}
