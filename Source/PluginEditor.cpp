#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "core/TempoInfo.h"
#include "core/FactoryPresets.h"
#include "dsp/Gater.h"

namespace {
constexpr int kW = 1280, kH = 624;
constexpr int MOD_X = 360, MOD_W = 896;
constexpr int CY_FWD = 64, CY_REV = 406;

int modIdx(const juce::String& m)
{
    if (m == "gate") return 0;
    if (m == "delay") return 1;
    if (m == "verb") return 2;
    return 3;
}
juce::String modAt(int i)
{
    if (i == 0) return "gate";
    if (i == 1) return "delay";
    if (i == 2) return "verb";
    return "gran";
}
}

int DeVerbEditor::modIndex(const juce::String& m) const { return modIdx(m); }

DeVerbEditor::DeVerbEditor(DeVerbProcessor& p)
    : AudioProcessorEditor(p), proc(p)
{
    setLookAndFeel(&lnf);

    selMod_ = proc.getSelMod();
    if (modIdx(selMod_) < 0 || modIdx(selMod_) > 3) selMod_ = "gate";

    auto* ap = &proc.apvts;
    auto accGate = WaterLnF::gateA, accDelay = WaterLnF::delayA,
         accVerb = WaterLnF::verbA, accGran = WaterLnF::granA;

    // ---- 8 painéis ----
    const char* mods[4] = { "gate", "delay", "verb", "gran" };
    for (int m = 0; m < 4; ++m)
    {
        panels[0][m] = std::make_unique<V4ModulePanel>(proc, "fwd", mods[m]);
        panels[1][m] = std::make_unique<V4ModulePanel>(proc, "rev", mods[m]);
        for (int e = 0; e < 2; ++e)
        {
            panels[e][m]->onTetherClick = [this](const juce::String& mm, juce::Component* a) {
                showTetherToast(mm, a);
            };
            addAndMakeVisible(panels[e][m].get());
        }
    }

    // ---- Header ----
    auto* tv = new V4Viz(V4Viz::Title, &proc);
    tv->setBounds(24, 0, 140, 44);
    addAndMakeVisible(tv); owned_.add(tv); titleViz = tv;

    juce::StringArray presetNames;
    for (auto& pr : deVerbFactoryPresets()) presetNames.add(pr.name);
    auto* ps = new V4Stepper(nullptr, presetNames, {}, {});
    ps->setBounds(508, 8, 272, 28);
    ps->onCustomPick = [this](int i) { applyFactoryPreset(proc.apvts, i); };
    addAndMakeVisible(ps); owned_.add(ps); presetStepper = ps;

    auto* rk = new V4Key(nullptr, "RANDOM", V4Key::Normal, WaterLnF::ink);
    rk->setBounds(788, 8, 88, 28);
    rk->onClickExtra = [this](V4Key*) { randomize(); };
    addAndMakeVisible(rk); owned_.add(rk); randomKey = rk;

    auto* bn = new V4Num(ap->getParameter("tempo_bpm"), "BPM", 120.0);
    bn->setBounds(1040, 8, 128, 28);
    addAndMakeVisible(bn); owned_.add(bn); bpmNum = bn;

    auto* bs = new V4Viz(V4Viz::BpmSrc, &proc, {}, "INT");
    bs->setBounds(1176, 8, 80, 28);
    addAndMakeVisible(bs); owned_.add(bs); bpmSrcViz = bs;

    // ---- Sidebars ----
    auto* ef = new V4Viz(V4Viz::EngineFwd, &proc);
    ef->setBounds(24, 64, 288, 28);
    addAndMakeVisible(ef); owned_.add(ef); engineFwdViz = ef;

    auto* sc = new V4Viz(V4Viz::Scope, &proc);
    sc->setBounds(24, 100, 288, 68);
    addAndMakeVisible(sc); owned_.add(sc); scopeViz = sc;

    fwdMixDial = new V4Dial("FWD MIX", "m", WaterLnF::ink, 1.0);
    fwdMixDial->setBounds(240, 176, 64, 86);
    fwdMixDial->setupRange(0.0, 1.0, "n2");
    addAndMakeVisible(fwdMixDial); owned_.add(fwdMixDial);
    fwdMixAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, "fwd_mix", *fwdMixDial);
    fwdMixDial->fixDoubleClick();

    auto* er = new V4Viz(V4Viz::EngineRev, &proc);
    er->setBounds(24, 406, 112, 28);
    addAndMakeVisible(er); owned_.add(er); engineRevViz = er;

    revModeSeg = new V4Seg(ap->getParameter("rev_mode"), { "Off", "Loop", "Throw" }, {}, WaterLnF::ink);
    revModeSeg->setDark(true);
    revModeSeg->setBounds(144, 406, 168, 28);
    addAndMakeVisible(revModeSeg); owned_.add(revModeSeg);

    revSourceSeg = new V4Seg(ap->getParameter("rev_source"), { "DRY", "POST" }, {}, WaterLnF::ink);
    revSourceSeg->setDark(true);
    revSourceSeg->setBounds(24, 440, 112, 28);
    addAndMakeVisible(revSourceSeg); owned_.add(revSourceSeg);

    revCaptureSeg = new V4Seg(ap->getParameter("rev_capture"), { "2", "3", "4" }, "BEATS", WaterLnF::ink);
    revCaptureSeg->setDark(true);
    revCaptureSeg->setBounds(144, 440, 148, 28);
    addAndMakeVisible(revCaptureSeg); owned_.add(revCaptureSeg);

    captureViz = new V4Viz(V4Viz::CaptureWin, &proc);
    captureViz->setBounds(24, 474, 288, 36);
    addAndMakeVisible(captureViz); owned_.add(captureViz);

    auto mkRev = [&](const char* id, const char* lb, int x, V4Dial*& out,
                     std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>& att,
                     double lo, double hi, const char* fmt)
    {
        out = new V4Dial(lb, "m", WaterLnF::ink, 0.0);
        out->setBounds(x, 518, 64, 86);
        out->setDark(true);
        out->setupRange(lo, hi, fmt);
        addAndMakeVisible(out); owned_.add(out);
        att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            proc.apvts, id, *out);
        out->fixDoubleClick();
    };
    mkRev("rev_rate", "RATE", 24, revRateDial, revRateAtt, 0.25, 2.0, "x");
    mkRev("rev_lfo", "LFO", 96, revLfoDial, revLfoAtt, 0.0, 1.0, "n2");
    mkRev("rev_duck", "DUCK", 168, revDuckDial, revDuckAtt, 0.0, 1.0, "n2");
    mkRev("rev_mix", "REV MIX", 240, revMixDial, revMixAtt, 0.0, 1.0, "n2");

    // ---- Meridiano: pedras + PWR/LINK + MORPH/THROW/TRIM/X/ORDER/MASTER ----
    const struct { const char* mod; int x; juce::Colour acc; } tileDef[4] = {
        { "gate", 106, accGate }, { "delay", 238, accDelay },
        { "verb", 370, accVerb }, { "gran", 502, accGran },
    };
    for (auto& td : tileDef)
    {
        auto* b = new V4Tile(td.mod, td.acc);
        b->setBounds(td.x, 290, 124, 88);
        juce::String mm = td.mod;
        b->onClick = [this, mm] { setSelMod(mm, true); };
        addAndMakeVisible(b); owned_.add(b);
        tiles_.push_back({ b, mm });
    }
    const char* pwrFwdIds[4] = { "fwd_gate_on", "fwd_delay_on", "fwd_verb_on", "gr_on" };
    const char* pwrRevIds[4] = { "rev_gate_on", "rev_delay_on", "rev_verb_on", "rev_gr_on" };
    const char* linkIds[4] = { "link_gate", "link_delay", "link_verb", "link_gran" };
    for (int i = 0; i < 4; ++i)
    {
        int tx = tileDef[i].x;
        juce::Colour acc = tileDef[i].acc;
        auto* kf = new V4Key(ap->getParameter(pwrFwdIds[i]), "", V4Key::PwrFwd, acc);
        kf->setBounds(tx + 8, 294, 40, 20);
        addAndMakeVisible(kf); owned_.add(kf); pwrFwdKeys[i] = kf;
        auto* kl = new V4Key(ap->getParameter(linkIds[i]), "", V4Key::Link, acc);
        kl->setBounds(tx + 8, 324, 40, 20);
        kl->onTetherClick = [this](V4Key*) {};
        addAndMakeVisible(kl); owned_.add(kl); linkKeys[i] = kl;
        auto* kr = new V4Key(ap->getParameter(pwrRevIds[i]), "", V4Key::PwrRev, acc);
        kr->setDark(true);
        kr->setBounds(tx + 8, 354, 40, 20);
        kr->onTetherClick = [this, i](V4Key* k) {
            juce::String m = modAt(i);
            showTetherToast(m, k);
        };
        addAndMakeVisible(kr); owned_.add(kr); pwrRevKeys[i] = kr;
    }

    inputDial = new V4Dial("INPUT", "mer", WaterLnF::ink, 1.0);
    inputDial->setBounds(24, 291, 64, 88);
    inputDial->setupRange(0.0, 2.0, "n2");
    addAndMakeVisible(inputDial); owned_.add(inputDial);
    inputAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, "input_gain", *inputDial);
    inputDial->fixDoubleClick();

    morphDial = new V4Dial("MORPH", "xl", WaterLnF::gateA, 0.0);
    morphDial->setBounds(644, 290, 88, 88);
    morphDial->setupRange(0.0, 1.0, "pct");
    addAndMakeVisible(morphDial); owned_.add(morphDial);
    morphAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, "morph", *morphDial);
    morphDial->fixDoubleClick();

    morphReadViz = new V4Viz(V4Viz::MorphRead, &proc, {}, "35%");
    morphReadViz->setBounds(740, 296, 56, 34);
    addAndMakeVisible(morphReadViz); owned_.add(morphReadViz);

    linkMasterKey = new V4Key(ap->getParameter("link_master"), "ALL", V4Key::LinkAll, WaterLnF::ink);
    linkMasterKey->setDark(true);
    linkMasterKey->setBounds(740, 342, 52, 20);
    addAndMakeVisible(linkMasterKey); owned_.add(linkMasterKey);

    throwKey = new V4Key(ap->getParameter("rev_throw"), "THROW", V4Key::Throw, WaterLnF::ink);
    throwKey->setBounds(814, 314, 72, 40);
    throwKey->onClickExtra = [this](V4Key* k) {
        if (ripples) ripples->fire((float)(k->getX() + k->getWidth()/2));
    };
    addAndMakeVisible(throwKey); owned_.add(throwKey);

    trimDelayDial = new V4Dial("T-DLY", "mer", WaterLnF::ink, 1.0);
    trimDelayDial->setBounds(904, 291, 64, 88);
    trimDelayDial->setupRange(0.25, 4.0, "x");
    addAndMakeVisible(trimDelayDial); owned_.add(trimDelayDial);
    trimDelayAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, "trim_delay", *trimDelayDial);
    trimDelayDial->fixDoubleClick();

    trimDecayDial = new V4Dial("T-DEC", "mer", WaterLnF::ink, 1.0);
    trimDecayDial->setBounds(976, 291, 64, 88);
    trimDecayDial->setupRange(0.25, 2.0, "x");
    addAndMakeVisible(trimDecayDial); owned_.add(trimDecayDial);
    trimDecayAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, "trim_decay", *trimDecayDial);
    trimDecayDial->fixDoubleClick();

    xmodeSeg = new V4Seg(ap->getParameter("x_mode"), { "Add", "XFade" }, {}, WaterLnF::ink);
    xmodeSeg->setBounds(1058, 296, 116, 28);
    addAndMakeVisible(xmodeSeg); owned_.add(xmodeSeg);

    orderStepper = new V4Stepper(ap->getParameter("chain_order"),
                                 { "G-D-V-Gr", "G-V-D-Gr", "D-G-V-Gr", "V-D-G-Gr" }, {}, {});
    orderStepper->setDark(true);
    orderStepper->setBounds(1058, 344, 116, 28);
    addAndMakeVisible(orderStepper); owned_.add(orderStepper);

    masterDial = new V4Dial("MASTER", "mer", WaterLnF::ink, 0.8);
    masterDial->setBounds(1192, 291, 64, 88);
    masterDial->setupRange(0.0, 1.0, "n2");
    addAndMakeVisible(masterDial); owned_.add(masterDial);
    masterAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, "master", *masterDial);
    masterDial->fixDoubleClick();

    ripples = std::make_unique<Ripples>();
    ripples->setBounds(0, 282, 1280, 104);
    ripples->setInterceptsMouseClicks(false, false);
    addAndMakeVisible(ripples.get());

    toast = std::make_unique<Toast>();
    toast->setBounds(0, 0, 354, 28);
    toast->setVisible(false);
    toast->onUnlink = [this] {};
    addAndMakeVisible(toast.get());

    // visibilidade inicial sem animação
    for (int e = 0; e < 2; ++e)
        for (int m = 0; m < 4; ++m)
        {
            bool vis = (modAt(m) == selMod_);
            panels[e][m]->setVisible(vis);
            panels[e][m]->setBounds(e == 0 ? MOD_X : MOD_X, e == 0 ? CY_FWD : CY_REV, MOD_W, 198);
        }
    for (auto& t : tiles_)
        t.btn->setToggleState(t.mod == selMod_, juce::dontSendNotification);

    setSize(kW, kH);
    resized();
    startTimerHz(30);
}

