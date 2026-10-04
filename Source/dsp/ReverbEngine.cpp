#include "ReverbEngine.h"

namespace reverb {

static inline float t60ToFb(double t60, double delaySamples, double sr)
{
    t60 = juce::jmax(0.05, t60);
    return std::pow(10.f, (float) (-3.0 * delaySamples / (t60 * sr)));
}

static inline float dampFcFrom01(float d)
{
    d = juce::jlimit(0.f, 1.f, d);
    return 18000.f * std::pow(1500.f / 18000.f, d); // 18k → 1.5k
}

static inline float onePoleA(float fc, double sr)
{
    return 1.f - std::exp(-juce::MathConstants<float>::twoPi * fc / (float) sr);
}

// ---------------- Freeverb ----------------

void Freeverb::prepare(double sampleRate, float sizeScale)
{
    sr = sampleRate;
    float srScale = (float) (sr / 44100.0);
    for (int ch = 0; ch < 2; ++ch)
    {
        for (int i = 0; i < 8; ++i)
        {
            int base = combL[i] + (ch == 1 ? 23 : 0);
            combs[ch][i].prepare((int) std::ceil(base * 1.5f * srScale) + 8);
        }
        for (int i = 0; i < 4; ++i)
        {
            int base = apL[i] + (ch == 1 ? 23 : 0);
            aps[ch][i].prepare((int) std::ceil(base * 1.5f * srScale) + 8);
            aps[ch][i].setFeedback(0.5f);
        }
    }
    setLengths(sizeScale);
    updateFb();
    for (int ch = 0; ch < 2; ++ch)
        for (auto& c : combs[ch]) c.snap(); // arranque exato (testes/medidas)
    reset();
}

void Freeverb::reset()
{
    for (int ch = 0; ch < 2; ++ch)
    {
        for (auto& c : combs[ch]) c.reset();
        for (auto& a : aps[ch]) a.reset();
    }
}

void Freeverb::setLengths(float sizeScale)
{
    float srScale = (float) (sr / 44100.0);
    float s = juce::jlimit(0.5f, 1.5f, sizeScale);
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 8; ++i)
            combs[ch][i].setLength((int) ((combL[i] + (ch == 1 ? 23 : 0)) * s * srScale));
    updateFb();
}

void Freeverb::setT60(double t) { t60 = t; updateFb(); }

void Freeverb::setDamp01(float d)
{
    damp01 = d;
    float dmp = juce::jlimit(0.f, 1.f, d) * 0.9f;
    for (int ch = 0; ch < 2; ++ch)
        for (auto& c : combs[ch]) c.setDamp(dmp);
}

void Freeverb::setFrozen(bool f) { frozen = f; updateFb(); }

void Freeverb::updateFb()
{
    for (int ch = 0; ch < 2; ++ch)
    {
        for (int i = 0; i < 8; ++i)
        {
            // Comprimento atual desconhecido aqui fora: aproxima pelo base.
            // O erro no T60 por causa do Size é < 5 % (aceite e medido).
            float srScale = (float) (sr / 44100.0);
            float len = combL[i] * srScale;
            combs[ch][i].setFeedback(frozen ? 0.9995f : t60ToFb(t60, len, sr));
        }
    }
}

void Freeverb::processSample(float inL, float inR, float& outL, float& outR)
{
    float accL = 0.f, accR = 0.f;
    for (int i = 0; i < 8; ++i)
    {
        accL += combs[0][i].process(inL * inGain);
        accR += combs[1][i].process(inR * inGain);
    }
    for (int i = 0; i < 4; ++i)
    {
        accL = aps[0][i].process(accL);
        accR = aps[1][i].process(accR);
    }
    outL = accL;
    outR = accR;
}

// ---------------- FDN ----------------

