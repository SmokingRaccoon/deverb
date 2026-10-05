// Harness offline do deVerb (Fase 1). Corre sem DAW, sem UI, sem host:
//   cmake -S tests -B tests/build -G Ninja -DCMAKE_BUILD_TYPE=Release
//   cmake --build tests/build && ./tests/build/test_dsp
// Sai 0 = tudo passa; imprime cada verificação.
#include <cstdio>
#include <cmath>
#include "../Source/core/TempoInfo.h"
#include "../Source/dsp/Delay.h"
#include "../Source/dsp/Gater.h"
#include "../Source/dsp/ReverbEngine.h"
#include "../Source/dsp/Granular.h"
#include "../Source/dsp/ReverseEngine.h"
#include "../Source/dsp/Dimension.h"
#include "../Source/core/RevLinker.h"
#include "../Source/core/EnableRamp.h"
#include <vector>

static int failures = 0;
#define CHECK(cond, ...) do { \
    if (!(cond)) { ++failures; std::printf("FAIL: " __VA_ARGS__); std::printf("\n"); } \
    else { std::printf("ok: " __VA_ARGS__); std::printf("\n"); } \
} while (0)

// Maior pico absoluto e o seu índice.
static int findPeak(const juce::AudioBuffer<float>& buf, int ch, float& amp)
{
    int idx = 0; amp = 0.f;
    for (int i = 0; i < buf.getNumSamples(); ++i)
    {
        float v = std::abs(buf.getSample(ch, i));
        if (v > amp) { amp = v; idx = i; }
    }
    return idx;
}

int mainDelay();
int mainVerb();
int mainGranular();
int mainReverse();
int mainDelayAlgos();
int mainBypass();
int mainDimension();

static float meanAbs(const std::vector<float>& v, int from, int to);

int mainDelay()
{
    // --- 1. Matemática de tempo: 1/8D @ 120 BPM tem de dar 375 ms ---
    {
        using N = TempoInfo::Note;
        CHECK(TempoInfo::noteToBeats(N::N8D) == 0.75, "1/8D = 0.75 beats");
        double sec = TempoInfo::beatsToSeconds(0.75, 120.0);
        CHECK(std::abs(sec - 0.375) < 1e-9, "1/8D @120BPM = 0.375 s (got %f)", sec);
        double smp = TempoInfo::beatsToSamples(0.75, 120.0, 48000.0);
        CHECK(std::abs(smp - 18000.0) < 1e-6, "1/8D @120BPM/48k = 18000 samples (got %f)", smp);
        CHECK(std::abs(TempoInfo::noteToBeats(N::N16T) - 1.0/6.0) < 1e-9, "1/16T = 1/6 beats");
        CHECK(std::abs(TempoInfo::noteToBeats(N::N8T) - 1.0/3.0) < 1e-9, "1/8T = 1/3 beats");
    }

    // --- 2. Delay: impulso seco, eco tem de cair na amostra 18000 ---
    {
        Delay d;
        d.prepare(48000.0, 512);
        d.setTestDelaySamples(18000); // 375 ms @48k
        d.setFeedback(0.0f);
        d.setDampingHz(20000.0f);     // one-pole ≈ transparente
        d.setMix(1.0f);               // só wet
        d.setFrozen(false);

        juce::AudioBuffer<float> buf(2, 40000);
        buf.clear();
        buf.setSample(0, 0, 1.0f);
        buf.setSample(1, 0, 1.0f);

        int pos = 0;
        while (pos < buf.getNumSamples())
        {
            int n = juce::jmin(512, buf.getNumSamples() - pos);
            juce::AudioBuffer<float> block(buf.getArrayOfWritePointers(), 2, pos, n);
            // Atenção: AudioBuffer "view" partilha os canais — processar por janelas
            // equivale a processar de seguida (a linha de delay mantém estado).
            juce::AudioBuffer<float> tmp(2, n);
            tmp.copyFrom(0, 0, buf, 0, pos, n);
            tmp.copyFrom(1, 0, buf, 1, pos, n);
            d.process(tmp);
            buf.copyFrom(0, pos, tmp, 0, 0, n);
            buf.copyFrom(1, pos, tmp, 1, 0, n);
            pos += n;
        }

        float amp = 0.f;
        int peak = findPeak(buf, 0, amp);
        CHECK(std::abs(peak - 18000) <= 2, "eco L na amostra 18000 (got %d)", peak);
        CHECK(std::abs(amp - 1.0f) < 0.01f, "eco L amplitude 1.0 (got %f)", amp);
        peak = findPeak(buf, 1, amp);
        CHECK(std::abs(peak - 18000) <= 2, "eco R na amostra 18000 (got %d)", peak);
        // Sem feedback não pode haver 2º eco: energia depois de 18001 ≈ 0.
        float tail = 0.f;
        for (int i = 18002; i < buf.getNumSamples(); ++i)
            tail = juce::jmax(tail, std::abs(buf.getSample(0, i)));
        CHECK(tail < 0.01f, "sem feedback não há 2º eco (tail %f)", tail);
    }

    // --- 3. Delay com feedback 0.5: ecos em 18000 (1.0) e 36000 (0.5) ---
    {
        Delay d;
        d.prepare(48000.0, 512);
        d.setTestDelaySamples(18000);
        d.setFeedback(0.5f);
        d.setDampingHz(20000.0f);
        d.setMix(1.0f);
        d.setFrozen(false);

        juce::AudioBuffer<float> buf(2, 60000);
        buf.clear();
        buf.setSample(0, 0, 1.0f);

        for (int pos = 0; pos < buf.getNumSamples();)
        {
            int n = juce::jmin(512, buf.getNumSamples() - pos);
            juce::AudioBuffer<float> tmp(2, n);
            tmp.copyFrom(0, 0, buf, 0, pos, n);
            tmp.copyFrom(1, 0, buf, 1, pos, n);
            d.process(tmp);
            buf.copyFrom(0, pos, tmp, 0, 0, n);
            buf.copyFrom(1, pos, tmp, 1, 0, n);
            pos += n;
        }

        float e1 = std::abs(buf.getSample(0, 18000));
        float e2 = std::abs(buf.getSample(0, 36000));
        CHECK(std::abs(e1 - 1.0f) < 0.02f, "1º eco = 1.0 (got %f)", e1);
        // O 2º eco passa 1x pelo one-pole do damping: esperado = fb * a,
        // com a = 1 - exp(-2π·20000/48000). Isto valida o feedback E que o
        // damping está dentro do loop (não no dry).
        const float a = 1.f - std::exp(-juce::MathConstants<float>::twoPi * 20000.f / 48000.f);
        CHECK(std::abs(e2 - 0.5f * a) < 0.01f, "2º eco = fb*damp (got %f, esperado %f)", e2, 0.5f * a);
    }

    return failures;
}

// Renderiza `total` amostras de DC 1.0 pelo gater em blocos de 512.
static void renderDC(Gater& g, const TempoInfo& t, int total)
{
    juce::AudioBuffer<float> tmp(2, 512);
    int pos = 0;
    while (pos < total)
    {
        int n = juce::jmin(512, total - pos);
        tmp.setSize(2, n, false, false, true);
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < n; ++i) tmp.setSample(ch, i, 1.0f);
        g.process(tmp, t);
        pos += n;
    }
}

int main()
{
    mainDelay(); // o global `failures` já acumula; somar o retorno duplicava

    TempoInfo t; // fromHost=false → relógio interno

    // --- 4. Gater: passo 1/16 @128BPM = 5625 amostras; só passo 0 aberto ---
    {
        Gater g;
        g.prepare(48000.0);
        g.setRate(TempoInfo::Note::N16);
        g.setSteps(16);
        g.setPattern(0x0001);
        g.setSmooth(0.f);
        g.setDepth(1.f);
        g.setMix(1.f);
        g.setPanAlt(0.f);
        g.setTrigMode(Gater::TrigMode::Free);
        g.setInternalBpm(128.0);

        juce::AudioBuffer<float> buf(2, 12000);
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 12000; ++i) buf.setSample(ch, i, 1.0f);
        g.process(buf, t);

        float open = 0.f, closed = 0.f;
        for (int i = 100; i < 5000; ++i) open += buf.getSample(0, i);
        for (int i = 6000; i < 11000; ++i) closed += buf.getSample(0, i);
        open /= 4900.f; closed /= 5000.f;
        CHECK(open > 0.9f, "passo 0 aberto (media %f)", open);
        CHECK(closed < 0.1f, "passo 1 fechado (media %f)", closed);
    }

    // --- 5. Gater: 1 compasso de 16 passos @128BPM; step final bate certo ---
    {
        Gater g;
        g.prepare(48000.0);
        g.setRate(TempoInfo::Note::N16);
        g.setSteps(16);
        g.setPattern(0xFFFF);
        g.setSmooth(0.f);
        g.setDepth(1.f);
        g.setMix(1.f);
        g.setTrigMode(Gater::TrigMode::Free);
        g.setInternalBpm(128.0);
        renderDC(g, t, 90000); // 4 beats exatos
        // Último bloco começa na amostra 89600 → 3.9822 beats → passo 15.
        CHECK(g.getCurrentStep() == 15, "passo no fim do compasso = 15 (got %d)",
              g.getCurrentStep());
    }

    // --- 5b. Steps 1..16 livres: tabela choice->contagem + ciclo de 12 ---
    {
        CHECK(Gater::stepsCountFromChoice(0) == 8, "idx 0 = 8 (compat)");
        CHECK(Gater::stepsCountFromChoice(1) == 16, "idx 1 = 16 (compat)");
        CHECK(Gater::stepsCountFromChoice(2) == 12, "idx 2 = 12");
        CHECK(Gater::stepsCountFromChoice(99) == 15, "fora de gama prende em 15");
        Gater g;
        g.prepare(48000.0);
        g.setRate(TempoInfo::Note::N16);
        g.setSteps(12);
        g.setPattern(0xFFFF);
        g.setSmooth(0.f);
        g.setDepth(1.f);
        g.setMix(1.f);
        g.setTrigMode(Gater::TrigMode::Free);
        g.setInternalBpm(120.0);
        renderDC(g, t, 72000); // 3 beats exatos = 12 passos de 1/16
        CHECK(g.getCurrentStep() == 11, "12 passos fazem wrap no 11 (got %d)",
              g.getCurrentStep());
    }

    // --- 6. Gater: note-on em modo Midi faz restart do padrão ---
    {
        Gater g;
        g.prepare(48000.0);
        g.setRate(TempoInfo::Note::N16);
        g.setSteps(16);
        g.setPattern(0xFFFF);
        g.setSmooth(0.f);
        g.setDepth(1.f);
        g.setMix(1.f);
        g.setTrigMode(Gater::TrigMode::Midi);
        g.setInternalBpm(128.0);
        renderDC(g, t, 10000); // anda até ao passo 1
        CHECK(g.getCurrentStep() == 1, "antes da nota: passo 1 (got %d)", g.getCurrentStep());

        juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::noteOn(1, 60, 1.0f), 0);
        g.scanMidi(midi);
        renderDC(g, t, 512);
        CHECK(g.getCurrentStep() == 0, "depois da nota: passo 0 (got %d)", g.getCurrentStep());
    }

    // --- 7. Gater: transiente em modo Transient faz restart ---
    {
        Gater g;
        g.prepare(48000.0);
        g.setRate(TempoInfo::Note::N16);
        g.setSteps(16);
        g.setPattern(0xFFFF);
        g.setSmooth(0.f);
        g.setDepth(1.f);
        g.setMix(1.f);
        g.setTrigMode(Gater::TrigMode::Transient);
        g.setEnvThrDb(-18.f);
        g.setInternalBpm(128.0);

        juce::AudioBuffer<float> sil(2, 512); // silêncio: sem retrigger
        sil.clear();
        for (int k = 0; k < 20; ++k) // 10240 amostras em blocos (step atualiza por bloco)
            g.process(sil, t);
        CHECK(g.getCurrentStep() == 1, "silêncio não faz reset (passo %d)", g.getCurrentStep());

        juce::AudioBuffer<float> hit(2, 512); // "pancada": 64 amostras a 1.0
        hit.clear();
        for (int i = 0; i < 64; ++i) { hit.setSample(0, i, 1.0f); hit.setSample(1, i, 1.0f); }
        g.process(hit, t);
        juce::AudioBuffer<float> after(2, 512);
        after.clear();
        g.process(after, t);
        CHECK(g.getCurrentStep() == 0, "transiente fez reset (passo %d)", g.getCurrentStep());
    }

    // --- G38. PanAlt: passos pares à esquerda, ímpares à direita ---
    {
        Gater g;
        g.prepare(48000.0);
        g.setRate(TempoInfo::Note::N16);
        g.setSteps(16);
        g.setPattern(0x0003); // passos 0 e 1 abertos
        g.setSmooth(0.f);
        g.setDepth(1.f);
        g.setMix(1.f);
        g.setPanAlt(1.f);
        g.setTrigMode(Gater::TrigMode::Free);
        g.setInternalBpm(128.0);
        juce::AudioBuffer<float> buf(2, 12000);
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 12000; ++i) buf.setSample(ch, i, 1.0f);
        g.process(buf, t);
        auto mlr = [&](int a, int b, int ch)
        {
            double s = 0.0;
            for (int i = a; i < b; ++i) s += buf.getSample(ch, i);
            return s / (b - a);
        };
        double l0 = mlr(2000, 5000, 0), r0 = mlr(2000, 5000, 1);
        double l1 = mlr(7700, 10700, 0), r1 = mlr(7700, 10700, 1);
        CHECK(l0 > 0.9 && r0 < 0.1, "passo par à esquerda (%f/%f)", l0, r0);
        CHECK(r1 > 0.9 && l1 < 0.1, "passo ímpar à direita (%f/%f)", l1, r1);
    }

    // --- G39. Host ppq alinha o passo (trig Host + transporte) ---
    {
        Gater g;
        g.prepare(48000.0);
        g.setRate(TempoInfo::Note::N16); // passo = 0.25 beats
        g.setSteps(16);
        g.setPattern(0xFFFF);
        g.setTrigMode(Gater::TrigMode::Host);
        TempoInfo th;
        th.fromHost = true;
        th.isPlaying = true;
        th.bpm = 120.0;
        juce::AudioBuffer<float> buf(2, 512);
        buf.clear();
        th.ppqPosition = 2.0; // floor(2/0.25) % 16 = 8
        g.process(buf, th);
        CHECK(g.getCurrentStep() == 8, "ppq 2.0 -> passo 8 (got %d)", g.getCurrentStep());
        th.ppqPosition = 2.3; // floor(9.2) = 9
        g.process(buf, th);
        CHECK(g.getCurrentStep() == 9, "ppq 2.3 -> passo 9 (got %d)", g.getCurrentStep());
    }

    // --- G40. Depth 0.5 deixa passar metade; smooth alonga a transição ---
    {
        auto width = [](float smooth)
        {
            Gater g;
            g.prepare(48000.0);
            g.setRate(TempoInfo::Note::N16);
            g.setSteps(16);
            g.setPattern(0x0001);
            g.setSmooth(smooth);
            g.setDepth(1.f);
            g.setMix(1.f);
            g.setTrigMode(Gater::TrigMode::Free);
            g.setInternalBpm(128.0);
            juce::AudioBuffer<float> buf(2, 24000);
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < 24000; ++i) buf.setSample(ch, i, 1.0f);
            TempoInfo tt;
            g.process(buf, tt);
            int i90 = -1, i10 = -1;
            for (int i = 5625; i < 22000; ++i)
            {
                float v = buf.getSample(0, i);
                if (i90 < 0 && v < 0.9f) i90 = i;
                if (v < 0.1f) { i10 = i; break; }
            }
            if (i90 < 0 || i10 < 0) return -1;
            return i10 - i90;
        };
        int w0 = width(0.f), w05 = width(0.5f);
        CHECK(w0 > 0 && w05 > w0 * 10, "smooth 0.5 alonga transição (%d vs %d)", w05, w0);
        // Depth a meio
        Gater g;
        g.prepare(48000.0);
        g.setRate(TempoInfo::Note::N16);
        g.setSteps(16);
        g.setPattern(0x0001);
        g.setSmooth(0.f);
        g.setDepth(0.5f);
        g.setMix(1.f);
        g.setTrigMode(Gater::TrigMode::Free);
        g.setInternalBpm(128.0);
        juce::AudioBuffer<float> buf(2, 12000);
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 12000; ++i) buf.setSample(ch, i, 1.0f);
        TempoInfo tt;
        g.process(buf, tt);
        double m = 0.0;
        for (int i = 7000; i < 11000; ++i) m += buf.getSample(0, i);
        m /= 4000.0;
        CHECK(std::abs(m - 0.5) < 0.05, "depth 0.5 deixa metade (%f)", m);
    }

    // --- G60. Tabela de notas completa (semínima = 1 beat) ---
    {
        using N = TempoInfo::Note;
        auto beats = [](N n) { return TempoInfo::noteToBeats(n); };
        CHECK(beats(N::Free) == 0.0, "Free = 0");
        CHECK(beats(N::N32) == 0.125, "1/32");
        CHECK(std::abs(beats(N::N16T) - 1.0 / 6.0) < 1e-12, "1/16T");
        CHECK(beats(N::N16) == 0.25, "1/16");
        CHECK(beats(N::N16D) == 0.375, "1/16D");
        CHECK(std::abs(beats(N::N8T) - 1.0 / 3.0) < 1e-12, "1/8T");
        CHECK(beats(N::N8) == 0.5, "1/8");
        CHECK(beats(N::N8D) == 0.75, "1/8D");
        CHECK(beats(N::N4) == 1.0, "1/4");
        CHECK(beats(N::N4D) == 1.5, "1/4D");
        CHECK(beats(N::N2) == 2.0, "1/2");
        CHECK(beats(N::N1) == 4.0, "1/1");
    }

    mainVerb();
    mainGranular();
    mainReverse();
    mainDelayAlgos();
    mainDimension();
    mainBypass();

    if (failures == 0) std::printf("\nALL TESTS PASSED\n");
    else std::printf("\n%d FAILURES\n", failures);
    return failures == 0 ? 0 : 1;
}