DeVerbEditor::~DeVerbEditor()
{
    // Attachments primeiro: os Sliders morrem no owned_/painéis e um
    // SliderAttachment vivo a seguir chamava removeListener() em morto
    // (EXC_BAD_ACCESS no teste Editor do pluginval/mac).
    inputAtt.reset();
    morphAtt.reset();
    trimDelayAtt.reset();
    trimDecayAtt.reset();
    masterAtt.reset();
    fwdMixAtt.reset();
    revRateAtt.reset();
    revLfoAtt.reset();
    revDuckAtt.reset();
    revMixAtt.reset();
    setLookAndFeel(nullptr);
}

void DeVerbEditor::setSelMod(const juce::String& m, bool animate)
{
    if (modAt(modIdx(m)) != m) return;
    if (m == selMod_) return;
    selMod_ = m;
    proc.setSelMod(m);
    bool reduce = reduceMotion_;
    for (auto& t : tiles_)
        t.btn->setToggleState(t.mod == selMod_, juce::dontSendNotification);
    for (int e = 0; e < 2; ++e)
        for (int i = 0; i < 4; ++i)
        {
            bool vis = (modAt(i) == selMod_);
            auto* p = panels[e][i].get();
            if (vis && animate && !reduce)
            {
                p->setVisible(true);
                p->setAlpha(0.f);
                animator_.fadeIn(p, 180);
            }
            else
            {
                animator_.cancelAnimation(p, false);
                p->setVisible(vis);
                p->setAlpha(1.f);
            }
            if (!vis) { animator_.cancelAnimation(p, false); p->setVisible(false); }
        }
}

