#pragma once
#include <juce_dsp/juce_dsp.h>
#include "ReverseEngine.h"

// Delay stereo com 5 algoritmos (Fase 7). Uma DelayLine com interpolação
// Lagrange3rd + one-pole lowpass no loop; feedback capado; freeze; mix.
// - Digital:  o clássico (bit-idêntico à Fase 1 — garantido por teste).
// - Tape:     drive tanh na escrita + wow/flutter (LFO no tempo de leitura).
// - PingPong: cross-feedback na mesma linha (spread = quanto cruza).
// - Multitap: 4 taps fixos (1, 3/4, 1/2, 1/4) com ganhos (1, .7, .5, .35);
//   o feedback sai do tap longo.
// - Reverse:  reusa ReverseEngine (janela = tempo do delay em beats);
//   ignora feedback/damping (documentado).
//
// Não aloca no audio thread depois de prepare().
class Delay
{
public:
    enum class Algo : int { Digital = 0, Tape, PingPong, Multitap, Reverse };

    Delay() = default;

    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    // Chamadas por bloco (a partir dos parâmetros APVTS, já resolvidos p/ ms):
    void setTimeMs(float ms);        // 1..2000, rampado (~20 ms anti-clique)
    void setFeedback(float fb);      // 0..0.95 (capado aqui dentro)
    void setDampingHz(float hz);     // lowpass no loop
    void setMix(float m);            // 0..1 wet
    void setFrozen(bool f);          // congela: deixa de escrever input
    void setAlgo(Algo a);
    void setDrive(float d);          // 0..1 saturação tape
    void setWowRate(float hz);       // 0.1..5 Hz
    void setWowDepthMs(float ms);    // 0..20 ms de vagueio
    void setSpread(float s);         // 0..1 cruzamento pingpong
    void setTempoBpm(double bpm);    // p/ janela do modo Reverse

    // Processa in-place um buffer stereo.
    void process(juce::AudioBuffer<float>& buffer);

    // Gancho de teste: atraso exato em amostras (ignora setTimeMs até ao
    // próximo setTimeMs). Só para a harness de testes offline.
    void setTestDelaySamples(int samples) { testDelaySamples = samples; }

private:
    static constexpr float maxDelaySec = 2.2f;
    static constexpr float tapFrac[4] = { 1.f, 0.75f, 0.5f, 0.25f };
    static constexpr float tapGain[4] = { 1.f, 0.7f, 0.5f, 0.35f };

    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> line;
    double sampleRate = 44100.0;
    double bpm = 120.0;

    Algo algo = Algo::Digital;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothDelay, smoothFb, smoothMix;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothFreeze; // 0..1
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothDrive, smoothSpread;
    float dampA = 1.0f;       // coef. one-pole (recalculado por bloco)
    float dampStateL = 0.f, dampStateR = 0.f;
    double wowPhase = 0.0;
    float wowRate = 1.f, wowDepthMs = 4.f;
    int testDelaySamples = -1; // >=0 = override de teste

    // Voz reverse (modo Reverse): anel próprio + scratch.
    ReverseEngine revVoice;
    juce::AudioBuffer<float> revTmp;
};
