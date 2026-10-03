#!/usr/bin/env python3
"""Gera esqueleto HTML/CSS fiel ao layout atual (1280x624, px fixos).
Fonte verdade geometria: Source/PluginEditor.cpp (resized, colDefs, kW/kH/kTopH).
Fonte verdade params: Source/PluginProcessor.cpp (122 IDs, congelados).
Fonte verdade cores: Source/ui/AbletonLnF.h.
Nao toca em DSP nem JUCE — so emite assets/mockup/index.html + styles.css.
"""
import os
HERE = os.path.dirname(os.path.abspath(__file__))

NOTES = ["Free","1/32","1/16T","1/16","1/16D","1/8T","1/8","1/8D","1/4","1/4D","1/2","1/1"]

def knob(x,y,w,h,label,fwd=None,rev=None,single=None,extra=""):
    if single:
        dp = f'data-param="{single}"'
    else:
        dp = f'data-param-fwd="{fwd}" data-param-rev="{rev}"'
    return f'<div class="knob" style="left:{x}px;top:{y}px;width:{w}px;height:{h}px" {dp} data-label="{label}" {extra}><div class="dial"><div class="ptr"></div></div><div class="klabel">{label}</div><div class="kval">--</div></div>'

def mini(x,y,w,h,label,fwd=None,rev=None,single=None):
    return knob(x,y,w,h,label,fwd,rev,single,extra='data-size="mini"')

def combo(x,y,w,h,label,fwd=None,rev=None,single=None,options="",role=""):
    if single:
        dp = f'data-param="{single}"'
    elif fwd or rev:
        dp = f'data-param-fwd="{fwd}" data-param-rev="{rev}"'
    else:
        dp = f'data-role="{role}"'
        if role and (fwd is None and rev is None and single is None):
            pass
    # pattern-preset escreve no pattern mas nao e attachment direto
    if role in ("factory-preset","random","mode-fwdrev","pattern-preset"):
        dp = f'data-role="{role}"'
        if fwd:
            dp += f' data-param-fwd="{fwd}" data-param-rev="{rev}"'
    return f'<div class="combo" style="left:{x}px;top:{y}px;width:{w}px;height:{h}px" {dp} title="{label}"><span class="clabel">{label}</span><span class="cval">{options} ▾</span></div>'

def btn(x,y,w,h,label,fwd=None,rev=None,single=None,role="",tog=False):
    cls = "tbtn tog" if tog else "tbtn"
    if single:
        dp = f'data-param="{single}"'
    elif fwd or rev:
        dp = f'data-param-fwd="{fwd}" data-param-rev="{rev}"'
    else:
        dp = f'data-role="{role}"'
    if role and not (fwd or single):
        # permite role + sem param (ex random)
        if not fwd and not single:
            dp = f'data-role="{role}"'
    return f'<button class="{cls}" style="left:{x}px;top:{y}px;width:{w}px;height:{h}px" {dp} title="{label}">{label}</button>'

def visual(x,y,w,h,cls2,label,txt=""):
    return f'<div class="visual {cls2}" style="left:{x}px;top:{y}px;width:{w}px;height:{h}px" data-visual="{label}">{txt or label}</div>'

parts = []

