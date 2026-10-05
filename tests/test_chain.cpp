// Teste ponta-a-ponta do processador completo (Fase 5): instancia o
// DeVerbProcessor real (via .a do build) e corre áudio com o REV ligado.
// Exe separado: tests/build/test_chain. Sai 0 = passa.
#include <cstdio>
#include <cmath>
#include <vector>
#include "../Source/PluginProcessor.h"
#include "../Source/core/FactoryPresets.h"

static int failures = 0;
#define CHECK(cond, ...) do { \
    if (!(cond)) { ++failures; std::printf("FAIL: " __VA_ARGS__); std::printf("\n"); } \
    else { std::printf("ok: " __VA_ARGS__); std::printf("\n"); } \
} while (0)

static void setF(DeVerbProcessor& p, const char* id, float v)
{
    if (auto* par = p.apvts.getParameter(id))
        par->setValueNotifyingHost(par->convertTo0to1(v));
}
static void setI(DeVerbProcessor& p, const char* id, int i)
{
    if (auto* par = p.apvts.getParameter(id))
        if (auto* c = dynamic_cast<juce::AudioParameterChoice*>(par))
            par->setValueNotifyingHost(c->convertTo0to1(i));
}

static double rms(const std::vector<float>& v, int from, int to)
{
    double s = 0.0;
    int c = 0;
    for (int i = from; i < to && i < (int) v.size(); ++i) { s += (double) v[(size_t) i] * v[(size_t) i]; ++c; }
    return c > 0 ? std::sqrt(s / c) : 0.0;
}

static int mainOrder();
static int mainFuzz();
static int mainCivil();
static int mainPower();
static int mainIds();
static int mainPreset();
static int mainRouting();
static int mainOut();
int main()
{
    const double sr = 48000.0;
    juce::MidiBuffer midi;

    // --- A. REV Off + tudo a 0 = passthrough bit-transparente ---
    {
        DeVerbProcessor p;
        p.prepareToPlay(sr, 512);
        setF(p, "master", 1.f);
        setF(p, "fwd_gate_mix", 0.f);
        setF(p, "fwd_delay_mix", 0.f);
        setF(p, "fwd_verb_mix", 0.f);
        // gr_mode Off + rev_mode Off já são os defaults.

        juce::AudioBuffer<float> buf(2, 512);
        bool exact = true;
        auto runSine = [&](int fromBlock)
        {
            // NOTA: amplitude 0.5 (abaixo do ceiling do limiter) para testar
            // passthrough verdadeiro; o limiter testar-se-ia sempre.
            for (int i = 0; i < 512; ++i)
            {
                float v = std::sin((fromBlock * 512 + i) * 0.01f) * 0.5f;
                buf.setSample(0, i, v);
                buf.setSample(1, i, v * 0.5f);
            }
        };
        for (int b = 0; b < 100; ++b) // assenta: rampas de 30 ms terminam aqui
        {
            runSine(b);
            p.processBlock(buf, midi);
        }
        for (int b = 100; b < 120; ++b)
        {
            runSine(b);
            juce::AudioBuffer<float> ref(2, 512);
            ref.copyFrom(0, 0, buf, 0, 0, 512);
            ref.copyFrom(1, 0, buf, 1, 0, 512);
            p.processBlock(buf, midi);
            for (int ch = 0; ch < 2 && exact; ++ch)
                for (int i = 0; i < 512; ++i)
                    if (buf.getSample(ch, i) != ref.getSample(ch, i)) exact = false;
        }
        CHECK(exact, "REV Off = passthrough exato");
    }

    // --- B. REV Loop com cadeia a 0: tail reverso audível, finito, sem NaN ---
    {
        DeVerbProcessor p;
        p.prepareToPlay(sr, 512);
        setF(p, "master", 1.f);
        setF(p, "fwd_gate_mix", 0.f);
        setF(p, "fwd_delay_mix", 0.f);
        setF(p, "fwd_verb_mix", 0.f);
        setI(p, "rev_mode", 1); // Loop
        setF(p, "rev_mix", 1.f);
        setF(p, "rev_duck", 0.f);

        std::vector<float> out;
        juce::AudioBuffer<float> buf(2, 512);
        for (int b = 0; b < 100; ++b) // ~1.07 s de seno 220 Hz
        {
            for (int i = 0; i < 512; ++i)
            {
                float v = std::sin((b * 512 + i) * 2.0f * 3.14159265f * 220.f / 48000.f);
                buf.setSample(0, i, v);
                buf.setSample(1, i, v);
            }
            p.processBlock(buf, midi);
        }
        for (int b = 0; b < 100; ++b) // 1.07 s de silêncio: só o tail REV
        {
            buf.clear();
            p.processBlock(buf, midi);
            for (int i = 0; i < 512; ++i) out.push_back(buf.getSample(0, i));
        }
        double r = rms(out, 1000, 20000);
        bool finite = true;
        float peak = 0.f;
        for (float v : out)
        {
            if (! std::isfinite(v)) finite = false;
            peak = juce::jmax(peak, std::abs(v));
        }
        CHECK(r > 0.1, "tail REV audível no silêncio (rms %f)", r);
        CHECK(finite && peak < 4.f, "tail finito sem NaN (pico %f)", peak);
    }

    // --- C. Morph/link defaults: REV espelha FWD sem explodir ---
    {
        DeVerbProcessor p;
        p.prepareToPlay(sr, 512);
        setI(p, "rev_mode", 1);
        setF(p, "rev_mix", 0.5f);
        // defaults: links on, morph 0, FWD nos defaults musicais.
        juce::AudioBuffer<float> buf(2, 512);
        float peak = 0.f;
        bool finite = true;
        for (int b = 0; b < 200; ++b)
        {
            for (int i = 0; i < 512; ++i)
            {
                float v = std::sin((b * 512 + i) * 0.02f) * 0.5f;
                buf.setSample(0, i, v);
                buf.setSample(1, i, v);
            }
            p.processBlock(buf, midi);
            for (int i = 0; i < 512; ++i)
            {
                float v = buf.getSample(0, i);
                if (! std::isfinite(v)) finite = false;
                peak = juce::jmax(peak, std::abs(v));
            }
        }
        CHECK(finite && peak < 4.f, "cadeia completa estável (pico %f)", peak);
    }

    // --- G. Bloco gigante (8k > FIFO 2k): scope não rebenta, som finito ---
    {
        DeVerbProcessor p;
        p.prepareToPlay(sr, 512);
        juce::AudioBuffer<float> buf(2, 8192);
        bool finite = true;
        for (int b = 0; b < 8; ++b)
        {
            for (int i = 0; i < 8192; ++i)
            {
                float v = std::sin((b * 8192 + i) * 0.02f) * 0.5f;
                buf.setSample(0, i, v);
                buf.setSample(1, i, v);
            }
            p.processBlock(buf, midi);
            for (int i = 0; i < 8192; ++i)
                if (! std::isfinite(buf.getSample(0, i))) finite = false;
        }
        float snap[128];
        p.getScopeSnapshot(snap, 128);
        bool snapOk = true;
        for (int i = 0; i < 128; ++i)
            if (! std::isfinite(snap[i]) || std::abs(snap[i]) > 1.f) snapOk = false;
        CHECK(finite, "bloco 8k finito");
        CHECK(snapOk, "scope snapshot sã após blocos 8k");
    }

    // --- H. Troca de algoritmo do delay a meio: sem clique, sem explosão ---
    {
        DeVerbProcessor p;
        p.prepareToPlay(sr, 512);
        setF(p, "fwd_delay_mix", 1.f);
        setF(p, "fwd_delay_fb", 0.4f);
        juce::AudioBuffer<float> buf(2, 512);
        float maxStep = 0.f, prev = 0.f;
        bool first = true;
        for (int b = 0; b < 200; ++b)
        {
            if (b == 100) setI(p, "fwd_delay_algo", 1); // Digital -> Tape
            for (int i = 0; i < 512; ++i)
            {
                float v = std::sin((b * 512 + i) * 0.03f) * 0.4f;
                buf.setSample(0, i, v);
                buf.setSample(1, i, v);
            }
            p.processBlock(buf, midi);
            for (int i = 0; i < 512; ++i)
            {
                float v = buf.getSample(0, i);
                if (! std::isfinite(v)) maxStep = 1e9f;
                if (! first) maxStep = juce::jmax(maxStep, std::abs(v - prev));
                prev = v;
                first = false;
            }
        }
        CHECK(maxStep < 0.4f, "troca de algo sem clique (maxStep %f)", maxStep);
    }

    mainOrder(); // o global `failures` já acumula; somar duplicava a conta
    mainFuzz();
    mainCivil();
    mainPower();
    mainIds();
    mainPreset();
    mainRouting();
    mainOut();

    if (failures == 0) std::printf("\nALL CHAIN TESTS PASSED\n");
    else std::printf("\n%d FAILURES\n", failures);
    return failures == 0 ? 0 : 1;
}

