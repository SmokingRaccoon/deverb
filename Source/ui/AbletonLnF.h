#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// Tema contrastado v2 (minimal escuro, um acento por módulo).
// Fundo quase-preto, panels por coluna com tint subtil, headers com
// barra de acento. Knobs em 3 tamanhos (células BIG/STD/MINI no editor).
// Só cosmética — não toca em parâmetros, DSP ou attachments.
class AbletonLnF : public juce::LookAndFeel_V4
{
public:
    AbletonLnF()
    {
        setColour(juce::Slider::textBoxTextColourId, greyText);
        setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0x00000000));
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0x00000000));
        setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff1c1c1c));
        setColour(juce::ComboBox::outlineColourId, border);
        setColour(juce::ComboBox::textColourId, greyText);
        setColour(juce::ComboBox::arrowColourId, dimText);
        setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1c1c1c));
        setColour(juce::TextButton::buttonOnColourId, accent);
        setColour(juce::TextButton::textColourOffId, greyText);
        setColour(juce::TextButton::textColourOnId, juce::Colours::black);
        setColour(juce::Label::textColourId, greyText);
        setColour(juce::Slider::rotarySliderFillColourId, greyText);
    }

    // Base.
    static inline const juce::Colour bg       { 0xff0e0e0e };
    static inline const juce::Colour border   { 0xff2e2e2e };
    static inline const juce::Colour greyText { 0xffd4d4d4 };
    static inline const juce::Colour dimText  { 0xff8f8f8f };
    static inline const juce::Colour faintText{ 0xff5c5c5c };
    static inline const juce::Colour accent   { 0xfffed134 }; // amarelo (steps, links)
    static inline const juce::Colour track    { 0xff383838 };

    // Por módulo: acento + tint do panel.
    static inline const juce::Colour motorAccent { 0xffb5b5b5 };
    static inline const juce::Colour gateAccent  { 0xfffed134 };
    static inline const juce::Colour delayAccent { 0xff58c472 };
    static inline const juce::Colour verbAccent  { 0xff5aa9e6 };
    static inline const juce::Colour granAccent  { 0xffff9034 };
    static inline const juce::Colour revAccent   { 0xffb48ce8 };
    static inline const juce::Colour motorPanel  { 0xff151515 };
    static inline const juce::Colour gatePanel   { 0xff1a1810 };
    static inline const juce::Colour delayPanel  { 0xff101a12 };
    static inline const juce::Colour verbPanel   { 0xff10141c };
    static inline const juce::Colour granPanel   { 0xff1c1410 };
    static inline const juce::Colour revStrip    { 0xff14141a };

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider& slider) override
    {
        // O rect recebido é o sliderBounds do getSliderLayout (sem value-box).
        // Zonas EXPLÍCITAS, ancoradas ao topo: pad 4 + knob + nome 12.
        // Diâmetros uniformes por construção (só dependem da célula).
        auto r = juce::Rectangle<float>((float) x, (float) y, (float) width, (float) height);
        constexpr float pad = 4.f, nameH = 12.f;
        bool showName = slider.getName().isNotEmpty();
        float nameZone = showName ? nameH : 0.f;
        float d = juce::jmin(r.getWidth() - 2.f * pad, r.getHeight() - pad - nameZone);
        d = juce::jmax(18.f, d);
        auto knob = juce::Rectangle<float>(r.getCentreX() - d * 0.5f, r.getY() + pad, d, d);
        float radius = knob.getWidth() * 0.5f;
        auto c = knob.getCentre();
        float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        // Acento por knob (definido por slider via setColour) ou cinzento.
        auto fill = slider.findColour(juce::Slider::rotarySliderFillColourId);

        juce::Path trackArc, valueArc;
        trackArc.addCentredArc(c.x, c.y, radius, radius, 0.f,
                               rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(track);
        g.strokePath(trackArc, juce::PathStrokeType(3.f, juce::PathStrokeType::curved));

        valueArc.addCentredArc(c.x, c.y, radius, radius, 0.f,
                               rotaryStartAngle, angle, true);
        g.setColour(slider.isEnabled() ? fill : faintText);
        g.strokePath(valueArc, juce::PathStrokeType(3.f, juce::PathStrokeType::curved));

        juce::Point<float> tip(c.x + std::cos(angle - juce::MathConstants<float>::halfPi) * (radius - 4.f),
                               c.y + std::sin(angle - juce::MathConstants<float>::halfPi) * (radius - 4.f));
        g.setColour(slider.isEnabled() ? greyText : faintText);
        g.drawLine(juce::Line<float>(c, tip), 2.f);

        if (showName)
        {
            g.setColour(slider.isEnabled() ? dimText : faintText);
            g.setFont(10.f);
            g.drawFittedText(slider.getName(),
                             juce::Rectangle<int>((int) r.getX(), (int) (knob.getBottom() + 1.f),
                                                  (int) r.getWidth(), (int) nameH),
                             juce::Justification::centred, 1);
        }
        juce::ignoreUnused(slider);
    }

    juce::Slider::SliderLayout getSliderLayout(juce::Slider& slider) override
    {
        // Value-box 20px (16px nos minis); o resto é rotary+nome.
        auto b = slider.getLocalBounds();
        int valueH = (b.getHeight() >= 70) ? 20 : 16;
        juce::Slider::SliderLayout layout;
        layout.sliderBounds = b.withTrimmedBottom(valueH);
        auto tb = b.withTrimmedTop(b.getHeight() - valueH);
        int w = juce::jmin(b.getHeight() >= 70 ? 64 : 48, tb.getWidth());
        layout.textBoxBounds = tb.withSizeKeepingCentre(w, valueH);
        return layout;
    }

    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox& box) override
    {
        juce::ignoreUnused(isButtonDown);
        bool en = box.isEnabled();
        g.setColour(en ? juce::Colour(0xff1c1c1c) : juce::Colour(0xff141414));
        g.fillRoundedRectangle(0.f, 0.f, (float) width, (float) height, 4.f);
        g.setColour(en ? border : juce::Colour(0xff222222));
        g.drawRoundedRectangle(0.5f, 0.5f, (float) width - 1.f, (float) height - 1.f, 4.f, 1.f);
        juce::Path arrow;
        float cx = (float) (buttonX + buttonW / 2), cy = (float) (buttonY + buttonH / 2);
        arrow.addTriangle(cx - 4.f, cy - 2.f, cx + 4.f, cy - 2.f, cx, cy + 3.f);
        g.setColour(en ? dimText : faintText);
        g.fillPath(arrow);
    }

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& b,
                          bool shouldDrawButtonAsHighlighted,
                          bool shouldDrawButtonAsDown) override
    {
        juce::ignoreUnused(shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
        auto r = b.getLocalBounds().toFloat().reduced(1.f);
        bool on = b.getToggleState();
        bool en = b.isEnabled();
        auto onCol = b.findColour(juce::TextButton::buttonOnColourId);
        g.setColour(on ? (en ? onCol : faintText) : juce::Colour(0xff1c1c1c));
        g.fillRoundedRectangle(r, 4.f);
        if (! on)
        {
            g.setColour(en ? border : juce::Colour(0xff222222));
            g.drawRoundedRectangle(r, 4.f, 1.f);
        }
        g.setColour(on ? (en ? juce::Colours::black : dimText) : (en ? greyText : faintText));
        g.setFont(11.f);
        g.drawFittedText(b.getButtonText(), r.toNearestInt(),
                         juce::Justification::centred, 1);
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button& b, const juce::Colour&,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override
    {
        juce::ignoreUnused(shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
        auto r = b.getLocalBounds().toFloat().reduced(1.f);
        bool on = b.getToggleState();
        g.setColour(on ? accent : juce::Colour(0xff1c1c1c));
        g.fillRoundedRectangle(r, 4.f);
        if (! on)
        {
            g.setColour(border);
            g.drawRoundedRectangle(r, 4.f, 1.f);
        }
        g.setColour(on ? juce::Colours::black : greyText);
        g.setFont(11.f);
        g.drawFittedText(b.getButtonText(), r.toNearestInt(),
                         juce::Justification::centred, 1);
    }

    void drawLabel(juce::Graphics& g, juce::Label& label) override
    {
        g.fillAll(juce::Colours::transparentBlack);
        g.setColour(label.findColour(juce::Label::textColourId));
        g.setFont(11.f);
        g.drawFittedText(label.getText(), label.getLocalBounds(),
                         label.getJustificationType(),
                         juce::jmax(1, (int) (label.getHeight() / 12.f)));
    }
};
