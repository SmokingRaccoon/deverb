#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

// Tecla v4 (.key): toggle ligado a bool via ParameterAttachment.
// Variantes: normal, pwr-fwd (seta + lampada), pwr-rev, link (aneis),
// link-all, throw. Tether: tracejado + 50%, clique pede UNLINK.
class V4Key : public juce::Component,
                 public juce::SettableTooltipClient
{
public:
    // Action = tecla de ação momentânea (RANDOM): sem estado, sem fill.
    enum Kind { Normal, PwrFwd, PwrRev, Link, LinkAll, Throw, Action };

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
        setWantsKeyboardFocus(true);
    }

    bool keyPressed(const juce::KeyPress& k) override
    {
        // Só Enter ativa: o Espaço passa ao host (play/stop) em vez de ser
        // comido pela tecla focada (THROW/RANDOM disparavam sem querer).
        if (k.isKeyCode(juce::KeyPress::returnKey))
        {
            press();
            return true;
        }
        return false;
    }

    void setTethered(bool t) { if (t != tethered_) { tethered_ = t; repaint(); } }
    void setAccent(const juce::Colour& a) { accent_ = a; repaint(); }
    void setDark(bool d) { dark_ = d; repaint(); }
    bool getState() const { return on_; }
    void setState(bool s, bool notify = true)
    {
        if (param_ != nullptr && notify && attach)
            attach->setValueAsCompleteGesture(s ? 1.f : 0.f);
        else { on_ = s; repaint(); }
    }

    std::function<void(V4Key*)> onTetherClick;
    std::function<void(V4Key*)> onClickExtra; // throw ripples, random, etc.

    void mouseUp(const juce::MouseEvent&) override { press(); }

    void press()
    {
        if (tethered_) { if (onTetherClick) onTetherClick(this); return; }
        if (kind_ == Action)
        {
            if (onClickExtra) onClickExtra(this);
            repaint();
            return;
        }
        if (kind_ == Throw)
        {
            // momentâneo com HOLD de 60 ms: o 1→0 imediato morria antes do
            // próximo bloco de áudio e o flanco do DSP nunca via o botão.
            if (param_ != nullptr && attach)
            {
                attach->setValueAsCompleteGesture(1.f);
                juce::Component::SafePointer<V4Key> safe(this);
                juce::Timer::callAfterDelay(60, [safe] {
                    if (safe != nullptr && safe->attach)
                        safe->attach->setValueAsCompleteGesture(0.f);
                });
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
        else if (on && (kind_ == Normal || kind_ == LinkAll))
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
        // fg por polaridade (teclas REV e LinkAll vivem sobre tinta)
        juce::Colour fg = dark_ ? juce::Colour(0xffECE9E2) : juce::Colour(0xff16171A);
        juce::Colour dim = dark_ ? juce::Colour(0xff9A9892) : juce::Colour(0xff55544F);

        // desenha o miolo (sem texto) com uma cor dada — usado nos 2 passes
        // das teclas cortadas pela linha de água (THROW, LINK).
        auto drawCore = [&](juce::Colour c)
        {
            if (kind_ == PwrFwd || kind_ == PwrRev)
            {
                // seta + lâmpada (sem fill de fundo: discreto como no mockup)
                float cx = r.getCentreX(), cy = r.getCentreY();
                juce::Path arr;
                if (kind_ == PwrFwd) arr.addTriangle(cx-8, cy-4, cx-8, cy+4, cx-2, cy);
                else arr.addTriangle(cx+8, cy-4, cx+8, cy+4, cx+2, cy);
                g.setColour(c);
                g.fillPath(arr);
                g.setColour(on ? accent_ : dim);
                if (on) g.fillEllipse(cx+2, cy-4, 8, 8);
                else g.drawEllipse(cx+2, cy-4, 8, 8, 1.5f);
            }
            else if (kind_ == Link)
            {
                // dois anéis: separados = OFF, entrelaçados no acento = ON
                auto c0 = r.getCentre();
                g.setColour(on ? accent_ : c);
                float lw = on ? 2.f : 1.5f;
                g.drawEllipse(c0.x-11, c0.y-5, 10, 10, lw);
                g.drawEllipse(c0.x + (on ? -4 : 1), c0.y-5, 10, 10, lw);
            }
            else
            {
                // ON com fill de acento: texto com contraste (papel no índigo).
                juce::Colour tc = c;
                if (on && (kind_ == Normal || kind_ == LinkAll))
                    tc = accent_.getPerceivedBrightness() < 0.4f
                       ? juce::Colour(0xffECE9E2) : juce::Colour(0xff16171A);
                g.setColour(tc);
                g.setFont(juce::Font(juce::FontOptions(10.f, juce::Font::bold)));
                g.drawFittedText(text_, getLocalBounds(), juce::Justification::centred, 1);
            }
        };

        if (kind_ == Link || kind_ == Throw)
        {
            // 2 passes com clip (difference do mockup): tinta em cima, papel
            // em baixo — legível nas duas metades.
            auto bounds = getLocalBounds();
            g.saveState();
            g.reduceClipRegion(0, 0, bounds.getWidth(), bounds.getHeight()/2);
            drawCore(juce::Colour(0xff16171A));
            g.restoreState();
            g.saveState();
            g.reduceClipRegion(0, bounds.getHeight()/2, bounds.getWidth(),
                               bounds.getHeight() - bounds.getHeight()/2);
            drawCore(juce::Colour(0xffECE9E2));
            g.restoreState();
        }
        else
        {
            drawCore(fg);
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
    bool dark_ = false;
};