// --- IDs: presets/RANDOM só usam IDs reais + contagem congelada (124) ---
static int mainIds()
{
    DeVerbProcessor p;
    int missing = 0;
    for (auto& pre : deVerbFactoryPresets())
        for (auto& [id, real] : pre.values)
        {
            (void) real;
            if (p.apvts.getParameter(id) == nullptr)
            {
                std::printf("  preset '%s' usa ID inexistente: %s\n",
                            pre.name, id.c_str());
                ++missing;
            }
        }
    juce::Random rng(1234);
    for (auto& [id, real] : civilizedRandom(rng))
    {
        (void) real;
        if (p.apvts.getParameter(id) == nullptr)
        {
            std::printf("  RANDOM usa ID inexistente: %s\n", id.c_str());
            ++missing;
        }
    }
    CHECK(missing == 0, "todos os IDs de presets/RANDOM existem (%d em falta)", missing);
    int nParams = p.getParameters().size();
    CHECK(nParams == 124, "124 IDs congelados (got %d)", nParams);
    return failures;
}

// --- OUT: dimension expander alarga + limiter trava depois dele ---
static int mainOut()
{
    const double sr = 48000.0;
    juce::MidiBuffer midi;
    // Correlação L/R cai com dim_mix (width real no OUT).
    {
        DeVerbProcessor p;
        p.prepareToPlay(sr, 512);
        setF(p, "master", 1.f);
        setF(p, "fwd_mix", 0.f);
        setF(p, "fwd_gate_mix", 0.f);
        setF(p, "fwd_delay_mix", 0.f);
        setF(p, "fwd_verb_mix", 0.f);
        setF(p, "dim_size", 0.5f);
        setF(p, "dim_mix", 1.f);
        juce::AudioBuffer<float> buf(2, 512);
        double corr = 0.0, eL = 0.0, eR = 0.0;
        int nn = 0;
        for (int b = 0; b < 100; ++b)
        {
            for (int i = 0; i < 512; ++i)
            {
                float v = std::sin((b * 512 + i) * 2.f * 3.14159265f * 440.f / 48000.f) * 0.5f;
                buf.setSample(0, i, v);
                buf.setSample(1, i, v);
            }
            p.processBlock(buf, midi);
            if (b < 10) continue; // settle dos slews
            for (int i = 0; i < 512; ++i)
            {
                float l = buf.getSample(0, i), r = buf.getSample(1, i);
                corr += (double) l * r;
                eL += (double) l * l;
                eR += (double) r * r;
                ++nn;
            }
        }
        corr /= std::sqrt(eL * eR + 1e-12);
        CHECK(corr < 0.9, "OUT alarga (correlação %f)", corr);
    }
    // Com o widener quente, o limiter continua a travar a -1 dBFS.
    {
        DeVerbProcessor p;
        p.prepareToPlay(sr, 512);
        setF(p, "master", 1.f);
        setF(p, "input_gain", 2.f);
        setF(p, "dim_size", 1.f);
        setF(p, "dim_mix", 1.f);
        juce::AudioBuffer<float> buf(2, 512);
        float peak = 0.f;
        for (int b = 0; b < 100; ++b)
        {
            for (int i = 0; i < 512; ++i)
            {
                float v = std::sin((b * 512 + i) * 0.05f) * 0.9f;
                buf.setSample(0, i, v);
                buf.setSample(1, i, v);
            }
            p.processBlock(buf, midi);
            for (int i = 0; i < 512; ++i)
                peak = juce::jmax(peak, std::abs(buf.getSample(0, i)));
        }
        CHECK(peak <= 0.90f, "limiter trava com widener (pico %f)", peak);
    }
    return failures;
}

