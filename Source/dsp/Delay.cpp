#include "Delay.h"

void Delay::prepare(double sr, int maxBlockSize)
{
    sampleRate = sr;
    int maxSamples = (int) std::ceil(maxDelaySec * (float) sr) + maxBlockSize + 64;
    line.setMaximumDelayInSamples(maxSamples);

    juce::dsp::ProcessSpec spec { sr, (juce::uint32) maxBlockSize, 2 };
    line.prepare(spec);

    // Rampas: tempo ~20 ms (anti-clique), resto ~30 ms.
    smoothDelay.reset(sr, 0.02);
    smoothFb.reset(sr, 0.03);
    smoothMix.reset(sr, 0.03);
    smoothFreeze.reset(sr, 0.03);
    smoothDrive.reset(sr, 0.03);
    smoothSpread.reset(sr, 0.03);

    smoothDelay.setCurrentAndTargetValue((float) (0.375 * sr)); // 375 ms @48k ≈
    smoothFb.setCurrentAndTargetValue(0.35f);
    smoothMix.setCurrentAndTargetValue(0.25f);
    smoothFreeze.setCurrentAndTargetValue(0.f);
    smoothDrive.setCurrentAndTargetValue(0.3f);
    smoothSpread.setCurrentAndTargetValue(1.f);

    revVoice.prepare(sr);
    revTmp.setSize(2, maxBlockSize, false, false, true);
    reset();
}

void Delay::reset()
{
    line.reset();
    dampStateL = dampStateR = 0.f;
    wowPhase = 0.0;
    revVoice.reset();
}

void Delay::setTimeMs(float ms)
{
    testDelaySamples = -1;
    ms = juce::jlimit(1.f, maxDelaySec * 1000.f, ms);
    smoothDelay.setTargetValue(ms * 0.001f * (float) sampleRate);
}

void Delay::setFeedback(float fb) { smoothFb.setTargetValue(juce::jlimit(0.f, 0.95f, fb)); }
void Delay::setMix(float m)       { smoothMix.setTargetValue(juce::jlimit(0.f, 1.f, m)); }
void Delay::setFrozen(bool f)     { smoothFreeze.setTargetValue(f ? 1.f : 0.f); }
void Delay::setAlgo(Algo a)       { algo = a; }
void Delay::setDrive(float d)     { smoothDrive.setTargetValue(juce::jlimit(0.f, 1.f, d)); }
void Delay::setSpread(float s)    { smoothSpread.setTargetValue(juce::jlimit(0.f, 1.f, s)); }
void Delay::setWowRate(float hz)  { wowRate = juce::jlimit(0.1f, 5.f, hz); }
void Delay::setWowDepthMs(float ms) { wowDepthMs = juce::jlimit(0.f, 20.f, ms); }
void Delay::setTempoBpm(double b) { bpm = juce::jlimit(40.0, 240.0, b); }

void Delay::setDampingHz(float hz)
{
    hz = juce::jlimit(100.f, 20000.f, hz);
    // one-pole lowpass: a = 1 - exp(-2π fc / sr)
    dampA = 1.f - std::exp(-juce::MathConstants<float>::twoPi * hz / (float) sampleRate);
}

