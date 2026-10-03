#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// Pedra v4 (.tile): seleciona o módulo.
// Objeto em contorno (tinta) + reflexo espelhado (papel 42%); selecionada:
// objeto cheio de acento (+ contorno tinta, buracos em papel) e reflexo a
// 55%, mais filete de acento — como no mockup.
// gate=quadrado, delay=3 arcos, verb=3 anéis, gran=triângulo + 2 estilhas.
// As 3 teclas (PWR FWD / LINK / PWR REV) são V4Keys separadas do editor.
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
        auto ink = juce::Colour(0xff16171A);
        auto paper = juce::Colour(0xffECE9E2);
        bool sel = getToggleState();

        g.setColour(juce::Colour(0xff55544F));
        g.setFont(juce::Font(juce::FontOptions(10.f, juce::Font::bold)));
        g.drawFittedText(getName(),
                         juce::Rectangle<int>(56, 4, getWidth() - 60, 14),
                         juce::Justification::centredLeft, 1);

        // objeto (caixa 56,22,44,22)
        shapes(g, 56.f, 22.f, sel ? accent_ : ink, sel);
        // reflexo: mesma caixa espelhada para 44..66 (y -> 88-y)
        g.saveState();
        g.addTransform(juce::AffineTransform::verticalFlip(88.f));
        g.setOpacity(sel ? 0.55f : 0.42f);
        shapes(g, 56.f, 22.f, sel ? accent_ : paper, sel);
        g.restoreState();
        g.setOpacity(1.f);

        if (sel)
        {
            g.setColour(accent_);
            g.fillRect(juce::Rectangle<float>(52, 43, getWidth() - 58, 2));
        }
    }

private:
    // Formas na caixa (x, y, 44, 22); sel = cheio de acento + buracos papel.
    void shapes(juce::Graphics& g, float x, float y, juce::Colour main, bool sel)
    {
        auto paper = juce::Colour(0xffECE9E2);
        auto ink = juce::Colour(0xff16171A);
        g.setColour(main);
        if (mod_ == "gate")
        {
            juce::Rectangle<float> r(x + 11, y, 22, 22);
            if (sel) { g.fillRect(r); g.setColour(ink); g.drawRect(r, 1.5f); }
            else g.drawRect(r, 1.5f);
        }
        else if (mod_ == "delay")
        {
            for (int k = 0; k < 3; ++k)
            {
                float cx = x + 9 + k * 12, cy = y + 11;
                float rad = 11 - k * 3;
                juce::Path p;
                p.addArc(cx - rad, cy - rad, rad * 2, rad * 2, -1.1f, 1.1f, true);
                if (! sel)
                    g.setColour(main.withAlpha(k == 0 ? 1.f : k == 1 ? 0.6f : 0.35f));
                if (sel && k == 0) g.fillPath(p);
                else g.strokePath(p, juce::PathStrokeType(k == 0 ? 2.f : 1.5f));
            }
        }
        else if (mod_ == "verb")
        {
            for (int k = 0; k < 3; ++k)
            {
                float rad = 11 - k * 3.5f;
                juce::Rectangle<float> r(x + 22 - rad, y + 11 - rad, rad * 2, rad * 2);
                if (sel && k == 0) g.fillEllipse(r);
                else g.drawEllipse(r, 1.5f);
            }
            if (sel)
            {
                g.setColour(paper);
                g.fillEllipse(x + 22 - 7.5f, y + 11 - 7.5f, 15.f, 15.f);
                g.fillEllipse(x + 22 - 4.f, y + 11 - 4.f, 8.f, 8.f);
            }
        }
        else // gran
        {
            juce::Path p;
            p.addTriangle(x + 11, y + 22, x + 33, y + 22, x + 22, y + 1);
            if (sel) g.fillPath(p);
            else g.strokePath(p, juce::PathStrokeType(1.5f));
            juce::Path s1, s2;
            s1.addTriangle(x + 36, y + 22, x + 42, y + 22, x + 39, y + 14);
            s2.addTriangle(x + 2, y + 22, x + 8, y + 22, x + 5, y + 15);
            if (sel) { g.fillPath(s1); g.fillPath(s2); }
            else
            {
                g.setColour(main.withAlpha(0.55f));
                g.strokePath(s1, juce::PathStrokeType(1.f));
                g.strokePath(s2, juce::PathStrokeType(1.f));
            }
        }
    }

    juce::String mod_;
    juce::Colour accent_;
};