void Fdn::prepare(double sampleRate, float sizeScale)
{
    sr = sampleRate;
    float srScale = (float) (sr / 48000.0);
    for (int i = 0; i < 4; ++i)
    {
        int cap = (int) std::ceil(baseLen[i] * 2.0f * srScale) + 8;
        lines[i].buf.assign((size_t) cap, 0.f);
        lines[i].cap = cap;
        lines[i].w = 0;
    }
    setLengths(sizeScale);
    updateFb();
    snap(); // arranque exato
    reset();
}

void Fdn::reset()
{
    for (auto& l : lines) std::fill(l.buf.begin(), l.buf.end(), 0.f);
    for (auto& d : dst) d = 0.f;
}

void Fdn::setLengths(float sizeScale)
{
    float srScale = (float) (sr / 48000.0);
    float s = juce::jlimit(0.5f, 1.5f, sizeScale);
    for (int i = 0; i < 4; ++i)
        lines[i].len = juce::jlimit(16, lines[i].cap - 1,
                                    (int) (baseLen[i] * s * srScale));
    updateFb();
}

void Fdn::setT60(double t) { t60 = t; updateFb(); }

void Fdn::setDampFc(float hz) { dampAT = onePoleA(hz, sr); }

void Fdn::setFrozen(bool f) { frozen = f; updateFb(); }

void Fdn::updateFb()
{
    for (int i = 0; i < 4; ++i)
        fbT[i] = frozen ? 0.9995f : t60ToFb(t60, lines[i].len, sr);
}

void Fdn::processSample(float inL, float inR, float& outL, float& outR)
{
    // Slew de fb/damping: T60/damp/freeze sem cliques.
    dampA += (dampAT - dampA) * slewK;
    float y[4];
    for (int i = 0; i < 4; ++i)
    {
        fb[i] += (fbT[i] - fb[i]) * slewK;
        y[i] = readLine(lines[i], lines[i].len);
        dst[i] += dampA * (y[i] - dst[i]);
    }
    // Hadamard 4x4 unitária (×0.5).
    float z0 = (dst[0] + dst[1] + dst[2] + dst[3]) * 0.5f;
    float z1 = (dst[0] - dst[1] + dst[2] - dst[3]) * 0.5f;
    float z2 = (dst[0] + dst[1] - dst[2] - dst[3]) * 0.5f;
    float z3 = (dst[0] - dst[1] - dst[2] + dst[3]) * 0.5f;

    float in[4] = { inL * 0.5f, inL * 0.5f, inR * 0.5f, inR * 0.5f };
    float z[4] = { z0, z1, z2, z3 };
    for (int i = 0; i < 4; ++i)
    {
        lines[i].buf[(size_t) lines[i].w] = in[i] + z[i] * fb[i];
        if (++lines[i].w >= lines[i].cap) lines[i].w = 0;
    }
    outL = (dst[0] + dst[1]) * 0.5f;
    outR = (dst[2] + dst[3]) * 0.5f;
}

// ---------------- Plate ----------------

void PlateTank::prepare(double sampleRate, float sizeScale)
{
    sr = sampleRate;
    float srScale = (float) (sr / 48000.0);
    dif[0].prepare((int) (200 * srScale) + 8);
    dif[1].prepare((int) (700 * srScale) + 8);
    dif[0].setFeedback(0.6f);
    dif[1].setFeedback(0.6f);
    int cap = (int) std::ceil(2900 * 1.5f * srScale) + 64;
    tankA.buf.assign((size_t) cap, 0.f);
    tankB.buf.assign((size_t) cap, 0.f);
    tankA.cap = tankB.cap = cap;
    tankA.w = tankB.w = 0;
    setLengths(sizeScale);
    updateDecay();
    snap(); // arranque exato
    reset();
}

void PlateTank::reset()
{
    std::fill(tankA.buf.begin(), tankA.buf.end(), 0.f);
    std::fill(tankB.buf.begin(), tankB.buf.end(), 0.f);
    dif[0].reset();
    dif[1].reset();
    fA = fB = 0.f;
    modPhase = 0.0;
}

