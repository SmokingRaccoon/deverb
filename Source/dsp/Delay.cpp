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
    dcA = 1.f - std::exp(-juce::MathConstants<float>::twoPi * 12.f / (float) sr);
    smoothFade.reset(sr, 0.04);
    smoothFade.setCurrentAndTargetValue(1.f);
    pendingSwap = false;

    smoothDelay.setCurrentAndTargetValue((float) (0.375 * sr)); // 375 ms @48k ≈
    smoothFb.setCurrentAndTargetValue(0.35f);
    smoothMix.setCurrentAndTargetValue(0.25f);
    smoothFreeze.setCurrentAndTargetValue(0.f);
    smoothDrive.setCurrentAndTargetValue(0.3f);
    smoothSpread.setCurrentAndTargetValue(1.f);

    revVoice.prepare(sr);
    revTmp.setSize(2, maxBlockSize, false, false, true);
    pw.prepare(sr);
    reset();
}

void Delay::reset()
{
    line.reset();
    dampStateL = dampStateR = 0.f;
    dampA = dampAT;
    dcLpL = dcLpR = 0.f;
    wowPhase = 0.0;
    revVoice.reset();
    pendingSwap = false;
    smoothFade.setCurrentAndTargetValue(1.f);
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
void Delay::setAlgo(Algo a)
{
    if (a == algo && ! pendingSwap)
        return;
    // fade-out, troca no silêncio, fade-in (a linha mantém-se: sem tails perdidos)
    pendingAlgo = a;
    pendingSwap = true;
    smoothFade.setTargetValue(0.f);
}
void Delay::setDrive(float d)     { smoothDrive.setTargetValue(juce::jlimit(0.f, 1.f, d)); }
void Delay::setSpread(float s)    { smoothSpread.setTargetValue(juce::jlimit(0.f, 1.f, s)); }
void Delay::setWowRate(float hz)  { wowRate = juce::jlimit(0.1f, 5.f, hz); }
void Delay::setWowDepthMs(float ms) { wowDepthMs = juce::jlimit(0.f, 20.f, ms); }
void Delay::setTempoBpm(double b) { bpm = juce::jlimit(40.0, 240.0, b); }

void Delay::setDampingHz(float hz)
{
    hz = juce::jlimit(100.f, 20000.f, hz);
    // one-pole lowpass: a = 1 - exp(-2π fc / sr); com slew (varrimentos).
    dampAT = 1.f - std::exp(-juce::MathConstants<float>::twoPi * hz / (float) sampleRate);
}

void Delay::process(juce::AudioBuffer<float>& buffer)
{
    juce::ScopedNoDenormals noDenormals;
    const int nCh = juce::jmin(2, buffer.getNumChannels());
    const int n = buffer.getNumSamples();
    if (nCh <= 0 || n <= 0)
        return;

    // --- Modo Reverse: voz dedicada (janela = tempo em beats). ---
    // (Se houver swap pendente a meio do bloco, este bloco ainda sai em
    // Reverse quase-muted; o próximo já segue o novo algoritmo.)
    if (algo == Algo::Reverse)
    {
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
        // 1 passo para decidir o swap; o loop de saída avança o resto
        // por amostra (n+1 passos/bloco — irrelevante na rampa de 40 ms).
        float fadeR = smoothFade.getNextValue();
        if (pendingSwap && fadeR <= 0.002f)
        {
            algo = pendingAlgo;
            pendingSwap = false;
            smoothFade.setTargetValue(1.f);
        }
        if (frz < 0.5f)
            revVoice.recordBlock(buffer);

        jassert(revTmp.getNumSamples() >= n); // pré-alocado no prepare
        if (revTmp.getNumSamples() < n)
            revTmp.setSize(2, n, false, false, true);
        juce::AudioBuffer<float> view(revTmp.getArrayOfWritePointers(), 2, 0, n);
        TempoInfo stub;
        stub.bpm = bpm;
        revVoice.renderBlock(view, stub, bpm, false);

        float* bp0 = buffer.getWritePointer(0);
        float* bp1 = nCh > 1 ? buffer.getWritePointer(1) : nullptr;
        const float* vp0 = view.getWritePointer(0);
        const float* vp1 = nCh > 1 ? view.getWritePointer(1) : nullptr;
        for (int i = 0; i < n; ++i)
        {
            // Mix + fade por amostra (como no caminho normal): automação
            // rápida do mix não faz zipper em blocos grandes.
            const float mix = smoothMix.getNextValue();
            const float dry = 1.f - mix;
            const float fade = smoothFade.getNextValue();
            const float e = pw.next(); // 1× por amostra
            const float wetG = mix * e * fade;
            const float dryG = dry + (1.f - dry) * (1.f - e); // e=0 → dry total
            bp0[i] = bp0[i] * dryG + vp0[i] * wetG;
            if (bp1 != nullptr)
                bp1[i] = bp1[i] * dryG + vp1[i] * wetG;
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
        float fade = smoothFade.getNextValue();
        // Troca de algoritmo no silêncio (a linha mantém-se: sem tails perdidos).
        if (pendingSwap && fade <= 0.002f)
        {
            algo = pendingAlgo;
            pendingSwap = false;
            smoothFade.setTargetValue(1.f);
            fade = 0.f;
        }
        const float dry = 1.f - mix;

        float inL = buffer.getSample(0, i);
        float inR = (nCh > 1) ? buffer.getSample(1, i) : inL;

        // Slew do damping + DC blocker (só wet; o dry fica transparente).
        dampA += (dampAT - dampA) * slewDampK;
        dcLpL += dcA * (inL - dcLpL);
        dcLpR += dcA * (inR - dcLpR);
        const float xL = inL - dcLpL, xR = inR - dcLpR;

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
            float drL = xL * (1.f - drv) + std::tanh(xL * k) * norm * drv;
            float drR = xR * (1.f - drv) + std::tanh(xR * k) * norm * drv;
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
            float w0 = xL * (1.f - frz)
                     + (dampStateL * (1.f - spread) + dampStateR * spread) * fb;
            float w1 = xR * (1.f - frz)
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
            line.pushSample(0, xL * (1.f - frz) + dampStateL * fb);
            if (nCh > 1)
                line.pushSample(1, xR * (1.f - frz) + dampStateR * fb);
        }
        else // Digital (estrutura da Fase 1 + slew/DC no wet)
        {
            wetL = line.popSample(0, d, true);
            wetR = (nCh > 1) ? line.popSample(1, d, true) : wetL;
            dampStateL += a * (wetL - dampStateL);
            dampStateR += a * (wetR - dampStateR);
            line.pushSample(0, xL * (1.f - frz) + dampStateL * fb);
            if (nCh > 1)
                line.pushSample(1, xR * (1.f - frz) + dampStateR * fb);
        }

        wetL *= fade;
        wetR *= fade;
        const float e = pw.next(); // rampa de bypass: 1× por amostra
        const float wetG = mix * e;
        const float dryG = dry + (1.f - dry) * (1.f - e);
        buffer.setSample(0, i, inL * dryG + wetL * wetG);
        if (nCh > 1)
            buffer.setSample(1, i, inR * dryG + wetR * wetG);
    }
}