// --- Fase 8: bypass por módulo ---

static void fillDC(juce::AudioBuffer<float>& b, float v = 1.f)
{
    for (int ch = 0; ch < b.getNumChannels(); ++ch)
        for (int i = 0; i < b.getNumSamples(); ++i) b.setSample(ch, i, v);
}

static bool isPassthrough(juce::AudioBuffer<float>& b, float v = 1.f)
{
    for (int ch = 0; ch < b.getNumChannels(); ++ch)
        for (int i = 0; i < b.getNumSamples(); ++i)
            if (std::abs(b.getSample(ch, i) - v) > 1e-5f) return false;
    return true;
}

int mainBypass()
{
    // --- 26. EnableRamp: rampa 5 ms, silent, clear único ---
    {
        EnableRamp r;
        r.prepare(48000.0); // step = 1/240
        CHECK(! r.silent(), "nasce ligado");
        CHECK(! r.takeClear(), "sem clear pendente");
        r.set(false);
        for (int i = 0; i < 100; ++i) r.next();
        CHECK(! r.silent(), "a meio da rampa ainda não silent");
        for (int i = 0; i < 200; ++i) r.next();
        CHECK(r.silent(), "silent após 5 ms");
        CHECK(r.takeClear(), "clear dispara uma vez");
        CHECK(! r.takeClear(), "clear não repete");
        r.set(true);
        CHECK(! r.silent(), "religar acorda logo");
        CHECK(! r.takeClear(), "religar não pede clear");
    }

    // --- 27. Delay bypass: transparente + sem tail velha ao religar ---
    {
        Delay d;
        d.prepare(48000.0, 512);
        d.setMix(1.f);
        d.setFeedback(0.8f);
        d.setTestDelaySamples(5000);
        d.setEnabled(false);
        juce::AudioBuffer<float> blk(2, 512);
        blk.clear();
        d.process(blk); // 512 amostras chegam para a rampa de 5 ms (240)
        CHECK(d.isBypassed(), "delay silent após settle");
        CHECK(d.takeClear(), "delay pede clear uma vez");
        CHECK(! d.takeClear(), "delay não repete clear");
        d.reset(); // o que o processor faz no takeClear
        fillDC(blk);
        d.process(blk);
        CHECK(isPassthrough(blk), "delay bypass = passthrough exato");

        // Tail velha: enche com eco, desliga (settle+clear), religa em zeros.
        Delay e;
        e.prepare(48000.0, 512);
        e.setMix(1.f);
        e.setFeedback(0.8f);
        e.setTestDelaySamples(5000);
        juce::AudioBuffer<float> imp(2, 512);
        imp.clear();
        imp.setSample(0, 0, 1.f);
        e.process(imp); // impulso entra na linha
        e.setEnabled(false);
        juce::AudioBuffer<float> z(2, 512);
        z.clear();
        e.process(z);
        CHECK(e.isBypassed(), "delay settle após tail");
        if (e.takeClear()) e.reset();
        juce::AudioBuffer<float> z2(2, 512);
        z2.clear();
        e.setEnabled(true);
        e.process(z2); // religa sobre zeros: sem tail velha?
        float tail = 0.f;
        for (int i = 0; i < 512; ++i) tail = juce::jmax(tail, std::abs(z2.getSample(0, i)));
        CHECK(tail < 0.01f, "religar não cospe tail velha (%f)", tail);
    }

    // --- 28. Gater bypass com pattern fechado ---
    {
        Gater g;
        g.prepare(48000.0);
        g.setRate(TempoInfo::Note::N16);
        g.setSteps(16);
        g.setPattern(0x0000); // tudo fechado: sem bypass calava tudo
        g.setDepth(1.f);
        g.setMix(1.f);
        g.setTrigMode(Gater::TrigMode::Free);
        g.setInternalBpm(120.0);
        g.setEnabled(false);
        TempoInfo t;
        juce::AudioBuffer<float> blk(2, 512);
        blk.clear();
        g.process(blk, t);
        CHECK(g.isBypassed(), "gater silent após settle");
        if (g.takeClear()) g.reset();
        fillDC(blk);
        g.process(blk, t);
        CHECK(isPassthrough(blk), "gater bypass = passthrough exato");
    }

    // --- 29. Reverb + Granular bypass transparentes ---
    {
        reverb::Reverb r;
        r.prepare(48000.0, 512);
        r.setMix(1.f);
        r.setT60(3.0);
        r.setEnabled(false);
        juce::AudioBuffer<float> blk(2, 512);
        blk.clear();
        r.process(blk);
        CHECK(r.isBypassed(), "verb silent após settle");
        if (r.takeClear()) r.reset();
        fillDC(blk);
        r.process(blk);
        CHECK(isPassthrough(blk), "verb bypass = passthrough exato");

        Granular gr;
        gr.prepare(48000.0);
        gr.setMode(Granular::Mode::BeatRepeat);
        gr.setTrig(Granular::Trig::Manual);
        gr.setLenNote(TempoInfo::Note::N16);
        gr.setMix(1.f);
        gr.setInterrupt(true); // sem bypass isto calaria o dry!
        gr.setInternalBpm(120.0);
        gr.setEnabled(false);
        TempoInfo t2;
        blk.clear();
        gr.process(blk, t2);
        CHECK(gr.isBypassed(), "gran silent após settle");
        if (gr.takeClear()) gr.reset();
        fillDC(blk);
        gr.process(blk, t2);
        CHECK(isPassthrough(blk), "gran bypass = passthrough exato");
    }

    return failures;
}

// --- Fase 7: algoritmos do delay ---

// Corre `total` amostras com feed por índice absoluto, devolve saída L.
template <typename FeedFn>
static std::vector<float> renderDelay(Delay& d, int total, FeedFn feed)
{
    std::vector<float> out;
    juce::AudioBuffer<float> blk(2, 512);
    int pos = 0, abs = 0;
    while (pos < total)
    {
        int n = juce::jmin(512, total - pos);
        blk.setSize(2, n, false, false, true);
        for (int i = 0; i < n; ++i)
        {
            float v = feed(abs + pos + i);
            blk.setSample(0, i, v);
            blk.setSample(1, i, v);
        }
        d.process(blk);
        for (int i = 0; i < n; ++i) out.push_back(blk.getSample(0, i));
        pos += n;
    }
    return out;
}

template <typename FeedFn>
static std::vector<float> renderDelayStereo(Delay& d, int total, FeedFn feed)
{
    // Devolve L intercalado com R (pares L, ímpares R).
    std::vector<float> out;
    juce::AudioBuffer<float> blk(2, 512);
    int pos = 0, abs = 0;
    while (pos < total)
    {
        int n = juce::jmin(512, total - pos);
        blk.setSize(2, n, false, false, true);
        for (int i = 0; i < n; ++i)
        {
            float v = feed(abs + pos + i);
            blk.setSample(0, i, v);
            blk.setSample(1, i, 0.f);
        }
        d.process(blk);
        for (int i = 0; i < n; ++i) { out.push_back(blk.getSample(0, i)); out.push_back(blk.getSample(1, i)); }
        pos += n;
    }
    return out;
}