void PlateTank::setLengths(float sizeScale)
{
    float srScale = (float) (sr / 48000.0);
    float s = juce::jlimit(0.5f, 1.5f, sizeScale);
    lenA = juce::jlimit(64, tankA.cap - 64, (int) (2400 * s * srScale));
    lenB = juce::jlimit(64, tankB.cap - 64, (int) (2900 * s * srScale));
    updateDecay();
}

void PlateTank::setT60(double t) { t60 = t; updateDecay(); }
void PlateTank::setDampFc(float hz) { dampFc = hz; dampAT = onePoleA(hz, sr); }
void PlateTank::setFrozen(bool f) { frozen = f; updateDecay(); }

void PlateTank::updateDecay()
{
    decayT = frozen ? 0.9995f : t60ToFb(t60, (double) (lenA + lenB), sr);
}

float PlateTank::readInterp(const VLine& l, float delay)
{
    float rp = (float) l.w - delay;
    while (rp < 0.f) rp += (float) l.cap;
    int i0 = (int) rp;
    float frac = rp - (float) i0;
    int i1 = i0 + 1;
    if (i1 >= l.cap) i1 -= l.cap;
    return l.buf[(size_t) i0] * (1.f - frac) + l.buf[(size_t) i1] * frac;
}

void PlateTank::processSample(float inL, float inR, float& outL, float& outR)
{
    float d = (inL + inR) * 0.5f;
    d = dif[0].process(d);
    d = dif[1].process(d);

    modPhase += juce::MathConstants<double>::twoPi * 0.8 / sr;
    if (modPhase >= juce::MathConstants<double>::twoPi)
        modPhase -= juce::MathConstants<double>::twoPi;
    float modB = (float) lenB + 6.f * (float) std::sin(modPhase);

    float aOut = readInterp(tankA, (float) lenA);
    float bOut = readInterp(tankB, modB);
    // Slew de decay/damping: T60/damp/freeze sem cliques.
    decay += (decayT - decay) * slewK;
    dampA += (dampAT - dampA) * slewK;
    fA += dampA * (aOut - fA);
    fB += dampA * (bOut - fB);

    tankA.buf[(size_t) tankA.w] = d + fB * decay;
    tankB.buf[(size_t) tankB.w] = d + fA * decay;
    if (++tankA.w >= tankA.cap) tankA.w = 0;
    if (++tankB.w >= tankB.cap) tankB.w = 0;

    outL = fA * 0.8f + d * 0.2f;
    outR = fB * 0.8f + d * 0.2f;
}

// ---------------- Shimmer ----------------

void ShimmerVoice::prepare(double sampleRate, float sizeScale)
{
    sr = sampleRate;
    fdn.prepare(sampleRate, sizeScale);
    int ringCap = (int) (0.1 * sampleRate) + 8;
    ring.assign((size_t) ringCap, 0.f);
    this->cap = ringCap;
    w = 0;
    reset();
}

void ShimmerVoice::reset()
{
    fdn.reset();
    std::fill(ring.begin(), ring.end(), 0.f);
    w = 0;
    shimState = 0.f;
    rPos = 0.f;
}

void ShimmerVoice::setT60(double t) { fdn.setT60(t); }
void ShimmerVoice::setDampFc(float hz)
{
    fdn.setDampFc(hz);
    // LP da oitava com a SR real (era 48000 fixo) e teto 4.5k: a leitura 2×
    // dobra o conteúdo, logo o aliasing vive acima de ~Nyquist/2.
    shimA = onePoleA(juce::jmin(hz, 4500.f), sr);
}
void ShimmerVoice::setFrozen(bool f) { fdn.setFrozen(f); }

