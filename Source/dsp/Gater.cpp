#include "Gater.h"

void Gater::prepare(double sr)
{
    sampleRate = sr;
    reset();
}

void Gater::reset()
{
    internalBeats = 0.0;
    envState = 0.0;
    cooldownLeft = 0;
    resetPending = false;
    gateState = 1.f;
    currentStep.store(0);
}

void Gater::setSmooth(float s)
{
    s = juce::jlimit(0.f, 1.f, s);
    // 0.5 ms (faca) .. 100 ms (suave); mesmo tempo p/ attack e release.
    double t = 0.0005 + (double) s * (0.1 - 0.0005);
    double c = 1.0 - std::exp(-1.0 / (t * sampleRate));
    attackA = releaseA = (float) c;
}

void Gater::scanMidi(const juce::MidiBuffer& midi)
{
    if (trigMode != TrigMode::Midi)
        return;
    for (auto meta : midi)
        if (meta.getMessage().isNoteOn()) { resetPending = true; break; }
}

void Gater::process(juce::AudioBuffer<float>& buffer, const TempoInfo& tempo)
{
    juce::ScopedNoDenormals noDenormals;
    const int nCh = juce::jmin(2, buffer.getNumChannels());
    const int n = buffer.getNumSamples();
    if (nCh <= 0 || n <= 0)
        return;

    // Fonte de fase: ppq do host (modo Host + transporte válido) ou relógio.
    bool useHost = (trigMode == TrigMode::Host) && tempo.fromHost && tempo.isPlaying;
    double bpm = useHost ? tempo.bpm : internalBpm;
    double beatsPerSample = bpm / 60.0 / sampleRate;
    double stepDur = stepBeats();
    float closedLevel = 1.f - depth;
    float dry = 1.f - mix;

    // Cooldown do transient em amostras (~50 ms).
    const int cooldownMax = (int) (0.05 * sampleRate);

    for (int i = 0; i < n; ++i)
    {
        double phaseBeats;
        if (useHost)
        {
            // ppq no início do bloco + avanço por amostra (quase sample-accurate).
            phaseBeats = tempo.ppqPosition + (double) i * beatsPerSample;
        }
        else
        {
            if (resetPending) { internalBeats = 0.0; resetPending = false; }
            phaseBeats = internalBeats;
            internalBeats += beatsPerSample;
        }

        // Detetor de transientes (só no modo Transient): reset na subida.
        if (trigMode == TrigMode::Transient && !useHost)
        {
            float mono = buffer.getSample(0, i);
            double a = (std::abs(mono) > envState) ? 0.01 : 0.0005;
            envState += a * (std::abs(mono) - envState);
            if (cooldownLeft > 0)
                --cooldownLeft;
            else if (envState > (double) envThr * 1.5 && std::abs(mono) > envThr)
            {
                // Confirma ataque real (não cauda): amostra bem acima do estado.
                internalBeats = 0.0;
                phaseBeats = 0.0;
                cooldownLeft = cooldownMax;
            }
        }

        int step = (int) std::floor(phaseBeats / stepDur);
        step = ((step % numSteps) + numSteps) % numSteps;
        if (i == 0)
            currentStep.store(step);

        float target = ((pattern >> step) & 1u) ? 1.f : closedLevel;
        float c = (target > gateState) ? attackA : releaseA;
        gateState += (target - gateState) * c;

        // Pan alternado por passo (só no caminho gated).
        float gL = gateState, gR = gateState;
        if (panAlt > 0.f && nCh > 1)
        {
            float p = ((step % 2) ? 1.f : -1.f) * panAlt; // -1..1
            float ang = (p * 0.5f + 0.5f) * juce::MathConstants<float>::halfPi;
            gL *= std::cos(ang);
            gR *= std::sin(ang);
        }

        float inL = buffer.getSample(0, i);
        buffer.setSample(0, i, inL * dry + inL * gL * mix);
        if (nCh > 1)
        {
            float inR = buffer.getSample(1, i);
            buffer.setSample(1, i, inR * dry + inR * gR * mix);
        }
    }
}
