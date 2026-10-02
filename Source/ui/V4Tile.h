#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// Pedra v4 (.tile): seleciona o módulo. Glifo Bauhaus objeto + reflexo,
// nome à direita, sublinhado de acento quando selecionada.
// As 3 teclas (PWR FWD / LINK / PWR REV) são V4Keys separadas do editor,
// posicionadas sobre a pedra (data-tile) — aqui só o seletor.
class V4Tile : public juce::Button
{
public:
    V4Tile(const juce::String& mod, const juce::Colour& accent)
        : juce::Button(mod.toUpperCase()), mod_(mod), accent_(accent)
    {
        setClickingTogglesState(true);
    }

    void paintButton(juce::Graphics& g, bool, bool) override
    {
        auto r = getLocalBounds().toFloat();
        auto ink = juce::Colour(0xff16171A);
        auto paper = juce::Colour(0xffECE9E2);
        auto labP = juce::Colour(0xff55544F);
        bool sel = getToggleState();

        // nome (canto superior direito da pedra)
        g.setColour(getToggleState() ? ink : labP);
        g.setFont(juce::Font(juce::FontOptions(10.f, juce::Font::bold)));
        g.drawFittedText(getName(),
                         juce::Rectangle<int>(56, 4, (int)r.getWidth() - 60, 14),
                         juce::Justification::centredLeft, 1);

        // glifo objeto (tinta) + reflexo (papel, 42%)
        auto obj = [&](juce::Graphics& gg, juce::Colour c, float oy, float alpha)
        {
            gg.setColour(c.withAlpha(alpha));
            if (mod_ == "gate")
            {
                gg.fillRect(juce::Rectangle<float>(56 + 11, oy + 22, 22, 22));
            }
            else if (mod_ == "delay")
            {
                juce::Path p;
                p.addArc(56 + 9 - 11, oy + 22 - 11, 22, 22, -1.57f, 1.57f);
                p.lineTo(56 + 9, oy + 22);
                p.closeSubPath();
                gg.fillPath(p);
            }
            else if (mod_ == "verb")
            {
                gg.drawEllipse(56 + 11, oy + 22, 22, 22, 1.5f);
                gg.drawEllipse(56 + 14.5f, oy + 25.5f, 15, 15, 1.f);
            }
            else // gran
            {
                juce::Path p;
                p.addTriangle(56 + 11, oy + 44, 56 + 33, oy + 44, 56 + 22, oy + 23);
                gg.fillPath(p);
            }
        };
        if (sel)
        {
            g.setColour(accent_);
            g.fillRect(juce::Rectangle<float>(56 + 11, 22 + 22, 22, 22)); // pastilha objeto
            obj(g, paper, 44, 0.55f);            // reflexo claro
        }
        else
        {
            obj(g, ink, 0, 1.f);
            obj(g, paper, 44, 0.42f);
        }

        if (sel)
        {
            g.setColour(accent_);
            g.fillRect(juce::Rectangle<float>(52, 43, r.getWidth() - 58, 2));
        }
    }

private:
    juce::String mod_;
    juce::Colour accent_;
};