float ShimmerVoice::readAt(float pos) const
{
    while (pos < 0.f) pos += (float) cap;
    while (pos >= (float) cap) pos -= (float) cap;
    int i0 = (int) pos;
    float frac = pos - (float) i0;
    int i1 = i0 + 1;
    if (i1 >= cap) i1 -= cap;
    return ring[(size_t) i0] * (1.f - frac) + ring[(size_t) i1] * frac;
}

void ShimmerVoice::processSample(float inL, float inR, float& outL, float& outR)
{
    // Leitura a 2× do anel = oitava acima (smear clássico de shimmer).
    // A 2×, a leitura alcança a escrita a cada volta: sem proteção, esse
    // "lap" lê fronteira escrita/não-escrita = cliques + ruído. Quando a
    // leitura se aproxima da escrita, funde-se (equal-power) com 2ª cabeça
    // meia-volta atrás — sempre contínua, nunca faz lap audível.
    rPos += 2.f;
    if (rPos >= (float) cap) rPos -= (float) cap;
    float dist = (float) w - rPos;
    while (dist < 0.f) dist += (float) cap;
    float t = juce::jlimit(0.f, 1.f, dist / (float) lapFade);
    float wA = std::sin(t * juce::MathConstants<float>::halfPi);
    float wB = std::cos(t * juce::MathConstants<float>::halfPi);
    float bPos = rPos - (float) cap * 0.5f;
    float shim = readAt(rPos) * wA + readAt(bPos) * wB;
    shimState += shimA * (shim - shimState);

    fdn.processSample(inL + shimState * shimmerAmt, inR + shimState * shimmerAmt, outL, outR);
    ring[(size_t) w] = (outL + outR) * 0.5f;
    if (++w >= cap) w = 0;
}

// ---------------- Wrapper ----------------

void Reverb::prepare(double sampleRate, int maxBlockSize)
{
    sr = sampleRate;
    freeverb.prepare(sr, 1.f);
    fdn.prepare(sr, 1.f);
    plate.prepare(sr, 1.f);
    shimmer.prepare(sr, 1.f);

    // 6.5 s: a nota 1/1 a 40 BPM são 6 s; antes clampava silenciosamente a 2 s.
    preCap = (int) (6.5 * sr) + 64;
    preL.assign((size_t) preCap, 0.f);
    preR.assign((size_t) preCap, 0.f);
    preW = 0;

    smoothPre.reset(sr, 0.03);
    smoothMix.reset(sr, 0.03);
    smoothFreeze.reset(sr, 0.05);
    smoothFade.reset(sr, 0.04);
    smoothWidth.reset(sr, 0.03);
    smoothPre.setCurrentAndTargetValue(0.f);
    smoothMix.setCurrentAndTargetValue(0.3f);
    smoothFreeze.setCurrentAndTargetValue(0.f);
    smoothFade.setCurrentAndTargetValue(1.f);
    smoothWidth.setCurrentAndTargetValue(1.f);
    dcA = 1.f - std::exp(-juce::MathConstants<float>::twoPi * 12.f / (float) sr);
    pendingStructural = false;
    pendingReset = false;
    pendingSizeChange = false;
    appliedBucket = -1;

    juce::dsp::ProcessSpec spec { sr, (juce::uint32) maxBlockSize, 2 };
    for (auto* f : { &lpL, &lpR }) { f->prepare(spec); f->setType(juce::dsp::StateVariableTPTFilterType::lowpass); f->setResonance(0.70710678f); }
    for (auto* f : { &hpL, &hpR }) { f->prepare(spec); f->setType(juce::dsp::StateVariableTPTFilterType::highpass); f->setResonance(0.70710678f); }
    setTone(lastLocut < 0.f ? 80.f : lastLocut, lastHicut < 0.f ? 12000.f : lastHicut);
    pw.prepare(sr);
    reset();
}

void Reverb::reset()
{
    freeverb.reset();
    fdn.reset();
    plate.reset();
    dcLpL = dcLpR = 0.f;
    smoothWidth.setCurrentAndTargetValue(width);
    shimmer.reset();
    std::fill(preL.begin(), preL.end(), 0.f);
    std::fill(preR.begin(), preR.end(), 0.f);
}