void DeVerbEditor::showTetherToast(const juce::String& mod, juce::Component* anchor)
{
    if (toast == nullptr || anchor == nullptr) return;
    toast->setMod(mod);
    auto ab = anchor->getBounds();
    // anchor pode ser filho de painel (coords locais) — converte para editor
    juce::Point<int> tl = anchor->getParentComponent() != nullptr
        ? anchor->getParentComponent()->getLocalPoint(this, ab.getPosition()) : ab.getPosition();
    // Na prática: usa posição global via getScreenPosition (robusto sob Xvfb)
    auto sp = anchor->getScreenPosition();
    auto ep = getScreenPosition();
    int lx = sp.x - ep.x, ly = sp.y - ep.y;
    int ty = (ly > 70) ? ly - 44 : ly + anchor->getHeight() + 8;
    toast->setTopLeftPosition(juce::jlimit(8, 1280 - 362, lx), juce::jlimit(0, 624 - 30, ty));
    toast->setVisible(true);
    juce::String linkId = "link_" + mod;
    toast->onUnlink = [this, linkId] {
        if (auto* p = proc.apvts.getParameter(linkId))
            p->setValueNotifyingHost(p->convertTo0to1(0.f));
        if (auto* lm = proc.apvts.getParameter("link_master"))
        {
            // mantém master; só desliga o módulo (como no mockup UNLINK X)
        }
        hideToast();
    };
    toastHideAt = juce::Time::getMillisecondCounter() + 3800;
    toast->toFront(false);
}

