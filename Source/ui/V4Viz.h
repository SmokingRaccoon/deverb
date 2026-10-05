#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

class DeVerbProcessor;

// Visuais v4 (.viz.*): só leitura. Alguns precisam de dados do DSP
// (scope FIFO, capture-window) — o editor chama update() no Timer e repaint.
// Os desenhados a partir de params (taps/decay/shards) leem APVTS direto.
class V4Viz : public juce::Component,
                  public juce::SettableTooltipClient
{
public:
    enum Kind { Title, EngineFwd, EngineRev, Scope, CaptureWin, Ruler, GateBig,
                DelayMs, Taps, VerbT60, Decay, IR, GranLed, Shards, MorphRead,
                BpmSrc, ModTitle };

    V4Viz(Kind k, DeVerbProcessor* proc = nullptr,
          const juce::String& mod = {}, const juce::String& txt = {});
    void setText(const juce::String& t) { if (t != text_) { text_ = t; repaint(); } }
    juce::String getText() const { return text_; }
    bool isHot() const { return hot_; }
    void setDark(bool d) { if (d != dark_) { dark_ = d; repaint(); } }
    void setHot(bool h) { if (h != hot_) { hot_ = h; repaint(); } }
    void setAux(int a, int b, int c)
    {
        if (a != auxA_ || b != auxB_ || c != auxC_) { auxA_ = a; auxB_ = b; auxC_ = c; repaint(); }
    }
    void paint(juce::Graphics& g) override;

    Kind kind;
    DeVerbProcessor* proc = nullptr;
    juce::String mod_, text_;
    bool dark_ = false, hot_ = false;
    int auxA_ = 16, auxB_ = -1, auxC_ = 0;
};
