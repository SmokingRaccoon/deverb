#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

class DeVerbProcessor;

// Visuais v4 (.viz.*): só leitura. Alguns precisam de dados do DSP
// (scope FIFO, capture-window) — o editor chama update() no Timer e repaint.
// Os desenhados a partir de params (taps/decay/shards) leem APVTS direto.
class V4Viz : public juce::Component
{
public:
    enum Kind { Title, EngineFwd, EngineRev, Scope, CaptureWin, Ruler, GateBig,
                DelayMs, Taps, VerbT60, Decay, IR, GranLed, Shards, MorphRead,
                BpmSrc, ModTitle };

    V4Viz(Kind k, DeVerbProcessor* proc = nullptr,
          const juce::String& mod = {}, const juce::String& txt = {});
    void setText(const juce::String& t) { text_ = t; repaint(); }
    void paint(juce::Graphics& g) override;

    Kind kind;
    DeVerbProcessor* proc = nullptr;
    juce::String mod_, text_;
};
