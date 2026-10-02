#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// Tema v4 "Espelho de Agua" (DESIGN-V4.md + styles-v4.css).
// Tokens papel/tinta, acentos por modulo, discos M 48 / XL 76.
// NOTA fonte: decisão embutir Inter/Nimbus — para já usa stack do sistema
// (Helvetica/Inter fallback); recalibrar larguras ?qa quando a fonte for
// embutida via BinaryData (ver DESIGN-V4 §9).
class WaterLnF : public juce::LookAndFeel_V4
{
public:
    WaterLnF();

    // Fonte embutida v4 (Nimbus Sans via BinaryData; mono fica no sistema).
    juce::Typeface::Ptr getTypefaceForFont(const juce::Font& f) override;

    static inline const juce::Colour paper   { 0xffECE9E2 };
    static inline const juce::Colour paper2  { 0xffE3DFD6 };
    static inline const juce::Colour paper3  { 0xffD6D1C5 };
    static inline const juce::Colour ink     { 0xff16171A };
    static inline const juce::Colour ink2    { 0xff1F2125 };
    static inline const juce::Colour ink3    { 0xff2B2D32 };
    static inline const juce::Colour labP    { 0xff55544F };
    static inline const juce::Colour labI    { 0xff9A9892 };
    static inline const juce::Colour hairP    { 0xffC7C2B6 };
    static inline const juce::Colour hairI    { 0xff3A3C42 };
    static inline const juce::Colour gateA    { 0xffEC563A };
    static inline const juce::Colour delayA   { 0xff2E9D6B };
    static inline const juce::Colour verbA    { 0xff3558C8 };
    static inline const juce::Colour granA    { 0xffF0B323 };
    static inline const juce::Colour onGate   { 0xff16171A };
    static inline const juce::Colour onDelay  { 0xff16171A };
    static inline const juce::Colour onVerb   { 0xffECE9E2 };
    static inline const juce::Colour onGran   { 0xff16171A };
    static inline const juce::Colour water    { 0xff8D8A82 };

    static juce::Colour accentFor(const juce::String& mod)
    {
        if (mod == "gate") return gateA;
        if (mod == "delay") return delayA;
        if (mod == "verb") return verbA;
        if (mod == "gran") return granA;
        return ink;
    }
    static juce::Colour onFor(const juce::String& mod)
    {
        if (mod == "gate") return onGate;
        if (mod == "delay") return onDelay;
        if (mod == "verb") return onVerb;
        if (mod == "gran") return onGran;
        return paper;
    }

    // Dial v4: 21 ticks em 270° a partir de -135°, disco 48 (M) / 76 (XL),
    // agulha propria + fantasma (REV linked) + ponto = efetivo.
    // O V4Dial expõe getGhost01/getEff01/isLinked/variant via dynamic_cast
    // seguro (fallback = comportamento normal se não for V4Dial).
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider& slider) override;

    juce::Slider::SliderLayout getSliderLayout(juce::Slider& slider) override
    {
        auto b = slider.getLocalBounds();
        juce::Slider::SliderLayout l;
        l.sliderBounds = b.withTrimmedBottom(26);
        l.textBoxBounds = b.withTrimmedTop(b.getHeight() - 20);
        return l;
    }

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& b,
                          bool, bool) override
    {
        auto r = b.getLocalBounds().toFloat().reduced(1.f);
        bool on = b.getToggleState();
        auto acc = b.findColour(juce::TextButton::buttonOnColourId);
        bool tether = b.getProperties().contains("tether");
        if (on)
        {
            g.setColour(acc);
            g.fillRoundedRectangle(r, 3.f);
            g.setColour(juce::Colours::black);
        }
        else
        {
            g.setColour(juce::Colour(0x00000000));
            g.fillRoundedRectangle(r, 3.f);
            g.setColour(labP);
            g.drawRoundedRectangle(r, 3.f, 1.f);
            g.setColour(ink);
        }
        if (tether)
        {
            float dash[2] = { 4.f, 3.f };
            g.drawDashedLine(juce::Line<float>(r.getX(), r.getY(), r.getRight(), r.getY()), dash, 2);
        }
        g.setFont(10.f);
        g.drawFittedText(b.getButtonText(), r.toNearestInt(), juce::Justification::centred, 1);
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button& b, const juce::Colour&,
                              bool, bool) override
    {
        // Botões de segmentado: respeitam as cores próprias (polaridade p/n).
        if (b.getProperties().contains("segbtn"))
        {
            bool on = b.getToggleState();
            auto r = b.getLocalBounds().toFloat();
            if (on)
            {
                g.setColour(b.findColour(juce::TextButton::buttonColourId));
                g.fillRect(r);
            }
            g.setColour(on ? b.findColour(juce::TextButton::textColourOnId)
                           : b.findColour(juce::TextButton::textColourOffId));
            g.setFont(10.f);
            g.drawFittedText(b.getButtonText(), b.getLocalBounds(),
                             juce::Justification::centred, 1);
            return;
        }
        juce::ToggleButton* tb = dynamic_cast<juce::ToggleButton*>(&b);
        bool on = tb != nullptr && tb->getToggleState();
        auto r = b.getLocalBounds().toFloat().reduced(1.f);
        auto acc = b.findColour(juce::TextButton::buttonOnColourId);
        g.setColour(on ? acc : juce::Colour(0x00000000));
        if (on) g.fillRoundedRectangle(r, 3.f);
        else { g.setColour(labP); g.drawRoundedRectangle(r, 3.f, 1.f); }
        g.setColour(on ? juce::Colours::black : ink);
        g.setFont(10.f);
        g.drawFittedText(b.getButtonText(), r.toNearestInt(), juce::Justification::centred, 1);
    }
};