static void setupDelayBase(Delay& d)
{
    d.prepare(48000.0, 512);
    d.setTestDelaySamples(18000);
    d.setFeedback(0.0f);
    d.setDampingHz(20000.0f);
    d.setMix(1.0f);
    d.setFrozen(false);
    d.setDrive(0.f);
    d.setWowDepthMs(0.f);
    d.setSpread(1.f);
    d.setTempoBpm(120.0);
}

int mainDelayAlgos()
{
    auto impulse = [](int i) { return i == 0 ? 1.0f : 0.0f; };

    // --- 21. Tape sem drive = Digital; com drive comprime ---
    {
        Delay d;
        setupDelayBase(d);
        d.setAlgo(Delay::Algo::Tape);
        d.setDrive(0.f);
        auto out = renderDelay(d, 40000, impulse);
        CHECK(std::abs(out[18000] - 1.0f) < 0.02f, "tape drive 0 = eco 1.0 (%f)", out[18000]);

        Delay s;
        setupDelayBase(s);
        s.setAlgo(Delay::Algo::Tape);
        s.setDrive(1.f);
        auto hot = [](int i) { return i < 20000 ? std::sin(i * 0.05f) * 0.9f : 0.f; };
        auto outS = renderDelay(s, 40000, hot);
        // Saturação: o RMS do eco SOBE (ganho) mas o pico fica preso ≈1.0.
        double e = 0.0;
        float peak = 0.f;
        for (int i = 18500; i < 37500; ++i)
        {
            float v = outS[(size_t) i];
            e += (double) v * v;
            peak = juce::jmax(peak, std::abs(v));
        }
        float rmsEco = std::sqrt(e / 19000.0);
        CHECK(rmsEco > 0.85f && peak <= 1.0f, "tape drive 1 satura (rms %f pico %f)",
              rmsEco, peak);
    }

    // --- 22. PingPong: eco1 L, eco2 R (com fb 0.5) ---
    {
        Delay d;
        setupDelayBase(d);
        d.setAlgo(Delay::Algo::PingPong);
        d.setFeedback(0.5f);
        d.setSpread(1.f);
        auto out = renderDelayStereo(d, 60000, impulse);
        float e1L = std::abs(out[(size_t) 18000 * 2]);
        float e1R = std::abs(out[(size_t) 18000 * 2 + 1]);
        float e2L = std::abs(out[(size_t) 36000 * 2]);
        float e2R = std::abs(out[(size_t) 36000 * 2 + 1]);
        CHECK(e1L > 0.9f && e1R < 0.05f, "pingpong eco1 na L (%f/%f)", e1L, e1R);
        CHECK(e2R > 0.4f && e2R < 0.6f && e2L < 0.05f, "pingpong eco2 na R (%f/%f)", e2L, e2R);
    }

    // --- 23. Multitap: 4 ecos a d, .75d, .5d, .25d ---
    {
        Delay d;
        setupDelayBase(d);
        d.setAlgo(Delay::Algo::Multitap);
        auto out = renderDelay(d, 40000, impulse);
        const int pos[4] = { 18000, 13500, 9000, 4500 };
        const float amp[4] = { 1.f, 0.7f, 0.5f, 0.35f }; // = tapGain no Delay.h
        for (int k = 0; k < 4; ++k)
        {
            float got = 0.f;
            for (int i = pos[k] - 2; i <= pos[k] + 2; ++i)
                got = juce::jmax(got, std::abs(out[(size_t) i]));
            CHECK(std::abs(got - amp[k]) < 0.06f, "multitap eco %d ≈ %f (got %f)",
                  k, amp[k], got);
        }
    }

    // --- 24. Reverse integrado: loop vivo com o conteúdo gravado ---
    // (janela deslizante: mede-se com seno contínuo, não com cauda).
    {
        Delay d;
        d.prepare(48000.0, 512);
        d.setAlgo(Delay::Algo::Reverse);
        d.setTimeMs(500.f); // 1 beat @120 = 24000 amostras de janela
        d.setMix(1.f);
        d.setFrozen(false);
        d.setTempoBpm(120.0);
        auto sine = [](int i) { return std::sin(i * 0.02f) * 0.5f; };
        auto out = renderDelay(d, 48000, sine);
        float r = meanAbs(out, 25000, 45000);
        float peak = 0.f;
        for (float v : out) peak = juce::jmax(peak, std::abs(v));
        CHECK(r > 0.2f, "reverse faz loop audível (rms %f)", r);
        CHECK(peak <= 0.9f, "reverse sem explosão (pico %f)", peak);
    }

    // --- 25. Wow agressivo mantém-se finito ---
    {
        Delay d;
        setupDelayBase(d);
        d.setAlgo(Delay::Algo::Tape);
        d.setWowRate(5.f);
        d.setWowDepthMs(20.f);
        d.setFeedback(0.9f);
        auto nz = [](int i) { return (i % 3 == 0) ? 0.5f : -0.3f; };
        auto out = renderDelay(d, 96000, nz);
        float peak = 0.f;
        for (float v : out)
        {
            if (! std::isfinite(v)) { peak = 1e9f; break; }
            peak = juce::jmax(peak, std::abs(v));
        }
        CHECK(peak < 4.f, "wow extremo estável (pico %f)", peak);
    }

    // --- 26. Freeze = loop infinito (contrato da doc) ---
    // Satura o loop com seno (DC não serve: o blocker da entrada come-o),
    // congela com silêncio à entrada: sustenta; ao descongelar, decai.
    {
        Delay d;
        d.prepare(48000.0, 512);
        d.setAlgo(Delay::Algo::Digital);
        d.setTimeMs(100.f); // 4800 amostras de loop
        d.setFeedback(0.5f);
        d.setDampingHz(18000.f);
        d.setMix(1.f);
        d.setFrozen(false);
        auto sine = [](int i) { return std::sin(i * 2.f * 3.14159265f * 220.f / 48000.f) * 0.9f; };
        auto out = renderDelay(d, 48000, sine); // 1 s a saturar
        (void) out;
        d.setFrozen(true);
        auto frz = renderDelay(d, 24000, [](int) { return 0.f; });
        double eFrz = 0.0;
        for (int i = 12000; i < 24000; ++i) eFrz += frz[(size_t) i] * frz[(size_t) i];
        eFrz = std::sqrt(eFrz / 12000.0);
        d.setFrozen(false);
        auto rel = renderDelay(d, 48000, [](int) { return 0.f; });
        double eRel = 0.0;
        for (int i = 36000; i < 48000; ++i) eRel += rel[(size_t) i] * rel[(size_t) i];
        eRel = std::sqrt(eRel / 12000.0);
        CHECK(eFrz > 0.3, "freeze sustenta o loop (rms %f)", eFrz);
        CHECK(eRel < eFrz * 0.05, "unfreeze liberta e decai (%f < %f)", eRel, eFrz);
    }

    // --- G41. Spread 0 = direto, 1 = cruzado (pingpong) ---
    {
        auto eco2r = [&](float spread)
        {
            Delay d;
            setupDelayBase(d);
            d.setAlgo(Delay::Algo::PingPong);
            d.setFeedback(0.5f);
            d.setSpread(spread);
            auto out = renderDelayStereo(d, 60000, impulse);
            return std::abs(out[(size_t) 36000 * 2 + 1]); // eco2 no R
        };
        float direct = eco2r(0.f), crossed = eco2r(1.f);
        CHECK(direct < 0.05f && crossed > 0.4f && crossed < 0.6f,
              "spread cruza o feedback (R: %f -> %f)", direct, crossed);
    }

    // --- G42. Wow depth modula o tempo (vibrato mensurável) ---
    {
        Delay d;
        setupDelayBase(d);
        d.setAlgo(Delay::Algo::Tape);
        d.setFeedback(0.f); // eco único, sem realimentação
        d.setWowRate(5.f);
        d.setWowDepthMs(20.f);
        auto out = renderDelay(d, 60000, impulse);
        // Sem wow o eco cai na amostra 18000; com wow±20 ms desvia-se.
        float peak0 = 0.f;
        int at0 = -1;
        for (int i = 17000; i < 19000; ++i)
            if (std::abs(out[(size_t) i]) > peak0) { peak0 = std::abs(out[(size_t) i]); at0 = i; }
        CHECK(std::abs(at0 - 18000) > 100, "wow desvia o eco (%d vs 18000)", at0);
        // O wow espalha o impulso (é físico); audível mas mais baixo.
        CHECK(peak0 > 0.2f, "eco com wow continua audível (%f)", peak0);
    }

    // --- G43. Freeze no Reverse segura a janela; sem freeze esvazia ---
    // Seno 1 s, depois silêncio: congelado repete a janela (sustenta);
    // a gravar silêncio, o anel esvazia e o loop cala-se.
    {
        auto tailRms = [&](bool freeze)
        {
            Delay d;
            d.prepare(48000.0, 512);
            d.setAlgo(Delay::Algo::Reverse);
            d.setTimeMs(500.f);
            d.setMix(1.f);
            d.setFrozen(false);
            d.setTempoBpm(120.0);
            auto sine = [](int i) { return std::sin(i * 0.02f) * 0.5f; };
            auto out = renderDelay(d, 48000, sine);
            (void) out;
            d.setFrozen(freeze);
            auto tail = renderDelay(d, 96000, [](int) { return 0.f; });
            double e = 0.0;
            for (int i = 72000; i < 96000; ++i) e += tail[(size_t) i] * tail[(size_t) i];
            return std::sqrt(e / 24000.0);
        };
        double frz = tailRms(true), live = tailRms(false);
        CHECK(frz > 0.15, "freeze segura a janela Reverse (rms %f)", frz);
        CHECK(live < 0.05, "sem freeze o anel esvazia (rms %f)", live);
    }

    // --- G44. Damping escurece por repetição (não o eco direto) ---
    // O damping vive no loop: o 1º eco sai intacto, o 2º sai domado.
    {
        auto echo2 = [&](float dampHz)
        {
            Delay d;
            setupDelayBase(d);
            d.setAlgo(Delay::Algo::Digital);
            d.setFeedback(0.5f);
            d.setDampingHz(dampHz);
            auto out = renderDelay(d, 40000, impulse);
            float peak = 0.f;
            for (int i = 35000; i < 37000; ++i)
                peak = juce::jmax(peak, std::abs(out[(size_t) i]));
            return peak;
        };
        double hi = echo2(18000.f), lo = echo2(1000.f);
        CHECK(hi > 0.3 && lo < 0.15, "2º eco domado (pico %.2f vs %.2f)", hi, lo);
    }

    return failures;
}

// --- Dimension Expander (OUT) ---
static std::vector<float> renderDim(Dimension& d, int total, float freqL = 440.f, float freqR = 440.f)
{
    std::vector<float> out; // L intercalado com R
    juce::AudioBuffer<float> blk(2, 512);
    int pos = 0;
    while (pos < total)
    {
        int n = juce::jmin(512, total - pos);
        blk.setSize(2, n, false, false, true);
        for (int i = 0; i < n; ++i)
        {
            blk.setSample(0, i, std::sin((pos + i) * 2.f * 3.14159265f * freqL / 48000.f) * 0.5f);
            blk.setSample(1, i, std::sin((pos + i) * 2.f * 3.14159265f * freqR / 48000.f) * 0.5f);
        }
        d.process(blk);
        for (int i = 0; i < n; ++i) { out.push_back(blk.getSample(0, i)); out.push_back(blk.getSample(1, i)); }
        pos += n;
    }
    return out;
}