# ---- TOPBAR linha 1 ----
parts.append(visual(12,0,200,36,"title","title","deVerb"))
parts.append(visual(220,7,110,22,"readout","bpm-readout","BPM 120.0 INT"))
parts.append(visual(340,7,130,22,"readout","gate-readout","GATE 01/16"))
parts.append(combo(560,7,240,22,"Preset fabrica",options="Preset...",role="factory-preset"))
parts.append(btn(806,7,68,22,"RANDOM",role="random"))
parts.append(btn(880,7,90,22,"FWD",role="mode-fwdrev"))
parts.append(btn(972,7,90,22,"REV",role="mode-fwdrev"))
parts.append(visual(1148,7,120,22,"led","gran-led","IDLE"))
# ---- TOPBAR linha 2 faixa REV ----
parts.append(visual(12,40,52,56,"sect","sect-rev","REV"))
parts.append(combo(70,42,110,30,"REV Mode",single="rev_mode",options="Loop"))
parts.append(combo(186,42,90,30,"REV Source",single="rev_source",options="Dry"))
parts.append(combo(282,42,90,30,"REV Capture",single="rev_capture",options="2 beats"))
parts.append(knob(378,38,56,58,"RATE",single="rev_rate"))
parts.append(knob(438,38,56,58,"LFO",single="rev_lfo"))
parts.append(knob(498,38,56,58,"DUCK",single="rev_duck"))
parts.append(btn(560,44,90,30,"THROW",single="rev_throw",tog=True))
parts.append(visual(656,40,40,56,"sect","sect-link","LINK"))
parts.append(btn(700,44,56,30,"ALL",single="link_master",tog=True))
parts.append(btn(758,44,56,30,"GATE",single="link_gate",tog=True))
parts.append(btn(816,44,56,30,"DLY",single="link_delay",tog=True))
parts.append(btn(874,44,56,30,"VRB",single="link_verb",tog=True))
parts.append(btn(932,44,56,30,"GRN",single="link_gran",tog=True))
parts.append(knob(996,38,56,58,"T-DLY",single="trim_delay"))
parts.append(knob(1056,38,56,58,"T-DEC",single="trim_decay"))
parts.append(combo(1116,44,76,30,"Chain Order",single="chain_order",options="G-D-V-G"))
parts.append(combo(1196,44,72,30,"X Mode",single="x_mode",options="Add"))

# ---- panels (fundo) ----
panels = [
    (8,108,184,508,"motor","MOTOR - PERFORM","#b5b5b5"),
    (200,108,296,508,"gate","GATE - TRANCE","#fed134"),
    (504,108,232,508,"delay","DELAY - ECHO","#58c472"),
    (744,108,256,508,"verb","VERB - SPACE","#5aa9e6"),
    (1008,108,264,508,"gran","GRAN - GLITCH","#ff9034"),
]

# ---- MOTOR ----
parts.append(knob(64,132,72,90,"INPUT",single="input_gain"))
parts.append(knob(12,226,84,90,"FWD MIX",single="fwd_mix"))
parts.append(knob(100,226,84,90,"REV MIX",single="rev_mix"))
parts.append(knob(52,320,88,110,"MORPH",single="morph"))
parts.append(knob(52,434,88,110,"MASTER",single="master"))

# ---- GATE ----
parts.append(visual(204,132,288,44,"ruler","ruler","RULER 16 steps"))
# steps 2x8
for i in range(16):
    row, col = i//8, i%8
    x = 204+col*36; y = 184+row*48
    parts.append(f'<button class="step" style="left:{x}px;top:{y}px;width:34px;height:46px" data-param-fwd="fwd_gate_pattern" data-param-rev="rev_gate_pattern" data-step="{i}" title="step {i+1}">{i+1}</button>')
parts.append(combo(204,280,144,32,"Pattern fabrica",fwd="fwd_gate_pattern",rev="rev_gate_pattern",options="Pattern...",role="pattern-preset"))
parts.append(combo(352,280,144,32,"Gate Rate",fwd="fwd_gate_rate",rev="rev_gate_rate",options="1/16"))
parts.append(combo(204,316,144,32,"Gate Steps",fwd="fwd_gate_steps",rev="rev_gate_steps",options="16"))
parts.append(combo(352,316,144,32,"Gate Trig",fwd="fwd_gate_trig",rev="rev_gate_trig",options="Host"))
for i,(lb,f,r) in enumerate([("SMOOTH","fwd_gate_smooth","rev_gate_smooth"),("DEPTH","fwd_gate_depth","rev_gate_depth"),("MIX","fwd_gate_mix","rev_gate_mix")]):
    parts.append(knob(204+i*97,362,96,110,lb,fwd=f,rev=r))
