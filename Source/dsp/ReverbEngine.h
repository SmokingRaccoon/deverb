#pragma once
#include <juce_dsp/juce_dsp.h>
#include "../core/EnableRamp.h"
#include <vector>
#include <cmath>

// Motores de reverb algorítmico do deVerb (Fase 3).
// Convenções:
// - Comprimentos em amostras, escalados pela sample rate e pelo Size.
// - Feedbacks calculados do T60 pedido: g = 10^(-3·d/(T60·sr)) —
//   decaimento uniforme em todas as linhas (técnica de Moorer).
// - Nenhuma alocação depois de prepare(). Mudanças estruturais
//   (algoritmo/size) pedem fade no wrapper, nunca aqui dentro.
namespace reverb {

// Pente com lowpass no feedback (Schroeder-Moorer). Comprimento variável
// até à capacidade pré-alocada — o Size só encolhe, nunca realoca.
class FbComb
{
public:
    void prepare(int capacitySamples)
    {
        buf.assign((size_t) juce::jmax(64, capacitySamples), 0.f);
        cap = (int) buf.size();
        w = 0; length = juce::jmin(64, cap); fstate = 0.f;
    }
    void reset() { std::fill(buf.begin(), buf.end(), 0.f); fstate = 0.f; }
    void setLength(int len)   { length = juce::jlimit(1, cap, len); }
    void setFeedback(float g) { fbT = g; }   // alvo; o process chega lá (~30 ms)
    void setDamp(float d)     { dampT = juce::jlimit(0.f, 0.95f, d); } // 0 = brilhante
    void snap()               { fb = fbT; damp = dampT; } // pós-prepare/testes
    float process(float x)
    {
        // Slew de fb/damp: mudanças de T60/damping/freeze sem cliques.
        fb += (fbT - fb) * slewK;
        damp += (dampT - damp) * slewK;
        int r = w - length;
        if (r < 0) r += cap;
        float out = buf[(size_t) r];
        fstate += (1.f - damp) * (out - fstate); // one-pole: damp=0 → passa tudo
        buf[(size_t) w] = x + fstate * fb;
        if (++w >= cap) w = 0;
        return out;
    }
private:
    std::vector<float> buf;
    int cap = 64, w = 0, length = 64;
    float fb = 0.f, damp = 0.f, fstate = 0.f;
    float fbT = 0.f, dampT = 0.f;            // alvos (slew no process)
    static constexpr float slewK = 0.003f;   // ~30 ms até 99 %
};

// Allpass de Schroeder, comprimento fixo após prepare.
class Allpass
{
public:
    void prepare(int lenSamples)
    {
        buf.assign((size_t) juce::jmax(8, lenSamples), 0.f);
        cap = (int) buf.size();
        w = 0;
    }
    void reset() { std::fill(buf.begin(), buf.end(), 0.f); }
    void setFeedback(float g) { fb = g; }
    float process(float x)
    {
        float b = buf[(size_t) w];
        float y = -x + b;
        buf[(size_t) w] = x + b * fb;
        if (++w >= cap) w = 0;
        return y;
    }
private:
    std::vector<float> buf;
    int cap = 8, w = 0;
    float fb = 0.5f;
};

// Freeverb (Jezar/Dreampoint, domínio público): 8 combs c/ damping em
// paralelo + 4 allpasses em série, por canal. Base do sabor ROOM.
class Freeverb
{
public:
    void prepare(double sr, float sizeScale);
    void reset();
    void setLengths(float sizeScale); // 0.5..1.5 (só encolhe da capacidade)
    void setT60(double t60);          // feedbacks p/ decaimento uniforme
    void setDamp01(float d);          // 0..1
    void setFrozen(bool f);
    void processSample(float inL, float inR, float& outL, float& outR);

private:
    static constexpr int combL[8] = { 1116, 1188, 1277, 1356, 1422, 1491, 1617, 1687 };
    static constexpr int apL[4]   = { 556, 441, 341, 225 };
    double sr = 44100.0;
    double t60 = 2.5;
    float damp01 = 0.3f;
    FbComb combs[2][8];
    Allpass aps[2][4];
    float inGain = 0.08f; // compensa a soma dos 8 combs
    bool frozen = false;

    void updateFb();
};

// FDN de 4 linhas com matriz Hadamard unitária (Stautner/Puckette, Jot).
// Base do sabor HALL.
class Fdn
{
public:
    void prepare(double sr, float sizeScale);
    void reset();
    void setLengths(float sizeScale);
    void setT60(double t60);
    void setDampFc(float hz); // lowpass por linha
    void setFrozen(bool f);
    void processSample(float inL, float inR, float& outL, float& outR);

private:
    static constexpr int baseLen[4] = { 1123, 1493, 1879, 2281 }; // @48k
    struct Line { std::vector<float> buf; int cap = 0, w = 0, len = 0; };
    double sr = 48000.0;
    double t60 = 2.5;
    float dampA = 1.f, dampAT = 1.f;
    Line lines[4];
    float fb[4] = { 0, 0, 0, 0 }; // feedbacks do T60 (atuais, com slew)
    float fbT[4] = { 0, 0, 0, 0 }; // alvos
    float dst[4] = { 0, 0, 0, 0 }; // estados lowpass por linha
    bool frozen = false;
    static constexpr float slewK = 0.003f;

