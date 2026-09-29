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
    double getTailLengthSeconds() const override { return 8.0; }
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
    float getUiBpm() const  { return bpmUi.load(); }
    // Leitura p/ UI (Timer): granular ativo ou não.
    bool isGranularActive() const { return granActiveUi.load(); }

private:
    TempoInfo tempo;   // relógio central (lido por todos os módulos)
    Delay fwdDelay;    // Fase 1: delay digital no caminho FWD
    Gater fwdGate;     // Fase 2: trance gate antes do delay
    reverb::Reverb fwdVerb; // Fase 3: reverb depois do delay
    Granular fwdGran;       // Fase 4: granular/glitch por último
    int lastVerbAlgo = -1, lastVerbSizeBucket = -1;
    // --- Fase 5: motor REV (cadeia gémea + leitor reverso) ---
    ReverseEngine revEngine;
    Gater revGate;
    Delay revDelay;
    reverb::Reverb revVerb;
    Granular revGran;
    juce::AudioBuffer<float> revBuf; // scratch stereo p/ o caminho REV
    juce::AudioBuffer<float> fwdBuf; // scratch: cadeia FWD (p/ XFADE/order)
    int lastRevAlgo = -1, lastRevSizeBucket = -1;
    // XFADE: ganhos suavizados por engine + fase de beats interna.
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> xfFwd, xfRev, smoothRevMix;
    double procBeats = 0.0; // relógio livre (standalone / fallback)
    std::atomic<bool> granActiveUi { false };
    std::atomic<int> gateStepUi { 0 };
    std::atomic<float> bpmUi { 120.f };
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothInput, smoothMaster;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DeVerbProcessor)
};
