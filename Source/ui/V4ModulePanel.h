#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "V4Dial.h"
#include "V4Seg.h"
#include "V4Stepper.h"
#include "V4Key.h"
#include "V4Viz.h"

class DeVerbProcessor;

// Painel de módulo v4: uma instância por (engine, module), 8x no total.
// Attachments criados UMA vez no ctor (elimina bind*Column + cross-talk).
// Coordenadas locais (origem = canto da área de módulo); o editor posiciona
// o painel em (360,64) FWD ou (360,406) REV. Só o módulo selecionado visível.
class V4ModulePanel : public juce::Component
{
public:
    V4ModulePanel(DeVerbProcessor& proc, const juce::String& engine, const juce::String& mod);
    // Os SliderAttachment têm de morrer ANTES dos Sliders (owned_ apaga os
    // componentes primeiro); senão removeListener() corre num slider morto
    // (EXC_BAD_ACCESS no mac, heap luck no Linux).
    ~V4ModulePanel() override;

    void resized() override {} // filhos com bounds absolutos locais
    void updateLinkState();    // tether + agulhas (chamado no Timer)
    void flipStep(int step);

    juce::String engine_, mod_;
    // Para o harness de testes: acesso aos widgets por paramID
    juce::Slider* findDial(const juce::String& paramId);
    V4Seg* findSeg(const juce::String& paramId);
    V4Stepper* findStepper(const juce::String& paramId);
    V4Key* findKey(const juce::String& paramId);
    V4Viz* findViz(V4Viz::Kind k);

    std::function<void(const juce::String& mod, juce::Component* anchor)> onTetherClick;

private:
    DeVerbProcessor& proc_;
    juce::String pid(const juce::String& base) const;
    juce::RangedAudioParameter* par(const juce::String& id) const;

    struct DialEntry { V4Dial* dial = nullptr; juce::String paramId; juce::String base; std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att; float lo=0, hi=1; };
    struct SegEntry { V4Seg* seg = nullptr; juce::String paramId; };
    struct StepEntry { V4Stepper* st = nullptr; juce::String paramId; };
    struct KeyEntry { V4Key* key = nullptr; juce::String paramId; };
    struct StepBtn { juce::TextButton* b = nullptr; int idx = 0; };

    std::vector<DialEntry> dials_;
    std::vector<SegEntry> segs_;
    std::vector<StepEntry> steps_;
    std::vector<KeyEntry> keys_;
    std::vector<StepBtn> stepBtns_;
    std::vector<V4Viz*> vizs_;
    juce::OwnedArray<juce::Component> owned_;

    V4Dial* addDial(const juce::String& base, const juce::String& label,
                    int x, int y, const juce::Colour& acc);
    V4Seg* addSeg(const juce::String& base, const juce::StringArray& labels,
                  const juce::String& cap, int x, int y, int w);
    V4Stepper* addStepper(const juce::String& base, const juce::StringArray& opts,
                          const juce::String& cap, int x, int y, int w,
                          const std::vector<int>& bits = {});
    V4Key* addKey(const juce::String& base, const juce::String& text,
                  int x, int y, int w, int h, V4Key::Kind k,
                  const juce::Colour& acc);
    void addViz(V4Viz::Kind k, int x, int y, int w, int h, const juce::String& txt = {});
};