for i,(lb,f,r) in enumerate([("PAN","fwd_gate_pan","rev_gate_pan"),("THR","fwd_gate_env_thr","rev_gate_env_thr")]):
    parts.append(knob(204+i*97,476,96,110,lb,fwd=f,rev=r))
parts.append(btn(444,110,48,20,"PWR",fwd="fwd_gate_on",rev="rev_gate_on",tog=True))

# ---- DELAY ----
parts.append(knob(508,132,72,90,"BPM",single="tempo_bpm"))
parts.append(knob(582,132,72,90,"MS",fwd="fwd_delay_time",rev="rev_delay_time"))
parts.append(knob(656,132,72,90,"FB",fwd="fwd_delay_fb",rev="rev_delay_fb"))
parts.append(knob(508,226,72,90,"DAMP",fwd="fwd_delay_damp",rev="rev_delay_damp"))
parts.append(knob(582,226,72,90,"MIX",fwd="fwd_delay_mix",rev="rev_delay_mix"))
parts.append(combo(508,320,224,32,"Delay Algo",fwd="fwd_delay_algo",rev="rev_delay_algo",options="Digital"))
parts.append(mini(508,356,110,64,"DRIVE",fwd="fwd_delay_drive",rev="rev_delay_drive"))
parts.append(mini(622,356,110,64,"WOW RT",fwd="fwd_delay_wow_rate",rev="rev_delay_wow_rate"))
parts.append(mini(508,424,110,64,"WOW DP",fwd="fwd_delay_wow_depth",rev="rev_delay_wow_depth"))
parts.append(mini(622,424,110,64,"SPREAD",fwd="fwd_delay_spread",rev="rev_delay_spread"))
parts.append(combo(508,492,224,32,"Delay Note",fwd="fwd_delay_note",rev="rev_delay_note",options="1/8D"))
parts.append(visual(508,528,224,20,"readout","delay-ms","375 ms - 1/8D @ INT"))
parts.append(btn(508,552,120,32,"Freeze",fwd="fwd_delay_freeze",rev="rev_delay_freeze",tog=True))
parts.append(btn(684,110,48,20,"PWR",fwd="fwd_delay_on",rev="rev_delay_on",tog=True))

# ---- VERB ----
for i,(lb,f,r) in enumerate([("SIZE","fwd_verb_size","rev_verb_size"),("DECAY","fwd_verb_decay","rev_verb_decay"),("DAMP","fwd_verb_damp","rev_verb_damp"),("WIDTH","fwd_verb_width","rev_verb_width")]):
    parts.append(knob(744+i*64,132,64,90,lb,fwd=f,rev=r))
for i,(lb,f,r) in enumerate([("PRE-DLY","fwd_verb_predelay","rev_verb_predelay"),("LO-CUT","fwd_verb_locut","rev_verb_locut"),("HI-CUT","fwd_verb_hicut","rev_verb_hicut"),("MIX","fwd_verb_mix","rev_verb_mix")]):
    parts.append(knob(744+i*64,226,64,90,lb,fwd=f,rev=r))
parts.append(combo(748,324,122,32,"Verb Algo",fwd="fwd_verb_algo",rev="rev_verb_algo",options="Hall"))
parts.append(combo(874,324,122,32,"PreDelay Note",fwd="fwd_verb_predelay_note",rev="rev_verb_predelay_note",options="Free"))
parts.append(visual(748,360,248,20,"readout","verb-t60","T60 2.5 s"))
parts.append(btn(748,384,124,32,"Freeze",fwd="fwd_verb_freeze",rev="rev_verb_freeze",tog=True))
parts.append(btn(948,110,48,20,"PWR",fwd="fwd_verb_on",rev="rev_verb_on",tog=True))
parts.append(visual(752,484,240,120,"irph","ir-placeholder","CONVOLUTION<br>FASE 8"))

