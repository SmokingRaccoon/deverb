#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

// Relógio central do deVerb. Cada bloco, o processor chama update() e todos
// os módulos (gate, delay, reverb pre-delay, granular, REV) leem daqui.
// Sem PlayHead válido (standalone sem sync): usa o BPM manual.
// Partilhado por FWD e REV — uma só fonte de verdade para o tempo.
class TempoInfo
{
public:
    // Divisões suportadas (índice = valor do parâmetro choice fwd_delay_note, etc).
    // "Free" usa tempo em ms; o resto converte-se via BPM.
    enum class Note : int
    {
        Free = 0,
        N32, N16T, N16, N16D, N8T, N8, N8D, N4, N4D, N2, N1
    };

    static const juce::StringArray& noteNames()
    {
        static const juce::StringArray names {
            "Free", "1/32", "1/16T", "1/16", "1/16D", "1/8T",
            "1/8", "1/8D", "1/4", "1/4D", "1/2", "1/1"
        };
        return names;
    }

    // Duração em beats de cada divisão (semínima = 1 beat).
    static double noteToBeats(Note n)
    {
        switch (n)
        {
            case Note::Free: return 0.0;
            case Note::N32:  return 0.125;
            case Note::N16T: return 1.0 / 6.0;
            case Note::N16:  return 0.25;
            case Note::N16D: return 0.375;
            case Note::N8T:  return 1.0 / 3.0;
            case Note::N8:   return 0.5;
            case Note::N8D:  return 0.75;
            case Note::N4:   return 1.0;
            case Note::N4D:  return 1.5;
            case Note::N2:   return 2.0;
            case Note::N1:   return 4.0;
        }
        return 0.0;
    }

    static double beatsToSeconds(double beats, double bpm)  { return beats * 60.0 / bpm; }
    static double beatsToSamples(double beats, double bpm, double sr)
    { return beatsToSeconds(beats, bpm) * sr; }

    // Política de sync (UI, fora dos 122 IDs): Host segue a DAW quando há
    // dados (fallback manual); Manual ignora o host por completo (BPM do
    // knob + relógio interno, mesmo com transporte a correr).
    enum class SyncPolicy { Host, Manual };

    void update(juce::AudioProcessor& proc, double manualBpm,
                SyncPolicy policy = SyncPolicy::Host)
    {
        bpm = manualBpm;
        ppqPosition = 0.0;
        isPlaying = false;
        fromHost = false;

        if (policy == SyncPolicy::Manual)
            return;

        if (auto* ph = proc.getPlayHead())
        {
            if (auto pos = ph->getPosition())
            {
                if (auto b = pos->getBpm())
                {
                    if (*b > 0.0) { bpm = *b; fromHost = true; }
                }
                if (auto ppq = pos->getPpqPosition())
                    ppqPosition = *ppq;
                isPlaying = pos->getIsPlaying();
            }
        }
    }

    double bpm = 120.0;        // efetivo (host ou manual)
    double ppqPosition = 0.0;  // posição musical em quartos de nota
    bool isPlaying = false;
    bool fromHost = false;     // true = BPM veio da DAW
};
