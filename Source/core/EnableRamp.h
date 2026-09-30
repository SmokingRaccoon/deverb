#pragma once
#include <juce_core/juce_core.h>

// Bypass por módulo (Fase 8): rampa de 5 ms no wet (sem cliques) +
// pedido de clear único (sem tails) + custo zero parado.
// Protocolo com o processor, por bloco e por módulo:
//   - mod.setEnabled(on)                       // a partir do param
//   - if (! mod.isBypassed()) mod.process(b)   // se não, salta (poupa CPU)
//     else if (mod.takeClear()) mod.reset()    // limpa buffers uma vez
//   - ao religar, os buffers estão limpos e a rampa sobe de 0.
// Sem alocações; seguro no audio thread depois de prepare().
struct EnableRamp
{
    void prepare(double sampleRate)
    {
        step = 1.f / (float) (0.005 * sampleRate); // 5 ms
    }

    void set(bool on)
    {
        if (! on && target)
            clearArmed = true; // só pede clear na transição on->off
        target = on;
    }

    // Avança a rampa; chamar 1× por amostra (só quando a processar).
    float next()
    {
        if (target)
            gain = juce::jmin(1.f, gain + step);
        else
            gain = juce::jmax(0.f, gain - step);
        return gain;
    }

    bool silent() const { return ! target && gain <= 0.0001f; }

    // Devolve true UMA vez quando fica silencioso (para o reset).
    bool takeClear()
    {
        if (clearArmed && silent())
        {
            clearArmed = false;
            return true;
        }
        return false;
    }

private:
    bool target = true; // módulos nascem ligados
    float gain = 1.f;
    float step = 1.f / 240.f;
    bool clearArmed = false;
};