# ---- GRAN ----
parts.append(combo(1012,132,126,30,"Gran Mode",fwd="gr_mode",rev="rev_gr_mode",options="Off"))
parts.append(combo(1142,132,126,30,"Gran Trig",fwd="gr_trigger",rev="rev_gr_trigger",options="Chance"))
parts.append(btn(1012,166,126,30,"GRAB",fwd="gr_manual",rev="rev_gr_manual",tog=True))
parts.append(combo(1142,166,126,30,"Gran Len",fwd="gr_len_note",rev="rev_gr_len_note",options="1/16"))
for i,(lb,f,r) in enumerate([("CHANCE","gr_chance","rev_gr_chance"),("THR","gr_env_thr","rev_gr_env_thr"),("REPEATS","gr_repeats","rev_gr_repeats"),("DECAY","gr_decay","rev_gr_decay")]):
    parts.append(knob(1012+i*65,204,64,90,lb,fwd=f,rev=r))
for i,(lb,f,r) in enumerate([("TIME","gr_time","rev_gr_time"),("PITCH","gr_pitch","rev_gr_pitch"),("FLUX","gr_flux","rev_gr_flux")]):
    parts.append(knob(1012+i*65,298,64,90,lb,fwd=f,rev=r))
for i,(lb,f,r) in enumerate([("XFADE","gr_xfade","rev_gr_xfade"),("MIX","gr_mix","rev_gr_mix")]):
    parts.append(knob(1012+i*65,392,64,90,lb,fwd=f,rev=r))
parts.append(btn(1142,392,126,30,"Interrupt",fwd="gr_interrupt",rev="rev_gr_interrupt",tog=True))
parts.append(combo(1142,426,126,30,"Gran Time Note",fwd="gr_time_note",rev="rev_gr_time_note",options="Free"))
parts.append(btn(1220,110,48,20,"PWR",fwd="gr_on",rev="rev_gr_on",tog=True))

body = "\n".join(parts)
panels_html = "\n".join(
    f'<div class="panel {cls}" style="left:{x}px;top:{y}px;width:{w}px;height:{h}px"><div class="phead"><span class="dot" style="background:{ac}"></span>{name}</div><div class="accent" style="background:{ac}"></div></div>'
    for x,y,w,h,cls,name,ac in panels
)

html = f"""<!DOCTYPE html>
<html lang="pt">
<head>
<meta charset="utf-8">
<title>deVerb — esqueleto 1280x624 (UI v2)</title>
<link rel="stylesheet" href="styles.css">
</head>
<body>
<div class="page">
<div class="meta">deVerb UI v2 · esqueleto 1:1 em px · origem (0,0) topo-esq · <b>FWD|REV</b> = uma coluna edita FWD ou REV (data-param-fwd/rev) · globais usam data-param · <span class="mono">assets/mockup/generate.py</span></div>
<div id="stage">
<div class="topbar"></div>
<div class="toplines"></div>
{panels_html}
<div class="widgets">
{body}
</div>
</div>
<div class="legend">Cobertura: 122 params (100 em pares FWD|REV via <span class="mono">data-param-fwd/rev</span> + 22 globais via <span class="mono">data-param</span>). Cinzento = visual/readout. Para propor layout novo, ver <span class="mono">assets/LAYOUT-TEMPLATE.md</span>.</div>
</div>
</body>
</html>
"""

