#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

// Tecla v4 (.key): toggle ligado a bool via ParameterAttachment.
// Variantes: normal, pwr-fwd (seta + lampada), pwr-rev, link (aneis),
// link-all, throw. Tether: tracejado + 50%, clique pede UNLINK.
class V4Key : public juce::Component
{
public:
    enum Kind { Normal, PwrFwd, PwrRev, Link, LinkAll, Throw };

    V4Key(juce::RangedAudioParameter* param, const juce::String& text,
          Kind k = Normal, const juce::Colour& accent = juce::Colour(0xff16171A))
        : param_(param), text_(text), kind_(k), accent_(accent)
    {
        if (param_ != nullptr)
        {
            attach = std::make_unique<juce::ParameterAttachment>(
                *param_, [this](float f) { on_ = f > 0.5f; repaint(); }, nullptr);
            attach->sendInitialUpdate();
        }
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
    }

    void setTethered(bool t) { tethered_ = t; repaint(); }
    void setAccent(const juce::Colour& a) { accent_ = a; repaint(); }
    bool getState() const { return on_; }
    void setState(bool s, bool notify = true)
    {
        if (param_ != nullptr && notify && attach)
            attach->setValueAsCompleteGesture(s ? 1.f : 0.f);
        else { on_ = s; repaint(); }
    }

    std::function<void(V4Key*)> onTetherClick;
    std::function<void(V4Key*)> onClickExtra; // throw ripples, random, etc.

    void mouseUp(const juce::MouseEvent&) override
    {
        if (tethered_) { if (onTetherClick) onTetherClick(this); return; }
        if (kind_ == Throw)
        {
            // momentâneo: pulso 1 depois 0 (para o flanco do DSP)
            if (param_ != nullptr && attach)
            {
                attach->setValueAsCompleteGesture(1.f);
                attach->setValueAsCompleteGesture(0.f);
            }
            if (onClickExtra) onClickExtra(this);
            repaint();
            return;
        }
        setState(!on_, true);
        if (onClickExtra) onClickExtra(this);
    }

    void paint(juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced(0.5f);
        bool on = on_;
        // fundo
        if (kind_ == Link || kind_ == Throw)
        {
            // corte tinta/papel pela linha de agua (aprox: metade/metade)
            g.setColour(juce::Colour(0xffECE9E2));
            g.fillRoundedRectangle(juce::Rectangle<float>(r.getX(), r.getY(), r.getWidth(), r.getHeight()/2), 3.f);
            g.setColour(juce::Colour(0xff16171A));
            g.fillRoundedRectangle(juce::Rectangle<float>(r.getX(), r.getY()+r.getHeight()/2, r.getWidth(), r.getHeight()/2), 3.f);
        }
        else if (on)
        {
            g.setColour(accent_);
            g.fillRoundedRectangle(r, 3.f);
        }
        // contorno
        g.setColour(juce::Colour(0xff8D8A82));
        if (tethered_)
        {
            float d[2] = { 4.f, 3.f };
            g.drawDashedLine(juce::Line<float>(r.getX(), r.getY(), r.getRight(), r.getY()), d, 2);
            g.drawDashedLine(juce::Line<float>(r.getX(), r.getBottom(), r.getRight(), r.getBottom()), d, 2);
        }
        else g.drawRoundedRectangle(r, 3.f, 1.f);

        setAlpha(tethered_ ? 0.55f : 1.f);
        juce::Colour fg = on ? (kind_==Normal||kind_==Throw||kind_==Link ? juce::Colour(0xff16171A) : juce::Colours::black) : juce::Colour(0xff16171A);
        if (kind_ == Link || kind_ == Throw) fg = juce::Colours::white; // difference aprox: usa branco com blend? mantém legível
        if (kind_ == Link || kind_ == Throw)
        {
            // texto com mix: desenha 2x (tinta sobre papel e vice-versa) — simplificado: contorno escuro
            g.setColour(juce::Colour(0xff16171A));
            g.setFont(10.f);
        }

        if (kind_ == PwrFwd || kind_ == PwrRev)
        {
            // seta + lâmpada
            float cx = r.getCentreX(), cy = r.getCentreY();
            juce::Path arr;
            if (kind_ == PwrFwd) arr.addTriangle(cx-8, cy-4, cx-8, cy+4, cx-2, cy);
            else arr.addTriangle(cx+8, cy-4, cx+8, cy+4, cx+2, cy);
            g.setColour(fg);
            g.fillPath(arr);
            g.setColour(on ? accent_ : juce::Colour(0xff55544F));
            if (on) g.fillEllipse(cx+2, cy-4, 8, 8);
            else g.drawEllipse(cx+2, cy-4, 8, 8, 1.5f);
        }
        else if (kind_ == Link)
        {
            // dois anéis entrelaçados
            auto c = r.getCentre();
            g.setColour(on ? accent_ : juce::Colours::white);
            float lw = on ? 2.f : 1.5f;
            g.drawEllipse(c.x-11, c.y-5, 10, 10, lw);
            g.drawEllipse(c.x + (on ? -4 : 1), c.y-5, 10, 10, lw);
            if (!text_.isEmpty() && text_ != " ") {}
        }
        else
        {
            g.setColour(on ? juce::Colours::black : juce::Colour(0xff16171A));
            g.setFont(10.f);
            g.drawFittedText(text_, getLocalBounds(), juce::Justification::centred, 1);
        }
    }

private:
    juce::RangedAudioParameter* param_ = nullptr;
    juce::String text_;
    Kind kind_;
    juce::Colour accent_;
    std::unique_ptr<juce::ParameterAttachment> attach;
    bool on_ = false;
    bool tethered_ = false;
};