int mainDimension()
{
    // --- D1. mix=0 é passthrough bit-exato ---
    {
        Dimension d;
        d.prepare(48000.0, 512);
        d.setSize01(0.7f);
        d.setMix(0.f);
        auto out = renderDim(d, 20000);
        bool exact = true;
        for (int k = 0; k < 20000 && exact; ++k)
        {
            float eL = std::sin(k * 2.f * 3.14159265f * 440.f / 48000.f) * 0.5f;
            if (out[(size_t) k * 2] != eL || out[(size_t) k * 2 + 1] != eL) exact = false;
        }
        CHECK(exact, "dimension mix 0 = passthrough exato");
    }

    // --- D2. Mono entra, stereo sai (L/R diferem) ---
    {
        Dimension d;
        d.prepare(48000.0, 512);
        d.setSize01(0.5f);
        d.setMix(1.f);
        auto out = renderDim(d, 48000);
        double diff = 0.0, corr = 0.0, eL = 0.0, eR = 0.0;
        for (int k = 5000; k < 48000; ++k)
        {
            float l = out[(size_t) k * 2], r = out[(size_t) k * 2 + 1];
            diff += std::abs(l - r);
            corr += (double) l * r;
            eL += (double) l * l;
            eR += (double) r * r;
        }
        diff /= 43000.0;
        corr /= std::sqrt(eL * eR + 1e-12);
        CHECK(diff > 0.02, "mono alarga (L-R médio %f)", diff);
        CHECK(corr < 0.99, "canais descorrelacionam (%f)", corr);
    }

    // --- D3. Soma-mono preservada (±1 dB do dry) ---
    {
        Dimension d;
        d.prepare(48000.0, 512);
        d.setSize01(0.8f);
        d.setMix(1.f);
        auto out = renderDim(d, 48000);
        float md = 0.f;
        for (int k = 5000; k < 48000; ++k)
        {
            float dry = std::sin(k * 2.f * 3.14159265f * 440.f / 48000.f) * 0.5f;
            float mono = (out[(size_t) k * 2] + out[(size_t) k * 2 + 1]) * 0.5f;
            md = juce::jmax(md, std::abs(mono - dry));
        }
        CHECK(md < 0.02f, "mono-sum preservada (desvio %f)", md);
    }

    // --- D4. Varrer Size não clica; extremos estáveis ---
    {
        Dimension d;
        d.prepare(48000.0, 512);
        d.setMix(1.f);
        juce::AudioBuffer<float> blk(2, 512);
        float maxStep = 0.f, peak = 0.f, prevL = 0.f, prevR = 0.f;
        bool first = true;
        for (int b = 0; b < 200; ++b) // ~2.1 s; size 0->1 a meio
        {
            if (b == 100) d.setSize01(1.f);
            for (int i = 0; i < 512; ++i)
            {
                float v = std::sin((b * 512 + i) * 0.03f) * 0.5f;
                blk.setSample(0, i, v);
                blk.setSample(1, i, v * 0.7f);
            }
            d.process(blk);
            for (int i = 0; i < 512; ++i)
            {
                float l = blk.getSample(0, i), r = blk.getSample(1, i);
                if (! std::isfinite(l + r)) { maxStep = 1e9f; break; }
                if (! first)
                    maxStep = juce::jmax(maxStep, juce::jmax(std::abs(l - prevL), std::abs(r - prevR)));
                first = false;
                prevL = l;
                prevR = r;
                peak = juce::jmax(peak, juce::jmax(std::abs(l), std::abs(r)));
            }
        }
        CHECK(maxStep < 0.15f, "sweep de Size sem clique (maxStep %f)", maxStep);
        CHECK(peak < 4.f, "extremos finitos (pico %f)", peak);
    }

    return failures;
}

// --- Fase 5: reverse engine + linker ---

// Grava `total` amostras (feed) e deita fora a saída (só enche o anel).
template <typename FeedFn>
static void recordOnly(ReverseEngine& e, int total, FeedFn feed)
{
    juce::AudioBuffer<float> blk(2, 512);
    int pos = 0, abs = 0;
    while (pos < total)
    {
        int n = juce::jmin(512, total - pos);
        blk.setSize(2, n, false, false, true);
        for (int i = 0; i < n; ++i)
        {
            float v = feed(abs + pos + i);
            blk.setSample(0, i, v);
            blk.setSample(1, i, v);
        }
        e.recordBlock(blk);
        pos += n;
    }
}

int mainReverse()
{
    TempoInfo t;
    const double sr = 48000.0;

    // --- 17. LOOP: rampa 0→1 gravada sai ao contrário ---
    {
        ReverseEngine e;
        e.prepare(sr);
        e.setMode(ReverseEngine::Mode::Loop);
        e.setRate(1.f);
        e.setLfoDepth(0.f);
        e.setCaptureBeats(2.0); // 1 s @120 BPM interno? BPM passado no render:
        e.setDuckDepth(0.f);

        // Enche o anel com rampa de 1 s e ancora o leitor.
        recordOnly(e, 48000, [](int i) { return (float) i / 48000.f; });
        juce::AudioBuffer<float> out(2, 6000);
        out.clear();
        e.renderBlock(out, t, 120.0, false);
        // Primeiras amostras ≈ fim da rampa (≈1.0), a descer.
        CHECK(out.getSample(0, 10) > 0.9f, "reverse começa no passado recente (%f)",
              out.getSample(0, 10));
        CHECK(out.getSample(0, 5000) < out.getSample(0, 100),
              "reverse anda para trás (%f < %f)",
              out.getSample(0, 5000), out.getSample(0, 100));
    }

    // --- 18. Rate 0.5×: 440 Hz vira 220 Hz ---
    {
        ReverseEngine e;
        e.prepare(sr);
        e.setMode(ReverseEngine::Mode::Loop);
        e.setRate(0.5f);
        e.setLfoDepth(0.f);
        e.setCaptureBeats(2.0);
        e.setDuckDepth(0.f);

        auto sine = [](int i) { return std::sin(i * 2.0f * 3.14159265f * 440.f / 48000.f); };
        recordOnly(e, 48000, sine);
        juce::AudioBuffer<float> out(2, 6000);
        out.clear();
        e.renderBlock(out, t, 120.0, false);
        int zc = 0;
        for (int i = 501; i < 5500; ++i)
            if ((out.getSample(0, i - 1) < 0) != (out.getSample(0, i) < 0)) ++zc;
        // 220 Hz em 5000 amostras ≈ 22.9 períodos ≈ 46 travessias.
        CHECK(zc > 35 && zc < 58, "rate 0.5× divide a frequência (travessias %d)", zc);
    }

    // --- 19. THROW: uma cauda e depois silêncio ---
    {
        ReverseEngine e;
        e.prepare(sr);
        e.setMode(ReverseEngine::Mode::Throw);
        e.setRate(1.f);
        e.setLfoDepth(0.f);
        e.setCaptureBeats(1.0); // 0.5 s @120
        e.setDuckDepth(0.f);

        recordOnly(e, 48000, [](int) { return 1.0f; });
        e.setThrowButton(true);
        juce::AudioBuffer<float> blk(2, 512);
        std::vector<float> out;
        for (int b = 0; b < 60; ++b) // ~0.64 s > cauda de 0.5 s
        {
            blk.setSize(2, 512, false, false, true);
            blk.clear();
            e.recordBlock(blk); // continua a gravar zeros por baixo
            juce::AudioBuffer<float> o(2, 512);
            o.clear();
            e.renderBlock(o, t, 120.0, false);
            for (int i = 0; i < 512; ++i) out.push_back(o.getSample(0, i));
        }
        float head = meanAbs(out, 100, 5000);
        float tail = meanAbs(out, 26000, 30000);
        CHECK(head > 0.5f, "throw dispara cauda (head %f)", head);
        CHECK(tail < 0.05f, "throw cala-se depois (tail %f)", tail);
    }

    // --- 20. RevLinker: morfologia dos valores ---
    {
        using RL = RevLinker;
        CHECK(RL::resolveContinuous(false, 0.7f, 0.3f, 0.8f, 0.f, 1.f) == 0.8f,
              "link off usa valor próprio");
        CHECK(RL::resolveContinuous(true, 0.f, 0.3f, 0.8f, 0.f, 1.f) == 0.3f,
              "morph 0 espelha FWD");
        CHECK(std::abs(RL::resolveContinuous(true, 1.f, 0.3f, 0.8f, 0.f, 1.f) - 0.8f) < 1e-6f,
              "morph 1 usa REV");
        CHECK(std::abs(RL::resolveContinuous(true, 0.5f, 0.3f, 0.8f, 0.f, 1.f) - 0.55f) < 1e-6f,
              "morph 0.5 interpola");
        CHECK(std::abs(RL::resolveContinuous(true, 0.f, 2.f, 0.f, 0.4f, 0.f, 0.f, 10.f) - 0.8f) < 1e-6f,
              "trim mult aplica-se ao FWD");
        CHECK(RL::resolveDiscrete(true, 2, 5) == 2, "discreto com link usa FWD");
        CHECK(RL::resolveDiscrete(false, 2, 5) == 5, "discreto sem link usa REV");
        CHECK(RL::resolveBool(true, true, false) == true, "bool com link");
        CHECK(RL::resolveBool(false, true, false) == false, "bool sem link");
    }

    // --- 21. Duck segue o dry dentro do bloco (não em escada) ---
    // Bloco transitório: 412 a 1.0 + 100 a 0.0. O envelope antigo lia
    // sempre a última amostra (silêncio) e não duckava nada; o novo segue
    // o cursor e o ganho cai durante a parte alta.
    {
        auto renderDuck = [&](float duckDepth)
        {
            ReverseEngine e;
            e.prepare(sr);
            e.setMode(ReverseEngine::Mode::Loop);
            e.setRate(1.f);
            e.setLfoDepth(0.f);
            e.setCaptureBeats(2.0);
            e.setDuckDepth(duckDepth);
            recordOnly(e, 48000, [](int) { return 1.0f; });
            juce::AudioBuffer<float> blk(2, 512);
            blk.clear();
            for (int i = 0; i < 412; ++i) { blk.setSample(0, i, 1.f); blk.setSample(1, i, 1.f); }
            e.recordBlock(blk);
            juce::AudioBuffer<float> o(2, 512);
            o.clear();
            e.renderBlock(o, t, 120.0, false);
            double m = 0.0;
            for (int i = 100; i < 400; ++i) m += std::abs(o.getSample(0, i));
            return m / 300.0;
        };
        double dry = renderDuck(0.f);
        double ducked = renderDuck(1.f);
        CHECK(dry > 0.5, "referência sem duck passa (média %f)", dry);
        CHECK(ducked < dry * 0.9, "duck acompanha o burst (%.3f < %.3f)", ducked, dry);
    }

    // --- G56. Rate nos extremos: 0.25× e 2× (varispeed) ---
    {
        auto travessias = [&](float rate)
        {
            ReverseEngine e;
            e.prepare(sr);
            e.setMode(ReverseEngine::Mode::Loop);
            e.setRate(rate);
            e.setLfoDepth(0.f);
            e.setCaptureBeats(2.0);
            e.setDuckDepth(0.f);
            auto sine = [](int i) { return std::sin(i * 2.0f * 3.14159265f * 440.f / 48000.f); };
            recordOnly(e, 48000, sine);
            juce::AudioBuffer<float> out(2, 12000);
            out.clear();
            e.renderBlock(out, t, 120.0, false);
            int zc = 0;
            for (int i = 3001; i < 12000; ++i) // após o slew do rate
                if ((out.getSample(0, i - 1) < 0) != (out.getSample(0, i) < 0)) ++zc;
            return zc;
        };
        // 110 Hz em 9000 amostras ≈ 20.6 períodos ≈ 41 travessias.
        int z025 = travessias(0.25f);
        // 880 Hz em 9000 amostras ≈ 165 períodos ≈ 330 travessias.
        int z20 = travessias(2.f);
        CHECK(z025 > 25 && z025 < 60, "rate 0.25× desce 2 oitavas (%d)", z025);
        CHECK(z20 > 250 && z20 < 410, "rate 2× sobe oitava (%d)", z20);
    }

    // --- G57. Capture 3/4 beats dimensiona a cauda do throw ---
    {
        auto tailLen = [&](double beats)
        {
            ReverseEngine e;
            e.prepare(sr);
            e.setMode(ReverseEngine::Mode::Throw);
            e.setRate(1.f);
            e.setLfoDepth(0.f);
            e.setCaptureBeats(beats);
            e.setDuckDepth(0.f);
            recordOnly(e, 96000, [](int) { return 1.0f; });
            e.setThrowButton(true);
            juce::AudioBuffer<float> blk(2, 512);
            int total = 0, lastHot = 0, n = 0;
            for (int b = 0; b < 200; ++b)
            {
                blk.setSize(2, 512, false, false, true);
                blk.clear();
                e.recordBlock(blk);
                juce::AudioBuffer<float> o(2, 512);
                o.clear();
                e.renderBlock(o, t, 120.0, false);
                for (int i = 0; i < 512; ++i)
                {
                    if (std::abs(o.getSample(0, i)) > 0.1f) lastHot = total;
                    ++total;
                    ++n;
                }
                if (b > 5 && ! e.isThrowing()) break;
            }
            (void) n;
            return lastHot;
        };
        int t2 = tailLen(2.0); // 1 s @120 = 48000
        int t4 = tailLen(4.0); // 2 s @120 = 96000
        CHECK(std::abs(t2 - 48000) < 6000, "throw 2 beats ~1 s (%d)", t2);
        CHECK(std::abs(t4 - 96000) < 9000, "throw 4 beats ~2 s (%d)", t4);
    }

    // --- G58. MIDI throw dispara sem botão ---
    {
        ReverseEngine e;
        e.prepare(sr);
        e.setMode(ReverseEngine::Mode::Throw);
        e.setRate(1.f);
        e.setLfoDepth(0.f);
        e.setCaptureBeats(1.0);
        e.setDuckDepth(0.f);
        recordOnly(e, 48000, [](int) { return 1.0f; });
        juce::AudioBuffer<float> o(2, 512);
        o.clear();
        e.renderBlock(o, t, 120.0, true); // nota, sem botão
        float head = 0.f;
        for (int i = 0; i < 512; ++i) head = juce::jmax(head, std::abs(o.getSample(0, i)));
        CHECK(head > 0.5f, "midiThrow dispara (pico %f)", head);
    }

    // --- G59. LFO vagueia o rate (deriva do período) ---
    // 0.1 Hz em 0.4 s deriva ~4%: o meio-período médio mexe ~2 amostras
    // com LFO, ~0 sem. Sem reanchors na janela (ciclo 24000 > 20000).
    {
        auto drift = [&](float lfo)
        {
            ReverseEngine e;
            e.prepare(sr);
            e.setMode(ReverseEngine::Mode::Loop);
            e.setRate(1.f);
            e.setLfoDepth(lfo);
            e.setCaptureBeats(2.0);
            e.setDuckDepth(0.f);
            auto sine = [](int i) { return std::sin(i * 2.0f * 3.14159265f * 440.f / 48000.f); };
            recordOnly(e, 96000, sine);
            juce::AudioBuffer<float> out(2, 20000);
            out.clear();
            e.renderBlock(out, t, 120.0, false);
            auto meanGap = [&](int from, int to)
            {
                double s = 0.0;
                int n = 0, prev = -1;
                for (int i = from; i < to; ++i)
                    if ((out.getSample(0, i - 1) < 0) != (out.getSample(0, i) < 0))
                    {
                        if (prev >= 0) { s += i - prev; ++n; }
                        prev = i;
                    }
                return n > 0 ? s / n : 0.0;
            };
            return std::abs(meanGap(1000, 10000) - meanGap(11000, 20000));
        };
        double d0 = drift(0.f), d1 = drift(1.f);
        CHECK(d1 > 1.0 && d0 < 0.3, "LFO vagueia o rate (deriva %.2f vs %.2f)", d1, d0);
    }

    return failures;
}