void DeVerbEditor::hideToast() { if (toast) toast->setVisible(false); }

void DeVerbEditor::applyOrder(int idx)
{
    // Ordem da cadeia = posições das pedras (animar com ComponentAnimator).
    // Só mexe quando o índice muda (o Timer corre a 30 Hz).
    idx = juce::jlimit(0, 3, idx);
    if (idx == lastOrder_)
        return;
    lastOrder_ = idx;
    static const char* orders[4][4] = {
        { "gate", "delay", "verb", "gran" }, { "gate", "verb", "delay", "gran" },
        { "delay", "gate", "verb", "gran" }, { "verb", "delay", "gate", "gran" },
    };
    int slotX[4] = { 106, 238, 370, 502 };
    for (int k = 0; k < 4; ++k)
    {
        juce::String mm = orders[idx][k];
        for (auto& t : tiles_)
        {
            if (t.mod != mm) continue;
            if (!reduceMotion_) animator_.animateComponent(t.btn, juce::Rectangle<int>(slotX[k], 290, 124, 88), 1.f, 280, false, 0.0, 0.0);
            else t.btn->setBounds(slotX[k], 290, 124, 88);
            // move as 3 teclas da pedra (PWR FWD/LINK/PWR REV)
            int pi = modIdx(mm);
            if (pwrFwdKeys[pi]) pwrFwdKeys[pi]->setTopLeftPosition(slotX[k] + 8, 294);
            if (linkKeys[pi]) linkKeys[pi]->setTopLeftPosition(slotX[k] + 8, 324);
            if (pwrRevKeys[pi]) pwrRevKeys[pi]->setTopLeftPosition(slotX[k] + 8, 354);
        }
    }
}

