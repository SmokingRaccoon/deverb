#include "Granular.h"

void Granular::prepare(double sr)
{
    sampleRate = sr;
    pw.prepare(sr);
    cap = (int) (ringSec * sr) + 64;
    ringL.assign((size_t) cap, 0.f);
    ringR.assign((size_t) cap, 0.f);
    rng.setSeed(0x12345); // determinístico: dois MPs iguais soam igual; testes repetíveis
    reset();
}

void Granular::reset()
{
    std::fill(ringL.begin(), ringL.end(), 0.f);
    std::fill(ringR.begin(), ringR.end(), 0.f);
    w = 0;
    playing = false;
    playPos = loopStart = loopLen = 0.0;
    playDir = 1.0;
    playRate = 1.0;
    loopsDone = 0.0;
    lastDecayLoops = -1.0;
    cachedDecayGain = 1.f;
    activeLeft = releaseLeft = 0;
    outFade = 0.f;
    dryCut = 1.f;
    internalBeats = 0.0;
    lastQuantum = -1.0;
    envState = 0.0;
    cooldownLeft = 0;
    lastManual = manual;
    sliceGate = 1.f;
    activeUi.store(false);
}

void Granular::setMode(Mode m)
{
    if (m != mode)
    {
        mode = m;
        playing = false; // trocar de modo liberta (sem cauda pendurada)
        releaseLeft = 0;
        outFade = 0.f; // e o próximo grab arranca do silêncio (sem clique)
    }
}

float Granular::readRing(int ch, float pos) const
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

void Granular::startGrab(double lenBeats, double bpm)
{
    double sr = sampleRate;
    loopLen = juce::jlimit(64.0, (double) cap - 64.0,
                           TempoInfo::beatsToSamples(lenBeats, bpm, sr));
    loopStart = (double) w - loopLen; // região [trigger-len, trigger]
    playRate = 1.0;
    playDir = 1.0;
    loopsDone = 0.0;

    if (mode == Mode::Reverse)
    {
        playDir = -1.0;
        playPos = loopStart + loopLen - 1.0;
    }
    else if (mode == Mode::Pitch)
    {
        playRate = std::pow(2.0, (double) pitchSt / 12.0);
        if (playRate < 0) playRate = 1.0;
        playPos = loopStart;
    }
    else if (mode == Mode::Stutter)
    {
        loopLen = juce::jmax(64.0, loopLen / 8.0); // só o 1º oitavo
        loopStart = (double) w - loopLen;
        playPos = loopStart;
    }
    else
    {
        playPos = loopStart; // BeatRepeat + Slice
    }

    if (mode == Mode::BeatRepeat)
        activeLeft = (int) (loopLen * (double) repeats) + 1;
    else
    {
        double durMs = (timeNote == TempoInfo::Note::Free)
                     ? (double) timeMs
                     : TempoInfo::beatsToSeconds(TempoInfo::noteToBeats(timeNote), bpm) * 1000.0;
        activeLeft = (int) juce::jlimit(64.0, 8.0 * sr, durMs * 0.001 * sr);
    }
    releaseLeft = 0;
    playing = true;
}