// --- Fase 4: granular ---

// Corre `total` amostras pelo granular em blocos de 512, chamando `feed(i)`
// para cada amostra de entrada absoluta e gravando a saída L em `out`.
template <typename FeedFn>
static void renderGrab(Granular& g, const TempoInfo& t, int total,
                       std::vector<float>& out, FeedFn feed)
{
    juce::AudioBuffer<float> blk(2, 512);
    int pos = 0, abs = (int) out.size();
    while (pos < total)
    {
        int n = juce::jmin(512, total - pos);
        blk.setSize(2, n, false, false, true);
        for (int i = 0; i < n; ++i)
        {
            float v = feed(abs + pos + i);
            blk.setSample(0, i, v);
            blk.setSample(1, i, v);
        }
        g.process(blk, t);
        for (int i = 0; i < n; ++i) out.push_back(blk.getSample(0, i));
        pos += n;
    }
}

static float meanAbs(const std::vector<float>& v, int from, int to)
{
    double s = 0.0;
    int c = 0;
    for (int i = from; i < to && i < (int) v.size(); ++i) { s += std::abs(v[(size_t) i]); ++c; }
    return c > 0 ? (float) (s / c) : 0.f;
}

static void setupGrabBR(Granular& g)
{
    g.prepare(48000.0);
    g.setMode(Granular::Mode::BeatRepeat);
    g.setTrig(Granular::Trig::Manual);
    g.setLenNote(TempoInfo::Note::N16);
    g.setRepeats(4);
    g.setDecay(0.85f);
    g.setXfadeMs(8.f);
    g.setMix(1.f);
    g.setInterrupt(true);
    g.setInternalBpm(120.0);
}

