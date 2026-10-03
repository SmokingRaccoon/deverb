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

    failures += mainOrder();
    failures += mainFuzz();
    failures += mainCivil();
    failures += mainPower();

    if (failures == 0) std::printf("\nALL CHAIN TESTS PASSED\n");
    else std::printf("\n%d FAILURES\n", failures);
    return failures == 0 ? 0 : 1;
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

    return failures;
}

// --- Réplica do fuzz do pluginval: params aleatórios × inputs × SRs × blocos.
// Deteta NaN/Inf/picos absurdos e imprime a combinação culpada.
static int mainFuzz()
{
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
    CHECK(failures == 0, "fuzz próprio: 3600 blocos sem NaN/Inf/picos");
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
    return failures;
}
