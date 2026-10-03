#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include "../core/TempoInfo.h"
#include "../core/EnableRamp.h"

// Trance gate (Fase 2). Ganho por amostra a partir de fase rítmica:
// ou da posição do host (ppq) ou de relógio interno com resets.
// Pattern de 16 bits (bit i = passo i aberto); profundidade, suavização
// attack/release, mix e pan alternado por passo.
// Não aloca no audio thread depois de prepare().
class Gater
{
public:
    enum class TrigMode : int { Host = 0, Midi, Transient, Free };

    Gater() = default;

    void prepare(double sampleRate);
    void reset();

    void setRate(TempoInfo::Note n)  { rate = n; }
    void setSteps(int s)             { numSteps = juce::jlimit(1, 16, s); }
    // Contagens de steps 1..16, livres (0 excluído: era silêncio garantido).
    // Os índices 0="8" e 1="16" ficam para compatibilidade de restauro.
    static constexpr const char* stepNames[16] =
        { "8","16","12","6","4","3","2","1","5","7","9","10","11","13","14","15" };
    static constexpr int stepCounts[16] =
        { 8,16,12,6,4,3,2,1,5,7,9,10,11,13,14,15 };
    static int stepsCountFromChoice(int idx)
    {
        return stepCounts[(size_t) juce::jlimit(0, 15, idx)];
    }
    void setPattern(int bits)        { pattern = (uint16_t) (bits & 0xFFFF); }
    void setSmooth(float s);         // 0..1 (faca..suave)
    void setDepth(float d)           { depth = juce::jlimit(0.f, 1.f, d); }
    void setMix(float m)             { mix = juce::jlimit(0.f, 1.f, m); }
    void setPanAlt(float p)          { panAlt = juce::jlimit(0.f, 1.f, p); }
    void setTrigMode(TrigMode m)     { trigMode = m; }
    void setEnvThrDb(float db)       { envThr = std::pow(10.f, juce::jlimit(-60.f, 0.f, db) / 20.f); }
    void setInternalBpm(double bpm)  { internalBpm = juce::jlimit(40.0, 240.0, bpm); }
    // Bypass por módulo (Fase 8). Ver EnableRamp.
    void setEnabled(bool b)          { pw.set(b); }
    bool isBypassed() const          { return pw.silent(); }
    bool takeClear()                 { return pw.takeClear(); }

    // Varre o MIDI do bloco à procura de note-on (modo Midi = restart).
    // Chamar antes de process() para o mesmo bloco.
    void scanMidi(const juce::MidiBuffer& midi);

    // Processa in-place. Lê tempo do `tempo` (host) ou relógio interno.
    void process(juce::AudioBuffer<float>& buffer, const TempoInfo& tempo);

    int getCurrentStep() const { return currentStep.load(); }

    // Patterns de fábrica (bit i = passo i+1). Para a ComboBox da UI.
    struct FactoryPattern { const char* name; uint16_t bits; };
    static constexpr FactoryPattern factoryPatterns[] = {
        { "Porta 4/4",  0x1111 }, // passos 1,5,9,13
        { "Offbeat",    0x4444 }, // passos 3,7,11,15
        { "8ths",       0x5555 }, // passos ímpares
        { "16ths",      0xFFFF }, // tudo aberto (só pan/acento)
        { "Saw Up",     0x844B }, // densidade a subir
        { "Euclid 5",   0x1249 }, // 5 em 16
        { "Tresillo",   0x1449 }, // 3-3-2: passos 1,4,7,11,13
        { "Cinquillo",  0x6DAD }, // passos 1,3,4,6,8,9,11,12,14,15
        { "Euclid 3",   0x0841 }, // 3 em 16: passos 1,7,12
        { "Euclid 7",   0x54A9 }, // 7 em 16
    };

private:
    double stepBeats() const
    {
        double b = TempoInfo::noteToBeats(rate);
        return (b > 0.0) ? b : 0.25; // Free → 1/16
    }

    TempoInfo::Note rate = TempoInfo::Note::N16;
    int numSteps = 16;
    uint16_t pattern = 0x1111;
    float depth = 1.f, mix = 1.f, panAlt = 0.f;
    float attackA = 1.f, releaseA = 1.f; // coefs one-pole p/ smooth
    float envThr = 0.126f;               // -18 dB
    double internalBpm = 120.0;
    TrigMode trigMode = TrigMode::Host;

    double sampleRate = 44100.0;
    double internalBeats = 0.0;  // relógio interno (beats absolutos)
    double envState = 0.0;       // seguidor de envelope (transient)
    int cooldownLeft = 0;        // amostras até permitir novo retrigger
    bool resetPending = false;   // pedido por scanMidi
    float gateState = 1.f;       // ganho suavizado atual
    EnableRamp pw;               // bypass por módulo
    std::atomic<int> currentStep { 0 };
};