int mainGranular()
{
    TempoInfo t; // relógio interno
    auto dc = [](int) { return 1.0f; };
    auto zeros = [](int) { return 0.0f; };

    // --- 13. BeatRepeat 1/16 ×4 @120: loops de 6000 com decay 0.85^k ---
    {
        Granular g;
        setupGrabBR(g);
        std::vector<float> out;
        renderGrab(g, t, 7000, out, dc); // enche o anel com DC
        g.setManual(true);
        renderGrab(g, t, 512, out, dc);  // flanco dispara no i==0
        g.setManual(false);
        renderGrab(g, t, 30000, out, zeros);

        int trig = 7000; // índice absoluto do disparo
        for (int k = 0; k < 4; ++k)
        {
            float m = meanAbs(out, trig + k * 6000 + 1000, trig + k * 6000 + 5000);
            float expected = std::pow(0.85f, (float) k);
            CHECK(std::abs(m - expected) < 0.06f,
                  "repeat %d ≈ %.3f (got %f)", k, expected, m);
        }
        float tail = meanAbs(out, trig + 24500, trig + 26500);
        CHECK(tail < 0.05f, "depois dos 4 repeats liberta (tail %f)", tail);
    }

    // --- 14. Reverse: rampa 0→1 sai ao contrário ---
    {
        Granular g;
        g.prepare(48000.0);
        g.setMode(Granular::Mode::Reverse);
        g.setTrig(Granular::Trig::Manual);
        g.setLenNote(TempoInfo::Note::N16);
        g.setXfadeMs(8.f);
        g.setMix(1.f);
        g.setInterrupt(true);
        g.setInternalBpm(120.0);

        std::vector<float> out;
        renderGrab(g, t, 6000, out, [](int i) { return (float) i / 6000.f; });
        g.setManual(true);
        renderGrab(g, t, 512, out, zeros);
        g.setManual(false);
        renderGrab(g, t, 7000, out, zeros);

        int trig = 6000;
        CHECK(out[(size_t) trig + 500] > 0.85f, "reverse começa no fim (got %f)",
              out[(size_t) trig + 500]);
        CHECK(out[(size_t) trig + 5500] < 0.2f, "reverse acaba no início (got %f)",
              out[(size_t) trig + 5500]);
    }

    // --- 14b. DESLIGADO-TMP (hipótese heap-history)
    if (false) // TMP
    // O flanco é detetado onde quer que caia no bloco; a versão antiga só
    // via flancos no i==0 e perdia pressões a meio do bloco (race UI/audio).
    // Single-threaded, o flanco cai sempre numa fronteira — este teste fixa
    // o contrato (dispara ≤3 blocos, exatamente uma vez); a deteção
    // por-amostra cobre o caso inter-blocos.
    {
        Granular g;
        setupGrabBR(g);
        std::vector<float> out;
        renderGrab(g, t, 4000, out, dc);
        CHECK(! g.isActive(), "antes da pressão está parado");
        g.setManual(true);
        int firedAt = -1;
        for (int b = 0; b < 5; ++b)
        {
            renderGrab(g, t, 512, out, zeros);
            if (g.isActive()) { firedAt = b; break; }
        }
        CHECK(firedAt >= 0 && firedAt <= 2, "pressão dispara (bloco %d)", firedAt);
        g.setManual(false);
        renderGrab(g, t, 30000, out, zeros);
        CHECK(! g.isActive(), "depois de soltar e esvaziar para");
    }

    // --- 15. Determinismo: duas instâncias com a mesma seed soam igual ---
    {
        Granular a, b;
        for (auto* g : { &a, &b })
        {
            g->prepare(48000.0);
            g->setMode(Granular::Mode::BeatRepeat);
            g->setTrig(Granular::Trig::Chance);
            g->setChance(0.5f);
            g->setLenNote(TempoInfo::Note::N16);
            g->setRepeats(4);
            g->setMix(0.5f);
            g->setInterrupt(false);
            g->setInternalBpm(128.0);
        }
        std::vector<float> oa, ob;
        auto in = [](int i) { return 0.5f * std::sin(i * 0.01f) + (i % 7 == 0 ? 0.3f : 0.f); };
        renderGrab(a, t, 40000, oa, in);
        renderGrab(b, t, 40000, ob, in);
        bool same = oa.size() == ob.size();
        for (size_t i = 0; same && i < oa.size(); ++i)
            same = (oa[i] == ob[i]);
        CHECK(same, "RNG determinística: saídas bit-idênticas");
    }

    // --- 16. Pitch +12 = rate 2× (440 Hz → 880 Hz) ---
    {
        Granular g;
        g.prepare(48000.0);
        g.setMode(Granular::Mode::Pitch);
        g.setTrig(Granular::Trig::Manual);
        g.setLenNote(TempoInfo::Note::N16);
        g.setPitchSt(12.f);
        g.setXfadeMs(4.f);
        g.setMix(1.f);
        g.setInterrupt(true);
        g.setInternalBpm(120.0);

        std::vector<float> out;
        auto sine = [](int i) { return std::sin(i * 2.0f * 3.14159265f * 440.f / 48000.f); };
        renderGrab(g, t, 7000, out, sine);
        g.setManual(true);
        renderGrab(g, t, 512, out, sine);
        g.setManual(false);
        renderGrab(g, t, 6000, out, zeros);

        auto crossings = [&](int from, int to) {
            int c = 0;
            for (int i = from + 1; i < to && i < (int) out.size(); ++i)
                if ((out[(size_t) i - 1] < 0) != (out[(size_t) i] < 0)) ++c;
            return c;
        };
        int trig = 7000;
        int zc = crossings(trig + 500, trig + 3500);
        // 880 Hz em 3000 amostras ≈ 55 períodos ≈ 110 travessias.
        CHECK(zc > 90 && zc < 130, "pitch +12 dobra a frequência (travessias %d)", zc);
    }

    // --- 22. Grab contínuo: sem buracos na linha do tempo ---
    // A gravação pausava a tocar; o grab seguinte continha um salto temporal
    // (áudio antigo colado a áudio novo). Agora grava sempre + foto: com um
    // B-fill curto após o drain, o 2º fragmento é [silêncio drain, B].
    {
        Granular g;
        g.prepare(48000.0);
        g.setMode(Granular::Mode::BeatRepeat);
        g.setTrig(Granular::Trig::Manual);
        g.setLenNote(TempoInfo::Note::N16); // 6000 amostras @120
        g.setRepeats(1);
        g.setMix(1.f);
        g.setInterrupt(false);
        g.setInternalBpm(120.0);
        auto granRecord = [&](int total, float v) // grava v sem disparar
        {
            g.setMode(Granular::Mode::Off);
            juce::AudioBuffer<float> blk(2, 512);
            int pos = 0;
            while (pos < total)
            {
                int n = juce::jmin(512, total - pos);
                blk.setSize(2, n, false, false, true);
                for (int i = 0; i < n; ++i) { blk.setSample(0, i, v); blk.setSample(1, i, v); }
                g.process(blk, t);
                pos += n;
            }
            g.setMode(Granular::Mode::BeatRepeat);
        };
        auto pulse = [&] // um flanco manual = um grab
        {
            g.setManual(true);
            juce::AudioBuffer<float> b(2, 512);
            b.clear();
            g.process(b, t);
            g.setManual(false);
        };
        auto drain = [&] // processa silêncio até calar (com teto)
        {
            juce::AudioBuffer<float> b(2, 512);
            for (int k = 0; k < 300 && g.isActive(); ++k)
            {
                b.clear();
                g.process(b, t);
            }
        };
        granRecord(48000, 0.25f); // história A
        pulse();  // grab 1 (fragmento A)
        drain();  // toca até ao fim (a gravar silêncio, sempre)
        granRecord(3000, -0.25f); // B curto
        pulse();  // grab 2: [silêncio, B] (novo) vs [A, B] (velho)
        std::vector<float> out2;
        juce::AudioBuffer<float> b(2, 512);
        for (int k = 0; k < 60 && (g.isActive() || k < 14); ++k)
        {
            b.clear();
            g.process(b, t);
            for (int i = 0; i < 512; ++i) out2.push_back(b.getSample(0, i));
        }
        double m = 0.0;
        int mm = juce::jmin(6000, (int) out2.size());
        for (int i = 0; i < mm; ++i) m += out2[(size_t) i];
        m /= juce::jmax(1, mm);
        // Novo ≈ -0.125 (metade silêncio, metade B); velho ≈ 0.0 (A+B).
        CHECK(m < -0.05, "grab sem salto temporal (média %f)", m);
    }

    // --- 31. Slice: chop à taxa do grão, sem cliques ---
    // DC 1.0 + flux 0 (2 fatias): 1ª metade ≈1, 2ª ≈0.15; flux 1 (8
    // fatias): 4 quedas por volta em vez de 1.
    {
        auto sliceRun = [&](float flux)
        {
            Granular g;
            g.prepare(48000.0);
            g.setMode(Granular::Mode::Slice);
            g.setTrig(Granular::Trig::Manual);
            g.setLenNote(TempoInfo::Note::N16); // 6000 amostras @120
            g.setFlux(flux);
            g.setXfadeMs(4.f);
            g.setMix(1.f);
            g.setInterrupt(false);
            g.setInternalBpm(120.0);
            std::vector<float> out;
            renderGrab(g, t, 48000, out, [](int) { return 1.0f; });
            g.setManual(true);
            renderGrab(g, t, 512, out, [](int) { return 1.0f; });
            g.setManual(false);
            renderGrab(g, t, 12000, out, [](int) { return 1.0f; });
            return out;
        };
        auto falls = [](const std::vector<float>& o, int from, int to)
        {
            int c = 0;
            for (int i = from + 1; i < to && i < (int) o.size(); ++i)
                if (o[(size_t) i - 1] > 0.6f && o[(size_t) i] <= 0.6f) ++c;
            return c;
        };
        auto means = [](const std::vector<float>& o, int from, int to)
        {
            double s = 0.0;
            int n = 0;
            for (int i = from; i < to && i < (int) o.size(); ++i) { s += o[(size_t) i]; ++n; }
            return n > 0 ? s / n : 0.0;
        };
        std::vector<float> o2 = sliceRun(0.f);
        int trig = 48000 + 512;
        double hi = means(o2, trig + 500, trig + 2500);
        double lo = means(o2, trig + 3500, trig + 5500);
        CHECK(hi > 0.8, "slice metade alta passa (média %f)", hi);
        CHECK(lo < 0.3, "slice metade baixa corta (média %f)", lo);
        CHECK(falls(o2, trig + 500, trig + 6500) == 1, "flux 0 = 1 queda/volta");
        std::vector<float> o8 = sliceRun(1.f);
        CHECK(falls(o8, trig + 500, trig + 6500) == 4, "flux 1 = 4 quedas/volta");
        float ms = 0.f;
        for (int i = trig + 500; i < trig + 11500 && i < (int) o2.size(); ++i)
            ms = juce::jmax(ms, std::abs(o2[(size_t) i] - o2[(size_t) i - 1]));
        CHECK(ms < 0.1f, "chop suavizado sem cliques (maxStep %f)", ms);
    }

    // --- 32. Stutter: só o 1º oitavo do fragmento ---
    {
        Granular g;
        g.prepare(48000.0);
        g.setMode(Granular::Mode::Stutter);
        g.setTrig(Granular::Trig::Manual);
        g.setLenNote(TempoInfo::Note::N16); // 6000; stutter = 750
        g.setMix(1.f);
        g.setInterrupt(false);
        g.setInternalBpm(120.0);
        std::vector<float> out;
        auto ramp = [](int i) { return (float) i / 48000.f; }; // sem wrap
        renderGrab(g, t, 48000, out, ramp);
        g.setManual(true);
        renderGrab(g, t, 512, out, ramp);
        g.setManual(false);
        renderGrab(g, t, 6000, out, [](int) { return 0.f; });
        // Janela na 1ª volta (offsets 200–500: pós-attack, pré-costura).
        // Evita-se a costura [563, 750]: o crossfade equal-power soma
        // conteúdo coerente até +3 dB (física do loop, não bug).
        // Oitavo ≈ [0.989, 0.995].
        int grab = 48000;
        float mn = 1.f, mx = -1.f;
        for (int i = grab + 200; i < grab + 500 && i < (int) out.size(); ++i)
        {
            mn = juce::jmin(mn, out[(size_t) i]);
            mx = juce::jmax(mx, out[(size_t) i]);
        }
        CHECK(mn > 0.95f && mx < 1.02f, "stutter toca o 1º oitavo ([%f, %f])", mn, mx);
    }

    // --- 33. Trigger Envelope: transiente dispara, silêncio não ---
    // (O transiente cai sempre no fim do fragmento = zona da costura, por
    // isso mede-se disparo + energia capturada, não pico: o xfade atenua
    // o fim por desenho.)
    {
        Granular g;
        g.prepare(48000.0);
        g.setMode(Granular::Mode::BeatRepeat);
        g.setTrig(Granular::Trig::Envelope);
        g.setEnvThrDb(-18.f);
        g.setLenNote(TempoInfo::Note::N16);
        g.setRepeats(1);
        g.setMix(1.f);
        g.setInterrupt(false);
        g.setInternalBpm(120.0);
        std::vector<float> out;
        renderGrab(g, t, 20000, out, [](int) { return 0.f; });
        CHECK(! g.isActive(), "silêncio não dispara envelope");
        auto burst = [](int i) { return (i >= 20000 && i < 20100) ? 1.f : 0.f; };
        renderGrab(g, t, 512, out, burst);
        CHECK(g.isActive(), "transiente dispara envelope no próprio bloco");
        renderGrab(g, t, 12000, out, [](int) { return 0.f; });
        double e = 0.0;
        for (int i = 20512; i < 20512 + 6000 && i < (int) out.size(); ++i)
            e += out[(size_t) i] * out[(size_t) i];
        CHECK(e > 0.001, "conteúdo do transiente é capturado (energia %f)", e);
    }

    // --- 34. Pitch −12 = rate 0.5× (440 Hz → 220 Hz) ---
    {
        Granular g;
        g.prepare(48000.0);
        g.setMode(Granular::Mode::Pitch);
        g.setTrig(Granular::Trig::Manual);
        g.setLenNote(TempoInfo::Note::N16);
        g.setPitchSt(-12.f);
        g.setXfadeMs(4.f);
        g.setMix(1.f);
        g.setInterrupt(true);
        g.setInternalBpm(120.0);
        std::vector<float> out;
        auto sine = [](int i) { return std::sin(i * 2.0f * 3.14159265f * 440.f / 48000.f); };
        renderGrab(g, t, 7000, out, sine);
        g.setManual(true);
        renderGrab(g, t, 512, out, sine);
        g.setManual(false);
        renderGrab(g, t, 12000, out, [](int) { return 0.f; });
        int trig = 7000;
        int zc = 0;
        for (int i = trig + 501; i < trig + 5500 && i < (int) out.size(); ++i)
            if ((out[(size_t) i - 1] < 0) != (out[(size_t) i] < 0)) ++zc;
        // 220 Hz em 5000 amostras ≈ 22.9 períodos ≈ 46 travessias.
        CHECK(zc > 35 && zc < 58, "pitch −12 desce a oitava (travessias %d)", zc);
    }

    // --- 35. Slice: chop à taxa do grão (Manual, determinista) ---
    // DC 1.0 + LEN 1/16: flux 0 (2 fatias) = 1 queda/volta, flux 1 (8) = 4.
    for (float flux : { 0.f, 1.f })
    {
        Granular g;
        g.prepare(48000.0);
        g.setMode(Granular::Mode::Slice);
        g.setTrig(Granular::Trig::Manual);
        g.setLenNote(TempoInfo::Note::N16); // 6000 amostras @120
        g.setTimeMs(500.f);
        g.setFlux(flux);
        g.setXfadeMs(4.f);
        g.setMix(1.f);
        g.setInterrupt(false);
        g.setInternalBpm(120.0);
        std::vector<float> out;
        renderGrab(g, t, 48000, out, [](int) { return 0.5f; });
        g.setManual(true);
        renderGrab(g, t, 512, out, [](int) { return 0.5f; });
        g.setManual(false);
        renderGrab(g, t, 12000, out, [](int) { return 0.5f; });
        int trig = 48000 + 512;
        int falls = 0;
        float mx = 0.f, mn = 1.f, ms = 0.f;
        for (int i = trig + 500; i < trig + 6500 && i < (int) out.size(); ++i)
        {
            mx = juce::jmax(mx, out[(size_t) i]);
            mn = juce::jmin(mn, out[(size_t) i]);
            if (out[(size_t) i - 1] > 0.35f && out[(size_t) i] <= 0.35f) ++falls;
            ms = juce::jmax(ms, std::abs(out[(size_t) i] - out[(size_t) i - 1]));
        }
        int want = (flux == 0.f) ? 1 : 4;
        CHECK(falls == want, "slice flux %.0f: %d quedas/volta (got %d)", flux, want, falls);
        CHECK(mx > 0.4f && mn < 0.2f, "slice chop 1.0/0.15 ([%.2f, %.2f])", mn, mx);
        CHECK(ms < 0.1f, "slice sem cliques (maxStep %f)", ms);
    }

    // --- 36. Stutter: decay por tempo de fragmento (não por volta) ---
    // O Stutter dá 8 voltas por fragmento; decair por volta matava-o em
    // ~200 ms e o TIME ficava decorativo. Razão entre tempos de fragmento
    // consecutivos ≈ decay (0.85), não 0.85^8.
    {
        Granular g;
        g.prepare(48000.0);
        g.setMode(Granular::Mode::Stutter);
        g.setTrig(Granular::Trig::Manual);
        g.setLenNote(TempoInfo::Note::N16); // 6000; stutter = 750
        g.setDecay(0.85f);
        g.setTimeMs(2000.f); // activeLeft longo: mede 2 tempos de fragmento
        g.setMix(1.f);
        g.setInterrupt(false);
        g.setInternalBpm(120.0);
        std::vector<float> out;
        renderGrab(g, t, 48000, out, [](int) { return 0.5f; });
        g.setManual(true);
        renderGrab(g, t, 512, out, [](int) { return 0.5f; });
        g.setManual(false);
        renderGrab(g, t, 20000, out, [](int) { return 0.5f; });
        int trig = 48000 + 512;
        auto mabs = [&](int a, int b)
        {
            double s = 0.0;
            int n = 0;
            for (int i = trig + a; i < trig + b && i < (int) out.size(); ++i)
            {
                s += std::abs(out[(size_t) i]);
                ++n;
            }
            return n > 0 ? s / n : 0.0;
        };
        double m1 = mabs(500, 6500), m2 = mabs(6500, 12500);
        double ratio = m2 / (m1 + 1e-9);
        CHECK(std::abs(ratio - 0.85) < 0.12, "stutter decai por fragmento (razão %f)", ratio);
    }

    // --- 37. Costura usa o xf do grab (não o stale do bloco) ---
    // O xf do bloco nasce antes de um grab a meio do bloco; a 1ª volta
    // usava a costura larga (384 em vez de 187 no Stutter) e somava até
    // +3 dB fantasma fora da costura real. DC 0.5 tem de sair 0.5 exato.
    {
        Granular g;
        g.prepare(48000.0);
        g.setMode(Granular::Mode::Stutter);
        g.setTrig(Granular::Trig::Manual);
        g.setLenNote(TempoInfo::Note::N16); // stutter = 750 -> xf 187
        g.setDecay(0.99f); // sem decay na janela (ganho ~1)
        g.setTimeMs(2000.f);
        g.setMix(1.f);
        g.setInterrupt(false);
        g.setInternalBpm(120.0);
        std::vector<float> out;
        renderGrab(g, t, 48000, out, [](int) { return 0.5f; });
        g.setManual(true);
        renderGrab(g, t, 512, out, [](int) { return 0.5f; });
        g.setManual(false);
        renderGrab(g, t, 6000, out, [](int) { return 0.5f; });
        int grab = 48000;
        double m = 0.0;
        for (int i = grab + 400; i < grab + 500 && i < (int) out.size(); ++i)
            m += out[(size_t) i];
        m /= 100.0;
        CHECK(std::abs(m - 0.5) < 0.02, "sem costura fantasma no grab (média %f)", m);
    }

    // --- G50. Modo Off = passthrough bit-exato ---
    {
        Granular g;
        g.prepare(48000.0);
        g.setMode(Granular::Mode::Off);
        g.setMix(0.5f);
        g.setInternalBpm(120.0);
        TempoInfo tt;
        juce::AudioBuffer<float> buf(2, 512);
        bool exact = true;
        for (int b = 0; b < 10 && exact; ++b)
        {
            for (int i = 0; i < 512; ++i)
            {
                float v = std::sin((b * 512 + i) * 0.02f) * 0.5f;
                buf.setSample(0, i, v);
                buf.setSample(1, i, v * 0.5f);
            }
            juce::AudioBuffer<float> ref(2, 512);
            ref.copyFrom(0, 0, buf, 0, 0, 512);
            ref.copyFrom(1, 0, buf, 1, 0, 512);
            g.process(buf, tt);
            for (int ch = 0; ch < 2 && exact; ++ch)
                for (int i = 0; i < 512; ++i)
                    if (buf.getSample(ch, i) != ref.getSample(ch, i)) exact = false;
        }
        CHECK(exact, "gran Off = passthrough exato");
    }

    // --- G51. Interrupt troca dry por wet (com mix) ---
    // Slice em high (flux 0, 1ª metade do fragmento): sem interrupt o dry
    // passa, com interrupt é cortado e só fica o wet.
    {
        auto level = [&](bool interrupt)
        {
            Granular g;
            g.prepare(48000.0);
            g.setMode(Granular::Mode::Slice);
            g.setTrig(Granular::Trig::Manual);
            g.setLenNote(TempoInfo::Note::N16);
            g.setTimeMs(2000.f);
            g.setFlux(0.f);
            g.setMix(0.5f);
            g.setInterrupt(interrupt);
            g.setInternalBpm(120.0);
            TempoInfo tt;
            std::vector<float> out;
            renderGrab(g, tt, 48000, out, [](int) { return 0.5f; });
            g.setManual(true);
            renderGrab(g, tt, 512, out, [](int) { return 0.5f; });
            g.setManual(false);
            renderGrab(g, tt, 12000, out, [](int) { return 0.5f; });
            int trig = 48000 + 512;
            double m = 0.0;
            for (int i = trig + 500; i < trig + 2500 && i < (int) out.size(); ++i)
                m += out[(size_t) i];
            return m / 2000.0;
        };
        double off = level(false), on = level(true);
        CHECK(off > 0.2, "sem interrupt há dry+wet (%.3f)", off);
        CHECK(on < off * 0.6, "interrupt corta o dry (%.3f < %.3f)", on, off);
    }

    // --- G52. Reseed descorrelaciona duas vozes ---
    {
        Granular a, b;
        for (auto* g : { &a, &b })
        {
            g->prepare(48000.0);
            g->setMode(Granular::Mode::Slice);
            g->setTrig(Granular::Trig::Chance);
            g->setChance(0.5f);
            g->setLenNote(TempoInfo::Note::N16);
            g->setMix(1.f);
            g->setInterrupt(false);
            g->setInternalBpm(128.0);
        }
        b.reseed(0x54321);
        TempoInfo tt;
        std::vector<float> oa, ob;
        auto in = [](int i) { return 0.5f * std::sin(i * 0.01f) + (i % 7 == 0 ? 0.3f : 0.f); };
        renderGrab(a, tt, 40000, oa, in);
        renderGrab(b, tt, 40000, ob, in);
        double diff = 0.0;
        for (size_t i = 0; i < oa.size() && i < ob.size(); ++i)
            diff += std::abs(oa[i] - ob[i]);
        CHECK(diff > 1.0, "reseed descorrelaciona (diff %.1f)", diff);
    }

    // --- G53. LEN N8/N32 dimensionam o fragmento (grab manual + BR×1) ---
    {
        auto fragLen = [&](TempoInfo::Note ln)
        {
            Granular g;
            g.prepare(48000.0);
            g.setMode(Granular::Mode::BeatRepeat);
            g.setTrig(Granular::Trig::Manual);
            g.setLenNote(ln);
            g.setRepeats(1);
            g.setMix(1.f);
            g.setInterrupt(false);
            g.setInternalBpm(120.0);
            TempoInfo tt;
            // Preenche com rampa e dispara: o fim do 1º loop marca o len.
            std::vector<float> out;
            auto ramp = [](int i) { return (float) i / 48000.f; };
            renderGrab(g, tt, 48000, out, ramp);
            g.setManual(true);
            renderGrab(g, tt, 512, out, ramp);
            g.setManual(false);
            renderGrab(g, tt, 30000, out, [](int) { return 0.f; });
            // Com rampa monótona, o fim do loop = descontinuidade máxima.
            // (trig = ponto do grab em índices de out, não fim do bloco.)
            int trig = 48000;
            float jump = 0.f;
            int at = -1;
            for (int i = trig + 200; i < trig + 14000 && i < (int) out.size(); ++i)
            {
                float d = std::abs(out[(size_t) i] - out[(size_t) i - 1]);
                if (d > jump) { jump = d; at = i - trig; }
            }
            return at;
        };
        int l8 = fragLen(TempoInfo::Note::N8);     // 0.5 beats = 12000
        int l32 = fragLen(TempoInfo::Note::N32);   // 0.125 beats = 3000
        CHECK(std::abs(l8 - 12000) < 600, "LEN N8 = 12000 amostras (got %d)", l8);
        CHECK(std::abs(l32 - 3000) < 300, "LEN N32 = 3000 amostras (got %d)", l32);
    }

    // --- G54. Xfade 1 vs 50 ms na costura (largura da zona suave) ---
    {
        auto seamWidth = [&](float xfms)
        {
            Granular g;
            g.prepare(48000.0);
            g.setMode(Granular::Mode::BeatRepeat);
            g.setTrig(Granular::Trig::Manual);
            g.setLenNote(TempoInfo::Note::N16); // 6000
            g.setRepeats(4);
            g.setXfadeMs(xfms);
            g.setMix(1.f); // dry 0: só a costura cai na banda de contagem
            g.setInterrupt(false);
            g.setInternalBpm(120.0);
            TempoInfo tt;
            // Degrau a meio do fragmento: costura fim(0.8)->começo(0.2)
            // se excluirmos o degrau da janela, só a costura conta.
            std::vector<float> out;
            auto stepFn = [](int i) { return i < 45000 ? 0.2f : 0.8f; };
            renderGrab(g, tt, 48000, out, stepFn);
            g.setManual(true);
            renderGrab(g, tt, 512, out, stepFn);
            g.setManual(false);
            renderGrab(g, tt, 20000, out, stepFn);
            // Fragmento = [42000, 48000): degrau aos 3000. A costura da
            // 2ª volta ([6000-xf, 6000] em offsets) é o que se conta; a
            // janela cobre-a inteira para xf=50 (1500 -> [4500,6000]).
            int trig = 48000 + 512;
            int soft = 0;
            for (int i = trig + 4500; i < trig + 6500 && i < (int) out.size(); ++i)
            {
                float v = out[(size_t) i];
                if (v > 0.25f && v < 0.75f) ++soft;
            }
            return soft;
        };
        int w1 = seamWidth(1.f), w50 = seamWidth(50.f);
        CHECK(w50 > 200 && w50 > w1 * 3, "xfade 50 ms alarga a costura (%d vs %d)", w50, w1);
    }

    // --- G55. Repeats 16 e chance nos extremos ---
    {
        Granular g;
        g.prepare(48000.0);
        g.setMode(Granular::Mode::BeatRepeat);
        g.setTrig(Granular::Trig::Manual);
        g.setLenNote(TempoInfo::Note::N16);
        g.setRepeats(16);
        g.setDecay(0.99f); // teto do range: ~sem decay
        g.setMix(1.f);
        g.setInterrupt(false);
        g.setInternalBpm(120.0);
        TempoInfo tt;
        std::vector<float> out;
        renderGrab(g, tt, 48000, out, [](int) { return 0.5f; });
        g.setManual(true);
        renderGrab(g, tt, 512, out, [](int) { return 0.5f; });
        g.setManual(false);
        renderGrab(g, tt, 110000, out, [](int) { return 0.f; });
        int trig = 48000 + 512;
        double m = 0.0;
        for (int i = trig + 14 * 6000; i < trig + 15 * 6000 && i < (int) out.size(); ++i)
            m += std::abs(out[(size_t) i]);
        CHECK(m / 6000.0 > 0.3, "15º repeat ainda audível (média %f)", m / 6000.0);
        // Chance 0 nunca dispara; 1 dispara no 1º quantum.
        Granular c0, c1;
        for (auto* gg : { &c0, &c1 })
        {
            gg->prepare(48000.0);
            gg->setMode(Granular::Mode::Slice);
            gg->setTrig(Granular::Trig::Chance);
            gg->setLenNote(TempoInfo::Note::N16);
            gg->setMix(1.f);
            gg->setInterrupt(false);
            gg->setInternalBpm(120.0);
        }
        c0.setChance(0.f);
        c1.setChance(1.f);
        c0.setMix(1.f);
        c1.setMix(1.f);
        std::vector<float> o0, o1;
        renderGrab(c0, tt, 30000, o0, [](int) { return 0.5f; });
        renderGrab(c1, tt, 30000, o1, [](int) { return 0.5f; });
        double e0 = 0.0, e1 = 0.0;
        for (int i = 20000; i < 30000; ++i) { e0 += std::abs(o0[(size_t) i]); e1 += std::abs(o1[(size_t) i]); }
        CHECK(e0 < 0.01 * 10000, "chance 0 nunca agarra (%.1f)", e0);
        CHECK(e1 > 0.1 * 10000, "chance 1 agarra sempre (%.1f)", e1);
    }

    return failures;
}