void DeVerbEditor::randomize()
{
    juce::Random rng;
    applyRealList(proc.apvts, civilizedRandom(rng));
}

juce::Slider* DeVerbEditor::findDial(const juce::String& paramId)
{
    for (int e = 0; e < 2; ++e)
        for (int m = 0; m < 4; ++m)
            if (auto* d = panels[e][m]->findDial(paramId)) return d;
    juce::Slider* globals[] = { fwdMixDial, revRateDial, revLfoDial, revDuckDial, revMixDial,
                                inputDial, morphDial, trimDelayDial, trimDecayDial, masterDial };
    for (auto* d : globals)
        if (d != nullptr && proc.apvts.getParameter(paramId) != nullptr)
        {
            // compara via attachment? simplifica: verifica se o attachment aponta para o id
            // (só usado em testes para fb/decay/master genéricos — devolve por nome conhecido)
        }
    // fallback por ids conhecidos
    if (paramId == "fwd_mix") return fwdMixDial;
    if (paramId == "rev_rate") return revRateDial;
    if (paramId == "rev_lfo") return revLfoDial;
    if (paramId == "rev_duck") return revDuckDial;
    if (paramId == "rev_mix") return revMixDial;
    if (paramId == "input_gain") return inputDial;
    if (paramId == "morph") return morphDial;
    if (paramId == "trim_delay") return trimDelayDial;
    if (paramId == "trim_decay") return trimDecayDial;
    if (paramId == "master") return masterDial;
    return nullptr;
}

V4Seg* DeVerbEditor::findSeg(const juce::String& paramId)
{
    for (int e = 0; e < 2; ++e)
        for (int m = 0; m < 4; ++m)
            if (auto* s = panels[e][m]->findSeg(paramId)) return s;
    if (paramId == "rev_mode") return revModeSeg;
    if (paramId == "rev_source") return revSourceSeg;
    if (paramId == "rev_capture") return revCaptureSeg;
    if (paramId == "x_mode") return xmodeSeg;
    return nullptr;
}

V4Stepper* DeVerbEditor::findStepper(const juce::String& paramId)
{
    for (int e = 0; e < 2; ++e)
        for (int m = 0; m < 4; ++m)
            if (auto* s = panels[e][m]->findStepper(paramId)) return s;
    if (paramId == "chain_order") return orderStepper;
    return nullptr;
}

