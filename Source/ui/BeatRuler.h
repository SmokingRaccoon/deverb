#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// Régua de beats do gater: mostra os 8/16 passos com o passo ativo iluminado
// e números de beat por baixo. Só leitura — o DSP escreve via setActiveStep
// (chamado da message thread por um Timer no editor, nunca do audio thread).
class BeatRuler : public juce::Component
{
public:
    BeatRuler() = default;

    void setNumSteps(int s)    { numSteps = juce::jlimit(1, 16, s); repaint(); }
    void setPattern(int bits)  { pattern = bits & 0xFFFF; repaint(); }
    void setActiveStep(int s)  { activeStep = s; repaint(); }

    void paint(juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        float w = r.getWidth() / (float) juce::jmax(1, numSteps);
        float h = r.getHeight() - 14.f; // reserva p/ números

        for (int i = 0; i < numSteps; ++i)
        {
            auto cell = juce::Rectangle<float>(i * w + 1.f, 0.f, w - 2.f, h);
            bool open = (pattern >> i) & 1;
            if (i == activeStep)
                g.setColour(open ? juce::Colour(0xfffed134) : juce::Colour(0xff7a5c00));
            else
                g.setColour(open ? juce::Colour(0xff4d4d4d) : juce::Colour(0xff242424));
            g.fillRoundedRectangle(cell, 2.f);
        }
        g.setColour(juce::Colour(0xff8a8a8a));
        g.setFont(10.f);
        for (int b = 0; b * 4 < numSteps; ++b) // número a cada 4 passos (1 beat a 1/16)
            g.drawText(juce::String(b + 1), juce::Rectangle<float>(b * 4 * w, h, 4 * w, 14.f),
                       juce::Justification::centred, false);
    }

private:
    int numSteps = 16;
    int pattern = 0x1111;
    int activeStep = -1;
};
