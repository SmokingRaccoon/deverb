#include "V4ModulePanel.h"
#include "../PluginProcessor.h"
#include "../core/TempoInfo.h"
#include "../dsp/Gater.h"
#include "../core/RevLinker.h"
#include "WaterLnF.h"

namespace {
juce::Colour accFor(const juce::String& m)
{
    if (m == "gate") return WaterLnF::gateA;
    if (m == "delay") return WaterLnF::delayA;
    if (m == "verb") return WaterLnF::verbA;
    if (m == "gran") return WaterLnF::granA;
    return WaterLnF::ink;
}
juce::StringArray notesArr() { return TempoInfo::noteNames(); }
}

V4ModulePanel::~V4ModulePanel()
{
    for (auto& e : dials_)
        e.att.reset();
}

V4ModulePanel::V4ModulePanel(DeVerbProcessor& proc, const juce::String& engine, const juce::String& mod)
    : engine_(engine), mod_(mod), proc_(proc)
{
    auto acc = accFor(mod);
    // título
    juce::String title = mod.toUpperCase();
    juce::String sub = mod == "gate" ? "TRANCE" : mod == "delay" ? "ECHO" : mod == "verb" ? "SPACE" : "GLITCH";
    addViz(V4Viz::ModTitle, 0, 0, 300, 24, title + " " + sub);

    if (mod == "gate")
    {
        addViz(V4Viz::Ruler, 0, 28, 660, 14);
        addViz(V4Viz::GateBig, 684, 28, 212, 74, "07/16");
        for (int i = 0; i < 16; ++i)
        {
            int gx = (i / 4) * 168 + (i % 4) * 40;
            auto* b = new juce::TextButton(juce::String(i + 1));
            b->setBounds(gx, 46, 36, 56);
            b->setColour(juce::TextButton::buttonOnColourId, acc);
            b->setClickingTogglesState(true);
            int idx = i;
            b->onClick = [this, idx] { flipStep(idx); };
            addAndMakeVisible(b);
            owned_.add(b);
            stepBtns_.push_back({ b, idx });
        }
        const char* dl[5] = { "SMOOTH", "DEPTH", "MIX", "PAN", "THR" };
        const char* db[5] = { "smooth", "depth", "mix", "pan", "env_thr" };
        for (int i = 0; i < 5; ++i) addDial(db[i], dl[i], i * 72, 112, acc);
        juce::StringArray patNames;
        std::vector<int> patBits;
        for (auto& fp : Gater::factoryPatterns) { patNames.add(fp.name); patBits.push_back(fp.bits); }
        addStepper("pattern", patNames, "PATTERN", 372, 120, 256, patBits);
        addStepper("rate", notesArr(), "RATE", 640, 120, 256);
        addSeg("steps", { "8", "16" }, "STEPS", 372, 156, 152);
        addSeg("trig", { "Host", "Midi", "Transient", "Free" }, "TRIG", 536, 156, 360);
    }
    else if (mod == "delay")
    {
        addViz(V4Viz::DelayMs, 560, 0, 336, 24, "375 ms");
        addSeg("algo", { "Digital", "Tape", "PingPong", "MultiTap", "Reverse" }, "ALGO", 0, 32, 400);
        addStepper("note", notesArr(), "DIV", 416, 32, 176);
        addKey("freeze", "FREEZE", 608, 32, 88, 28, V4Key::Normal, acc);
        addViz(V4Viz::Taps, 0, 70, 560, 36);
        const char* dl[8] = { "MS", "FB", "DAMP", "MIX", "DRIVE", "WOW RT", "WOW DP", "SPREAD" };
        const char* db[8] = { "time", "fb", "damp", "mix", "drive", "wow_rate", "wow_depth", "spread" };
        for (int i = 0; i < 8; ++i) addDial(db[i], dl[i], i * 72, 112, acc);
    }
    else if (mod == "verb")
    {
        addViz(V4Viz::VerbT60, 560, 0, 336, 24, "T60 2.5 s");
        addViz(V4Viz::IR, 600, 30, 296, 76);
        addSeg("algo", { "Room", "Hall", "Plate", "Shimmer" }, "ALGO", 0, 32, 275);
        addStepper("predelay_note", notesArr(), "PRE", 291, 32, 180);
        addKey("freeze", "FREEZE", 487, 32, 88, 28, V4Key::Normal, acc);
        addViz(V4Viz::Decay, 0, 70, 540, 36);
        const char* dl[8] = { "SIZE", "DECAY", "DAMP", "WIDTH", "PRE-DLY", "LO-CUT", "HI-CUT", "MIX" };
        const char* db[8] = { "size", "decay", "damp", "width", "predelay", "locut", "hicut", "mix" };
        for (int i = 0; i < 8; ++i) addDial(db[i], dl[i], i * 72, 112, acc);
    }
    else if (mod == "gran")
    {
        addViz(V4Viz::GranLed, 790, 0, 106, 24, "IDLE");
        // Labels curtos (recalibrado com Nimbus real: "BeatRepeat" transbordava
        // em 48px/botão; o mockup usa BEAT/SLICE/REV/PITCH/STUT — DESIGN-V4 §9).
        addSeg("mode", { "Off", "BEAT", "SLICE", "REV", "PITCH", "STUT" }, "MODE", 0, 30, 340);
        addSeg("trigger", { "CHANCE", "ENV", "MANUAL" }, "TRIG", 356, 30, 220);
        addKey("manual", "GRAB", 592, 30, 72, 28, V4Key::Normal, acc);
        addKey("interrupt", "INTERRUPT", 672, 30, 96, 28, V4Key::Normal, acc);
        addStepper("len_note", notesArr(), "LEN", 0, 66, 176);
        addStepper("time_note", notesArr(), "T-NOTE", 192, 66, 196);
        addViz(V4Viz::Shards, 410, 64, 330, 42);
        const char* dl[9] = { "CHANCE", "THR", "REPEATS", "DECAY", "TIME", "PITCH", "FLUX", "XFADE", "MIX" };
        const char* db[9] = { "chance", "env_thr", "repeats", "decay", "time", "pitch", "flux", "xfade", "mix" };
        for (int i = 0; i < 9; ++i) addDial(db[i], dl[i], i * 72, 112, acc);
    }
}