V4Key* DeVerbEditor::findKey(const juce::String& paramId)
{
    for (int e = 0; e < 2; ++e)
        for (int m = 0; m < 4; ++m)
            if (auto* k = panels[e][m]->findKey(paramId)) return k;
    if (paramId == "rev_throw") return throwKey;
    if (paramId == "link_master") return linkMasterKey;
    const char* lk[4] = { "link_gate", "link_delay", "link_verb", "link_gran" };
    const char* pf[4] = { "fwd_gate_on", "fwd_delay_on", "fwd_verb_on", "gr_on" };
    const char* pr[4] = { "rev_gate_on", "rev_delay_on", "rev_verb_on", "rev_gr_on" };
    for (int i = 0; i < 4; ++i)
    {
        if (paramId == lk[i]) return linkKeys[i];
        if (paramId == pf[i]) return pwrFwdKeys[i];
        if (paramId == pr[i]) return pwrRevKeys[i];
    }
    return nullptr;
}

void DeVerbEditor::timerCallback()
{
    // painéis: tether + agulhas
    for (int e = 0; e < 2; ++e)
        for (int m = 0; m < 4; ++m)
            panels[e][m]->updateLinkState();

    // dials globais: efetivo = próprio + refresh da caixa
    for (auto* d : { fwdMixDial, revRateDial, revLfoDial, revDuckDial, revMixDial,
                     inputDial, morphDial, trimDelayDial, trimDecayDial, masterDial })
        if (d != nullptr)
            d->syncEffToOwn();

    // order -> pedras (lê choice 0..3)
    if (auto* v = proc.apvts.getRawParameterValue("chain_order"))
        applyOrder((int)v->load());

    // readouts calculados
    float bpm = proc.getUiBpm();
    juce::String src = proc.isTempoFromHost() ? "HOST" : "INT";
    if (bpmSrcViz) bpmSrcViz->setText(src);
    if (morphReadViz)
        morphReadViz->setText(juce::String((int)std::round(proc.apvts.getRawParameterValue("morph")->load() * 100)) + "%");
    if (scopeViz) scopeViz->repaint();
    if (captureViz) captureViz->repaint();
    if (ripples && ripples->active()) { ripples->repaint(); ripples->gc(); }

    if (toast && toast->isVisible() && juce::Time::getMillisecondCounter() > toastHideAt)
        hideToast();

    // Sem repaint geral: cada viz/dial/seg repinta-se sozinho quando o valor
    // muda (guards em setText/setAux/setNeedles/setTethered).
}

void DeVerbEditor::paint(juce::Graphics& g)
{
    // fundo Espelho: papel em cima da linha (y=334), tinta em baixo
    g.setColour(WaterLnF::paper);
    g.fillRect(0, 0, 1280, 334);
    g.setColour(WaterLnF::ink);
    g.fillRect(0, 334, 1280, 624 - 334);
    // barras laterais
    g.setColour(WaterLnF::paper2);
    g.fillRect(0, 44, 336, 238);
    g.setColour(WaterLnF::ink2);
    g.fillRect(0, 386, 336, 238);
    // cabeçalho
    g.setColour(WaterLnF::paper);
    g.fillRect(0, 0, 1280, 44);
    g.setColour(WaterLnF::hairP);
    g.drawLine(0, 44, 1280, 44, 1.f);
    // linha de agua
    g.setColour(WaterLnF::water);
    g.drawLine(0, 334, 1280, 334, 1.f);
    // legendas fora das caixas (meridiano)
    g.setFont(10.f);
    g.setColour(WaterLnF::labP);
    g.drawFittedText("X-MODE", juce::Rectangle<int>(1058, 282, 116, 14),
                     juce::Justification::centredLeft, 1);
    g.setColour(WaterLnF::labI);
    g.drawFittedText("ORDER", juce::Rectangle<int>(1058, 372, 116, 14),
                     juce::Justification::centredLeft, 1);
}

void DeVerbEditor::resized()
{
    // Janela fixa; posiciona painéis (idempotente, sem addItemList).
    // Guarda contra setSize() durante o ctor (painéis ainda nulos).
    for (int m = 0; m < 4; ++m)
    {
        if (panels[0][m]) panels[0][m]->setBounds(MOD_X, CY_FWD, MOD_W, 198);
        if (panels[1][m]) panels[1][m]->setBounds(MOD_X, CY_REV, MOD_W, 198);
    }
    if (ripples) ripples->setBounds(0, 282, 1280, 104);
}
