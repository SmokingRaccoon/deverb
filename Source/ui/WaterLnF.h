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
        // Faixas sem sobreposição (célula 64×86: anel 0..58, etiqueta 60..72,
        // caixa 72..86; mer 64×88: etiqueta 0..12, anel 14..72, caixa 74..88).
        auto b = slider.getLocalBounds();
        juce::Slider::SliderLayout l;
        l.sliderBounds = b.withTrimmedBottom(26);
        l.textBoxBounds = b.withTrimmedTop(b.getHeight() - 14).reduced(4, 0);
        return l;
    }

    // Valores tabulares mono (caixas dos dials); o resto fica em Nimbus Sans.
    juce::Font getLabelFont(juce::Label& label) override
    {
        if (dynamic_cast<juce::Slider*>(label.getParentComponent()) != nullptr)
            return juce::Font(juce::FontOptions("DejaVu Sans Mono", 11.f, juce::Font::plain));
        return juce::Font(juce::FontOptions(11.f, juce::Font::plain));
    }

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& b,
                          bool, bool) override
    {
        auto r = b.getLocalBounds().toFloat().reduced(1.f);
        bool on = b.getToggleState();
        bool dark = b.getProperties().contains("dark");
        auto fg = dark ? paper : ink;
        auto lab = dark ? labI : labP;
        auto acc = b.findColour(juce::TextButton::buttonOnColourId);
        auto onTx = b.findColour(juce::TextButton::textColourOnId);
        bool tether = b.getProperties().contains("tether");
        auto playBar = [&] {
            if (b.getProperties().contains("play"))
                g.fillRect(juce::Rectangle<float>(r.getX(), r.getBottom() - 4.f,
                                                  r.getWidth(), 3.f));
        };
        if (on)
        {
            g.setColour(acc);
            g.fillRoundedRectangle(r, 3.f);
            g.setColour(onTx);
            playBar();
        }
        else
        {
            g.setColour(juce::Colour(0x00000000));
            g.fillRoundedRectangle(r, 3.f);
            g.setColour(lab);
            g.drawRoundedRectangle(r, 3.f, 1.f);
            g.setColour(fg);
            playBar();
        }
        if (tether)
        {
            float dash[2] = { 4.f, 3.f };
            g.setColour(lab);
            g.drawDashedLine(juce::Line<float>(r.getX(), r.getY(), r.getRight(), r.getY()), dash, 2);
        }
        g.setFont(juce::Font(juce::FontOptions("DejaVu Sans Mono", 10.f, juce::Font::plain)));
        g.drawFittedText(b.getButtonText(), r.toNearestInt(), juce::Justification::centred, 1);
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button& b, const juce::Colour&,
                              bool, bool) override
    {
        // Botões de segmentado: só o fundo (o texto vai no drawButtonText,
        // senão saía duplicado). Setas dos steppers: nada (o pai desenha).
        if (b.getProperties().contains("segbtn"))
        {
            if (b.getToggleState())
            {
                g.setColour(b.findColour(juce::TextButton::buttonColourId));
                g.fillRect(b.getLocalBounds().toFloat());
            }
            return;
        }
        if (b.getProperties().contains("nobg"))
            return;
        // NOTA: getToggleState() direto no Button — os steps são TextButton
        // com toggle (dynamic_cast para ToggleButton falhava sempre e o ON
        // nunca pintava).
        bool on = b.getToggleState();
        auto r = b.getLocalBounds().toFloat().reduced(1.f);
        auto acc = b.findColour(juce::TextButton::buttonOnColourId);
        g.setColour(on ? acc : juce::Colour(0x00000000));
        if (on) g.fillRoundedRectangle(r, 3.f);
        else
        {
            g.setColour(b.getProperties().contains("dark") ? labI : labP);
            g.drawRoundedRectangle(r, 3.f, 1.f);
        }
        // (sem texto aqui: o drawButtonText trata disso, senão duplicava)
    }

    void drawButtonText(juce::Graphics& g, juce::TextButton& b,
                        bool, bool) override
    {
        bool on = b.getToggleState();
        g.setColour(on ? b.findColour(juce::TextButton::textColourOnId)
                       : b.findColour(juce::TextButton::textColourOffId));
        if (b.getProperties().contains("stepnum"))
            g.setFont(juce::Font(juce::FontOptions("DejaVu Sans Mono", 10.f, juce::Font::plain)));
        else
            g.setFont(10.f);
        g.drawFittedText(b.getButtonText(), b.getLocalBounds(),
                         juce::Justification::centred, 1);
    }
};