// --- Lote B: routing — POST, MIDI-throw, orders 1/3, MAN sync ---
static int mainRouting()
{
    const double sr = 48000.0;

    // B1. POST captura a saída do FWD; DRY só o input.
    // Seno 440 2 s + 1 s silêncio, FWD = granular Pitch (880) sempre a
    // agarrar (mix cortado aos 2 s para a cauda ser só REV). O anel POST
    // fica cheio de 880, o DRY só tem o seno 440: na cauda o 880 denuncia.
    // (Janela [2.05, 2.4 s]: o leitor a 1× alcança sempre a era 880, que
    // ainda não envelheceu para fora da captura de 2 beats.)
    auto e880tail = [&](int source)
    {
        DeVerbProcessor q;
        q.prepareToPlay(sr, 512);
        setF(q, "master", 1.f);
        setF(q, "fwd_mix", 0.f); // isola: out = dry + REV
        setF(q, "fwd_gate_mix", 0.f);
        setF(q, "fwd_delay_mix", 0.f);
        setF(q, "fwd_verb_mix", 0.f);
        setI(q, "gr_mode", 4); // Pitch +12
        setF(q, "gr_chance", 1.f);
        setF(q, "gr_mix", 1.f);
        setI(q, "rev_mode", 1); // Loop
        setI(q, "rev_source", source);
        setF(q, "rev_mix", 1.f);
        setF(q, "rev_duck", 0.f);
        setF(q, "rev_gate_mix", 0.f);
        setF(q, "rev_delay_mix", 0.f);
        setF(q, "rev_verb_mix", 0.f);
        juce::MidiBuffer midi;
        juce::AudioBuffer<float> b2(2, 512);
        std::vector<float> out;
        for (int k = 0; k < 144000 / 512; ++k) // 3 s: 2 s seno + 1 s nada
        {
            if (k == 96000 / 512) setF(q, "gr_mix", 0.f); // cala o FWD na cauda
            for (int i = 0; i < 512; ++i)
            {
                float v = (k * 512 + i < 96000)
                    ? std::sin((k * 512 + i) * 2.f * 3.14159265f * 440.f / 48000.f) * 0.5f : 0.f;
                b2.setSample(0, i, v);
                b2.setSample(1, i, v);
            }
            q.processBlock(b2, midi);
            for (int i = 0; i < 512; ++i) out.push_back(b2.getSample(0, i));
        }
        double w = 2.0 * 3.14159265358979 * 880.0 / 48000.0;
        double c = std::cos(w), p0 = 0.0, pp = 0.0;
        int from = 98400, to = 115200, m = 0;
        for (int i = from; i < to && i < (int) out.size(); ++i)
        {
            double x = out[(size_t) i] + 2.0 * c * p0 - pp;
            pp = p0;
            p0 = x;
            ++m;
        }
        return std::sqrt(p0 * p0 + pp * pp - 2.0 * c * p0 * pp) / juce::jmax(1, m);
    };
    double eDry = e880tail(0);
    double ePost = e880tail(1);
    CHECK(ePost > 0.02 && ePost > eDry * 5.0, "POST contém o FWD (E880 %.2e vs %.2e)", ePost, eDry);

    // B2. MIDI note-on dispara Throw no processador.
    {
        DeVerbProcessor q;
        q.prepareToPlay(sr, 512);
        setF(q, "master", 1.f);
        setF(q, "fwd_mix", 0.f);
        setF(q, "fwd_gate_mix", 0.f);
        setF(q, "fwd_delay_mix", 0.f);
        setF(q, "fwd_verb_mix", 0.f);
        setI(q, "rev_mode", 2); // Throw
        setF(q, "rev_mix", 1.f);
        setF(q, "rev_duck", 0.f);
        setF(q, "rev_gate_mix", 0.f);
        setF(q, "rev_delay_mix", 0.f);
        setF(q, "rev_verb_mix", 0.f);
        juce::MidiBuffer empty, withNote;
        withNote.addEvent(juce::MidiMessage::noteOn(1, 60, 1.0f), 0);
        juce::AudioBuffer<float> b2(2, 512);
        std::vector<float> out;
        for (int k = 0; k < 96000 / 512; ++k) // 2 s de seno (enche o anel)
        {
            for (int i = 0; i < 512; ++i)
            {
                float v = std::sin((k * 512 + i) * 0.02f) * 0.5f;
                b2.setSample(0, i, v);
                b2.setSample(1, i, v);
            }
            // A nota entra no bloco 100 (a seguir, o buffer volta a vazio).
            q.processBlock(b2, k == 100 ? withNote : empty);
            for (int i = 0; i < 512; ++i) out.push_back(b2.getSample(0, i));
        }
        for (int k = 0; k < 96000 / 512; ++k) // 2 s de silêncio
        {
            b2.clear();
            q.processBlock(b2, empty);
            for (int i = 0; i < 512; ++i) out.push_back(b2.getSample(0, i));
        }
        auto eWin = [&](int from, int to)
        {
            double e = 0.0;
            for (int i = from; i < to && i < (int) out.size(); ++i)
                e += out[(size_t) i] * out[(size_t) i];
            return e / (to - from);
        };
        int tnote = 100 * 512;
        double head = eWin(tnote + 4800, tnote + 24000);
        double tail = eWin(tnote + 72000, tnote + 96000);
        CHECK(head > 0.005, "MIDI dispara throw (head %.2e)", head);
        CHECK(tail < head * 0.1, "throw cala-se depois (%.2e < %.2e)", tail, head);
    }

    // B3. Ordens 1 e 3: estáveis e audíveis.
    for (int ord : { 1, 3 })
    {
        DeVerbProcessor q;
        q.prepareToPlay(sr, 512);
        setI(q, "chain_order", ord);
        setI(q, "x_mode", 1);
        setI(q, "rev_mode", 1);
        setF(q, "rev_mix", 0.5f);
        juce::MidiBuffer midi;
        juce::AudioBuffer<float> b2(2, 512);
        float peak = 0.f;
        bool finite = true;
        for (int b = 0; b < 300; ++b)
        {
            for (int i = 0; i < 512; ++i)
            {
                float v = std::sin((b * 512 + i) * 0.02f) * 0.5f;
                b2.setSample(0, i, v);
                b2.setSample(1, i, v);
            }
            q.processBlock(b2, midi);
            for (int i = 0; i < 512; ++i)
            {
                float v = b2.getSample(0, i);
                if (! std::isfinite(v)) finite = false;
                peak = juce::jmax(peak, std::abs(v));
            }
        }
        CHECK(finite && peak < 4.f, "ordem %d + XFADE estável (pico %f)", ord, peak);
        CHECK(peak > 0.01f, "ordem %d deixa passar som (pico %f)", ord, peak);
    }

    // B4. MAN sync: o delay 1/8 segue o knob (90 vs 150 BPM).
    auto echoAt = [&](float bpm)
    {
        DeVerbProcessor q;
        q.prepareToPlay(sr, 512);
        q.setSyncMode("man");
        setF(q, "tempo_bpm", bpm);
        setF(q, "master", 1.f);
        setF(q, "fwd_mix", 1.f);
        setF(q, "fwd_gate_mix", 0.f);
        setI(q, "fwd_delay_note", 6); // 1/8
        setF(q, "fwd_delay_fb", 0.f); // eco único
        setF(q, "fwd_delay_mix", 1.f);
        setF(q, "fwd_verb_mix", 0.f);
        juce::MidiBuffer midi;
        juce::AudioBuffer<float> b2(2, 512);
        std::vector<float> out;
        for (int k = 0; k < 48000 / 512; ++k)
        {
            for (int i = 0; i < 512; ++i)
            {
                float v = (k * 512 + i == 0) ? 1.f : 0.f;
                b2.setSample(0, i, v);
                b2.setSample(1, i, v);
            }
            q.processBlock(b2, midi);
            for (int i = 0; i < 512; ++i) out.push_back(b2.getSample(0, i));
        }
        int best = -1;
        float bv = 0.f;
        for (int i = 2000; i < 20000 && i < (int) out.size(); ++i)
            if (std::abs(out[(size_t) i]) > bv) { bv = std::abs(out[(size_t) i]); best = i; }
        return best;
    };
    // 1/8 = 0.5 beats: @90 -> 0.5*60/90 = 0.333 s = 16000; @150 -> 0.2 s = 9600.
    int e90 = echoAt(90.f);
    int e150 = echoAt(150.f);
    CHECK(std::abs(e90 - 16000) < 300, "MAN 90 BPM: eco 1/8 a 16000 (got %d)", e90);
    CHECK(std::abs(e150 - 9600) < 300, "MAN 150 BPM: eco 1/8 a 9600 (got %d)", e150);

    // F1. Restore repõe syncPolicyUi: com playhead falso a 150 e sync MAN
    // guardado a 90, o reload tem de seguir o knob (16000), não o host.
    {
        struct FakePh : juce::AudioPlayHead
        {
            juce::Optional<juce::AudioPlayHead::PositionInfo> getPosition() const override
            {
                juce::AudioPlayHead::PositionInfo p;
                p.setBpm(150.0);
                p.setIsPlaying(true);
                p.setPpqPosition(8.0);
                return p;
            }
        } ph;
        DeVerbProcessor q;
        q.prepareToPlay(sr, 512);
        q.setPlayHead(&ph);
        q.setSyncMode("man");
        setF(q, "tempo_bpm", 90.f);
        setF(q, "master", 1.f);
        setF(q, "fwd_mix", 1.f);
        setF(q, "fwd_gate_mix", 0.f);
        setI(q, "fwd_delay_note", 6); // 1/8
        setF(q, "fwd_delay_fb", 0.f);
        setF(q, "fwd_delay_mix", 1.f);
        setF(q, "fwd_verb_mix", 0.f);
        juce::MemoryBlock mb;
        q.getStateInformation(mb);
        q.setSyncMode("host"); // suja; o restore tem de repor MAN
        q.setStateInformation(mb.getData(), (int) mb.getSize());
        juce::MidiBuffer midi;
        juce::AudioBuffer<float> b2(2, 512);
        std::vector<float> out;
        for (int k = 0; k < 48000 / 512; ++k)
        {
            for (int i = 0; i < 512; ++i)
            {
                float v = (k * 512 + i == 0) ? 1.f : 0.f;
                b2.setSample(0, i, v);
                b2.setSample(1, i, v);
            }
            q.processBlock(b2, midi);
            for (int i = 0; i < 512; ++i) out.push_back(b2.getSample(0, i));
        }
        int best = -1;
        float bv = 0.f;
        for (int i = 2000; i < 20000 && i < (int) out.size(); ++i)
            if (std::abs(out[(size_t) i]) > bv) { bv = std::abs(out[(size_t) i]); best = i; }
        q.setPlayHead(nullptr);
        CHECK(std::abs(best - 16000) < 300, "restore MAN ignora host (eco %d)", best);
    }
    return failures;
}