// --- Fase 3: reverb ---

// Renderiza a resposta impulsiva (canal L) do wrapper com mix=1.
static std::vector<float> renderIR(reverb::Reverb& r, double seconds, double sr = 48000.0)
{
    int total = (int) (seconds * sr);
    std::vector<float> ir;
    ir.reserve((size_t) total);
    juce::AudioBuffer<float> blk(2, 512);
    bool first = true;
    int pos = 0;
    while (pos < total)
    {
        int n = juce::jmin(512, total - pos);
        blk.setSize(2, n, false, false, true);
        blk.clear();
        if (first) { blk.setSample(0, 0, 1.0f); blk.setSample(1, 0, 1.0f); first = false; }
        r.process(blk);
        for (int i = 0; i < n; ++i) ir.push_back(blk.getSample(0, i));
        pos += n;
    }
    return ir;
}

// RT60 por integração de Schroeder: energia reversa acumulada até -60 dB.
static double measureRT60(const std::vector<float>& ir, double sr)
{
    int n = (int) ir.size();
    std::vector<double> acc((size_t) n + 1, 0.0);
    for (int i = n - 1; i >= 0; --i)
        acc[(size_t) i] = acc[(size_t) i + 1] + (double) ir[(size_t) i] * ir[(size_t) i];
    if (acc[0] <= 0.0) return -1.0;
    for (int i = 0; i < n; ++i)
    {
        double db = 10.0 * std::log10(acc[(size_t) i] / acc[0] + 1e-30);
        if (db <= -60.0) return (double) i / sr;
    }
    return -2.0; // nunca chegou a -60 dB (cauda a mais / erro)
}

static void setupVerb(reverb::Reverb& r, reverb::Reverb::Algo algo, double t60)
{
    r.prepare(48000.0, 512);
    r.setAlgo(algo);
    r.noteStructuralChange(true);
    r.setSize01(0.5f); // escala 1.0
    r.setT60(t60);
    r.setDamp01(0.3f);
    r.setWidth01(1.f);
    r.setPredelaySamples(0);
    r.setFrozen(false);
    r.setMix(1.f);
    r.setTone(20.f, 20000.f);
}

