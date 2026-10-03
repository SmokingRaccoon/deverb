#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

// Segmentado v4 (.seg): todas as opções visíveis (<=6), opção ativa = fundo
// fg + sublinhado de acento. Liga a um AudioParameterChoice via
// juce::ParameterAttachment (sem ComboBox). Suporta tether: com link ligado,
// o REV mostra o valor FWD, fica tracejado + 50% e o clique abre CallOutBox
// em vez de escrever (ver DESIGN-V4 §4 Tether).
class V4Seg : public juce::Component,
                 public juce::SettableTooltipClient
{
public:
    V4Seg(juce::RangedAudioParameter* param,
          const juce::StringArray& labels,
          const juce::String& cap = {},
          const juce::Colour& accent = juce::Colour(0xff16171A))
        : param_(param), labels_(labels), cap_(cap), accent_(accent)
    {
        for (int i = 0; i < labels_.size(); ++i)
        {
            auto* b = new juce::TextButton(labels_[i]);
            b->setClickingTogglesState(false);
            b->getProperties().set("segbtn", true);
            b->onClick = [this, i] { userPick(i); };
            addAndMakeVisible(b);
            btns.add(b);
        }
        if (param_ != nullptr)
        {
            attach = std::make_unique<juce::ParameterAttachment>(
                *param_, [this](float) { updateFromParam(); }, nullptr);
            attach->sendInitialUpdate();
        }
        else
        {
            idx_ = 0;
        }
    }

    void setAccent(const juce::Colour& a) { accent_ = a; repaint(); }
    void setDark(bool d) { dark_ = d; repaint(); }
    void setTethered(bool t, int fwdIdx = -1)
    {
        if (t == tethered_ && fwdIdx == fwdIdx_) return;
        tethered_ = t;
        fwdIdx_ = fwdIdx;
        repaint();
    }
    bool isTethered() const { return tethered_; }
    int getIndex() const { return idx_; }
    void setIndex(int i, bool notify = true)
    {
        i = juce::jlimit(0, labels_.size() - 1, i);
        if (param_ != nullptr && notify)
        {
            if (auto* c = dynamic_cast<juce::AudioParameterChoice*>(param_))
                attach->setValueAsCompleteGesture((float)i);
            else
                attach->setValueAsCompleteGesture(param_->convertTo0to1((float)i));
        }
        else
        {
            idx_ = i;
            repaint();
        }
    }

    // Chamado pelo editor quando o tether é clicado: abre o toast UNLINK.
    std::function<void(V4Seg*)> onTetherClick;

    int capWidth() const
    {
        if (cap_.isEmpty()) return 0;
        return juce::jmax(40, (int)(cap_.length() * 7.75 + 19));
    }

    void resized() override
    {
        auto r = getLocalBounds();
        int capW = capWidth();
        int bx = capW;
        int bw = juce::jmax(1, (r.getWidth() - capW) / juce::jmax(1, btns.size()));
        for (auto* b : btns)
        {
            b->setBounds(bx, 0, bw, r.getHeight());
            bx += bw;
        }
    }

    void paint(juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour(juce::Colour(0x00000000));
        g.fillRoundedRectangle(r, 3.f);
        g.setColour(juce::Colour(0xff8D8A82).withAlpha(0.6f));
        if (tethered_)
        {
            float d[2] = { 4.f, 3.f };
            // contorno tracejado: 4 linhas
            g.drawDashedLine(juce::Line<float>(r.getX(), r.getY(), r.getRight(), r.getY()), d, 2);
            g.drawDashedLine(juce::Line<float>(r.getX(), r.getBottom(), r.getRight(), r.getBottom()), d, 2);
        }
        else
        {
            g.drawRoundedRectangle(r, 3.f, 1.f);
        }
        auto fg = dark_ ? juce::Colour(0xffECE9E2) : juce::Colour(0xff16171A);
        auto bg = dark_ ? juce::Colour(0xff16171A) : juce::Colour(0xffECE9E2);
        auto lab = dark_ ? juce::Colour(0xff9A9892) : juce::Colour(0xff55544F);
        juce::ignoreUnused(bg);
        if (cap_.isNotEmpty())
        {
            g.setColour(lab);
            g.setFont(juce::Font(juce::FontOptions(10.f, juce::Font::bold)));
            g.drawFittedText(cap_, juce::Rectangle<int>(0, 0, capWidth(), r.getHeight()).toNearestInt(),
                             juce::Justification::centred, 1);
        }
        if (tethered_)
        {
            // overlay 50% — pintado por cima dos botões via alpha do comp? usa dim.
            g.setColour(juce::Colours::white.withAlpha(0.0f));
        }
        setAlpha(tethered_ ? 0.62f : 1.f);
        // realce da opção ativa: fundo fg + sublinhado acento (nos botões)
        for (int i = 0; i < btns.size(); ++i)
        {
            auto* b = btns[i];
            int shown = (tethered_ && fwdIdx_ >= 0) ? fwdIdx_ : idx_;
            bool on = (i == shown);
            b->setColour(juce::TextButton::buttonColourId,
                         on ? fg : juce::Colour(0x00000000));
            b->setColour(juce::TextButton::textColourOffId, fg);
            b->setColour(juce::TextButton::textColourOnId, dark_ ? juce::Colour(0xff16171A) : juce::Colour(0xffECE9E2));
            b->setToggleState(on, juce::dontSendNotification);
            // sublinhado de acento: via propriedade lida no LnF? pinta aqui:
            if (on)
            {
                auto br = b->getBounds().toFloat().reduced(8.f, 0.f);
                br = br.withTrimmedTop(br.getHeight() - 5.f).withHeight(2.f);
                // converte para coords locais deste comp:
                br += juce::Point<float>((float)b->getX(), (float)b->getY());
                // NOTE: b->getX/Y já em coords do parent; br local = b bounds + offset
                auto lr = juce::Rectangle<float>((float)(b->getX() + 8), (float)(getHeight() - 5), (float)(b->getWidth() - 16), 2.f);
                g.setColour(accent_);
                g.fillRect(lr);
            }
        }
    }

private:
    void userPick(int i)
    {
        if (tethered_)
        {
            if (onTetherClick) onTetherClick(this);
            return;
        }
        setIndex(i, true);
    }
    void updateFromParam()
    {
        if (param_ == nullptr) return;
        int ni = 0;
        if (auto* c = dynamic_cast<juce::AudioParameterChoice*>(param_))
            ni = c->getIndex();
        else
            ni = (int)std::round(param_->convertFrom0to1(param_->getValue()));
        idx_ = juce::jlimit(0, labels_.size() - 1, ni);
        // atualiza botões na message thread (este callback já é na message thread)
        repaint();
    }

    juce::RangedAudioParameter* param_ = nullptr;
    juce::StringArray labels_;
    juce::String cap_;
    juce::Colour accent_;
    juce::OwnedArray<juce::TextButton> btns;
    std::unique_ptr<juce::ParameterAttachment> attach;
    bool dark_ = false;
    int idx_ = 0;
    bool tethered_ = false;
    int fwdIdx_ = -1;
};