// --- Presets deterministas: navegar não deixa motores presos ---
// Os presets são deltas sobre base neutra; sem a base, o REV Loop ou o
// granular de um preset ficavam a tocar nos presets seguintes ("o seletor
// não escolhia": o nome mudava mas o som ficava preso).
static int mainPreset()
{
    DeVerbProcessor p;
    p.prepareToPlay(48000.0, 512);
    auto snap = [&]
    {
        std::vector<float> v;
        for (auto* par : p.getParameters()) v.push_back(par->getValue());
        return v;
    };
    // 1. Reverse Tail liga o Loop; Init a seguir tem de o calar.
    applyFactoryPreset(p.apvts, 4); // Reverse Tail (rev_mode Loop)
    juce::MidiBuffer midi;
    juce::AudioBuffer<float> buf(2, 512);
    for (int b = 0; b < 10; ++b) { buf.clear(); p.processBlock(buf, midi); }
    applyFactoryPreset(p.apvts, 0); // Init
    CHECK(p.apvts.getRawParameterValue("rev_mode")->load() == 0.f,
          "Init cala o REV Loop do preset anterior");
    CHECK(p.apvts.getRawParameterValue("gr_mode")->load() == 0.f,
          "Init cala o granular do preset anterior");
    // 2. Determinismo: A -> B -> A repõe os mesmos valores.
    applyFactoryPreset(p.apvts, 3); // Big Hall Space
    auto a3 = snap();
    applyFactoryPreset(p.apvts, 9); // Shimmer Pad
    applyFactoryPreset(p.apvts, 3); // voltar
    auto a3b = snap();
    bool same = (a3.size() == a3b.size());
    for (size_t i = 0; same && i < a3.size(); ++i)
        same = (a3[i] == a3b[i]);
    CHECK(same, "preset deterministicos: 3->9->3 repõe tudo");
    return failures;
}