void Granular::process(juce::AudioBuffer<float>& buffer, const TempoInfo& tempo)
{
    juce::ScopedNoDenormals noDenormals;
    const int nCh = juce::jmin(2, buffer.getNumChannels());
    const int n = buffer.getNumSamples();
    if (nCh <= 0 || n <= 0)
        return;

    double bpm = (tempo.fromHost && tempo.bpm > 0.0) ? tempo.bpm : internalBpm;
    double beatsPerSample = bpm / 60.0 / sampleRate;

    double lenBeats = juce::jmax(0.125, TempoInfo::noteToBeats(lenNote)); // mín 1/32
    int xf = juce::jmax(16, (int) (xfadeMs * 0.001 * sampleRate));
    if (playing) // o xfade nunca passa de 1/4 do loop (senão degenera)
        xf = juce::jmax(16, juce::jmin(xf, (int) (loopLen / 4)));
    const int cooldownMax = (int) (0.05 * sampleRate);
    const float sliceA = 1.f - std::exp(-1.f / (float) (0.003 * sampleRate)); // ~3 ms

    // Flanco do botão manual.
    bool manualEdge = manual && !lastManual;
    lastManual = manual;

    for (int i = 0; i < n; ++i)
    {
        // Fase p/ quantização: ppq do host ou relógio interno.
        double phase;
        if (tempo.fromHost && tempo.isPlaying)
            phase = tempo.ppqPosition + (double) i * beatsPerSample;
        else
        {
            phase = internalBeats;
            internalBeats += beatsPerSample;
        }
        double quantum = std::floor(phase / lenBeats);

        float inL = buffer.getSample(0, i);
        float inR = (nCh > 1) ? buffer.getSample(1, i) : inL;

        // Grava sempre (mesmo a tocar: permite retrigger sobre o novo áudio).
        ringL[(size_t) w] = inL;
        ringR[(size_t) w] = inR;
        if (++w >= cap) w = 0;

        // --- Triggers (só com modo ativo e sem reprodução em curso) ---
        if (mode != Mode::Off && !playing)
        {
            bool fire = false;
            if (trig == Trig::Manual)
                fire = manualEdge; // flanco dispara onde quer que caia no bloco
                                   // (o `i == 0` antigo perdia flancos a meio do bloco)
            else if (trig == Trig::Chance)
            {
                if (quantum != lastQuantum)
                {
                    lastQuantum = quantum;
                    fire = rng.nextFloat() < chance;
                }
            }
            else // Envelope
            {
                double a = (std::abs(inL) > envState) ? 0.01 : 0.0005;
                envState += a * (std::abs(inL) - envState);
                if (cooldownLeft > 0)
                    --cooldownLeft;
                else if (envState > (double) envThr * 1.5 && std::abs(inL) > envThr)
                {
                    fire = true;
                    cooldownLeft = cooldownMax;
                }
            }
            if (fire)
                startGrab(lenBeats, bpm);
        }
        if (mode == Mode::Off)
            lastQuantum = quantum; // não disparar atrasado ao ligar

        // --- Reprodução ---
        float wetL = 0.f, wetR = 0.f;
        if (playing)
        {
            // Fim: BeatRepeat pelo nº de loops, outros por duração.
            bool finished = (activeLeft <= 0);
            if (finished && releaseLeft <= 0)
                releaseLeft = xf * 2; // fade de saída
            if (releaseLeft > 0)
            {
                --releaseLeft;
                outFade = juce::jmax(0.f, outFade - 1.f / (float) (xf * 2));
                if (releaseLeft == 0) { playing = false; }
            }
            else
            {
                outFade = juce::jmin(1.f, outFade + 1.f / (float) juce::jmax(1, xf / 2));
                --activeLeft;
            }

            // Avanço + wrap com crossfade equal-power na costura.
            playPos += playDir * playRate;
            double loopEnd = loopStart + loopLen;
            float t = 1.f; // peso da leitura "nova" no xfade
            double altPos = playPos;
            if (playDir > 0 && playPos >= loopEnd)
            {
                playPos -= loopLen;
                loopsDone += 1.0;
            }
            else if (playDir < 0 && playPos < loopStart)
            {
                playPos += loopLen;
                loopsDone += 1.0;
            }
            // Distância à costura mais próxima → crossfade se < xf.
            double distEdge = (playDir > 0) ? (loopEnd - playPos) : (playPos - loopStart);
            if (distEdge < (double) xf && distEdge >= 0.0)
            {
                t = (float) (distEdge / (double) xf);
                altPos = playPos - playDir * loopLen; // volta anterior (ainda válida)
            }
            float wNew = std::sin(t * juce::MathConstants<float>::halfPi);
            float wOld = std::cos(t * juce::MathConstants<float>::halfPi);

            float vL = readRing(0, (float) playPos);
            float vR = readRing(nCh > 1 ? 1 : 0, (float) playPos);
            float oL = readRing(0, (float) altPos);
            float oR = readRing(nCh > 1 ? 1 : 0, (float) altPos);
            wetL = (vL * wNew + oL * wOld);
            wetR = (vR * wNew + oR * wOld);

            // Slice: chop do loop à taxa do grão (flux = densidade 2..8).
            // Suavizado (~3 ms): o corte seco por fronteira de grão = cliques.
            if (mode == Mode::Slice)
            {
                int divs = 2 + (int) (flux * 6.99f);
                double gphase = (playPos - loopStart) / loopLen * (double) divs;
                float target = ((int) gphase % 2 == 0) ? 1.f : 0.15f;
                sliceGate += (target - sliceGate) * sliceA;
                wetL *= sliceGate;
                wetR *= sliceGate;
            }
            else
            {
                sliceGate += (1.f - sliceGate) * sliceA;
            }

            // Decay por volta (BeatRepeat e Stutter): o pow() só corre
            // quando muda o nº de loops (era 1× por amostra).
            if (mode == Mode::BeatRepeat || mode == Mode::Stutter)
            {
                if (loopsDone != lastDecayLoops)
                {
                    lastDecayLoops = loopsDone;
                    cachedDecayGain = std::pow(decay, (float) loopsDone);
                }
                wetL *= cachedDecayGain;
                wetR *= cachedDecayGain;
            }

            wetL *= outFade;
            wetR *= outFade;
        }
        else
        {
            outFade = 0.f;
        }

        // Interrupt com rampa (sem cliques ao entrar/sair).
        // Modo Off = bypass transparente (ignora o mix).
        float wantCut = (interrupt && playing) ? 0.f : 1.f;
        float cutA = 1.f - std::exp(-1.0 / (0.005 * sampleRate));
        dryCut += (wantCut - dryCut) * cutA;

        float dry = (mode == Mode::Off) ? 1.f : (1.f - mix) * dryCut;
        const float e = pw.next(); // rampa de bypass
        const float wetG = mix * e;
        const float dryG = dry + (1.f - dry) * (1.f - e);
        buffer.setSample(0, i, inL * dryG + wetL * wetG);
        if (nCh > 1)
            buffer.setSample(1, i, inR * dryG + wetR * wetG);
    }

    activeUi.store(playing);
}