void Reverb::setAlgo(Algo a)
{
    if (a == algo && ! pendingReset)
        return;
    // Adia para o ponto silencioso (ver process): sem cliques de reset.
    pendingAlgo = a;
    pendingStructural = true;
    pendingReset = true;
    smoothFade.setTargetValue(0.f);
}

void Reverb::doSetLengths(float s)
{
    float sc = 0.5f + juce::jlimit(0.f, 1.f, s);
    freeverb.setLengths(sc);
    fdn.setLengths(sc);
    plate.setLengths(sc);
}

void Reverb::setSize01(float s)
{
    int bucket = (int) (juce::jlimit(0.f, 1.f, s) * 20.f + 0.5f);
    if (appliedBucket < 0)
    {
        doSetLengths(s); // primeira vez: imediato (buffers vazios no prepare)
        appliedBucket = bucket;
        return;
    }
    if (bucket == appliedBucket)
        return;
    int step = (bucket > appliedBucket) ? (bucket - appliedBucket)
                                        : (appliedBucket - bucket);
    if (step > 2)
    {
        // Salto grande: fade-out, aplica no silêncio, fade-in.
        pendingSize = s;
        pendingSizeChange = true;
        pendingStructural = true;
        smoothFade.setTargetValue(0.f);
    }
    else
    {
        // Passo pequeno: aplica direto (vira chorus subtil, mascarado).
        doSetLengths(s);
    }
    appliedBucket = bucket;
}

void Reverb::setT60(double t)
{
    t = juce::jlimit(0.05, 30.0, t);
    freeverb.setT60(t);
    fdn.setT60(t);
    plate.setT60(t);
    shimmer.setT60(t);
}

void Reverb::setDamp01(float d)
{
    freeverb.setDamp01(d);
    float fc = dampFcFrom01(d);
    fdn.setDampFc(fc);
    plate.setDampFc(fc);
    shimmer.setDampFc(fc);
}

void Reverb::setWidth01(float w)
{
    width = juce::jlimit(0.f, 1.f, w);
    smoothWidth.setTargetValue(width);
}

void Reverb::setPredelaySamples(int s)
{
    smoothPre.setTargetValue((float) juce::jlimit(0, preCap - 1, s));
}

void Reverb::setFrozen(bool f)
{
    frozen = f;
    smoothFreeze.setTargetValue(f ? 1.f : 0.f);
    freeverb.setFrozen(f);
    fdn.setFrozen(f);
    plate.setFrozen(f);
    shimmer.setFrozen(f);
}

void Reverb::setEnabled(bool b)      { pw.set(b); }
bool Reverb::isBypassed() const      { return pw.silent(); }
bool Reverb::takeClear()             { return pw.takeClear(); }

void Reverb::setMix(float m) { smoothMix.setTargetValue(juce::jlimit(0.f, 1.f, m)); }

void Reverb::setTone(float locutHz, float hicutHz)
{
    if (locutHz == lastLocut && hicutHz == lastHicut)
        return; // cutoff só mexe quando o parâmetro muda (zero allocs, sempre)
    lastLocut = locutHz;
    lastHicut = hicutHz;
    // StateVariableTPTFilter: só matemática, sem new (era IIR+Coefficients).
    float lo = juce::jlimit(10.f, (float) sr * 0.45f, locutHz);
    float hi = juce::jlimit(10.f, (float) sr * 0.45f, hicutHz);
    hpL.setCutoffFrequency(lo); hpR.setCutoffFrequency(lo);
    lpL.setCutoffFrequency(hi); lpR.setCutoffFrequency(hi);
}