// --- Fase 8: bypass por módulo no processador ---

static void setB(DeVerbProcessor& p, const char* id, bool b)
{
    if (auto* par = p.apvts.getParameter(id))
        par->setValueNotifyingHost(b ? 1.f : 0.f);
}

static int mainPower()
{
    const double sr = 48000.0;
    juce::MidiBuffer midi;

    // --- P1. Tudo off = passthrough exato (após settle) ---
    {
        DeVerbProcessor p;
        p.prepareToPlay(sr, 512);
        setF(p, "master", 1.f);
        setB(p, "fwd_gate_on", false);
        setB(p, "fwd_delay_on", false);
        setB(p, "fwd_verb_on", false);
        setB(p, "gr_on", false);
        juce::AudioBuffer<float> buf(2, 512);
        for (int b = 0; b < 20; ++b) // settle: rampas de 5 ms
        {
            for (int i = 0; i < 512; ++i)
            {
                float v = std::sin((b * 512 + i) * 0.01f) * 0.5f;
                buf.setSample(0, i, v);
                buf.setSample(1, i, v);
            }
            p.processBlock(buf, midi);
        }
        bool exact = true;
        for (int b = 0; b < 5 && exact; ++b)
        {
            for (int i = 0; i < 512; ++i)
            {
                float v = std::sin((b * 512 + i) * 0.01f) * 0.5f;
                buf.setSample(0, i, v);
                buf.setSample(1, i, v);
            }
            juce::AudioBuffer<float> ref(2, 512);
            ref.copyFrom(0, 0, buf, 0, 0, 512);
            ref.copyFrom(1, 0, buf, 1, 0, 512);
            p.processBlock(buf, midi);
            for (int ch = 0; ch < 2 && exact; ++ch)
                for (int i = 0; i < 512; ++i)
                    if (buf.getSample(ch, i) != ref.getSample(ch, i)) exact = false;
        }
        CHECK(exact, "tudo off = passthrough exato");
    }

    // --- P2. Toggle a meio do stream não clica ---
    {
        DeVerbProcessor p;
        p.prepareToPlay(sr, 512);
        setF(p, "master", 1.f);
        setF(p, "fwd_delay_mix", 0.5f);
        setF(p, "fwd_delay_fb", 0.5f);
        juce::AudioBuffer<float> buf(2, 512);
        float maxStep = 0.f;
        float prev = 0.f;
        bool first = true;
        for (int b = 0; b < 40; ++b)
        {
            for (int i = 0; i < 512; ++i)
            {
                float v = std::sin((b * 512 + i) * 0.01f) * 0.5f;
                buf.setSample(0, i, v);
                buf.setSample(1, i, v);
            }
            if (b == 20) setB(p, "fwd_delay_on", false); // toggle a meio!
            p.processBlock(buf, midi);
            for (int i = 0; i < 512; ++i)
            {
                float v = buf.getSample(0, i);
                if (! first) maxStep = juce::jmax(maxStep, std::abs(v - prev));
                first = false;
                prev = v;
                if (! std::isfinite(v)) { maxStep = 1e9f; break; }
            }
        }
        CHECK(maxStep < 0.2f, "toggle delay sem clique (max step %f)", maxStep);
    }

    // --- P3. Link: REV segue o bypass do FWD ---
    {
        DeVerbProcessor p;
        p.prepareToPlay(sr, 512);
        setF(p, "master", 1.f);
        setB(p, "fwd_gate_on", false); // REV linkado segue
        setF(p, "fwd_delay_mix", 0.f);
        setF(p, "fwd_verb_mix", 0.f);
        setI(p, "rev_mode", 1);
        setF(p, "rev_mix", 1.f);
        setF(p, "rev_duck", 0.f);
        juce::AudioBuffer<float> buf(2, 512);
        std::vector<float> out;
        for (int b = 0; b < 100; ++b)
        {
            for (int i = 0; i < 512; ++i) { buf.setSample(0, i, 1.f); buf.setSample(1, i, 1.f); }
            p.processBlock(buf, midi);
            for (int i = 0; i < 512; ++i) out.push_back(buf.getSample(0, i));
        }
        // Gate bypassed nas duas cadeias + resto neutro: saída ≈ DC,
        // limitada ao ceiling do limiter (-1 dBFS = 0.891).
        double m = 0.0;
        for (size_t i = 20000; i < out.size(); ++i) m += out[i];
        m /= (double) (out.size() - 20000);
        CHECK(m > 0.85, "link: REV segue bypass do FWD (média %f)", m);
    }

    return failures;
}