juce::String V4ModulePanel::pid(const juce::String& base) const
{
    if (mod_ == "gran")
        return engine_ == "fwd" ? ("gr_" + base) : ("rev_gr_" + base);
    return engine_ + "_" + mod_ + "_" + base;
}

juce::RangedAudioParameter* V4ModulePanel::par(const juce::String& id) const
{
    return proc_.apvts.getParameter(id);
}

V4Dial* V4ModulePanel::addDial(const juce::String& base, const juce::String& label,
                               int x, int y, const juce::Colour& acc)
{
    juce::String id = pid(base);
    auto* p = par(id);
    double defV = 0.0;
    if (p != nullptr) defV = p->convertFrom0to1(p->getDefaultValue());
    auto* d = new V4Dial(label, "m", acc, defV);
    d->setBounds(x, y, 64, 86);
    addAndMakeVisible(d);
    owned_.add(d);
    DialEntry e;
    e.dial = d; e.paramId = id;
    if (auto* rp = dynamic_cast<juce::RangedAudioParameter*>(p))
    {
        // SliderAttachment precisa de Slider; V4Dial é Slider
        e.att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            proc_.apvts, id, *d);
        auto nr = rp->getNormalisableRange();
        e.lo = nr.start; e.hi = nr.end;
    }
    dials_.push_back(std::move(e));
    return d;
}

V4Seg* V4ModulePanel::addSeg(const juce::String& base, const juce::StringArray& labels,
                             const juce::String& cap, int x, int y, int w)
{
    juce::String id = pid(base);
    auto* s = new V4Seg(par(id), labels, cap, accFor(mod_));
    s->setDark(engine_ == "rev");
    s->setBounds(x, y, w, 28);
    s->onTetherClick = [this](V4Seg* sg) {
        if (onTetherClick) onTetherClick(mod_, sg);
    };
    addAndMakeVisible(s);
    owned_.add(s);
    segs_.push_back({ s, id });
    return s;
}

V4Stepper* V4ModulePanel::addStepper(const juce::String& base, const juce::StringArray& opts,
                                     const juce::String& cap, int x, int y, int w,
                                     const std::vector<int>& bits)
{
    juce::String id = pid(base);
    auto* s = new V4Stepper(par(id), opts, cap, bits);
    s->setDark(engine_ == "rev");
    s->setBounds(x, y, w, 28);
    s->onTetherClick = [this](V4Stepper* sg) {
        if (onTetherClick) onTetherClick(mod_, sg);
    };
    addAndMakeVisible(s);
    owned_.add(s);
    steps_.push_back({ s, id });
    return s;
}

