#pragma once
#include <juce_dsp/juce_dsp.h>
#include "../core/EnableRamp.h"

// Dimension Expander do OUT (inspirado no Roland Dimension D / Massive):
// 4 vozes de delay ultra-curto (2 por canal) somadas FORA DE FASE entre L e
// R. O mesmo bus wet W soma-se ao L e subtrai-se ao R: a soma-mono cancela
// EXATAMENTE (mono-compatível por construção) enquanto o stereo abre.
// Estático (sem LFO, como o D original): sem wobble de pitch.
// - Size: escala os 4 atrasos (0..~15 ms; em cima vira slap/room pequena).
// - Mix: 0 = passthrough bit-exato.
// Não aloca no audio thread depois de prepare().
class Dimension
{
public:
    Dimension() = default;

    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    void setSize01(float s); // 0..1, com slew (~30 ms, sem zipper)
    void setMix(float m);    // 0..1 wet, com slew
    // Bypass por módulo. Ver EnableRamp. Sem PWR dedicado nesta iteração:
    // mix=0 esbate o wet (o process corre sempre; custo < 0.5%/core).
    void setEnabled(bool b)          { pw.set(b); }
    bool isBypassed() const          { return pw.silent(); }
    bool takeClear()                 { return pw.takeClear(); }

    // Processa in-place um buffer stereo.
    void process(juce::AudioBuffer<float>& buffer);

private:
    // Frações dos ~15 ms máximos (primos entre si: sem ressonâncias).
    static constexpr float tapFrac[4] = { 0.187f, 0.407f, 0.287f, 0.580f };
    static constexpr float maxDelaySec = 0.02f; // 15 ms + margem
    static constexpr float wetGain = 0.5f;      // nível do bus wet

    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> line;
    double sampleRate = 44100.0;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothSize, smoothMix;
    EnableRamp pw;
};
