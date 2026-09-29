#pragma once
#include <algorithm>

// RevLinker (Fase 5): resolve o valor EFETIVO de cada parâmetro da cadeia REV.
// - link off → usa o valor próprio do REV (morph irrelevante).
// - link on  → parte do FWD com trim (mult+add) e interpola para o valor
//   próprio conforme o macro MORPH (0 = espelha FWD, 1 = valores próprios).
// - Parâmetros discretos (choices, patterns, bools): só seguem o link
//   (morph não interpola discretos); o trim não se aplica.
// Header-only e sem estado: a cablagem no processor chama por bloco.
class RevLinker
{
public:
    static float resolveContinuous(bool linkOn, float morph01,
                                   float trimMult, float trimAdd,
                                   float fwdVal, float revVal, float lo, float hi)
    {
        float m = std::clamp(morph01, 0.f, 1.f);
        if (! linkOn)
            return std::clamp(revVal, lo, hi);
        float linked = fwdVal * trimMult + trimAdd;
        return std::clamp(linked + (revVal - linked) * m, lo, hi);
    }

    // Versão sem trim (maioria dos parâmetros).
    static float resolveContinuous(bool linkOn, float morph01,
                                   float fwdVal, float revVal, float lo, float hi)
    {
        return resolveContinuous(linkOn, morph01, 1.f, 0.f, fwdVal, revVal, lo, hi);
    }

    static int resolveDiscrete(bool linkOn, int fwdVal, int revVal)
    {
        return linkOn ? fwdVal : revVal;
    }

    static bool resolveBool(bool linkOn, bool fwdVal, bool revVal)
    {
        return linkOn ? fwdVal : revVal;
    }
};
