#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "ui/WaterLnF.h"
#include "ui/V4Dial.h"
#include "ui/V4Seg.h"
#include "ui/V4Stepper.h"
#include "ui/V4Num.h"
#include "ui/V4Key.h"
#include "ui/V4Viz.h"
#include "ui/V4ModulePanel.h"
#include "ui/V4Tile.h"

class DeVerbProcessor;

// UI v4 "Espelho de Agua" (DESIGN-V4.md, LAYOUT-V4.md).
// Janela fixa 1280x624. FWD em cima (papel), REV em baixo (tinta),
// meridiano com a relação (LINK/MORPH/TRIM). 8 ModulePanels (4 mods x
// FWD/REV) com attachments criados UMA vez no ctor — sem bind*Column.
// selectedModule vive em apvts.state > v4ui > selMod (fora dos 122 IDs).
class DeVerbEditor : public juce::AudioProcessorEditor,
                     private juce::Timer,
                     private juce::ValueTree::Listener
{
    friend struct UiSessionDriver;
public:
    explicit DeVerbEditor(DeVerbProcessor&);
    ~DeVerbEditor() override;
    void paint(juce::Graphics& g) override;
    void resized() override;

    juce::String getSelMod() const { return selMod_; }
    void setSelMod(const juce::String& m, bool animate = true);

    // Para testes: acesso por paramID através dos painéis + globais
    juce::Slider* findDial(const juce::String& paramId);
    V4Seg* findSeg(const juce::String& paramId);
    V4Stepper* findStepper(const juce::String& paramId);
    V4Key* findKey(const juce::String& paramId);

private:
    void timerCallback() override;
    // Segue selMod vindo de fora (restore de estado): o editor mostra o
    // módulo guardado em vez de ficar dessincronizado. O APVTS troca o
    // objeto (`state = newState`, dispara redirected e não property).
    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& prop) override;
    void valueTreeRedirected(juce::ValueTree&) override;
    void showTetherToast(const juce::String& mod, juce::Component* anchor);
    void hideToast();
    void applyOrder(int idx);
    void randomize();

    DeVerbProcessor& proc;
    WaterLnF lnf;
    juce::String selMod_ { "gate" };

    // 8 painéis (o módulo selecionado visível em FWD+REV)
    std::unique_ptr<V4ModulePanel> panels[2][4]; // [fwd=0/rev=1][gate/delay/verb/gran]
    int modIndex(const juce::String& m) const;

    // Header 44
    V4Viz* titleViz = nullptr;
    V4Stepper* presetStepper = nullptr;
    V4Key* randomKey = nullptr;
    V4Num* bpmNum = nullptr;
    V4Viz* bpmSrcViz = nullptr;

    // Sidebars
    V4Viz* engineFwdViz = nullptr;
    V4Viz* scopeViz = nullptr;
    V4Dial* fwdMixDial = nullptr;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> fwdMixAtt;
    V4Viz* engineRevViz = nullptr;
    V4Seg* revModeSeg = nullptr;
    V4Seg* revSourceSeg = nullptr;
    V4Seg* revCaptureSeg = nullptr;
    V4Viz* captureViz = nullptr;
    V4Dial* revRateDial = nullptr;
    V4Dial* revLfoDial = nullptr;
    V4Dial* revDuckDial = nullptr;
    V4Dial* revMixDial = nullptr;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> revRateAtt, revLfoAtt, revDuckAtt, revMixAtt;

    // Meridiano
    struct Tile { V4Tile* btn = nullptr; juce::String mod; };
    std::vector<Tile> tiles_;
    V4Dial* inputDial = nullptr;
    V4Dial* morphDial = nullptr;
    V4Dial* trimDelayDial = nullptr;
    V4Dial* trimDecayDial = nullptr;
    V4Dial* masterDial = nullptr;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> inputAtt, morphAtt, trimDelayAtt, trimDecayAtt, masterAtt;
    V4Key* throwKey = nullptr;
    V4Key* linkMasterKey = nullptr;
    V4Key* linkKeys[4] = {};
    V4Key* pwrFwdKeys[4] = {};
    V4Key* pwrRevKeys[4] = {};
    V4Seg* xmodeSeg = nullptr;
    V4Stepper* orderStepper = nullptr;
    V4Viz* morphReadViz = nullptr;

    // Toast UNLINK (CallOutBox-like, mas Component próprio para posição livre)
    struct Toast : public juce::Component
    {
        juce::Label label;
        juce::TextButton unlinkBtn { "UNLINK" };
        std::function<void()> onUnlink;
        Toast() { addAndMakeVisible(label); addAndMakeVisible(unlinkBtn);
                  unlinkBtn.onClick = [this] { if (onUnlink) onUnlink(); }; }
        void setMod(const juce::String& m)
        {
            label.setText("LINK " + m.toUpperCase() + " ON - REV FOLLOWS FWD",
                          juce::dontSendNotification);
            unlinkBtn.setButtonText("UNLINK " + m.toUpperCase());
        }
        void resized() override
        {
            label.setBounds(0, 0, 240, 28);
            unlinkBtn.setBounds(244, 0, 110, 28);
        }
    };
    std::unique_ptr<Toast> toast;
    int toastHideAt = 0; // Time::getMillisecondCounter

    // Ripples do THROW (3 anéis 1.5s, só com Timer enquanto ativos)
    struct Ripples : public juce::Component
    {
        struct R { float x = 0; int born = 0; };
        std::vector<R> live_;
        void fire(float x)
        {
            int now = juce::Time::getMillisecondCounter();
            for (int i = 0; i < 3; ++i) live_.push_back({ x, now + i * 180 });
        }
        void paint(juce::Graphics& g) override
        {
            int now = juce::Time::getMillisecondCounter();
            g.setColour(juce::Colours::white);
            for (auto& r : live_)
            {
                float t = (now - r.born) / 1500.f;
                if (t < 0 || t > 1) continue;
                float w = 10 + t * 90, h = 4 + t * 16;
                g.drawEllipse(r.x - w/2, getHeight()/2 - h/2, w, h, 1.5f);
            }
        }
        bool active() const
        {
            int now = juce::Time::getMillisecondCounter();
            for (auto& r : live_) if (now - r.born < 2200) return true;
            return false;
        }
        void gc() { int now = juce::Time::getMillisecondCounter();
                    live_.erase(std::remove_if(live_.begin(), live_.end(),
                        [now](auto& r){ return now - r.born > 2300; }), live_.end()); }
    };
    std::unique_ptr<Ripples> ripples;

    juce::OwnedArray<juce::Component> owned_;
    juce::TooltipWindow tipWin; // mostra os setTooltip() ao pairar
    juce::ComponentAnimator animator_;
    bool reduceMotion_ = false;
    int lastOrder_ = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DeVerbEditor)
};