V4Key* V4ModulePanel::addKey(const juce::String& base, const juce::String& text,
                             int x, int y, int w, int h, V4Key::Kind k,
                             const juce::Colour& acc)
{
    juce::String id = pid(base);
    auto* key = new V4Key(par(id), text, k, acc);
    key->setBounds(x, y, w, h);
    key->onTetherClick = [this](V4Key* kk) {
        if (onTetherClick) onTetherClick(mod_, kk);
    };
    addAndMakeVisible(key);
    owned_.add(key);
    keys_.push_back({ key, id });
    return key;
}

void V4ModulePanel::addViz(V4Viz::Kind k, int x, int y, int w, int h, const juce::String& txt)
{
    auto* v = new V4Viz(k, &proc_, mod_, txt);
    v->setBounds(x, y, w, h);
    addAndMakeVisible(v);
    owned_.add(v);
    vizs_.push_back(v);
}

void V4ModulePanel::flipStep(int step)
{
    juce::String id = engine_ == "fwd" ? "fwd_gate_pattern" : "rev_gate_pattern";
    // Se REV com link, não escreve — pede UNLINK (tether)
    if (engine_ == "rev")
    {
        bool linkM = proc_.apvts.getRawParameterValue("link_master")->load() > 0.5f;
        bool linkG = proc_.apvts.getRawParameterValue("link_gate")->load() > 0.5f;
        if (linkM && linkG)
        {
            if (onTetherClick && step >= 0 && step < (int)stepBtns_.size())
                onTetherClick("gate", stepBtns_[(size_t)step].b);
            return;
        }
    }
    if (auto* p = proc_.apvts.getParameter(id))
    {
        int bits = (int)proc_.apvts.getRawParameterValue(id)->load();
        bits ^= (1 << step);
        p->setValueNotifyingHost(p->convertTo0to1((float)bits));
    }
}

static float getFloat(juce::AudioProcessorValueTreeState& apvts, const juce::String& id, float dflt = 0.f)
{
    if (auto* v = apvts.getRawParameterValue(id)) return v->load();
    return dflt;
}

