#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include "../core/TempoInfo.h"
#include "../core/EnableRamp.h"

// Granular/glitch (Fase 4). Um anel de captura (até 4 s) + uma voz de
// reprodução com direção/rate/loop por modo:
//
//   BeatRepeat: repete o fragmento N vezes com decay, depois liberta.
//   Slice:      loop cortado (gate) do fragmento enquanto ativo.
//   Reverse:    fragmento ao contrário em loop.
//   Pitch:      loop com rate de semitons (tape-style, pitch+tempo juntos).
//   Stutter:    repete depressa o 1º oitavo do fragmento, com decay.
//
// Triggers: Chance (por fronteira de fragmento, RNG determinística),
// Envelope (transiente), Manual (flanco). Interrupt troca dry por wet.
// Não aloca no audio thread depois de prepare().
class Granular
{
public:
    enum class Mode : int { Off = 0, BeatRepeat, Slice, Reverse, Pitch, Stutter };
    enum class Trig : int { Chance = 0, Envelope, Manual };

    Granular() = default;

    void prepare(double sampleRate);
    void reset();

    void setMode(Mode m);
    void setTrig(Trig t)             { trig = t; }
    void setChance(float c)          { chance = juce::jlimit(0.f, 1.f, c); }
    void setEnvThrDb(float db)       { envThr = std::pow(10.f, juce::jlimit(-60.f, 0.f, db) / 20.f); }
    void setManual(bool b)           { manual = b; }
    void setLenNote(TempoInfo::Note n) { lenNote = n; }
    void setRepeats(int r)           { repeats = juce::jlimit(1, 16, r); }
    void setDecay(float d)           { decay = juce::jlimit(0.5f, 0.99f, d); }
    void setTimeMs(float ms)         { timeMs = juce::jlimit(60.f, 8000.f, ms); }
    void setTimeNote(TempoInfo::Note n) { timeNote = n; }
    void setPitchSt(float st)        { pitchSt = juce::jlimit(-12.f, 12.f, st); }
    void setFlux(float f)            { flux = juce::jlimit(0.f, 1.f, f); }
    void setXfadeMs(float ms)        { xfadeMs = juce::jlimit(1.f, 50.f, ms); }
    void setMix(float m)             { mix = juce::jlimit(0.f, 1.f, m); }
    void setInterrupt(bool b)        { interrupt = b; }
    void setInternalBpm(double bpm)  { internalBpm = juce::jlimit(40.0, 240.0, bpm); }
    // Bypass por módulo (Fase 8). Ver EnableRamp.
    void setEnabled(bool b)          { pw.set(b); }
    bool isBypassed() const          { return pw.silent(); }
    bool takeClear()                 { return pw.takeClear(); }

    void process(juce::AudioBuffer<float>& buffer, const TempoInfo& tempo);

    bool isActive() const { return activeUi.load(); }

private:
    static constexpr double ringSec = 4.0;

    float readRing(int ch, float pos) const; // interpolação linear, pos em amostras
    void startGrab(double lenBeats, double bpm);

    Mode mode = Mode::Off;
    Trig trig = Trig::Chance;
    float chance = 0.2f, envThr = 0.126f;
    bool manual = false, lastManual = false, interrupt = false;
    TempoInfo::Note lenNote = TempoInfo::Note::N16;
    TempoInfo::Note timeNote = TempoInfo::Note::Free;
    int repeats = 4;
    float decay = 0.85f, timeMs = 250.f, pitchSt = 12.f, flux = 0.3f;
    float xfadeMs = 8.f, mix = 0.5f;

    double sampleRate = 48000.0;
    double internalBpm = 120.0;
    std::vector<float> ringL, ringR;
    int cap = 0, w = 0;

    // Estado de reprodução:
    bool playing = false;
    double loopStart = 0.0, loopLen = 0.0; // em amostras (float p/ rate)
    double playPos = 0.0, playDir = 1.0, playRate = 1.0;
    double loopsDone = 0.0;
    int activeLeft = 0;        // amostras restantes (modos temporizados)
    int releaseLeft = 0;       // fade de saída ao libertar
    float outFade = 0.f;       // 0..1 envelope de saída
    float dryCut = 1.f;        // 1 normal, 0 em interrupt (com rampa)
    float sliceGate = 1.f;     // chop do Slice suavizado (~3 ms, anti-clique)
    EnableRamp pw;             // bypass por módulo

    // Relógio p/ quantização + detetores:
    double internalBeats = 0.0, lastQuantum = -1.0;
    double envState = 0.0;
    int cooldownLeft = 0;
    juce::Random rng;
    std::atomic<bool> activeUi { false };
};
