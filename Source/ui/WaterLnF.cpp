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
    bool linked = props.contains("linked") ? (bool)props["linked"] : false;
    // Ticks/dot seguem a agulha (espaço COM skew): sem link, a posição viva
    // do slider; com link, o efetivo resolvido (já com skew).
    float ghost01 = props.contains("ghost") ? (float)props["ghost"] : sliderPos;
    float eff01 = linked && props.contains("geff") ? (float)props["geff"] : sliderPos;
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
        // aceso até ao efetivo; no meridiano, ticks água e acesos por metade
        bool lit = i <= eff01 * 20.f;
        if (isMer)
        {
            bool above = (p0.y + p1.y) * 0.5f < cy;
            g.setColour(lit ? (above ? ink : paper) : water.withAlpha(0.55f));
        }
        else
            g.setColour(lit ? fg : lab.withAlpha(0.45f));
        g.drawLine(juce::Line<float>(p0, p1), 1.8f);
    }

    // disco (no REV inverte: papel sobre tinta, como no mockup)
    if (isMer)
    {
        // UM círculo, cortado pela linha de água com clip (duas elipses
        // deixavam costura visível nas bordas).
        g.setColour(ink);
        g.fillEllipse(cx - discD/2, cy - discD/2, discD, discD);
        g.saveState();
        g.reduceClipRegion((int)(cx - discD), (int)cy,
                           (int)(discD * 2), (int)(discD + 4));
        g.setColour(paper);
        g.fillEllipse(cx - discD/2, cy - discD/2, discD, discD);
        g.restoreState();
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
        float len = discD * 0.5f - 4.f;
        juce::Point<float> tip(cx + std::cos(a) * len, cy + std::sin(a) * len);
        g.setColour(c);
        if (hollow)
        {
            // fantasma oca: contorno fino (mockup: anel vazado na cor do fundo)
            float px = -std::sin(a) * 2.5f, py = std::cos(a) * 2.5f;
            juce::Point<float> c0(cx, cy);
            g.drawLine(juce::Line<float>(c0 + juce::Point<float>(px, py),
                                         tip + juce::Point<float>(px, py)), 1.5f);
            g.drawLine(juce::Line<float>(c0 - juce::Point<float>(px, py),
                                         tip - juce::Point<float>(px, py)), 1.5f);
            g.drawLine(juce::Line<float>(tip + juce::Point<float>(px, py),
                                         tip - juce::Point<float>(px, py)), 1.5f);
        }
        else
            g.drawLine(juce::Line<float>({cx, cy}, tip), w);
    };

    if (linked)
        needleLine(ghost01, bg, 5.f, true); // fantasma oca, cor do fundo
    if (isMer)
    {
        // agulha em 2 passes com clip (difference do mockup): o disco de
        // cima é tinta logo a agulha é papel; em baixo é ao contrário.
        g.saveState();
        g.reduceClipRegion((int)cx - 60, (int)(cy - discD/2) - 4, 120, (int)(discD/2) + 4);
        needleLine(sliderPos, paper, 3.f, false);
        g.restoreState();
        g.saveState();
        g.reduceClipRegion((int)cx - 60, (int)cy, 120, (int)(discD/2) + 4);
        needleLine(sliderPos, ink, 3.f, false);
        g.restoreState();
    }
    else
    {
        needleLine(sliderPos, bg, 2.f, false);
    }

    // ponto = efetivo, na cor do modulo, com halo que contrasta onde cai
    {
        float a = ang(eff01) - juce::MathConstants<float>::halfPi;
        float rr = ringD * 0.5f + 3.f;
        juce::Point<float> p(cx + std::cos(a) * rr, cy + std::sin(a) * rr);
        g.setColour(acc);
        g.fillEllipse(p.x - 3.f, p.y - 3.f, 6.f, 6.f);
        g.setColour(isMer ? (p.y < cy ? paper : ink) : bg);
        g.drawEllipse(p.x - 3.f, p.y - 3.f, 6.f, 6.f, 1.5f);
    }

    // etiqueta colada ao knob (faixa própria, sem colidir com anel/caixa).
    // O XL (MORPH) não leva etiqueta interna: sobrepunha-se ao anel; o valor
    // e o nome vivem no readout ao lado.
    juce::String lb = slider.getName();
    g.setColour(lab);
    g.setFont(juce::Font(juce::FontOptions(10.f, juce::Font::bold)));
    if (variant == "xl")
    {
    }
    else if (isMer)
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