    void updateFb();
    void snap() { for (int i = 0; i < 4; ++i) fb[i] = fbT[i]; dampA = dampAT; }

    static float readLine(const Line& l, int delay)
    {
        int r = l.w - delay;
        if (r < 0) r += l.cap;
        return l.buf[(size_t) r];
    }
};

// Tank "plate" inspirado em Dattorro (JAES 1997): difusores de entrada +
// 2 linhas cruzadas com damping e modulação lenta. Sabor PLATE.
class PlateTank
{
public:
    void prepare(double sr, float sizeScale);
    void reset();
    void setLengths(float sizeScale);
    void setT60(double t60);
    void setDampFc(float hz);
    void setFrozen(bool f);
    void processSample(float inL, float inR, float& outL, float& outR);

private:
    struct VLine { std::vector<float> buf; int cap = 0, w = 0; };
    double sr = 48000.0;
    double t60 = 2.5;
    float dampFc = 8000.f;
    Allpass dif[2];
    VLine tankA, tankB;
    int lenA = 2400, lenB = 2900;
    float dampA = 1.f, fA = 0.f, fB = 0.f; // coef + estados lowpass
    float decay = 0.5f;
    float dampAT = 1.f, decayT = 0.5f;    // alvos (slew no process)
    double modPhase = 0.0;
    bool frozen = false;
    static constexpr float slewK = 0.003f;

    void updateDecay();
    void snap() { decay = decayT; dampA = dampAT; }

    float readInterp(const VLine& l, float delay);
};

// Shimmer: FDN + realimentação oitavada (leitura a 2× de um anel).
// Oitavas acumulam-se na cauda — efeito, não realismo.
class ShimmerVoice
{
public:
    void prepare(double sr, float sizeScale);
    void reset();
    void setT60(double t60);
    void setDampFc(float hz);
    void setFrozen(bool f);
    void processSample(float inL, float inR, float& outL, float& outR);

private:
    Fdn fdn;
    std::vector<float> ring;
    int cap = 0, w = 0;
    double sr = 48000.0;
    float shimmerAmt = 0.35f;
    float shimState = 0.f, shimA = 0.3f, rPos = 0.f;
    static constexpr int lapFade = 256; // crossfade anti-lap da leitura 2×
    float readAt(float pos) const;
};

// Wrapper: pre-delay (livre+sync) → motor → width → locut/hicut → mix.
// Trata do fade anti-clique em mudanças estruturais e do freeze.
class Reverb
{
public:
    enum class Algo : int { Room = 0, Hall, Plate, Shimmer };

    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    void setAlgo(Algo a);
    void setSize01(float s);      // 0..1 → escala 0.5..1.5
    void setT60(double t60);      // segundos
    void setDamp01(float d);      // 0..1
    void setWidth01(float w);     // 0 mono .. 1 stereo
    void setPredelaySamples(int s);
    void setFrozen(bool f);
    void setMix(float m);
    void setTone(float locutHz, float hicutHz);
    // Bypass por módulo (Fase 8). Ver EnableRamp (core/).
    void setEnabled(bool b);
    bool isBypassed() const;
    bool takeClear();

    // Chamar quando algo/size mudam de "escalão": fade + (só p/ algo) reset.
    // Mantido p/ aplicação imediata (testes); o caminho normal por bloco
    // usa setAlgo/setSize01, que adiam para o ponto silencioso.
    void noteStructuralChange(bool resetEngines);

    void process(juce::AudioBuffer<float>& buffer);

private:
    double sr = 44100.0;
    Algo algo = Algo::Room;
    Freeverb freeverb;
    Fdn fdn;
    PlateTank plate;
    ShimmerVoice shimmer;

    // Pre-delay: anel stereo com leitura interpolada (rampa suave).
    std::vector<float> preL, preR;
    int preCap = 0, preW = 0;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothPre;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothMix, smoothFreeze;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothFade; // estrutural
    float width = 1.f;
    bool frozen = false;
    EnableRamp pw; // bypass por módulo
    // Troca estrutural adiada: fade-out → aplica no silêncio → fade-in.
    Algo pendingAlgo = Algo::Room;
    float pendingSize = 0.5f;
    bool pendingStructural = false, pendingReset = false, pendingSizeChange = false;
    int appliedBucket = -1; // -1 = primeira aplicação é imediata (prepare)
    void doSetLengths(float s); // aplica já (motores), sem fade
    // Tone com StateVariableTPTFilter: setCutoffFrequency não aloca
    // (o IIR::Filter anterior reconstruía Coefficients com new a cada gesto).
    juce::dsp::StateVariableTPTFilter<float> lpL, lpR, hpL, hpR;
    float lastLocut = -1.f, lastHicut = -1.f;
};

} // namespace reverb
