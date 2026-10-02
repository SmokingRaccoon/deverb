#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include "../core/TempoInfo.h"

// Motor REV do deVerb (Fase 5): lê o passado recente ao contrário.
//
// - LOOP: emite em contínuo os últimos N beats (N = 2/3/4, sync ao host)
//   lidos de trás para a frente com rate varispeed (0.25–2×, pitch acompanha
//   — efeito tape) + LFO lento opcional no rate. Crossfade equal-power na
//   costura do loop. É um tail decorativo: chega sempre atrasado por física.
// - THROW: calado até um flanco (botão ou nota MIDI) disparar UMA cauda do
//   tamanho da janela; depois volta a calar-se.
// - DUCK: seguidor de envelope do dry baixa o REV quando há sinal novo.
// - Off: custo zero (o processor nem chama o render).
//
// O anel grava SEMPRE (mesmo em Off) para o tail aparecer logo ao ligar.
// Não aloca no audio thread depois de prepare().
class ReverseEngine
{
public:
    enum class Mode : int { Off = 0, Loop, Throw };

    ReverseEngine() = default;

    void prepare(double sampleRate);
    void reset();

    void setMode(Mode m);
    void setRate(float r)            { rate = juce::jlimit(0.25f, 2.f, r); }
    void setLfoDepth(float d)        { lfoDepth = juce::jlimit(0.f, 1.f, d); }
    void setCaptureBeats(double b)   { captureBeats = juce::jlimit(0.5, 8.0, b); }
    void setDuckDepth(float d)       { duckDepth = juce::jlimit(0.f, 1.f, d); }
    void setThrowButton(bool b)      { throwBtn = b; }

    double getPlayPos() const { return playPos; }
    int getWritePos() const { return w; }
    int getCapacity() const { return cap; }
    double getCaptureBeatsVal() const { return captureBeats; }

    // Grava o dry (pós input-gain). Barato: chamar sempre.
    void recordBlock(const juce::AudioBuffer<float>& dry);

    // Escreve o sinal reverso (+duck) em `out` (stereo). Não chama em Off.
    // midiThrow: houve note-on neste bloco (para o modo Throw).
    void renderBlock(juce::AudioBuffer<float>& out, const TempoInfo& tempo,
                     double bpm, bool midiThrow);

private:
    static constexpr double ringSec = 6.5; // 4 beats @40 BPM = 6 s + margem

    float readInterp(int ch, float pos) const;

    double sampleRate = 48000.0;
    std::vector<float> ringL, ringR;
    int cap = 0, w = 0;

    Mode mode = Mode::Off;
    float rate = 1.f, lfoDepth = 0.f;
    double captureBeats = 2.0;
    float duckDepth = 0.3f;
    bool throwBtn = false, lastThrowBtn = false;

    // Leitor contínuo (LOOP):
    double playPos = 0.0;
    bool readerLive = false;
    double lfoPhase = 0.0;
    // THROW one-shot:
    bool throwing = false;
    double throwPos = 0.0, throwEnd = 0.0;
    // Duck:
    double duckEnv = 0.0, duckGain = 1.0;
};
