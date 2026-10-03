#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

// Stepper v4 (.stepper): prev valor next + roda. Serve notas (12),
// presets de pattern (6 nomes -> bits), chain_order (4), global preset
// (role factory-preset, sem param: escreve N params).
// Para pattern int (0..65535): options = nomes de fábrica, value = bits.
class V4Stepper : public juce::Component,
                 public juce::SettableTooltipClient
{
public:
    // Param choice/int ou nullptr (só visual com role). patternBits: se true,
    // mapeia options[i] -> bits[i] no param int.
    V4Stepper(juce::RangedAudioParameter* param,
              const juce::StringArray& options,
              const juce::String& cap = {},
              const std::vector<int>& bits = {})
        : param_(param), options_(options), cap_(cap), bits_(bits)
    {
        prev_.onClick = [this] { step(-1); };
        next_.onClick = [this] { step(+1); };
        prev_.getProperties().set("nobg", true);
        next_.getProperties().set("nobg", true);
        addAndMakeVisible(prev_);
        addAndMakeVisible(next_);
        if (param_ != nullptr)
        {
            attach = std::make_unique<juce::ParameterAttachment>(
                *param_, [this](float) { updateFromParam(); }, nullptr);
            attach->sendInitialUpdate();
        }
        setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
    }

    std::function<void(V4Stepper*)> onTetherClick;
    // Preset global (sem param): mostra o último aplicado.
    void setExternalIndex(int i)
    {
        idx_ = juce::jlimit(0, options_.size() - 1, i);
        repaint();
    }
    void setTethered(bool t, int fwdIdx = -1)
    {
        if (t == tethered_ && fwdIdx == fwdIdx_) return;
        tethered_ = t; fwdIdx_ = fwdIdx; repaint();
    }
    void setDark(bool d) { dark_ = d; repaint(); }
    int getIndex() const { return idx_; }
    void setRole(const juce::String& r) { role_ = r; }

    void step(int d)
    {
        if (tethered_) { if (onTetherClick) onTetherClick(this); return; }
        int ni = (idx_ + d + options_.size() * 4) % juce::jmax(1, options_.size());
        applyIndex(ni);
    }

    void applyIndex(int ni)
    {
        ni = juce::jlimit(0, options_.size() - 1, ni);
        if (param_ == nullptr) { idx_ = ni; repaint(); if (onCustomPick) onCustomPick(ni); return; }
        if (!bits_.empty())
        {
            int b = bits_[(size_t)ni];
            attach->setValueAsCompleteGesture(param_->convertTo0to1((float)b));
        }
        else if (auto* c = dynamic_cast<juce::AudioParameterChoice*>(param_))
            attach->setValueAsCompleteGesture((float)ni);
        else if (auto* ip = dynamic_cast<juce::AudioParameterInt*>(param_))
            attach->setValueAsCompleteGesture(param_->convertTo0to1((float)ni));
        else
            attach->setValueAsCompleteGesture(param_->convertTo0to1((float)ni));
    }

    std::function<void(int)> onCustomPick; // para factory-preset (sem param)

    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& w) override
    {
        if (std::abs(w.deltaY) > 0.001f) step(w.deltaY > 0 ? -1 : +1);
    }

    void resized() override
    {
        auto r = getLocalBounds();
        prev_.setBounds(0, 0, 28, r.getHeight());
        next_.setBounds(r.getWidth() - 28, 0, 28, r.getHeight());
    }

    void paint(juce::Graphics& g) override
    {
        auto fg = dark_ ? juce::Colour(0xffECE9E2) : juce::Colour(0xff16171A);
        auto lab = dark_ ? juce::Colour(0xff9A9892) : juce::Colour(0xff55544F);
        auto r = getLocalBounds().toFloat();
        g.setColour(juce::Colour(0x00000000));
        g.fillRoundedRectangle(r, 3.f);
        g.setColour(juce::Colour(0xff8D8A82).withAlpha(0.6f));
        if (tethered_)
        {
            float d[2] = { 4.f, 3.f };
            g.drawDashedLine(juce::Line<float>(r.getX(), r.getY(), r.getRight(), r.getY()), d, 2);
            g.drawDashedLine(juce::Line<float>(r.getX(), r.getBottom(), r.getRight(), r.getBottom()), d, 2);
        }
        else g.drawRoundedRectangle(r, 3.f, 1.f);
        setAlpha(tethered_ ? 0.62f : 1.f);
        int shown = (tethered_ && fwdIdx_ >= 0) ? fwdIdx_ : idx_;
        juce::String txt = options_[juce::jlimit(0, options_.size()-1, shown)];
        g.setColour(fg);
        g.setFont(11.f);
        if (cap_.isNotEmpty())
        {
            g.setFont(10.f);
            g.setColour(lab);
            g.drawFittedText(cap_, juce::Rectangle<int>(28, 0, 52, r.getHeight()).toNearestInt(),
                             juce::Justification::centredLeft, 1);
            g.setColour(fg);
            g.setFont(juce::Font(juce::FontOptions(11.f, juce::Font::plain)));
            g.drawFittedText(txt, juce::Rectangle<int>(80, 0, r.getWidth()-108, r.getHeight()).toNearestInt(),
                             juce::Justification::centred, 1);
        }
        else
        {
            g.drawFittedText(txt, juce::Rectangle<int>(28, 0, r.getWidth()-56, r.getHeight()).toNearestInt(),
                             juce::Justification::centred, 1);
        }
        // setas
        auto arrow = [&](juce::Rectangle<int> br, bool left)
        {
            juce::Path p;
            auto c = br.toFloat().getCentre();
            if (left) p.addTriangle(c.x+4, c.y-5, c.x+4, c.y+5, c.x-4, c.y);
            else p.addTriangle(c.x-4, c.y-5, c.x-4, c.y+5, c.x+4, c.y);
            g.setColour(fg);
            g.fillPath(p);
        };
        arrow(prev_.getBounds(), true);
        arrow(next_.getBounds(), false);
    }

private:
    void updateFromParam()
    {
        if (param_ == nullptr) return;
        int ni = 0;
        if (!bits_.empty())
        {
            int b = (int)std::round(param_->convertFrom0to1(param_->getValue()));
            ni = 0;
            for (size_t i = 0; i < bits_.size(); ++i)
                if (bits_[i] == b) { ni = (int)i; break; }
        }
        else if (auto* c = dynamic_cast<juce::AudioParameterChoice*>(param_))
            ni = c->getIndex();
        else
            ni = (int)std::round(param_->convertFrom0to1(param_->getValue()));
        idx_ = juce::jlimit(0, options_.size()-1, ni);
        repaint();
    }

    juce::RangedAudioParameter* param_ = nullptr;
    juce::StringArray options_;
    juce::String cap_, role_;
    std::vector<int> bits_;
    juce::TextButton prev_ { "" }, next_ { "" };
    std::unique_ptr<juce::ParameterAttachment> attach;
    bool dark_ = false;
    int idx_ = 0;
    bool tethered_ = false;
    int fwdIdx_ = -1;
};