css = """:root{
--bg:#0e0e0e; --border:#2e2e2e; --grey:#d4d4d4; --dim:#8f8f8f; --faint:#5c5c5c;
--acc:#fed134; --track:#383838;
--motor-a:#b5b5b5; --gate-a:#fed134; --delay-a:#58c472; --verb-a:#5aa9e6; --gran-a:#ff9034; --rev-a:#b48ce8;
--motor-p:#151515; --gate-p:#1a1810; --delay-p:#101a12; --verb-p:#10141c; --gran-p:#1c1410;
}
*{box-sizing:border-box}
body{margin:0;background:#0a0a0a;color:var(--grey);font:12px/1.4 system-ui,sans-serif}
.page{padding:12px}
.meta{margin:0 0 8px 0;color:var(--dim)}
.mono{font-family:monospace;color:var(--grey)}
#stage{position:relative;width:1280px;height:624px;background:var(--bg);border:1px solid var(--border);overflow:hidden}
.topbar{position:absolute;left:0;top:0;width:1280px;height:100px;background:#111;border-bottom:1px solid #2a2a2a}
.toplines{position:absolute;left:0;top:35px;width:1280px;height:1px;background:#2a2a2a;box-shadow:0 64px 0 #2a2a2a}
.panel{position:absolute;border-radius:8px;border:1px solid var(--border)}
.panel.motor{background:var(--motor-p)} .panel.gate{background:var(--gate-p)}
.panel.delay{background:var(--delay-p)} .panel.verb{background:var(--verb-p)}
.panel.gran{background:var(--gran-p)}
.phead{position:absolute;left:12px;top:0;height:24px;line-height:24px;font-size:11px;font-weight:700;color:var(--grey);white-space:nowrap}
.phead .dot{display:inline-block;width:8px;height:8px;border-radius:50%;margin-right:6px}
.accent{position:absolute;left:12px;top:21px;width:44px;height:3px}
.widgets>div,.widgets>button{position:absolute}
.knob{display:flex;flex-direction:column;align-items:center;justify-content:flex-start;padding-top:4px;border:1px dashed #333;border-radius:6px;background:rgba(255,255,255,.015)}
.dial{width:44px;height:44px;border-radius:50%;background:#1c1c1c;border:3px solid var(--track);position:relative;flex:none}
.knob[data-size="mini"] .dial{width:30px;height:30px}
.ptr{position:absolute;left:50%;top:50%;width:2px;height:16px;background:var(--grey);transform-origin:50% 0;transform:rotate(35deg) translate(-50%,0)}
.klabel{font-size:10px;color:var(--dim);margin-top:2px;white-space:nowrap}
.kval{font-size:10px;color:var(--faint);background:#000;border:1px solid #222;border-radius:3px;padding:0 6px;margin-top:1px}
.combo{background:#1c1c1c;border:1px solid var(--border);border-radius:4px;display:flex;align-items:center;justify-content:space-between;padding:0 8px;color:var(--grey);font-size:11px}
.combo .clabel{color:var(--faint);font-size:10px;margin-right:6px;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
.combo .cval{white-space:nowrap}
.tbtn{background:#1c1c1c;border:1px solid var(--border);border-radius:4px;color:var(--grey);font-size:11px;cursor:default}
.tbtn.tog[data-param-fwd],.tbtn.tog[data-param]{border-color:#444}
.visual{border:1px dotted #333;border-radius:4px;color:var(--dim);display:flex;align-items:center;justify-content:center;text-align:center;font-size:11px}
.visual.title{justify-content:flex-start;font-size:17px;font-weight:800;color:var(--grey);border:none}
.visual.readout{background:#000;border-style:solid}
.visual.led{border-style:solid;color:var(--gran-a)}
.visual.sect{border:none;color:var(--faint)}
.visual.ruler{background:#141414;border-style:solid}
.visual.irph{color:var(--faint);line-height:1.6}
.step{background:#1c1c1c;border:1px solid var(--border);border-radius:4px;color:var(--grey);font-size:11px}
.step[data-step]{cursor:default}
.legend{margin-top:8px;color:var(--dim);max-width:1280px}
"""

open(os.path.join(HERE,"index.html"),"w").write(html)
open(os.path.join(HERE,"styles.css"),"w").write(css)
print(f"wrote index.html ({len(html)} bytes) + styles.css")
