#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

// Dial v4 (.dial.m / .mer / .xl): rotary com agulha propria + fantasma + ponto.
// A pintura vive em WaterLnF::drawRotarySlider (lê properties ghost/eff/linked).
// Tether (discreto ligado) tratado nos Seg/Stepper/Keys, não aqui —
// aqui `linked` só controla a 2a agulha dos contínuos REV.
class V4Dial : public juce::Slider
{
public:
    V4Dial(const juce::String& label, const juce::String& variant,
           const juce::Colour& accent, double defVal)
        : juce::Slider(juce::Slider::RotaryHorizontalVerticalDrag,
                       juce::Slider::TextBoxBelow),
          accentCol(accent), defV(defVal)
    {
        setName(label);
        getProperties().set("variant", variant);
        getProperties().set("accent", (long long)(unsigned int)accent.getARGB());
        getProperties().set("ghost", 0.0);
        getProperties().set("eff", 0.0);
        getProperties().set("linked", false);
        setTextBoxStyle(juce::Slider::TextBoxBelow, false, 56, 14);
        setDoubleClickReturnValue(true, defVal);
        setVelocityModeParameters(1.0, 1, 0.0, true);
    }

    void setGhost01(float v) { getProperties().set("ghost", (double)v); repaint(); }
    void setEff01(float v) { getProperties().set("eff", (double)v); repaint(); }
    void setLinked(bool l) { getProperties().set("linked", l); repaint(); }

    // Atualiza ghost/eff a partir de valores reais (chamado no Timer do editor).
    // own01 = valor próprio 0..1, ghost01 = herdado, eff01 = efetivo.
    void setNeedles(float own01, float ghost, float eff, bool linked)
    {
        getProperties().set("ghost", (double)ghost);
        getProperties().set("eff", (double)eff);
        getProperties().set("linked", linked);
        juce::ignoreUnused(own01);
        repaint();
    }

    double defV = 0.0;
    juce::Colour accentCol;

    // Arrastar fino com Shift (como no mockup: 700 vs 170px).
    // O Slider já faz wheel + dblclick; aqui só garantimos cursor + tooltip.
    void mouseEnter(const juce::MouseEvent& e) override { juce::Slider::mouseEnter(e); }
};