// --- Plugin civilizado: limiter + RANDOM sempre audível ---

// Corre `total` amostras com feed e devolve RMS + pico da saída L.
template <typename FeedFn>
static void renderProc(DeVerbProcessor& p, int total, FeedFn feed,
                       float& rmsOut, float& peakOut)
{
    juce::MidiBuffer midi;
    double e = 0.0;
    int c = 0;
    peakOut = 0.f;
    int pos = 0, abs = 0;
    while (pos < total)
    {
        int n = juce::jmin(512, total - pos);
        juce::AudioBuffer<float> tmp(2, n);
        for (int i = 0; i < n; ++i)
        {
            float v = feed(abs + pos + i);
            tmp.setSample(0, i, v);
            tmp.setSample(1, i, v);
        }
        p.processBlock(tmp, midi);
        for (int i = 0; i < n; ++i)
        {
            float v = tmp.getSample(0, i);
            e += (double) v * v;
            ++c;
            peakOut = juce::jmax(peakOut, std::abs(v));
            if (! std::isfinite(v)) { peakOut = 1e9f; return; }
        }
        pos += n;
    }
    rmsOut = std::sqrt(e / (double) juce::jmax(1, c));
}

static int mainCivil()
{
    const double sr = 48000.0;

    // --- C1. Limiter: entrada a arder não passa de -1 dBFS ---
    {
        DeVerbProcessor p;
        p.prepareToPlay(sr, 512);
        setF(p, "master", 1.f);
        setF(p, "input_gain", 2.f); // máximo!
        setF(p, "fwd_verb_mix", 1.f);
        setF(p, "fwd_delay_mix", 0.5f);
        float rms = 0.f, peak = 0.f;
        renderProc(p, 48000,
                   [](int i) { return std::sin(i * 0.05f) * 0.9f; }, rms, peak);
        CHECK(peak <= 0.90f, "limiter trava a -1 dBFS (pico %f)", peak);
        CHECK(rms > 0.3f, "limiter não esmaga (rms %f)", rms);
    }

    // --- C2. RANDOM civilizado: 20 seeds, sempre audível, sempre finito ---
    {
        bool allOk = true;
        for (int seed = 1; seed <= 20 && allOk; ++seed)
        {
            DeVerbProcessor p;
            p.prepareToPlay(sr, 512);
            juce::Random rng(seed);
            applyRealList(p.apvts, civilizedRandom(rng));
            float rms = 0.f, peak = 0.f;
            auto feed = [](int i)
            {
                float s = std::sin(i * 0.02f) * 0.6f;
                if (i % 12000 < 64) s += 0.5f; // transiente periódico
                return s;
            };
            renderProc(p, 96000, feed, rms, peak);
            if (! (rms > 0.02f && peak <= 1.0f))
            {
                std::printf("  seed %d: rms %f peak %f\n", seed, rms, peak);
                allOk = false;
            }
        }
        CHECK(allOk, "20 RANDOMs civilizados todos audíveis e finitos");
    }

    // --- C3. Duck no processador: dry forte cala o REV ---
    // Isola o REV (gate FWD fechado mata dry+FWD; out = REV puro) para o
    // duck não se esconder atrás do dry.
    {
        auto withDuck = [&](float duck)
        {
            DeVerbProcessor p;
            p.prepareToPlay(sr, 512);
            setF(p, "master", 1.f);
            setF(p, "fwd_mix", 1.f);
            setF(p, "fwd_gate_pattern", 0.f);
            setF(p, "fwd_gate_depth", 1.f);
            setF(p, "fwd_gate_mix", 1.f);
            setF(p, "fwd_delay_mix", 0.f);
            setF(p, "fwd_verb_mix", 0.f);
            setI(p, "rev_mode", 1); // Loop
            setF(p, "rev_mix", 1.f);
            setF(p, "rev_duck", duck);
            // Desliga o link do gate: senão o REV herdava o pattern 0x0000
            // fechado do FWD e calava a própria cadeia (correto, mas não é
            // o que se mede aqui).
            setB(p, "link_gate", false);
            setF(p, "rev_gate_mix", 0.f);
            setF(p, "rev_delay_mix", 0.f);
            setF(p, "rev_verb_mix", 0.f);
            float rms = 0.f, peak = 0.f;
            renderProc(p, 96000,
                       [](int i) { return std::sin(i * 0.02f) * 0.9f; }, rms, peak);
            (void) peak;
            return rms;
        };
        float r0 = withDuck(0.f), r1 = withDuck(1.f);
        CHECK(r0 > 0.1f, "REV audível sem duck (%.3f)", r0);
        CHECK(r1 < r0 * 0.7f, "duck cala o REV (%.3f < %.3f)", r1, r0);
    }

    return failures;
}

