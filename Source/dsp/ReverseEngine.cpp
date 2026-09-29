#include "ReverseEngine.h"

void ReverseEngine::prepare(double sr)
{
    sampleRate = sr;
    cap = (int) (ringSec * sr) + 64;
    ringL.assign((size_t) cap, 0.f);
    ringR.assign((size_t) cap, 0.f);
    reset();
}

void ReverseEngine::reset()
{
    std::fill(ringL.begin(), ringL.end(), 0.f);
    std::fill(ringR.begin(), ringR.end(), 0.f);
    w = 0;
    playPos = 0.0;
    readerLive = false;
    lfoPhase = 0.0;
    throwing = false;
    throwPos = throwEnd = 0.0;
    lastThrowBtn = throwBtn;
    duckEnv = 0.0;
    duckGain = 1.0;
}

void ReverseEngine::setMode(Mode m)
{
    if (m != mode)
    {
        mode = m;
        readerLive = false; // reancora o leitor na próxima render
        throwing = false;
    }
}

float ReverseEngine::readInterp(int ch, float pos) const
{
    while (pos < 0.f) pos += (float) cap;
    while (pos >= (float) cap) pos -= (float) cap;
    int i0 = (int) pos;
    float fr = pos - (float) i0;
    int i1 = i0 + 1;
    if (i1 >= cap) i1 -= cap;
    const auto& ring = (ch == 0) ? ringL : ringR;
    return ring[(size_t) i0] * (1.f - fr) + ring[(size_t) i1] * fr;
}

void ReverseEngine::recordBlock(const juce::AudioBuffer<float>& dry)
{
    const int nCh = juce::jmin(2, dry.getNumChannels());
    const int n = dry.getNumSamples();
    for (int i = 0; i < n; ++i)
    {
        ringL[(size_t) w] = dry.getSample(0, i);
        ringR[(size_t) w] = (nCh > 1) ? dry.getSample(1, i) : dry.getSample(0, i);
        if (++w >= cap) w = 0;
    }
}

void ReverseEngine::renderBlock(juce::AudioBuffer<float>& out, const TempoInfo& tempo,
                                double bpm, bool midiThrow)
{
    juce::ScopedNoDenormals noDenormals;
    const int nCh = juce::jmin(2, out.getNumChannels());
    const int n = out.getNumSamples();
    if (nCh <= 0 || n <= 0)
        return;

    double captureLen = juce::jlimit(64.0, (double) cap - 64.0,
                                     TempoInfo::beatsToSamples(captureBeats, bpm, sampleRate));
    int xf = juce::jmax(32, (int) (0.008 * sampleRate)); // 8 ms de costura
    juce::ignoreUnused(tempo);

    // Flanco do botão THROW.
    bool btnEdge = throwBtn && !lastThrowBtn;
    lastThrowBtn = throwBtn;

    if (mode == Mode::Throw && (btnEdge || midiThrow) && !throwing)
    {
        throwing = true;
        throwPos = (double) w - 1.0;   // começa no passado mais recente...
        throwEnd = (double) w - captureLen; // ...e anda para trás até aqui
    }

    // Duck: envelope do dry (lido do anel = passado imediato, grátis).
    const double thrLin = std::pow(10.0, -24.0 / 20.0); // threshold fixo -24 dB
    const double atkA = 1.0 - std::exp(-1.0 / (0.005 * sampleRate));
    const double relA = 1.0 - std::exp(-1.0 / (0.250 * sampleRate));

    for (int i = 0; i < n; ++i)
    {
        // Envelope a partir da amostra mais recente gravada.
        int latest = w - 1;
        if (latest < 0) latest += cap;
        float dryMono = (std::abs(ringL[(size_t) latest])
                       + std::abs(ringR[(size_t) latest])) * 0.5f;
        double a = (dryMono > duckEnv) ? atkA : relA;
        duckEnv += a * (dryMono - duckEnv);
        double over = juce::jlimit(0.0, 1.0, (duckEnv - thrLin) / (1.0 - thrLin));
        double wantGain = 1.0 - (double) duckDepth * over;
        double ga = (wantGain < duckGain) ? atkA : relA;
        duckGain += (wantGain - duckGain) * ga;

        float oL = 0.f, oR = 0.f;

        if (mode == Mode::Loop)
        {
            if (! readerLive) { playPos = (double) w - 1.0; readerLive = true; }
            // LFO lento no rate (±15 % a 0.1 Hz): reverse orgânico.
            lfoPhase += juce::MathConstants<double>::twoPi * 0.1 / sampleRate;
            if (lfoPhase >= juce::MathConstants<double>::twoPi)
                lfoPhase -= juce::MathConstants<double>::twoPi;
            double step = (double) rate * (1.0 + (double) lfoDepth * 0.15 * std::sin(lfoPhase));

            double edge = (double) w - captureLen;
            double dist = playPos - edge;
            float t = 1.f;
            double alt = playPos;
            if (dist < (double) xf && dist >= 0.0)
            {
                t = (float) (dist / (double) xf);
                alt = playPos + captureLen; // volta anterior (contínua no anel)
            }
            if (dist < 0.0) // apanhado pela janela deslizante: reancora
            {
                playPos = (double) w - 1.0;
                dist = captureLen;
                t = 1.f;
                alt = playPos;
            }
            float wNew = std::sin(t * juce::MathConstants<float>::halfPi);
            float wOld = std::cos(t * juce::MathConstants<float>::halfPi);
            oL = readInterp(0, (float) playPos) * wNew + readInterp(0, (float) alt) * wOld;
            oR = readInterp(nCh > 1 ? 1 : 0, (float) playPos) * wNew
               + readInterp(nCh > 1 ? 1 : 0, (float) alt) * wOld;
            playPos -= step;
        }
        else if (mode == Mode::Throw && throwing)
        {
            oL = readInterp(0, (float) throwPos);
            oR = readInterp(nCh > 1 ? 1 : 0, (float) throwPos);
            // Fade-out nos últimos xf amostras da cauda (sem clique ao calar).
            double remain = throwPos - throwEnd;
            if (remain < (double) xf && remain >= 0.0)
            {
                float g = (float) (remain / (double) xf);
                oL *= g * g;
                oR *= g * g;
            }
            throwPos -= (double) rate;
            if (throwPos <= throwEnd)
                throwing = false;
        }

        float dg = (float) duckGain;
        out.setSample(0, i, oL * dg);
        if (nCh > 1)
            out.setSample(1, i, oR * dg);
    }
}
