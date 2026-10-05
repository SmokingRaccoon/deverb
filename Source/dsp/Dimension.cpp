#include "Dimension.h"

void Dimension::prepare(double sr, int maxBlockSize)
{
    sampleRate = sr;
    int maxSamples = (int) std::ceil(maxDelaySec * (float) sr) + maxBlockSize + 64;
    line.setMaximumDelayInSamples(maxSamples);

    juce::dsp::ProcessSpec spec { sr, (juce::uint32) maxBlockSize, 2 };
    line.prepare(spec);

    smoothSize.reset(sr, 0.03);
    smoothMix.reset(sr, 0.03);
    smoothSize.setCurrentAndTargetValue(0.35f);
    smoothMix.setCurrentAndTargetValue(0.f);
    pw.prepare(sr);
    reset();
}

void Dimension::reset()
{
    line.reset();
}

void Dimension::setSize01(float s) { smoothSize.setTargetValue(juce::jlimit(0.f, 1.f, s)); }
void Dimension::setMix(float m)   { smoothMix.setTargetValue(juce::jlimit(0.f, 1.f, m)); }

void Dimension::process(juce::AudioBuffer<float>& buffer)
{
    juce::ScopedNoDenormals noDenormals;
    const int nCh = juce::jmin(2, buffer.getNumChannels());
    const int n = buffer.getNumSamples();
    if (nCh <= 0 || n <= 0)
        return;

    const float maxSmp = maxDelaySec * (float) sampleRate;
    for (int i = 0; i < n; ++i)
    {
        const float size = smoothSize.getNextValue();
        const float mix = smoothMix.getNextValue();
        float inL = buffer.getSample(0, i);
        float inR = (nCh > 1) ? buffer.getSample(1, i) : inL;

        line.pushSample(0, inL);
        if (nCh > 1)
            line.pushSample(1, inR);

        // 4 vozes; a 1ª de CADA canal avança o read pointer (a JUCE
        // DelayLine lê readPos+delay: sem avançar, o canal lê sempre a
        // mesma zona e salta a cada volta do anel — cliques periódicos).
        int chR = (nCh > 1) ? 1 : 0;
        float v0 = line.popSample(0, tapFrac[0] * size * maxSmp, true);
        float v1 = line.popSample(0, tapFrac[1] * size * maxSmp, false);
        float v2 = line.popSample(chR, tapFrac[2] * size * maxSmp, chR != 0);
        float v3 = line.popSample(chR, tapFrac[3] * size * maxSmp, false);

        // Um só bus wet: soma em L, subtrai em R. A soma-mono cancela
        // exatamente (mono = dry); o stereo abre pela diferença.
        float w = (v0 - v1 + v2 - v3) * wetGain;
        const float e = pw.next();
        const float wetG = mix * e;
        buffer.setSample(0, i, inL + w * wetG);
        if (nCh > 1)
            buffer.setSample(1, i, inR - w * wetG);
    }
}