// --- Réplica do fuzz do pluginval: params aleatórios × inputs × SRs × blocos.
// Deteta NaN/Inf/picos absurdos e imprime a combinação culpada.
static int mainFuzz()
{
    int before = failures; // o global já pode trazer falhas de fases anteriores
    juce::Random rng(0xF422);
    const double srs[3] = { 44100.0, 48000.0, 96000.0 };
    const int blks[2] = { 64, 1024 };
    juce::MidiBuffer midi;

    for (double sr : srs)
    {
        for (int blk : blks)
        {
            DeVerbProcessor p;
            p.prepareToPlay(sr, blk);
            juce::AudioBuffer<float> buf(2, blk);

            for (int iter = 0; iter < 60; ++iter)
            {
                // Randomiza TODOS os parâmetros (como o fuzz faz).
                for (int i = 0; i < p.getParameters().size(); ++i)
                    p.getParameters()[i]->setValueNotifyingHost(rng.nextFloat());

                for (int stim = 0; stim < 4; ++stim) // silêncio/seno/ruído/DC
                {
                    for (int i = 0; i < blk; ++i)
                    {
                        float v = 0.f;
                        if (stim == 1) v = std::sin(i * 0.05f) * 0.5f;
                        else if (stim == 2) v = rng.nextFloat() * 2.f - 1.f;
                        else if (stim == 3) v = 0.9f;
                        buf.setSample(0, i, v);
                        buf.setSample(1, i, v);
                    }
                    p.processBlock(buf, midi);
                    for (int i = 0; i < blk; ++i)
                    {
                        for (int ch = 0; ch < 2; ++ch)
                        {
                            float v = buf.getSample(ch, i);
                            if (! std::isfinite(v) || std::abs(v) > 20.f)
                            {
                                std::printf("fuzz anomaly: sr %.0f blk %d iter %d stim %d ch %d i %d v %f\n",
                                            sr, blk, iter, stim, ch, i, v);
                                // despeja os params para reproduzir
                                for (int k = 0; k < p.getParameters().size(); ++k)
                                    std::printf("  p%-3d %-18s = %f\n", k,
                                        p.getParameters()[k]->getName(64).toRawUTF8(),
                                        p.getParameters()[k]->getValue());
                                ++failures;
                                return failures;
                            }
                        }
                    }
                }
            }
        }
    }
    CHECK(failures == before, "fuzz próprio: 3600 blocos sem NaN/Inf/picos");
    return failures;
}