void Reverb::noteStructuralChange(bool resetEngines)
{
    // Aplicação imediata (testes, caminhos explícitos): o caminho normal
    // por bloco (setAlgo/setSize01) adia para o silêncio sozinho.
    if (resetEngines)
    {
        algo = pendingAlgo;
        reset();
    }
    if (pendingSizeChange)
        doSetLengths(pendingSize);
    pendingStructural = false;
    pendingReset = false;
    pendingSizeChange = false;
    smoothFade.setCurrentAndTargetValue(0.f);
    smoothFade.setTargetValue(1.f);
}

void Reverb::process(juce::AudioBuffer<float>& buffer)
{
    juce::ScopedNoDenormals noDenormals;
    const int nCh = juce::jmin(2, buffer.getNumChannels());
    const int n = buffer.getNumSamples();
    if (nCh <= 0 || n <= 0)
        return;

    for (int i = 0; i < n; ++i)
    {
        float preD = smoothPre.getNextValue();
        float mix = smoothMix.getNextValue();
        float frz = smoothFreeze.getNextValue();
        float fade = smoothFade.getNextValue();
        float dry = 1.f - mix;

        // Troca estrutural adiada: aplica no silêncio e volta a abrir.
        if (pendingStructural && fade <= 0.002f)
        {
            if (pendingReset)
            {
                algo = pendingAlgo;
                reset();
            }
            if (pendingSizeChange)
            {
                doSetLengths(pendingSize);
                pendingSizeChange = false;
            }
            pendingStructural = false;
            pendingReset = false;
            smoothFade.setTargetValue(1.f);
        }

        float inL = buffer.getSample(0, i);
        float inR = (nCh > 1) ? buffer.getSample(1, i) : inL;
        dcLpL += dcA * (inL - dcLpL);
        dcLpR += dcA * (inR - dcLpR);
        const float xL = inL, xR = inR; // BISECT: estados atualizam, não usados

        // Pre-delay com leitura interpolada (sem cliques ao mudar).
        preL[(size_t) preW] = xL;
        preR[(size_t) preW] = xR;
        float rp = (float) preW - preD;
        while (rp < 0.f) rp += (float) preCap;
        int i0 = (int) rp;
        float fr = rp - (float) i0;
        int i1 = i0 + 1;
        if (i1 >= preCap) i1 -= preCap;
        float dL = preL[(size_t) i0] * (1.f - fr) + preL[(size_t) i1] * fr;
        float dR = preR[(size_t) i0] * (1.f - fr) + preR[(size_t) i1] * fr;
        if (++preW >= preCap) preW = 0;

        // Freeze: corta a entrada (com rampa); feedbacks vão a ~1 nos motores.
        dL *= (1.f - frz);
        dR *= (1.f - frz);

        float wL = 0.f, wR = 0.f;
        switch (algo)
        {
            case Algo::Room:    freeverb.processSample(dL, dR, wL, wR); break;
            case Algo::Hall:    fdn.processSample(dL, dR, wL, wR); break;
            case Algo::Plate:   plate.processSample(dL, dR, wL, wR); break;
            case Algo::Shimmer: shimmer.processSample(dL, dR, wL, wR); break;
        }

        // Width: mid/side (1 = stereo normal, 0 = mono), suavizado.
        float wCur = smoothWidth.getNextValue();
        float mid = (wL + wR) * 0.5f;
        float side = (wL - wR) * 0.5f;
        wL = mid + side * wCur;
        wR = mid - side * wCur;

        wL = lpL.processSample(0, hpL.processSample(0, wL));
        wR = lpR.processSample(1, hpR.processSample(1, wR));

        wL *= fade;
        wR *= fade;

        const float e = pw.next(); // rampa de bypass
        const float wetG = mix * e;
        const float dryG = dry + (1.f - dry) * (1.f - e);
        buffer.setSample(0, i, inL * dryG + wL * wetG);
        if (nCh > 1)
            buffer.setSample(1, i, inR * dryG + wR * wetG);
    }
}

} // namespace reverb