void V4ModulePanel::updateLinkState()
{
    bool linkM = getFloat(proc_.apvts, "link_master") > 0.5f;
    bool linkMod = true;
    if (mod_ == "gate") linkMod = getFloat(proc_.apvts, "link_gate") > 0.5f;
    else if (mod_ == "delay") linkMod = getFloat(proc_.apvts, "link_delay") > 0.5f;
    else if (mod_ == "verb") linkMod = getFloat(proc_.apvts, "link_verb") > 0.5f;
    else if (mod_ == "gran") linkMod = getFloat(proc_.apvts, "link_gran") > 0.5f;
    bool linked = linkM && linkMod && engine_ == "rev";
    float morph = getFloat(proc_.apvts, "morph");
    float trimD = getFloat(proc_.apvts, "trim_delay", 1.f);
    float trimV = getFloat(proc_.apvts, "trim_decay", 1.f);

    // Dials contínuos: 2 agulhas no REV
    for (auto& e : dials_)
    {
        if (e.dial == nullptr || e.att == nullptr) continue;
        float own = getFloat(proc_.apvts, e.paramId);
        float own01 = juce::jlimit(0.f, 1.f, (own - e.lo) / juce::jmax(1e-6f, e.hi - e.lo));
        if (engine_ == "fwd" || !linked)
        {
            e.dial->setNeedles(own01, own01, own01, false);
            e.dial->setEnabled(true);
        }
        else
        {
            // gémeo FWD
            juce::String fwdId = e.paramId;
            fwdId = fwdId.replace("rev_gr_", "gr_").replace("rev_", "fwd_");
            // gr_ vs fwd_ já tratado: rev_gr_X -> gr_X; rev_delay_X -> fwd_delay_X etc.
            float fwd = getFloat(proc_.apvts, fwdId, own);
            float trim = 1.f;
            if (e.paramId == "rev_delay_time") trim = trimD;
            if (e.paramId == "rev_verb_decay") trim = trimV;
            float ghost = juce::jlimit(e.lo, e.hi, fwd * trim);
            float eff = juce::jlimit(e.lo, e.hi, ghost + (own - ghost) * morph);
            float g01 = (ghost - e.lo) / juce::jmax(1e-6f, e.hi - e.lo);
            float e01 = (eff - e.lo) / juce::jmax(1e-6f, e.hi - e.lo);
            e.dial->setNeedles(own01, g01, e01, true);
        }
        // MS só conta com DIV=Free (esbate)
        if (e.paramId == "fwd_delay_time" || e.paramId == "rev_delay_time")
        {
            juce::String nid = engine_ == "fwd" ? "fwd_delay_note" : "rev_delay_note";
            // Se REV linkado, a nota segue FWD
            juce::String effNid = (linked ? "fwd_delay_note" : nid);
            int ni = (int)getFloat(proc_.apvts, effNid);
            e.dial->setAlpha(ni == 0 ? 1.f : 0.38f);
        }
    }

    // Discretos REV: tether (seguem FWD, tracejado, clique = UNLINK)
    if (engine_ == "rev")
    {
        for (auto& s : segs_)
        {
            int fwdI = -1, revI = 0;
            juce::String fwdId = s.paramId;
            fwdId = fwdId.replace("rev_gr_", "gr_");
            if (fwdId.startsWith("rev_")) fwdId = "fwd_" + fwdId.substring(4);
            if (auto* fp = dynamic_cast<juce::AudioParameterChoice*>(proc_.apvts.getParameter(fwdId)))
                fwdI = fp->getIndex();
            if (auto* rp = dynamic_cast<juce::AudioParameterChoice*>(proc_.apvts.getParameter(s.paramId)))
                revI = rp->getIndex();
            juce::ignoreUnused(revI);
            s.seg->setTethered(linked, fwdI);
        }
        for (auto& s : steps_)
        {
            int fwdI = 0;
            juce::String fwdId = s.paramId;
            fwdId = fwdId.replace("rev_gr_", "gr_");
            if (fwdId.startsWith("rev_")) fwdId = "fwd_" + fwdId.substring(4);
            if (auto* fp = dynamic_cast<juce::AudioParameterChoice*>(proc_.apvts.getParameter(fwdId)))
                fwdI = fp->getIndex();
            else if (fwdId.endsWith("gate_pattern"))
            {
                // pattern int (bits) -> índice na lista de fábrica
                int b = (int)proc_.apvts.getRawParameterValue(fwdId)->load();
                fwdI = 0;
                int k = 0;
                for (auto& fp : Gater::factoryPatterns)
                {
                    if (fp.bits == b) { fwdI = k; break; }
                    ++k;
                }
            }
            else if (proc_.apvts.getParameter(fwdId) != nullptr)
                fwdI = (int)std::round(proc_.apvts.getRawParameterValue(fwdId)->load());
            s.st->setTethered(linked, fwdI);
        }
        for (auto& k : keys_)
        {
            k.key->setTethered(linked);
        }
        // passos
        int bits = 0, steps = 16;
        if (mod_ == "gate")
        {
            juce::String pidUse = linked ? "fwd_gate_pattern" : "rev_gate_pattern";
            juce::String sidUse = linked ? "fwd_gate_steps" : "rev_gate_steps";
            bits = (int)getFloat(proc_.apvts, pidUse);
            steps = getFloat(proc_.apvts, sidUse) > 0.5f ? 16 : 8;
            for (auto& sb : stepBtns_)
            {
                // REV espelha índice 15-i (corre ao contrário)
                int disp = 15 - sb.idx;
                bool on = linked ? ((bits >> sb.idx) & 1) : ((bits >> sb.idx) & 1);
                juce::ignoreUnused(disp);
                sb.b->setToggleState(on, juce::dontSendNotification);
                sb.b->setAlpha((!linked && sb.idx >= steps) ? 0.3f : (linked ? 0.62f : 1.f));
                sb.b->getProperties().set("tether", linked);
            }
        }
    }
    else
    {
        if (mod_ == "gate")
        {
            int bits = (int)getFloat(proc_.apvts, "fwd_gate_pattern");
            int steps = getFloat(proc_.apvts, "fwd_gate_steps") > 0.5f ? 16 : 8;
            for (auto& sb : stepBtns_)
            {
                sb.b->setToggleState(((bits >> sb.idx) & 1), juce::dontSendNotification);
                sb.b->setAlpha(sb.idx >= steps ? 0.3f : 1.f);
            }
        }
    }
}

juce::Slider* V4ModulePanel::findDial(const juce::String& paramId)
{
    for (auto& e : dials_) if (e.paramId == paramId) return e.dial;
    return nullptr;
}
V4Seg* V4ModulePanel::findSeg(const juce::String& paramId)
{
    for (auto& s : segs_) if (s.paramId == paramId) return s.seg;
    return nullptr;
}
V4Stepper* V4ModulePanel::findStepper(const juce::String& paramId)
{
    for (auto& s : steps_) if (s.paramId == paramId) return s.st;
    return nullptr;
}
V4Key* V4ModulePanel::findKey(const juce::String& paramId)
{
    for (auto& k : keys_) if (k.paramId == paramId) return k.key;
    return nullptr;
}
