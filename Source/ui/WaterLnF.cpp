#include "WaterLnF.h"
#include "DeVerbFonts.h"

WaterLnF::WaterLnF()
{
    setColour(juce::Slider::textBoxTextColourId, ink);
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0x00000000));
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0x00000000));
    setColour(juce::ComboBox::backgroundColourId, paper2);
    setColour(juce::ComboBox::textColourId, ink);
    setColour(juce::TextButton::buttonColourId, paper2);
    setColour(juce::TextButton::buttonOnColourId, ink);
    setColour(juce::TextButton::textColourOffId, ink);
    setColour(juce::TextButton::textColourOnId, paper);
    setColour(juce::Label::textColourId, ink);
}

juce::Typeface::Ptr WaterLnF::getTypefaceForFont(const juce::Font& f)
{
    // Nimbus Sans embutida (a fonte do render v4); o bold usa o corte Bold.
    // Mono (valores tabulares) continua no sistema (DejaVu Sans Mono).
    static juce::Typeface::Ptr reg = juce::Typeface::createSystemTypefaceFor(
        DeVerbFonts::NimbusSansRegular_otf, DeVerbFonts::NimbusSansRegular_otfSize);
    static juce::Typeface::Ptr bold = juce::Typeface::createSystemTypefaceFor(
        DeVerbFonts::NimbusSansBold_otf, DeVerbFonts::NimbusSansBold_otfSize);
    if (f.isBold())
        return bold != nullptr ? bold : reg;
    return reg;
}

void WaterLnF::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                juce::Slider& slider)
{
    auto r = juce::Rectangle<float>((float)x, (float)y, (float)width, (float)height);
    auto props = slider.getProperties();
    juce::String variant = props.contains("variant") ? props["variant"].toString() : "m";
    bool isMer = variant == "mer" || variant == "xl";
    float discD = (variant == "xl") ? 76.f : 48.f;
    float ringD = (variant == "xl") ? 88.f : 58.f;

    float cx = r.getCentreX();
    float cy = r.getY() + (isMer ? 14.f : 0.f) + ringD * 0.5f;
    float ghost01 = props.contains("ghost") ? (float)props["ghost"] : sliderPos;
    float eff01 = props.contains("eff") ? (float)props["eff"] : sliderPos;
    bool linked = props.contains("linked") ? (bool)props["linked"] : false;
    juce::Colour acc = juce::Colour(0xff16171A);
    if (props.contains("accent"))
        acc = juce::Colour((juce::uint32)(long long)props["accent"]);

    auto ang = [&](float v01) { return rotaryStartAngle + v01 * (rotaryEndAngle - rotaryStartAngle); };

    // 21 ticks em 270° desde -135°
    g.setColour(labP.withAlpha(0.55f));
    for (int i = 0; i < 21; ++i)
    {
        float a = -135.f + i * (270.f / 20.f);
        float rad = juce::MathConstants<float>::pi * a / 180.f;
        float r1 = ringD * 0.5f - 1.f, r0 = ringD * 0.5f - 5.f;
        juce::Point<float> p0(cx + std::cos(rad) * r0, cy + std::sin(rad) * r0);
        juce::Point<float> p1(cx + std::cos(rad) * r1, cy + std::sin(rad) * r1);
        // aceso até ao efetivo
        float lit = eff01 * 20.f;
        g.setColour(i <= lit ? ink : labP.withAlpha(0.45f));
        g.drawLine(juce::Line<float>(p0, p1), 1.8f);
    }

    // disco
    if (isMer)
    {
        // cortado pela linha de agua: metade tinta / metade papel
        g.setColour(ink);
        g.fillEllipse(cx - discD/2, cy - discD/2, discD, discD/2);
        g.setColour(paper);
        g.fillEllipse(cx - discD/2, cy, discD, discD/2);
        g.setColour(water);
        g.drawEllipse(cx - discD/2, cy - discD/2, discD, discD, 1.f);
    }
    else
    {
        g.setColour(ink);
        g.fillEllipse(cx - discD/2, cy - discD/2, discD, discD);
    }

    auto needle = [&](float v01, juce::Colour c, float w, float lenFrac, bool hollow)
    {
        float a = ang(v01) - juce::MathConstants<float>::halfPi;
        juce::Point<float> tip(cx + std::cos(a) * (discD*0.5f - 4.f) * lenFrac,
                               cy + std::sin(a) * (discD*0.5f - 4.f) * lenFrac);
        g.setColour(c);
        if (hollow)
            g.drawLine(juce::Line<float>({cx, cy}, tip), w + 2.f);
        else
            g.drawLine(juce::Line<float>({cx, cy}, tip), w);
    };

    if (linked)
        needle(ghost01, paper.withAlpha(0.85f), 5.f, 1.f, true); // fantasma oca
    if (isMer)
        needle(sliderPos, juce::Colours::white, 3.f, 1.f, false);
    else
        needle(sliderPos, paper, 2.f, 1.f, false);

    // ponto = efetivo, na cor do modulo
    {
        float a = ang(eff01) - juce::MathConstants<float>::halfPi;
        float rr = ringD * 0.5f + 3.f;
        juce::Point<float> p(cx + std::cos(a) * rr, cy + std::sin(a) * rr);
        g.setColour(acc);
        g.fillEllipse(p.x - 3.f, p.y - 3.f, 6.f, 6.f);
        g.setColour(paper);
        g.drawEllipse(p.x - 3.f, p.y - 3.f, 6.f, 6.f, 1.5f);
    }

    // label + valor (propriedades do dial)
    juce::String lb = slider.getName();
    g.setColour(isMer && false ? labP : labP);
    g.setFont(10.f);
    if (isMer)
    {
        g.setColour(labP);
        g.drawFittedText(lb, juce::Rectangle<int>((int)r.getX(), (int)r.getY(), (int)r.getWidth(), 12),
                         juce::Justification::centred, 1);
    }
    else if (lb.isNotEmpty())
    {
        g.drawFittedText(lb, juce::Rectangle<int>((int)r.getX(), (int)(cy + discD/2 + 2), (int)r.getWidth(), 12),
                         juce::Justification::centred, 1);
    }
}