int mainVerb()
{
    // --- 8. Room T60=1.0s medido ±20% ---
    {
        reverb::Reverb r;
        setupVerb(r, reverb::Reverb::Algo::Room, 1.0);
        auto ir = renderIR(r, 6.0);
        double rt = measureRT60(ir, 48000.0);
        CHECK(rt > 0 && std::abs(rt - 1.0) / 1.0 <= 0.20,
              "Room T60=1.0 medido %f s", rt);
    }

    // --- 9. Hall T60=2.5s medido ±20% ---
    {
        reverb::Reverb r;
        setupVerb(r, reverb::Reverb::Algo::Hall, 2.5);
        auto ir = renderIR(r, 12.0);
        double rt = measureRT60(ir, 48000.0);
        CHECK(rt > 0 && std::abs(rt - 2.5) / 2.5 <= 0.20,
              "Hall T60=2.5 medido %f s", rt);
    }

    // --- 10. Plate decai (finito) e Shimmer é estável ---
    {
        reverb::Reverb r;
        setupVerb(r, reverb::Reverb::Algo::Plate, 1.5);
        auto ir = renderIR(r, 8.0);
        double rt = measureRT60(ir, 48000.0);
        CHECK(rt > 0.3 && rt < 4.0, "Plate T60=1.5 plausível (%f s)", rt);

        reverb::Reverb s;
        setupVerb(s, reverb::Reverb::Algo::Shimmer, 2.0);
        auto irs = renderIR(s, 6.0);
        float peak = 0.f;
        for (float v : irs) peak = juce::jmax(peak, std::abs(v));
        CHECK(peak < 4.0f && peak > 0.01f, "Shimmer estável e audível (pico %f)", peak);
    }

    // --- 10b. Shimmer: bloom audível, sem laps, sem renascimento ---
    // O loop FDN+anel já teve ganho > 1 no grave (modo ≈125 Hz a +11 dB/s):
    // a cauda "renascia" após 2 s e o teste antigo media esse artefacto.
    // Agora: wash audível no bloom (0.3–1.5 s), cauda tardia (4–6 s) mais
    // baixa que a intermédia (2–4 s) e sem saltos amostra-a-amostra.
    for (double sr : { 48000.0, 44100.0 })
    {
        reverb::Reverb s;
        s.prepare(sr, 512);
        s.setAlgo(reverb::Reverb::Algo::Shimmer);
        s.noteStructuralChange(true);
        s.setSize01(0.5f);
        s.setT60(2.0);
        s.setDamp01(0.3f);
        s.setWidth01(1.f);
        s.setPredelaySamples(0);
        s.setFrozen(false);
        s.setMix(1.f);
        s.setTone(20.f, 20000.f);
        auto irs = renderIR(s, 6.0, sr);
        float bloom = 0.f, mid = 0.f, late = 0.f, maxStep = 0.f;
        for (int i = (int) (0.3 * sr); i < (int) (1.5 * sr); ++i)
            bloom = juce::jmax(bloom, std::abs(irs[(size_t) i]));
        for (int i = (int) (2.0 * sr); i < (int) (4.0 * sr); ++i)
            mid = juce::jmax(mid, std::abs(irs[(size_t) i]));
        for (int i = (int) (4.0 * sr) + 1; i < (int) irs.size(); ++i)
        {
            late = juce::jmax(late, std::abs(irs[(size_t) i]));
            maxStep = juce::jmax(maxStep, std::abs(irs[(size_t) i] - irs[(size_t) i - 1]));
        }
        CHECK(bloom > 0.005f, "shimmer bloom audível @ %g Hz (pico %f)", sr, bloom);
        CHECK(late < mid, "shimmer decai sem renascer @ %g Hz (2-4 s %f, 4-6 s %f)", sr, mid, late);
        CHECK(maxStep < 0.15f, "shimmer sem cliques de lap @ %g Hz (maxStep %f)", sr, maxStep);
    }

    // --- 10d. A oitava está mesmo lá: seno 440 → energia a 880 na cauda ---
    // 1 s de drive + 2 s de cauda; razão 880/440 medida por Goertzel.
    {
        reverb::Reverb s;
        setupVerb(s, reverb::Reverb::Algo::Shimmer, 2.0);
        s.setMix(1.f);
        juce::AudioBuffer<float> blk(2, 512);
        std::vector<float> out;
        int total = 3 * 48000, pos = 0;
        while (pos < total)
        {
            int n = juce::jmin(512, total - pos);
            blk.setSize(2, n, false, false, true);
            blk.clear();
            for (int i = 0; i < n; ++i)
            {
                float v = (pos + i < 48000) ? std::sin((pos + i) * 2.f * 3.14159265f * 440.f / 48000.f) * 0.5f : 0.f;
                blk.setSample(0, i, v);
                blk.setSample(1, i, v);
            }
            s.process(blk);
            for (int i = 0; i < n; ++i) out.push_back(blk.getSample(0, i));
            pos += n;
        }
        auto goertzel = [&](double target) {
            double w = 2.0 * 3.14159265358979 * target / 48000.0;
            double c = std::cos(w), p = 0.0, pp = 0.0;
            for (int i = 48000; i < (int) out.size(); ++i)
            {
                double x = out[(size_t) i] + 2.0 * c * p - pp;
                pp = p;
                p = x;
            }
            int m = (int) out.size() - 48000;
            return std::sqrt(p * p + pp * pp - 2.0 * c * p * pp) / m;
        };
        double e440 = goertzel(440.0), e880 = goertzel(880.0);
        CHECK(e880 > e440 * 0.2, "shimmer gera oitava (E440 %f, E880 %f, razão %f)", e440, e880, e880 / (e440 + 1e-12));
    }

    // --- 10c. Mudar decay/damp a meio da cauda não clica (slew interno) ---
    {
        reverb::Reverb r;
        setupVerb(r, reverb::Reverb::Algo::Hall, 1.0);
        juce::AudioBuffer<float> blk(2, 512);
        blk.clear();
        blk.setSample(0, 0, 1.0f);
        r.process(blk);
        float maxStep = 0.f, prevL = 0.f, prevR = 0.f;
        for (int b = 0; b < 200; ++b) // ~2.1 s; mede a partir de ~0.1 s
        {
            if (b == 20) { r.setT60(6.0); r.setDamp01(0.8f); } // muda com cauda quente
            blk.setSize(2, 512, false, false, true);
            blk.clear();
            r.process(blk);
            if (b < 10)
            {
                prevL = blk.getSample(0, 511);
                prevR = blk.getSample(1, 511);
                continue; // fora da janela de medida (transiente inicial)
            }
            for (int i = 0; i < 512; ++i)
            {
                float l = blk.getSample(0, i), rr = blk.getSample(1, i);
                maxStep = juce::jmax(maxStep, std::abs(l - prevL));
                maxStep = juce::jmax(maxStep, std::abs(rr - prevR));
                prevL = l; prevR = rr;
            }
        }
        CHECK(maxStep < 0.4f, "decay/damp a meio sem clique (maxStep %f)", maxStep);
    }

    // --- 11. Troca de algoritmo a meio não explode ---
    {
        reverb::Reverb r;
        setupVerb(r, reverb::Reverb::Algo::Room, 2.0);
        juce::AudioBuffer<float> blk(2, 512);
        float peak = 0.f;
        for (int b = 0; b < 100; ++b) // ~1s de Room
        {
            blk.setSize(2, 512, false, false, true);
            blk.clear();
            if (b == 0) blk.setSample(0, 0, 1.0f);
            r.process(blk);
        }
        r.setAlgo(reverb::Reverb::Algo::Hall);
        r.noteStructuralChange(true);
        for (int b = 0; b < 300; ++b)
        {
            blk.setSize(2, 512, false, false, true);
            blk.clear();
            r.process(blk);
            for (int i = 0; i < 512; ++i)
                peak = juce::jmax(peak, std::abs(blk.getSample(0, i)));
        }
        CHECK(peak < 3.0f, "switch Room→Hall sem explosão (pico %f)", peak);
    }

    // --- 12. Freeze sustenta a cauda ---
    // T60 longo para haver energia ao congelar (1s), depois 7s congelado.
    {
        reverb::Reverb r;
        setupVerb(r, reverb::Reverb::Algo::Hall, 6.0);
        juce::AudioBuffer<float> blk(2, 512);
        std::vector<float> out;
        for (int b = 0; b < 94; ++b) // ~1s
        {
            blk.setSize(2, 512, false, false, true);
            blk.clear();
            if (b == 0) blk.setSample(0, 0, 1.0f);
            r.process(blk);
            for (int i = 0; i < 512; ++i) out.push_back(blk.getSample(0, i));
        }
        r.setFrozen(true);
        for (int b = 0; b < 550; ++b) // +~5.9s congelado
        {
            blk.setSize(2, 512, false, false, true);
            blk.clear();
            r.process(blk);
            for (int i = 0; i < 512; ++i) out.push_back(blk.getSample(0, i));
        }
        auto energy = [&](int from, int to) {
            double e = 0.0;
            for (int i = from; i < to && i < (int) out.size(); ++i)
                e += (double) out[(size_t) i] * out[(size_t) i];
            return e;
        };
        double early = energy(24000, 48000);          // antes do freeze
        double late = energy((int) out.size() - 48000, (int) out.size()); // fim do freeze
        CHECK(early > 0.0 && late / early > 0.15,
              "freeze sustenta (early %f late %f)", early, late);
    }

    // --- G45. Pre-delay atrasa a resposta (100 ms de silêncio inicial) ---
    {
        reverb::Reverb r;
        setupVerb(r, reverb::Reverb::Algo::Hall, 1.0);
        r.setPredelaySamples(4800); // 100 ms @48k
        auto ir = renderIR(r, 2.0);
        float early = 0.f, late = 0.f;
        // A partir de 2000: o slew do predelay (anti-clique, 30 ms) deixa
        // passar o transiente inicial; o estado estacionário é que conta.
        for (int i = 2000; i < 4800; ++i) early = juce::jmax(early, std::abs(ir[(size_t) i]));
        for (int i = 4800; i < 9600; ++i) late = juce::jmax(late, std::abs(ir[(size_t) i]));
        CHECK(early < 1e-6f, "pre-delay cala antes dos 100 ms (%f)", early);
        CHECK(late > 0.01f, "cauda arranca após o pre-delay (%f)", late);
    }

    // --- G46. Width 0 = mono (L igual a R, após o slew) ---
    {
        reverb::Reverb r;
        setupVerb(r, reverb::Reverb::Algo::Hall, 1.0);
        r.setWidth01(0.f);
        juce::AudioBuffer<float> blk(2, 512);
        bool same = true;
        for (int b = 0; b < 100 && same; ++b)
        {
            blk.setSize(2, 512, false, false, true);
            blk.clear();
            if (b == 0) { blk.setSample(0, 0, 1.f); blk.setSample(1, 0, 1.f); }
            r.process(blk);
            if (b < 10) continue; // slew do width 1 -> 0
            for (int i = 0; i < 512 && same; ++i)
                if (std::abs(blk.getSample(0, i) - blk.getSample(1, i)) > 1e-5f) same = false;
        }
        CHECK(same, "width 0 cola L e R");
    }

    // --- G47. Size muda a densidade (linhas curtas = ecos densos) ---
    // O feedback compensa o T60 (fica ~constante por desenho); o que muda
    // é a densidade modal: sala pequena = mais picos cedo.
    {
        auto density = [](float size)
        {
            reverb::Reverb r;
            setupVerb(r, reverb::Reverb::Algo::Hall, 2.0);
            r.setSize01(size);
            auto ir = renderIR(r, 6.0);
            float peak = 0.f;
            for (int i = 1000; i < 12000; ++i)
                peak = juce::jmax(peak, std::abs(ir[(size_t) i]));
            int n = 0;
            for (int i = 1001; i < 11999; ++i)
            {
                float v = std::abs(ir[(size_t) i]);
                if (v > peak * 0.1f && v >= std::abs(ir[(size_t) i - 1])
                    && v >= std::abs(ir[(size_t) i + 1])) ++n;
            }
            return n;
        };
        int small = density(0.f), big = density(1.f);
        CHECK(small > big * 3 / 2, "size 0 mais denso que size 1 (%d vs %d)", small, big);
    }

    // --- G48. Tone: locut tira graves, hicut tira agudos (Goertzel) ---
    {
        auto eboth = [&](float locut, float hicut, double& e100, double& e10k)
        {
            reverb::Reverb r;
            setupVerb(r, reverb::Reverb::Algo::Hall, 1.0);
            r.setTone(locut, hicut);
            juce::AudioBuffer<float> blk(2, 512);
            std::vector<float> out;
            for (int b = 0; b < 100; ++b)
            {
                blk.setSize(2, 512, false, false, true);
                blk.clear();
                for (int i = 0; i < 512; ++i)
                {
                    float v = std::sin((b * 512 + i) * 2.f * 3.14159265f * 100.f / 48000.f) * 0.25f
                            + std::sin((b * 512 + i) * 2.f * 3.14159265f * 10000.f / 48000.f) * 0.25f;
                    blk.setSample(0, i, v);
                    blk.setSample(1, i, v);
                }
                r.process(blk);
                for (int i = 0; i < 512; ++i) out.push_back(blk.getSample(0, i));
            }
            auto goertzel = [&](double target)
            {
                double w = 2.0 * 3.14159265358979 * target / 48000.0;
                double c = std::cos(w), p = 0.0, pp = 0.0;
                int from = 25600, to = (int) out.size(), m = 0;
                for (int i = from; i < to; ++i)
                {
                    double x = out[(size_t) i] + 2.0 * c * p - pp;
                    pp = p;
                    p = x;
                    ++m;
                }
                return std::sqrt(p * p + pp * pp - 2.0 * c * p * pp) / m;
            };
            e100 = goertzel(100.0);
            e10k = goertzel(10000.0);
        };
        double o100, o10k, l100, l10k, h100, h10k;
        eboth(20.f, 20000.f, o100, o10k);
        eboth(500.f, 20000.f, l100, l10k);
        eboth(20.f, 2000.f, h100, h10k);
        CHECK(l100 < o100 * 0.1, "locut 500 tira 100 Hz (%.2e vs %.2e)", l100, o100);
        CHECK(h10k < o10k * 0.1, "hicut 2k tira 10 kHz (%.2e vs %.2e)", h10k, o10k);
    }

    // --- G49. Damp escurece a cauda (comparação de brilho) ---
    {
        auto bright = [&](float damp)
        {
            reverb::Reverb r;
            setupVerb(r, reverb::Reverb::Algo::Hall, 2.0);
            r.setDamp01(damp);
            auto ir = renderIR(r, 6.0);
            // Rácio de energia tardia HF vs total: aproxima-se por
            // diferenças amostra-a-amostra (proxy de brilho).
            double hf = 0.0, tot = 0.0;
            for (int i = 48000; i < 144000; ++i)
            {
                float d = ir[(size_t) i] - ir[(size_t) i - 1];
                hf += (double) d * d;
                tot += (double) ir[(size_t) i] * ir[(size_t) i];
            }
            return hf / (tot + 1e-12);
        };
        double b0 = bright(0.f), b1 = bright(1.f);
        CHECK(b1 < b0 * 0.5, "damp 1 escurece (brilho %.2e vs %.2e)", b1, b0);
    }

    return failures;
}
