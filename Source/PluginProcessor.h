#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "core/TempoInfo.h"
#include "dsp/Delay.h"
#include "dsp/Gater.h"
#include "dsp/ReverbEngine.h"
#include "dsp/Granular.h"
#include "dsp/ReverseEngine.h"
#include "core/RevLinker.h"

// Fase 0: esqueleto de efeito. processBlock é passthrough bit-transparente.
// Os 5 parâmetros dummy existem para validar APVTS + state + automação
// antes de haver DSP real. IDs congelados a partir do v1 — não renomear.
class DeVerbProcessor : public juce::AudioProcessor
{
public:
    DeVerbProcessor();
    ~DeVerbProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "deVerb"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    // Cauda real: T60 até 20 s + delay 2.2 s + granular 8 s. Reportar menos
    // fazia os hosts cortar a cauda a meio (P0).
    double getTailLengthSeconds() const override { return 30.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParams();

    // Leitura p/ UI (Timer): passo atual do gate e BPM efetivo.
    int getGateStep() const { return gateStepUi.load(); }
    int getRevGateStep() const { return revGateStepUi.load(); }
    float getUiBpm() const  { return bpmUi.load(); }
    bool isTempoFromHost() const { return fromHostUi.load(); }
    // Leitura p/ UI (Timer): granular ativo ou não (FWD e REV).
    bool isGranularActive() const { return granActiveUi.load(); }
    bool isRevGranularActive() const { return revGranActiveUi.load(); }

    // --- v4 Espelho de Agua: dados vivos + estado de UI ---
    void getScopeSnapshot(float* dst, int n); // FIFO lock-free, mono -1..1
    float getRevCapBeatsUi() const { return revCapBeatsUi.load(); }
    float getRevReadPosUi() const { return revReadPosUi.load(); } // 0..1 na janela
    juce::String getSelMod() const;
    void setSelMod(const juce::String& m);

private:
    TempoInfo tempo;   // relógio central (lido por todos os módulos)
    Delay fwdDelay;    // Fase 1: delay digital no caminho FWD
    Gater fwdGate;     // Fase 2: trance gate antes do delay
    reverb::Reverb fwdVerb; // Fase 3: reverb depois do delay
    Granular fwdGran;       // Fase 4: granular/glitch por último
    // --- Fase 5: motor REV (cadeia gémea + leitor reverso) ---
    ReverseEngine revEngine;
    Gater revGate;
    Delay revDelay;
    reverb::Reverb revVerb;
    Granular revGran;
    juce::AudioBuffer<float> revBuf; // scratch stereo p/ o caminho REV
    juce::AudioBuffer<float> fwdBuf; // scratch: cadeia FWD (p/ XFADE/order)
    // XFADE: ganhos suavizados por engine + fase de beats interna.
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> xfFwd, xfRev, smoothRevMix;
    double procBeats = 0.0; // relógio livre (standalone / fallback)
    // Limiter de segurança no master (ceiling -1 dBFS, attack instantâneo).
    // Usa dspSr guardada no prepare (getSampleRate() sem host não é fiável).
    float limPeak = 0.f, limRelCoef = 1.f;
    double dspSr = 44100.0;
    std::atomic<bool> granActiveUi { false };
    std::atomic<bool> revGranActiveUi { false };
    std::atomic<int> gateStepUi { 0 };
    std::atomic<int> revGateStepUi { 0 };
    std::atomic<float> bpmUi { 120.f };
    std::atomic<bool> fromHostUi { false };
    // v4 scope FIFO (escrita no audio thread, leitura na message thread)
    static constexpr int scopeCap = 2048;
    std::vector<float> scopeBuf { std::vector<float>((size_t)scopeCap, 0.f) };
    juce::AbstractFifo scopeFifo { scopeCap };
    std::atomic<float> revCapBeatsUi { 2.f };
    std::atomic<float> revReadPosUi { 0.f };
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothInput, smoothMaster;
    // MORPH + trims suavizados (50 ms): sem eles a cadeia REV fazia zipper
    // ao rodar o knob herói (os módulos só suavizam o lado FWD).
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothMorph, smoothTrimDly, smoothTrimDec;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DeVerbProcessor)
};
