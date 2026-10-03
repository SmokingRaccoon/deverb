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
        DeVerbFonts::NimbusSansRegular_ttf, DeVerbFonts::NimbusSansRegular_ttfSize);
    static juce::Typeface::Ptr bold = juce::Typeface::createSystemTypefaceFor(
        DeVerbFonts::NimbusSansBold_ttf, DeVerbFonts::NimbusSansBold_ttfSize);
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

    bool dark = props.contains("dark") ? (bool) props["dark"] : false;
    auto fg = dark ? paper : ink;   // disco / texto principal
    auto bg = dark ? ink : paper;   // fundo atrás do dial
    auto lab = dark ? labI : labP;  // etiquetas

    float cx = r.getCentreX();
    float cy = r.getY() + (isMer ? 14.f : 0.f) + ringD * 0.5f;
    float ghost01 = props.contains("ghost") ? (float)props["ghost"] : sliderPos;
    float eff01 = props.contains("eff") ? (float)props["eff"] : sliderPos;
    bool linked = props.contains("linked") ? (bool)props["linked"] : false;
    juce::Colour acc = juce::Colour(0xff16171A);
    if (props.contains("accent"))
        acc = juce::Colour((juce::uint32)(long long)props["accent"]);

    auto ang = [&](float v01) { return rotaryStartAngle + v01 * (rotaryEndAngle - rotaryStartAngle); };
    // Ticks NO MESMO mapeamento da agulha (21 em 270°, gap em baixo).
    auto tickPt = [&](float v01, float rad) {
        float a = ang(v01) - juce::MathConstants<float>::halfPi;
        return juce::Point<float>(cx + std::cos(a) * rad, cy + std::sin(a) * rad);
    };
    for (int i = 0; i < 21; ++i)
    {
        float v = (float) i / 20.f;
        auto p0 = tickPt(v, ringD * 0.5f - 5.f);
        auto p1 = tickPt(v, ringD * 0.5f - 1.f);
        // aceso até ao efetivo
        g.setColour(i <= eff01 * 20.f ? fg : lab.withAlpha(0.45f));
        g.drawLine(juce::Line<float>(p0, p1), 1.8f);
    }

    // disco (no REV inverte: papel sobre tinta, como no mockup)
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
        g.setColour(fg);
        g.fillEllipse(cx - discD/2, cy - discD/2, discD, discD);
    }

    auto needleLine = [&](float v01, juce::Colour c, float w, bool hollow)
    {
        float a = ang(v01) - juce::MathConstants<float>::halfPi;
        juce::Point<float> tip(cx + std::cos(a) * (discD*0.5f - 4.f),
                               cy + std::sin(a) * (discD*0.5f - 4.f));
        g.setColour(c);
        g.drawLine(juce::Line<float>({cx, cy}, tip), hollow ? w + 2.f : w);
    };

    if (linked)
        needleLine(ghost01, bg, 5.f, true); // fantasma oca, cor do fundo
    if (isMer)
    {
        // agulha em 2 passes com clip (difference do mockup): tinta em cima,
        // papel em baixo — sempre legível nas duas metades.
        g.saveState();
        g.reduceClipRegion((int)cx - 60, (int)(cy - discD/2) - 4, 120, (int)(discD/2) + 4);
        needleLine(sliderPos, ink, 3.f, false);
        g.restoreState();
        g.saveState();
        g.reduceClipRegion((int)cx - 60, (int)cy, 120, (int)(discD/2) + 4);
        needleLine(sliderPos, paper, 3.f, false);
        g.restoreState();
    }
    else
    {
        needleLine(sliderPos, bg, 2.f, false);
    }

    // ponto = efetivo, na cor do modulo, com halo da cor do fundo
    {
        float a = ang(eff01) - juce::MathConstants<float>::halfPi;
        float rr = ringD * 0.5f + 3.f;
        juce::Point<float> p(cx + std::cos(a) * rr, cy + std::sin(a) * rr);
        g.setColour(acc);
        g.fillEllipse(p.x - 3.f, p.y - 3.f, 6.f, 6.f);
        g.setColour(bg);
        g.drawEllipse(p.x - 3.f, p.y - 3.f, 6.f, 6.f, 1.5f);
    }

    // etiqueta colada ao knob (faixa própria, sem colidir com anel/caixa)
    juce::String lb = slider.getName();
    g.setColour(lab);
    g.setFont(10.f);
    if (isMer)
    {
        g.drawFittedText(lb, juce::Rectangle<int>((int)r.getX(), (int)r.getY(), (int)r.getWidth(), 12),
                         juce::Justification::centred, 1);
    }
    else if (lb.isNotEmpty())
    {
        g.drawFittedText(lb, juce::Rectangle<int>((int)r.getX(), (int)(r.getY() + 60), (int)r.getWidth(), 12),
                         juce::Justification::centred, 1);
    }
}
