#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

// Numbox v4 (.numbox): número arrastável (BPM). Só tem efeito sem host
// (o editor mostra HOST/INT ao lado). Duplo clique = default.
class V4Num : public juce::Component
{
public:
    V4Num(juce::RangedAudioParameter* param, const juce::String& cap, double defV)
        : param_(param), cap_(cap), defV_(defV), val_(defV)
    {
        if (param_ != nullptr)
        {
            attach = std::make_unique<juce::ParameterAttachment>(
                *param_, [this](float f) { val_ = f; repaint(); }, nullptr);
            attach->sendInitialUpdate();
        }
        setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        dragY_ = e.position.y; dragV_ = val_;
        if (attach) attach->beginGesture(); // equilibra o endGesture do mouseUp
    }
    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (param_ == nullptr) return;
        float range = 200.f; // 40..240 em ~200px
        float dv = (dragY_ - e.position.y) / range * 200.f;
        float nv = juce::jlimit(40.f, 240.f, dragV_ + dv);
        attach->setValueAsPartOfGesture(nv);
    }
    void mouseUp(const juce::MouseEvent&) override
    {
        if (attach) attach->endGesture();
    }
    void mouseDoubleClick(const juce::MouseEvent&) override
    {
        if (attach) attach->setValueAsCompleteGesture((float)defV_);
    }
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& w) override
    {
        if (param_ == nullptr || attach == nullptr) return;
        float nv = juce::jlimit(40.f, 240.f, val_ + (w.deltaY > 0 ? 1.f : -1.f));
        attach->setValueAsCompleteGesture(nv);
    }

    void paint(juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour(juce::Colour(0x00000000));
        g.fillRoundedRectangle(r, 3.f);
        g.setColour(juce::Colour(0xff8D8A82).withAlpha(0.6f));
        g.drawRoundedRectangle(r, 3.f, 1.f);
        g.setColour(juce::Colour(0xff55544F));
        g.setFont(juce::Font(juce::FontOptions(10.f, juce::Font::bold)));
        g.drawFittedText(cap_, juce::Rectangle<int>(0, 0, 52, (int)r.getHeight()),
                         juce::Justification::centred, 1);
        g.drawLine(52.f, 4.f, 52.f, r.getHeight() - 4.f);
        g.setColour(juce::Colour(0xff16171A));
        g.setFont(juce::Font(juce::FontOptions("DejaVu Sans Mono", 13.f, juce::Font::plain)));
        g.drawFittedText(juce::String(val_, 1), juce::Rectangle<int>(52, 0, (int)r.getWidth()-52, (int)r.getHeight()),
                         juce::Justification::centred, 1);
    }

private:
    juce::RangedAudioParameter* param_ = nullptr;
    juce::String cap_;
    double defV_ = 120.0;
    float val_ = 120.f;
    float dragY_ = 0.f, dragV_ = 120.f;
    std::unique_ptr<juce::ParameterAttachment> attach;
};
