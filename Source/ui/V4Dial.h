#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

// Dial v4 (.dial.m / .mer / .xl): rotary 270° com gap em baixo (igual ao
// mockup: -135°..+135° em torno do topo), 21 ticks calculados no mesmo
// mapeamento da agulha, agulha própria + fantasma (REV com link) + ponto.
// A caixa mostra o valor EFETIVO formatado com unidades (editável; ao
// escrever, edita o valor próprio). XL (MORPH) não tem caixa própria.
class V4Dial : public juce::Slider
{
public:
    V4Dial(const juce::String& label, const juce::String& variant,
           const juce::Colour& accent, double defVal)
        : juce::Slider(juce::Slider::RotaryHorizontalVerticalDrag,
                       juce::Slider::TextBoxBelow),
          accentCol(accent), defV(defVal)
    {
        using juce::MathConstants;
        // 270° com gap em baixo (mockup: agulha -135°..+135° em torno do topo).
        setRotaryParameters(MathConstants<float>::pi * 1.25f,
                            MathConstants<float>::pi * 2.75f, true);
        setName(label);
        getProperties().set("variant", variant);
        getProperties().set("accent", (long long)(unsigned int)accent.getARGB());
        // Espaços (ver B15b): a agulha usa sliderPos (JUCE, COM skew); os
        // ticks/dot usam "geff" (COM skew, para bater na agulha); o texto
        // mostra "geffReal" (unidades reais) quando há link, senão o valor
        // vivo. Tudo o resto é linear e NUNCA se mistura.
        getProperties().set("ghost", 0.0);
        getProperties().set("geff", 0.0);
        getProperties().set("geffReal", defVal);
        getProperties().set("linked", false);
        getProperties().set("dark", false);
        if (variant == "xl")
            setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        else
            setTextBoxStyle(juce::Slider::TextBoxBelow, false, 56, 14);
        if (variant == "mer")
            setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffECE9E2));
        setDoubleClickReturnValue(true, defVal);
        setVelocityModeParameters(1.0, 1, 0.0, true);
        setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xff55544F));
        updateTextFns();
    }

    // Gama real + formato (tabela de DESIGN-V4 §3 / generate-v4.py).
    // fmt: n2 n1 int ms ms1 hz hz1 s db st x pct
    void setupRange(double lo, double hi, const juce::String& fmt)
    {
        lo_ = lo; hi_ = hi; fmt_ = fmt;
        updateTextFns();
    }
    void setDark(bool d)
    {
        getProperties().set("dark", d);
        setColour(juce::Slider::textBoxTextColourId,
                  d ? juce::Colour(0xffECE9E2) : juce::Colour(0xff55544F));
        repaint();
    }

    void setGhost01(float v) { getProperties().set("ghost", (double)v); repaint(); }
    void setLinked(bool l) { getProperties().set("linked", l); repaint(); }

    // ghostSk/effSk em proporção COM skew (igual à agulha); effReal em
    // unidades reais para a caixa. Só repinta se mudar (Timer a 30 Hz).
    void setNeedles(float ghostSkewed, float effSkewed, double effReal, bool linked)
    {
        auto& pr = getProperties(); // referência! (cópia deitava tudo fora)
        bool ch = (float)pr["ghost"] != ghostSkewed || (float)pr["geff"] != effSkewed
               || (bool)pr["linked"] != linked
               || (double)pr["geffReal"] != effReal;
        pr.set("ghost", (double)ghostSkewed);
        pr.set("geff", (double)effSkewed);
        pr.set("geffReal", effReal);
        pr.set("linked", linked);
        juce::ignoreUnused(ch);
        if (ch) repaint();
        refreshTextIfNeeded();
    }

    // A caixa só refresca no setValue; o Timer atualiza eff/agulhas sem
    // mexer no valor — sem isto a caixa mostrava o texto inicial para sempre.
    // Early-out barato (o Timer corre a 30 Hz): só formata se algo mudou.
    void refreshTextIfNeeded()
    {
        double v = getValue();
        double effReal = getProperties().contains("geffReal")
            ? (double) getProperties()["geffReal"] : v;
        bool linked = getProperties().contains("linked")
                   && (bool) getProperties()["linked"];
        double key = linked ? effReal : v;
        if (key != lastKey_)
        {
            lastKey_ = key;
            juce::String t = getTextFromValue(v);
            if (t != lastShown_) { lastShown_ = t; updateText(); }
        }
    }
    // Para dials sem link (globais): efetivo = próprio.
    void syncEffToOwn()
    {
        double v = getValue();
        getProperties().set("geffReal", v);
        refreshTextIfNeeded();
    }
    // O SliderAttachment impõe double-click normalizado; repõe o real.
    void fixDoubleClick() { setDoubleClickReturnValue(true, defV); }

    double rangeLo() const { return lo_; }
    double rangeHi() const { return hi_; }

    // own01/ghost/eff em 0..1; só repinta se mudar (o Timer corre a 30 Hz).
    void setNeedles(float own01, float ghost, float eff, bool linked)
    {
        auto& pr = getProperties(); // referência! (cópia deitava tudo fora)
        bool ch = (float)pr["ghost"] != ghost || (float)pr["eff"] != eff
               || (bool)pr["linked"] != linked;
        pr.set("ghost", (double)ghost);
        pr.set("eff", (double)eff);
        pr.set("linked", linked);
        juce::ignoreUnused(own01);
        if (ch) repaint();
    }

    double defV = 0.0;
    juce::Colour accentCol;
    juce::String lastShown_ { "@" };
    double lastKey_ = std::numeric_limits<double>::quiet_NaN();

    static juce::String formatValue(const juce::String& fmt, double v)
    {
        if (fmt == "n2") return juce::String(v, 2);
        if (fmt == "n1") return juce::String(v, 1);
        if (fmt == "int") return juce::String(juce::roundToInt(v));
        if (fmt == "ms") return juce::String(juce::roundToInt(v)) + " ms";
        if (fmt == "ms1") return juce::String(v, 1) + " ms";
        if (fmt == "hz")
            return v >= 1000 ? juce::String(v / 1000.0, 1) + " kHz"
                             : juce::String(juce::roundToInt(v)) + " Hz";
        if (fmt == "hz1") return juce::String(v, 1) + " Hz";
        if (fmt == "s") return juce::String(v, 1) + " s";
        if (fmt == "db") return juce::String(v, 1) + " dB";
        if (fmt == "st")
            return (v >= 0 ? juce::String("+") : juce::String()) + juce::String(v, 1) + " st";
        if (fmt == "x") return juce::String(v, 2) + "x";
        if (fmt == "pct") return juce::String(juce::roundToInt(v * 100)) + "%";
        return juce::String(v, 2);
    }

    // Overrides diretos (o Slider consulta-os sempre). Com link mostra o
    // EFETIVO (geffReal); sem link, o valor vivo (sem lag de 33 ms).
    juce::String getTextFromValue(double v) override
    {
        bool linked = getProperties().contains("linked")
                   && (bool) getProperties()["linked"];
        if (linked)
        {
            double effReal = getProperties().contains("geffReal")
                ? (double) getProperties()["geffReal"] : v;
            return formatValue(fmt_, effReal);
        }
        return formatValue(fmt_, v);
    }
    double getValueFromText(const juce::String& text) override
    {
        juce::String s = text.trim().toLowerCase();
        double mult = 1.0;
        if (s.endsWith("khz")) { mult = 1000.0; s = s.upToLastOccurrenceOf("khz", false, true); }
        else if (s.endsWith("hz")) s = s.upToLastOccurrenceOf("hz", false, true);
        else if (s.endsWith("ms")) s = s.upToLastOccurrenceOf("ms", false, true);
        else if (s.endsWith("db")) s = s.upToLastOccurrenceOf("db", false, true);
        else if (s.endsWith("st")) s = s.upToLastOccurrenceOf("st", false, true);
        else if (s.endsWith("x")) s = s.upToLastOccurrenceOf("x", false, true);
        else if (s.endsWith("%")) s = s.upToLastOccurrenceOf("%", false, true);
        else if (s.endsWith("s")) s = s.upToLastOccurrenceOf("s", false, true);
        double v = s.trim().getDoubleValue() * mult;
        return juce::jlimit(lo_, hi_, v);
    }

private:
    void updateTextFns()
    {
        // Mantém as lambdas sincronizadas com os overrides.
        textFromValueFunction = [this](double v) { return getTextFromValue(v); };
        valueFromTextFunction = [this](const juce::String& t) { return getValueFromText(t); };
    }

    double lo_ = 0.0, hi_ = 1.0;
    juce::String fmt_ = "n2";
};