void Delay::process(juce::AudioBuffer<float>& buffer)
{
    juce::ScopedNoDenormals noDenormals;
    const int nCh = juce::jmin(2, buffer.getNumChannels());
    const int n = buffer.getNumSamples();
    if (nCh <= 0 || n <= 0)
        return;

    // --- Modo Reverse: voz dedicada (janela = tempo em beats). ---
    if (algo == Algo::Reverse)
    {
        float mix = smoothMix.getNextValue();
        // (rampa por bloco chega: o mix mexe-se devagar; resto é direto)
        for (int i = 1; i < n; ++i) smoothMix.getNextValue();
        float dry = 1.f - mix;

        double beats = juce::jlimit(0.25, 4.0,
            smoothDelay.getNextValue() / (float) sampleRate * bpm / 60.0);
        for (int i = 1; i < n; ++i) smoothDelay.getNextValue();
        revVoice.setMode(ReverseEngine::Mode::Loop);
        revVoice.setRate(1.f);
        revVoice.setLfoDepth(0.f);
        revVoice.setCaptureBeats(beats);
        revVoice.setDuckDepth(0.f);

        float frz = smoothFreeze.getNextValue();
        for (int i = 1; i < n; ++i) smoothFreeze.getNextValue();
        if (frz < 0.5f)
            revVoice.recordBlock(buffer);

        if (revTmp.getNumSamples() < n)
            revTmp.setSize(2, n, false, false, true);
        juce::AudioBuffer<float> view(revTmp.getArrayOfWritePointers(), 2, 0, n);
        TempoInfo stub;
        stub.bpm = bpm;
        revVoice.renderBlock(view, stub, bpm, false);

        for (int ch = 0; ch < nCh; ++ch)
        {
            auto* d = buffer.getWritePointer(ch);
            auto* w = view.getWritePointer(ch);
            for (int i = 0; i < n; ++i)
                d[i] = d[i] * dry + w[i] * mix;
        }
        return;
    }

    const float a = dampA;
    const double wowStep = juce::MathConstants<double>::twoPi * (double) wowRate / sampleRate;
    const float wowSmp = wowDepthMs * 0.001f * (float) sampleRate;

    for (int i = 0; i < n; ++i)
    {
        const float d = (testDelaySamples >= 0)
                      ? (float) testDelaySamples
                      : smoothDelay.getNextValue();
        const float fb = smoothFb.getNextValue();
        const float mix = smoothMix.getNextValue();
        const float frz = smoothFreeze.getNextValue(); // 0 normal, 1 frozen
        const float drv = smoothDrive.getNextValue();
        const float spread = smoothSpread.getNextValue();
        const float dry = 1.f - mix;

        float inL = buffer.getSample(0, i);
        float inR = (nCh > 1) ? buffer.getSample(1, i) : inL;

        float wetL = 0.f, wetR = 0.f;

        if (algo == Algo::Tape)
        {
            // Wow: vagueio do ponto de leitura; drive tanh na escrita.
            wowPhase += wowStep;
            if (wowPhase >= juce::MathConstants<double>::twoPi)
                wowPhase -= juce::MathConstants<double>::twoPi;
            float dEff = juce::jmax(1.f, d + wowSmp * (float) std::sin(wowPhase));
            wetL = line.popSample(0, dEff, true);
            wetR = (nCh > 1) ? line.popSample(1, dEff, true) : wetL;
            dampStateL += a * (wetL - dampStateL);
            dampStateR += a * (wetR - dampStateR);
            // Drive: mistura seco→saturado (drive 0 = transparente).
            float k = 1.f + drv * 4.f;
            float norm = 1.f / std::tanh(k);
            float drL = inL * (1.f - drv) + std::tanh(inL * k) * norm * drv;
            float drR = inR * (1.f - drv) + std::tanh(inR * k) * norm * drv;
            line.pushSample(0, drL * (1.f - frz) + dampStateL * fb);
            if (nCh > 1)
                line.pushSample(1, drR * (1.f - frz) + dampStateR * fb);
        }
        else if (algo == Algo::PingPong)
        {
            wetL = line.popSample(0, d, true);
            wetR = (nCh > 1) ? line.popSample(1, d, true) : wetL;
            dampStateL += a * (wetL - dampStateL);
            dampStateR += a * (wetR - dampStateR);
            // Cruzamento: spread=1 → L alimenta R e vice-versa (clássico).
            float w0 = inL * (1.f - frz)
                     + (dampStateL * (1.f - spread) + dampStateR * spread) * fb;
            float w1 = inR * (1.f - frz)
                     + (dampStateR * (1.f - spread) + dampStateL * spread) * fb;
            line.pushSample(0, w0);
            if (nCh > 1)
                line.pushSample(1, w1);
        }
        else if (algo == Algo::Multitap)
        {
            // 4 taps; só o 1º avança o read pointer (ver juce_DelayLine).
            wetL = line.popSample(0, d * tapFrac[0], true) * tapGain[0];
            wetR = (nCh > 1) ? line.popSample(1, d * tapFrac[0], true) * tapGain[0] : 0.f;
            for (int t = 1; t < 4; ++t)
            {
                wetL += line.popSample(0, d * tapFrac[t], false) * tapGain[t];
                if (nCh > 1)
                    wetR += line.popSample(1, d * tapFrac[t], false) * tapGain[t];
            }
            if (nCh <= 1)
                wetR = wetL;
            dampStateL += a * (wetL - dampStateL);
            dampStateR += a * (wetR - dampStateR);
            // Feedback da soma dos taps com damping (denso e escuro).
            line.pushSample(0, inL * (1.f - frz) + dampStateL * fb);
            if (nCh > 1)
                line.pushSample(1, inR * (1.f - frz) + dampStateR * fb);
        }
        else // Digital (bit-idêntico à Fase 1)
        {
            wetL = line.popSample(0, d, true);
            wetR = (nCh > 1) ? line.popSample(1, d, true) : wetL;
            dampStateL += a * (wetL - dampStateL);
            dampStateR += a * (wetR - dampStateR);
            line.pushSample(0, inL * (1.f - frz) + dampStateL * fb);
            if (nCh > 1)
                line.pushSample(1, inR * (1.f - frz) + dampStateR * fb);
        }

        buffer.setSample(0, i, inL * dry + wetL * mix);
        if (nCh > 1)
            buffer.setSample(1, i, inR * dry + wetR * mix);
    }
}