// --- Fase 6: ordem alternativa + XFADE ---
static int mainOrder()
{
    const double sr = 48000.0;
    juce::MidiBuffer midi;
    DeVerbProcessor p;
    p.prepareToPlay(sr, 512);
    setI(p, "chain_order", 2); // D-G-V-Gr
    setI(p, "x_mode", 1);      // XFade
    setI(p, "rev_mode", 1);    // Loop (XFADE precisa do REV)
    setF(p, "rev_mix", 0.5f);
    juce::AudioBuffer<float> buf(2, 512);
    float peak = 0.f;
    bool finite = true;
    for (int b = 0; b < 300; ++b) // ~3.2 s atravessa vários beats a 120 BPM
    {
        for (int i = 0; i < 512; ++i)
        {
            float v = std::sin((b * 512 + i) * 0.02f) * 0.5f;
            buf.setSample(0, i, v);
            buf.setSample(1, i, v);
        }
        p.processBlock(buf, midi);
        for (int i = 0; i < 512; ++i)
        {
            float v = buf.getSample(0, i);
            if (! std::isfinite(v)) finite = false;
            peak = juce::jmax(peak, std::abs(v));
        }
    }
    CHECK(finite && peak < 4.f, "ordem D-G-V-Gr + XFADE estável (pico %f)", peak);
    CHECK(peak > 0.01f, "XFADE deixa passar som (pico %f)", peak);

    // --- XFADE alterna mesmo: pares FWD (gran Pitch = 880), ímpares dry+REV ---
    // FWD = granular Pitch+12 contínuo (880) com mix 1: nos pares o dry some
    // ((1-gF) = 0) e só há 880; nos ímpares o FWD cala-se (sem 880) e passam
    // dry 440 + REV Loop a 0.5× (220). Três assinaturas independentes.
    {
        DeVerbProcessor q;
        q.prepareToPlay(sr, 512);
        setF(q, "master", 1.f);
        setF(q, "fwd_mix", 1.f);
        setF(q, "fwd_gate_mix", 0.f);
        setF(q, "fwd_delay_mix", 0.f);
        setF(q, "fwd_verb_mix", 0.f);
        setI(q, "gr_mode", 4); // Pitch
        setF(q, "gr_chance", 1.f);
        setF(q, "gr_mix", 1.f);
        setB(q, "link_gran", false); // senão o REV seguia o Pitch e o 220 sumia
        setI(q, "x_mode", 1);
        setI(q, "rev_mode", 1); // Loop
        setF(q, "rev_rate", 0.5f);
        setF(q, "rev_mix", 1.f);
        setF(q, "rev_duck", 0.f);
        setF(q, "rev_gate_mix", 0.f);
        setF(q, "rev_delay_mix", 0.f);
        setF(q, "rev_verb_mix", 0.f);
        std::vector<float> out;
        juce::AudioBuffer<float> b2(2, 512);
        const int beats = 10, beatLen = 24000; // 0.5 s @120
        for (int k = 0; k < beats * beatLen / 512; ++k)
        {
            for (int i = 0; i < 512; ++i)
            {
                float v = std::sin((k * 512 + i) * 2.f * 3.14159265f * 440.f / 48000.f) * 0.5f;
                b2.setSample(0, i, v);
                b2.setSample(1, i, v);
            }
            q.processBlock(b2, midi);
            for (int i = 0; i < 512; ++i) out.push_back(b2.getSample(0, i));
        }
        auto goertzel = [&](int from, int to, double target)
        {
            double w = 2.0 * 3.14159265358979 * target / 48000.0;
            double c = std::cos(w), p0 = 0.0, pp = 0.0;
            for (int i = from; i < to; ++i)
            {
                double x = out[(size_t) i] + 2.0 * c * p0 - pp;
                pp = p0;
                p0 = x;
            }
            int m = to - from;
            return std::sqrt(p0 * p0 + pp * pp - 2.0 * c * p0 * pp) / m;
        };
        bool altOk = true;
        for (int k = 4; k < 9; ++k) // salta settle (REV + granular)
        {
            int from = k * beatLen + 4800, to = k * beatLen + 19200;
            double e220 = goertzel(from, to, 220.0);
            double e440 = goertzel(from, to, 440.0);
            double e880 = goertzel(from, to, 880.0);
            std::printf("  beat %d (%s): E220 %.3f E440 %.3f E880 %.3f\n",
                        k, (k % 2 == 0) ? "par" : "ímpar", e220, e440, e880);
            bool ok;
            if (k % 2 == 0) // par: só FWD (880 domina, dry+REV mudos)
                ok = (e880 > e440 * 2.0) && (e880 > e220 * 2.0);
            else // ímpar: FWD mudo (sem 880), dry 440 + REV 220 audíveis.
                 // (O 220 varia com os reanchors do leitor a 0.5×: limiar
                 // folgado; o par-220-baixo + ímpar-220-presente prova o xfRev.)
                ok = (e880 < 0.05) && (e440 > 0.1) && (e220 > 0.03);
            if (! ok) altOk = false;
        }
        CHECK(altOk, "XFADE alterna FWD/REV por beat");
    }
    return failures;
}
