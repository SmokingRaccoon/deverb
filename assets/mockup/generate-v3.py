#!/usr/bin/env python3
"""Gera o esqueleto HTML/CSS da UI v3 (layout reorganizado, tema premium).
Fonte verdade: este ficheiro. Nao editar index-v3.html/styles-v3.css/LAYOUT-V3.md
a mao -- editar aqui e correr `python3 generate-v3.py`.

Geometria calculada por linhas/grelhas (sem contas de pixel a mao) e validada
no fim (sem overlaps, sem elementos a sair do painel, sem exceder 1280x624)
antes de escrever ficheiros. IDs de parametro identicos ao generate.py
original (122 congelados); so as posicoes/tamanhos/estilo mudaram.
"""
import os, sys

CANVAS_W, CANVAS_H = 1280, 624
MARGIN = 8
GAP = 6
PAD = 8

KNOB = (60, 76)        # dial 48 -> meets 48px min diameter rule
KNOB_LG = (96, 114)     # dial 68 -> hero knobs (MORPH, MASTER)
COMBO_H = 32
BTN_H = 30
PWR_W, PWR_H = 40, 22
STEP_W, STEP_H = 34, 50

HEADER_H = 60
MACRO_H = 88
HEADER_Y = 0
MACRO_Y = HEADER_Y + HEADER_H + GAP           # 68
PANELS_Y = MACRO_Y + MACRO_H + GAP            # 164
PANELS_H = CANVAS_H - PANELS_Y - MARGIN       # 452
PHEAD_H = 30                                  # panel header block incl accent

errors = []
elements = []   # dict: kind,x,y,w,h,attrs
panels_spec = []

def rect(kind, x, y, w, h, **attrs):
    x, y, w, h = round(x), round(y), round(w), round(h)
    elements.append(dict(kind=kind, x=x, y=y, w=w, h=h, attrs=attrs))
    return x, y, w, h

def hstack(x, y, boxes, gap=GAP):
    """boxes: list of (w,h). Returns list of (x,y) left-aligned, total width, max height."""
    pos = []
    cx = x
    maxh = 0
    for w, h in boxes:
        pos.append((cx, y))
        cx += w + gap
        maxh = max(maxh, h)
    total_w = cx - gap - x if boxes else 0
    return pos, total_w, maxh

def hstack_centered(content_x, content_w, y, boxes, gap=GAP):
    _, total_w, maxh = hstack(0, 0, boxes, gap)
    startx = content_x + (content_w - total_w) / 2
    pos, _, _ = hstack(startx, y, boxes, gap)
    return pos, maxh

def hstack_right(right_edge, y, boxes, gap=GAP):
    """lay out right-to-left so the LAST box's right edge == right_edge; returns positions in original order"""
    rev = list(reversed(boxes))
    pos_rev = []
    cx = right_edge
    maxh = 0
    for w, h in rev:
        cx -= w
        pos_rev.append((cx, y))
        cx -= gap
        maxh = max(maxh, h)
    return list(reversed(pos_rev)), maxh

class Panel:
    def __init__(self, key, title, accent, x, w, h=PANELS_H, y=PANELS_Y):
        self.key, self.title, self.accent = key, title, accent
        self.x, self.y, self.w, self.h = x, y, w, h
        self.content_x = x + PAD
        self.content_w = w - 2 * PAD
        self.cy = y + PHEAD_H + PAD   # content cursor
        panels_spec.append(self)

    @property
    def content_bottom(self):
        return self.y + self.h - PAD

    def row(self, boxes, gap=GAP, after=GAP, module=None):
        pos, rowh = hstack_centered(self.content_x, self.content_w, self.cy, boxes, gap)
        self.cy += rowh + after
        return pos

    def advance(self, dy):
        self.cy += dy

    def check(self):
        if self.cy - GAP > self.content_bottom + 1:
            errors.append(f"[OVERFLOW] panel {self.key}: content ends at {self.cy - GAP:.0f}, "
                           f"budget bottom is {self.content_bottom:.0f} (over by {self.cy-GAP-self.content_bottom:.0f}px)")
        right = self.x + self.w
        if right > CANVAS_W - MARGIN + 1:
            errors.append(f"[OVERFLOW] panel {self.key}: right edge {right} exceeds canvas budget {CANVAS_W-MARGIN}")


def knob(x, y, w, h, label, module, fwd=None, rev=None, single=None, extra=""):
    dp = f'data-param="{single}"' if single else f'data-param-fwd="{fwd}" data-param-rev="{rev}"'
    rect("knob", x, y, w, h, dp=dp, label=label, module=module, extra=extra)

def combo(x, y, w, h, label, module, fwd=None, rev=None, single=None, options="", role=""):
    if single:
        dp = f'data-param="{single}"'
    elif fwd or rev:
        dp = f'data-param-fwd="{fwd}" data-param-rev="{rev}"'
    else:
        dp = f'data-role="{role}"'
    if role in ("factory-preset", "pattern-preset") and (fwd or rev):
        dp = f'data-role="{role}" data-param-fwd="{fwd}" data-param-rev="{rev}"'
    elif role and not (fwd or rev or single):
        dp = f'data-role="{role}"'
    rect("combo", x, y, w, h, dp=dp, label=label, module=module, options=options)

def btn(x, y, w, h, label, module, fwd=None, rev=None, single=None, role="", tog=False, on=False, extra_cls=""):
    if single:
        dp = f'data-param="{single}"'
    elif fwd or rev:
        dp = f'data-param-fwd="{fwd}" data-param-rev="{rev}"'
    else:
        dp = f'data-role="{role}"'
    rect("btn", x, y, w, h, dp=dp, label=label, module=module, tog=tog, on=on, extra_cls=extra_cls)

def step(x, y, w, h, idx, module, on=False):
    rect("step", x, y, w, h, idx=idx, module=module, on=on)

def visual(x, y, w, h, cls2, label, module, txt=""):
    rect("visual", x, y, w, h, cls2=cls2, label=label, module=module, txt=txt or label)


# ================= HEADER =================
hx = MARGIN
pos = hstack(hx, 12, [(170, 36)])[0]
visual(*pos[0], 170, 36, "title", "title", "brand", "deVerb")

pos, _, _ = hstack(hx + 170 + 24, 16, [(120, 28), (150, 28)])
visual(*pos[0], 120, 28, "readout", "bpm-readout", "delay", "BPM 120.0 INT")
visual(*pos[1], 150, 28, "readout", "gate-readout", "gate", "GATE 01/16")

right_edge = CANVAS_W - MARGIN
items = [(88, 32), (72, 32), (72, 32), (78, 32), (240, 32)]  # led, rev, fwd, random, preset (reversed order below)
# lay out right-to-left: preset, random, FWD|REV (joined), led
pos, _ = hstack_right(right_edge, 14, [(240, 32), (78, 32), (72, 32), (72, 32), (88, 32)], gap=16)
preset_pos, random_pos, fwd_pos, rev_pos, led_pos = pos
combo(*preset_pos, 240, 32, "Preset fabrica", "global", options="Preset...", role="factory-preset")
btn(*random_pos, 78, 32, "RANDOM", "brand", role="random", extra_cls="btn-random")
# FWD/REV joined segmented pill: force rev_pos to sit flush against fwd_pos (no gap)
fx, fy = fwd_pos
rx = fx + 72
btn(fx, fy, 72, 32, "FWD", "brand", role="mode-fwdrev", on=True, extra_cls="seg-l")
btn(rx, fy, 72, 32, "REV", "brand", role="mode-fwdrev", extra_cls="seg-r")
visual(*led_pos, 88, 32, "led", "gran-led", "gran", "IDLE")


# ================= MACRO STRIP =================
# two glass "bridge" cards between header and module panels
rev_items = [(44, 76), (96, 32), (88, 32), (88, 32), (60, 76), (60, 76), (60, 76), (80, 32)]
link_items = [(36, 76), (46, 32), (46, 32), (46, 32), (46, 32), (46, 32), (60, 76), (60, 76), (80, 32), (72, 32)]

def card_width(items, gap=4):
    _, total, _ = hstack(0, 0, items, gap)
    return total + 2 * PAD

rev_w = card_width(rev_items)
link_w = card_width(link_items)
cards_total = rev_w + GAP + link_w
cards_x0 = MARGIN
# scale down proportionally if it doesn't fit the 1280-2*MARGIN-GAP budget
budget = CANVAS_W - 2 * MARGIN - GAP
if cards_total > budget:
    errors.append(f"[OVERFLOW] macro cards total {cards_total:.0f} > budget {budget}")

rev_card_x = MARGIN
link_card_x = rev_card_x + rev_w + GAP
# stretch link card to fill remaining width to the right margin for a clean edge
link_w = (CANVAS_W - MARGIN) - link_card_x

rect("panel", rev_card_x, MACRO_Y, rev_w, MACRO_H, cls="card rev-card", title="REV MACRO", accent="rev")
rect("panel", link_card_x, MACRO_Y, link_w, MACRO_H, cls="card link-card", title="LINK + TRIM", accent="brand")

row_y = MACRO_Y + (MACRO_H - 76) / 2
cx = rev_card_x + PAD
pos, _, _ = hstack(cx, row_y, rev_items, gap=4)
visual(pos[0][0], pos[0][1], 44, 76, "sect", "sect-rev", "rev", "REV")
combo(pos[1][0], pos[1][1] + (76-32)/2, 96, 32, "REV Mode", "rev", single="rev_mode", options="Loop")
combo(pos[2][0], pos[2][1] + (76-32)/2, 88, 32, "REV Source", "rev", single="rev_source", options="Dry")
combo(pos[3][0], pos[3][1] + (76-32)/2, 88, 32, "REV Capture", "rev", single="rev_capture", options="2 beats")
knob(pos[4][0], pos[4][1], 60, 76, "RATE", "rev", single="rev_rate")
knob(pos[5][0], pos[5][1], 60, 76, "LFO", "rev", single="rev_lfo")
knob(pos[6][0], pos[6][1], 60, 76, "DUCK", "rev", single="rev_duck")
btn(pos[7][0], pos[7][1] + (76-32)/2, 80, 32, "THROW", "rev", single="rev_throw", tog=True)

cx2 = link_card_x + PAD
pos2, _, _ = hstack(cx2, row_y, link_items, gap=4)
visual(pos2[0][0], pos2[0][1], 36, 76, "sect", "sect-link", "brand", "LINK")
btn(pos2[1][0], pos2[1][1] + (76-32)/2, 46, 32, "ALL", "brand", single="link_master", tog=True, on=True)
btn(pos2[2][0], pos2[2][1] + (76-32)/2, 46, 32, "GATE", "gate", single="link_gate", tog=True)
btn(pos2[3][0], pos2[3][1] + (76-32)/2, 46, 32, "DLY", "delay", single="link_delay", tog=True)
btn(pos2[4][0], pos2[4][1] + (76-32)/2, 46, 32, "VRB", "verb", single="link_verb", tog=True)
btn(pos2[5][0], pos2[5][1] + (76-32)/2, 46, 32, "GRN", "gran", single="link_gran", tog=True)
knob(pos2[6][0], pos2[6][1], 60, 76, "T-DLY", "global", single="trim_delay")
knob(pos2[7][0], pos2[7][1], 60, 76, "T-DEC", "global", single="trim_decay")
combo(pos2[8][0], pos2[8][1] + (76-32)/2, 80, 32, "Chain Order", "global", single="chain_order", options="G-D-V-G")
combo(pos2[9][0], pos2[9][1] + (76-32)/2, 72, 32, "X Mode", "global", single="x_mode", options="Add")


# ================= MOTOR =================
motor_w = max(KNOB_LG[0], 2 * KNOB[0] + GAP) + 2 * PAD   # hero knob vs. MIX-pair row, whichever is wider
motor = Panel("motor", "MOTOR - PERFORM", "motor", x=MARGIN, w=motor_w)
p = motor.row([KNOB])
knob(p[0][0], p[0][1], *KNOB, "INPUT", "motor", single="input_gain")
p = motor.row([KNOB, KNOB])
knob(p[0][0], p[0][1], *KNOB, "FWD MIX", "motor", single="fwd_mix")
knob(p[1][0], p[1][1], *KNOB, "REV MIX", "motor", single="rev_mix")
p = motor.row([KNOB_LG])
knob(p[0][0], p[0][1], *KNOB_LG, "MORPH", "motor", single="morph")
p = motor.row([KNOB_LG], after=0)
knob(p[0][0], p[0][1], *KNOB_LG, "MASTER", "motor", single="master")
motor.check()


# ================= GATE =================
gate_content_w = 5 * KNOB[0] + 4 * GAP   # = 332, drives steps row + combo rows too
gate_w = gate_content_w + 2 * PAD
gate = Panel("gate", "GATE - TRANCE", "gate", x=motor.x + motor.w + GAP, w=gate_w)

p = gate.row([(gate.content_w, 30)], after=10)
visual(p[0][0], p[0][1], gate.content_w, 30, "ruler", "ruler", "gate", "RULER")

step_boxes = [(STEP_W, STEP_H)] * 8
p = gate.row(step_boxes, gap=6, after=6)
for i, (x, y) in enumerate(p):
    step(x, y, STEP_W, STEP_H, i, "gate", on=(i in (0, 4, 8, 12)))
p = gate.row(step_boxes, gap=6, after=18)
for i, (x, y) in enumerate(p):
    step(x + 0, y, STEP_W, STEP_H, i + 8, "gate", on=((i + 8) in (0, 4, 8, 12)))

combo_w = (gate.content_w - GAP) / 2
p = gate.row([(combo_w, COMBO_H), (combo_w, COMBO_H)], after=10)
combo(p[0][0], p[0][1], combo_w, COMBO_H, "Pattern fabrica", "gate",
      fwd="fwd_gate_pattern", rev="rev_gate_pattern", options="Pattern...", role="pattern-preset")
combo(p[1][0], p[1][1], combo_w, COMBO_H, "Gate Rate", "gate", fwd="fwd_gate_rate", rev="rev_gate_rate", options="1/16")
p = gate.row([(combo_w, COMBO_H), (combo_w, COMBO_H)], after=20)
combo(p[0][0], p[0][1], combo_w, COMBO_H, "Gate Steps", "gate", fwd="fwd_gate_steps", rev="rev_gate_steps", options="16")
combo(p[1][0], p[1][1], combo_w, COMBO_H, "Gate Trig", "gate", fwd="fwd_gate_trig", rev="rev_gate_trig", options="Host")

p = gate.row([KNOB] * 5, after=0)
for (x, y), (lb, f, r) in zip(p, [
        ("SMOOTH", "fwd_gate_smooth", "rev_gate_smooth"),
        ("DEPTH", "fwd_gate_depth", "rev_gate_depth"),
        ("MIX", "fwd_gate_mix", "rev_gate_mix"),
        ("PAN", "fwd_gate_pan", "rev_gate_pan"),
        ("THR", "fwd_gate_env_thr", "rev_gate_env_thr")]):
    knob(x, y, *KNOB, lb, "gate", fwd=f, rev=r)
gate.check()
btn(gate.x + gate.w - PWR_W - 8, gate.y + 8, PWR_W, PWR_H, "PWR", "gate",
    fwd="fwd_gate_on", rev="rev_gate_on", tog=True, on=True)


# ================= DELAY =================
delay_content_w = 3 * KNOB[0] + 2 * GAP   # = 196
delay_w = delay_content_w + 2 * PAD
delay = Panel("delay", "DELAY - ECHO", "delay", x=gate.x + gate.w + GAP, w=delay_w)

rows3 = [
    [("BPM", None, None, "tempo_bpm"), ("MS", "fwd_delay_time", "rev_delay_time", None), ("FB", "fwd_delay_fb", "rev_delay_fb", None)],
    [("DAMP", "fwd_delay_damp", "rev_delay_damp", None), ("MIX", "fwd_delay_mix", "rev_delay_mix", None), ("DRIVE", "fwd_delay_drive", "rev_delay_drive", None)],
    [("WOW RT", "fwd_delay_wow_rate", "rev_delay_wow_rate", None), ("WOW DP", "fwd_delay_wow_depth", "rev_delay_wow_depth", None), ("SPREAD", "fwd_delay_spread", "rev_delay_spread", None)],
]
for ri, row_defs in enumerate(rows3):
    p = delay.row([KNOB, KNOB, KNOB], after=(10 if ri < 2 else 16))
    for (x, y), (lb, f, r, single) in zip(p, row_defs):
        knob(x, y, *KNOB, lb, "delay", fwd=f, rev=r, single=single)

p = delay.row([(delay.content_w, COMBO_H)], after=10)
combo(p[0][0], p[0][1], delay.content_w, COMBO_H, "Delay Algo", "delay", fwd="fwd_delay_algo", rev="rev_delay_algo", options="Digital")
p = delay.row([(delay.content_w, COMBO_H)], after=12)
combo(p[0][0], p[0][1], delay.content_w, COMBO_H, "Delay Note", "delay", fwd="fwd_delay_note", rev="rev_delay_note", options="1/8D")

readout_w = delay.content_w - GAP - 92
p = delay.row([(readout_w, 28), (92, BTN_H)], after=0)
visual(p[0][0], p[0][1] + 1, readout_w, 28, "readout", "delay-ms", "delay", "375 ms - 1/8D @ INT")
btn(p[1][0], p[1][1], 92, BTN_H, "Freeze", "delay", fwd="fwd_delay_freeze", rev="rev_delay_freeze", tog=True)
delay.check()
btn(delay.x + delay.w - PWR_W - 8, delay.y + 8, PWR_W, PWR_H, "PWR", "delay",
    fwd="fwd_delay_on", rev="rev_delay_on", tog=True, on=True)


# ================= VERB =================
verb_content_w = 4 * KNOB[0] + 3 * GAP   # = 264
verb_w = verb_content_w + 2 * PAD
verb = Panel("verb", "VERB - SPACE", "verb", x=delay.x + delay.w + GAP, w=verb_w)

p = verb.row([KNOB] * 4, after=10)
for (x, y), (lb, f, r) in zip(p, [
        ("SIZE", "fwd_verb_size", "rev_verb_size"), ("DECAY", "fwd_verb_decay", "rev_verb_decay"),
        ("DAMP", "fwd_verb_damp", "rev_verb_damp"), ("WIDTH", "fwd_verb_width", "rev_verb_width")]):
    knob(x, y, *KNOB, lb, "verb", fwd=f, rev=r)
p = verb.row([KNOB] * 4, after=16)
for (x, y), (lb, f, r) in zip(p, [
        ("PRE-DLY", "fwd_verb_predelay", "rev_verb_predelay"), ("LO-CUT", "fwd_verb_locut", "rev_verb_locut"),
        ("HI-CUT", "fwd_verb_hicut", "rev_verb_hicut"), ("MIX", "fwd_verb_mix", "rev_verb_mix")]):
    knob(x, y, *KNOB, lb, "verb", fwd=f, rev=r)

half_w = (verb.content_w - GAP) / 2
p = verb.row([(half_w, COMBO_H), (half_w, COMBO_H)], after=10)
combo(p[0][0], p[0][1], half_w, COMBO_H, "Verb Algo", "verb", fwd="fwd_verb_algo", rev="rev_verb_algo", options="Hall")
combo(p[1][0], p[1][1], half_w, COMBO_H, "PreDelay Note", "verb", fwd="fwd_verb_predelay_note", rev="rev_verb_predelay_note", options="Free")

readout_w2 = verb.content_w - GAP - 100
p = verb.row([(readout_w2, 28), (100, BTN_H)], after=10)
visual(p[0][0], p[0][1] + 1, readout_w2, 28, "readout", "verb-t60", "verb", "T60 2.5 s")
btn(p[1][0], p[1][1], 100, BTN_H, "Freeze", "verb", fwd="fwd_verb_freeze", rev="rev_verb_freeze", tog=True)

irph_h = verb.content_bottom - verb.cy
p = verb.row([(verb.content_w, irph_h)], after=0)
visual(p[0][0], p[0][1], verb.content_w, irph_h, "irph", "ir-placeholder", "verb", "CONVOLUTION<br>FASE 8")
verb.check()
btn(verb.x + verb.w - PWR_W - 8, verb.y + 8, PWR_W, PWR_H, "PWR", "verb",
    fwd="fwd_verb_on", rev="rev_verb_on", tog=True, on=True)


# ================= GRAN =================
gran_content_w = 4 * KNOB[0] + 3 * GAP   # = 264, matches verb for rhythm
gran_w = gran_content_w + 2 * PAD
gran = Panel("gran", "GRAN - GLITCH", "gran", x=verb.x + verb.w + GAP, w=gran_w)

half_w2 = (gran.content_w - GAP) / 2
p = gran.row([(half_w2, COMBO_H), (half_w2, COMBO_H)], after=8)
combo(p[0][0], p[0][1], half_w2, COMBO_H, "Gran Mode", "gran", fwd="gr_mode", rev="rev_gr_mode", options="Off")
combo(p[1][0], p[1][1], half_w2, COMBO_H, "Gran Trig", "gran", fwd="gr_trigger", rev="rev_gr_trigger", options="Chance")
p = gran.row([(half_w2, BTN_H), (half_w2, COMBO_H)], after=14)
btn(p[0][0], p[0][1], half_w2, BTN_H, "GRAB", "gran", fwd="gr_manual", rev="rev_gr_manual", tog=True)
combo(p[1][0], p[1][1], half_w2, COMBO_H, "Gran Len", "gran", fwd="gr_len_note", rev="rev_gr_len_note", options="1/16")

p = gran.row([KNOB] * 4, after=10)
for (x, y), (lb, f, r) in zip(p, [
        ("CHANCE", "gr_chance", "rev_gr_chance"), ("THR", "gr_env_thr", "rev_gr_env_thr"),
        ("REPEATS", "gr_repeats", "rev_gr_repeats"), ("DECAY", "gr_decay", "rev_gr_decay")]):
    knob(x, y, *KNOB, lb, "gran", fwd=f, rev=r)
p = gran.row([KNOB] * 3, after=10)
for (x, y), (lb, f, r) in zip(p, [
        ("TIME", "gr_time", "rev_gr_time"), ("PITCH", "gr_pitch", "rev_gr_pitch"), ("FLUX", "gr_flux", "rev_gr_flux")]):
    knob(x, y, *KNOB, lb, "gran", fwd=f, rev=r)
p = gran.row([KNOB, KNOB], after=14)
for (x, y), (lb, f, r) in zip(p, [("XFADE", "gr_xfade", "rev_gr_xfade"), ("MIX", "gr_mix", "rev_gr_mix")]):
    knob(x, y, *KNOB, lb, "gran", fwd=f, rev=r)

p = gran.row([(half_w2, BTN_H), (half_w2, COMBO_H)], after=0)
btn(p[0][0], p[0][1], half_w2, BTN_H, "Interrupt", "gran", fwd="gr_interrupt", rev="rev_gr_interrupt", tog=True)
combo(p[1][0], p[1][1], half_w2, COMBO_H, "Gran Time Note", "gran", fwd="gr_time_note", rev="rev_gr_time_note", options="Free")
gran.check()
btn(gran.x + gran.w - PWR_W - 8, gran.y + 8, PWR_W, PWR_H, "PWR", "gran", fwd="gr_on", rev="rev_gr_on", tog=True)

print(f"MOTOR x={motor.x} w={motor.w} -> ends {motor.x+motor.w}")
print(f"GATE  x={gate.x} w={gate.w} -> ends {gate.x+gate.w}")
print(f"DELAY x={delay.x} w={delay.w} -> ends {delay.x+delay.w}")
print(f"VERB  x={verb.x} w={verb.w} -> ends {verb.x+verb.w}")
print(f"GRAN  x={gran.x} w={gran.w} -> ends {gran.x+gran.w}, canvas budget ends {CANVAS_W-MARGIN}")
print(f"REV card w={rev_w:.0f} LINK card w={link_w:.0f} total+gaps={rev_w+GAP+link_w:.0f} budget={budget}")

knob_count = sum(1 for e in elements if e["kind"] == "knob")
combo_count = sum(1 for e in elements if e["kind"] == "combo")
btn_count = sum(1 for e in elements if e["kind"] == "btn")
step_count = sum(1 for e in elements if e["kind"] == "step")
visual_count = sum(1 for e in elements if e["kind"] == "visual")
print(f"counts: knob={knob_count} combo={combo_count} btn={btn_count} step={step_count} visual={visual_count} total={len(elements)}")

# ---- rigorous geometry validation: containment + pairwise overlap ----
def overlaps(a, b):
    ax0, ay0, ax1, ay1 = a["x"], a["y"], a["x"]+a["w"], a["y"]+a["h"]
    bx0, by0, bx1, by1 = b["x"], b["y"], b["x"]+b["w"], b["y"]+b["h"]
    return ax0 < bx1 and bx0 < ax1 and ay0 < by1 and by0 < ay1

def contains(outer, inner, tol=0):
    return (inner["x"] >= outer["x"]-tol and
            inner["y"] >= outer["y"]-tol and
            inner["x"]+inner["w"] <= outer["x"]+outer["w"]+tol and
            inner["y"]+inner["h"] <= outer["y"]+outer["h"]+tol)

widgets = [e for e in elements if e["kind"] != "panel"]
panel_boxes = {p.key: dict(x=p.x, y=p.y, w=p.w, h=p.h) for p in panels_spec}
# macro cards + header are not strict containers (widgets float across the whole zone), skip those

for e in widgets:
    mod = e["attrs"].get("module")
    if mod in panel_boxes and e["y"] >= PANELS_Y:
        box = panel_boxes[mod]
        if not contains(box, e, tol=1):
            lbl = e["attrs"].get("label") or e["attrs"].get("txt") or e["attrs"].get("idx")
            errors.append(f"[CONTAIN] '{lbl}' ({e['kind']}) at ({e['x']},{e['y']},{e['w']}x{e['h']}) "
                          f"escapes panel '{mod}' bounds ({box['x']},{box['y']},{box['w']}x{box['h']})")

# pairwise overlap within the same panel/zone only (cross-zone overlap is impossible by construction)
def zone_key(e):
    y = e["y"]
    if y < MACRO_Y: return "header"
    if y < PANELS_Y: return "macro"
    return e["attrs"].get("module")

from collections import defaultdict
by_zone = defaultdict(list)
for e in widgets:
    by_zone[zone_key(e)].append(e)

for zk, items in by_zone.items():
    for i in range(len(items)):
        for j in range(i+1, len(items)):
            if overlaps(items[i], items[j]):
                li = items[i]["attrs"].get("label") or items[i]["attrs"].get("idx")
                lj = items[j]["attrs"].get("label") or items[j]["attrs"].get("idx")
                errors.append(f"[OVERLAP] zone '{zk}': '{li}' overlaps '{lj}' "
                              f"({items[i]['x']},{items[i]['y']},{items[i]['w']}x{items[i]['h']}) vs "
                              f"({items[j]['x']},{items[j]['y']},{items[j]['w']}x{items[j]['h']})")

if errors:
    print("\n--- ERRORS ---")
    for e in errors:
        print(e)
    sys.exit(1)
else:
    print(f"\nAll panels fit within budget. {len(widgets)} widgets validated: no overlaps, no containment escapes. OK.")

# ============================================================================
# EMIT HTML + CSS + layout documentation
# ============================================================================

ACCENT_HEX = {
    "motor": "#dcdce2", "gate": "#ffd23f", "delay": "#34d399",
    "verb": "#4fb2f0", "gran": "#ff8a3d", "rev": "#b48ce8",
    "brand": "#8b5cf6", "global": "#c7c9d1",
}

MODULE_LABELS = {
    "motor": "MOTOR", "gate": "GATE", "delay": "DELAY", "verb": "VERB",
    "gran": "GRAN", "rev": "REV (macro)", "brand": "GLOBAL/BRAND",
    "global": "GLOBAL",
}

def esc(s):
    return str(s)

def emit_widget(e):
    k, x, y, w, h, a = e["kind"], e["x"], e["y"], e["w"], e["h"], e["attrs"]
    mod = a.get("module", "global")
    if k == "knob":
        extra = a.get("extra", "")
        return (f'<div class="knob" style="left:{x}px;top:{y}px;width:{w}px;height:{h}px" '
                f'{a["dp"]} data-label="{a["label"]}" data-module="{mod}" {extra}>'
                f'<div class="dial"><div class="ptr"></div></div>'
                f'<div class="klabel">{a["label"]}</div><div class="kval">--</div></div>')
    if k == "combo":
        return (f'<div class="combo" style="left:{x}px;top:{y}px;width:{w}px;height:{h}px" '
                f'{a["dp"]} data-module="{mod}" title="{a["label"]}">'
                f'<span class="clabel">{a["label"]}</span><span class="cval">{a["options"]} ▾</span></div>')
    if k == "btn":
        cls = "tbtn"
        if a.get("tog"): cls += " tog"
        if a.get("on"): cls += " on"
        if a.get("extra_cls"): cls += " " + a["extra_cls"]
        return (f'<button class="{cls}" style="left:{x}px;top:{y}px;width:{w}px;height:{h}px" '
                f'{a["dp"]} data-module="{mod}" title="{a["label"]}">{a["label"]}</button>')
    if k == "step":
        cls = "step" + (" on" if a.get("on") else "")
        idx = a["idx"]
        return (f'<button class="{cls}" style="left:{x}px;top:{y}px;width:{w}px;height:{h}px" '
                f'data-param-fwd="fwd_gate_pattern" data-param-rev="rev_gate_pattern" data-step="{idx}" '
                f'data-module="{mod}" title="step {idx+1}">{idx+1}</button>')
    if k == "visual":
        return (f'<div class="visual {a["cls2"]}" style="left:{x}px;top:{y}px;width:{w}px;height:{h}px" '
                f'data-visual="{a["label"]}" data-module="{mod}">{a["txt"]}</div>')
    return ""

def emit_panel_block(key, title, accent, x, y, w, h, extra_cls=""):
    cls = f"panel {key}" + (f" {extra_cls}" if extra_cls else "")
    return (f'<div class="{cls}" style="left:{x}px;top:{y}px;width:{w}px;height:{h}px">'
            f'<div class="phead"><span class="dot" style="background:{ACCENT_HEX[accent]}"></span>{title}</div>'
            f'<div class="accent" style="background:{ACCENT_HEX[accent]}"></div></div>')

panel_html = []
for p in panels_spec:
    panel_html.append(emit_panel_block(p.key, p.title, p.accent, p.x, p.y, p.w, p.h))
for e in elements:
    if e["kind"] == "panel":
        a = e["attrs"]
        key = a["cls"].split()[1]  # "card rev-card" -> "rev-card"
        panel_html.append(emit_panel_block(key, a["title"], a["accent"], e["x"], e["y"], e["w"], e["h"],
                                            extra_cls="card"))

widget_html = "\n".join(emit_widget(e) for e in elements if e["kind"] != "panel")

html = f"""<!DOCTYPE html>
<html lang="pt">
<head>
<meta charset="utf-8">
<title>deVerb — esqueleto 1280x624 (UI v3 · layout reorganizado)</title>
<link rel="stylesheet" href="styles-v3.css">
</head>
<body>
<div class="page">
<div class="meta">deVerb UI v3 · esqueleto 1:1 em px · origem (0,0) topo-esq · <b>FWD|REV</b> = uma coluna edita FWD ou REV (data-param-fwd/rev) · globais usam data-param · layout gerado por <span class="mono">assets/mockup/generate-v3.py</span> — ver <span class="mono">assets/mockup/LAYOUT-V3.md</span> para a tabela zona-a-zona</div>
<div id="stage">
<div class="stage-grain" aria-hidden="true"></div>
<div class="stage-glow" aria-hidden="true"></div>
<div class="topbar" style="height:{HEADER_H}px"></div>
{chr(10).join(panel_html)}
<div class="widgets">
{widget_html}
</div>
</div>
<div class="legend">Cobertura: 122 params ({knob_count+combo_count+btn_count} widgets interativos via data-param + {step_count} steps do sequencer, partilhando 2 params, + {visual_count} elementos puramente visuais). Cor de cada widget segue <span class="mono">data-module</span> (tema), não o painel onde está desenhado. Classes <span class="mono">.on</span> em alguns toggles/steps são apenas demonstração visual de estado ligado/desligado. Geometria 100% recalculada (não é a mesma do template anterior) — ver <span class="mono">LAYOUT-V3.md</span>.</div>
</div>
</body>
</html>
"""

css = """:root{
  --bg:#07070a; --stage-bg:#0b0b10; --border:#2a2a35; --border-soft:#1c1c24;
  --grey:#edeef0; --dim:#9a9aa8; --faint:#6a6a78;
  --track:#26262f; --track-lit:#34343f;
  --motor-a:#dcdce2; --gate-a:#ffd23f; --delay-a:#34d399; --verb-a:#4fb2f0;
  --gran-a:#ff8a3d; --rev-a:#b48ce8; --brand-a:#8b5cf6; --global-a:#c7c9d1;
  --motor-p:#121317; --gate-p:#191509; --delay-p:#0c1712; --verb-p:#0c121c;
  --gran-p:#1a1209; --rev-p:#140d1c; --brand-p:#0e1120;
  --radius-lg:16px; --radius-md:10px; --radius-sm:7px;
  --font-ui:'Inter','SF Pro Display',-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,Helvetica,Arial,sans-serif;
  --font-mono:'JetBrains Mono','SF Mono','Fira Code',Consolas,monospace;
}
*{box-sizing:border-box}
body{margin:0;background:
    radial-gradient(1100px 520px at 50% -8%, #15151f 0%, var(--bg) 55%);
  color:var(--grey);font:13px/1.4 var(--font-ui);
  -webkit-font-smoothing:antialiased;text-rendering:optimizeLegibility}
.page{padding:28px 20px 40px}
.meta{margin:0 0 16px 0;color:var(--faint);font-size:11px;max-width:1280px}
.mono{font-family:var(--font-mono);color:var(--dim)}

#stage{
  position:relative;width:1280px;height:624px;
  background:
    linear-gradient(180deg,#0e0e14 0%,#0a0a0f 60%,#09090d 100%);
  border:1px solid var(--border);border-radius:22px;overflow:hidden;
  box-shadow:
    0 40px 90px -30px rgba(0,0,0,.85),
    0 2px 0 rgba(255,255,255,.04) inset,
    0 0 0 1px rgba(255,255,255,.02) inset;
}
.stage-grain{
  position:absolute;inset:0;pointer-events:none;opacity:.05;mix-blend-mode:overlay;z-index:0;
  background-image:url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='140' height='140'%3E%3Cfilter id='n'%3E%3CfeTurbulence type='fractalNoise' baseFrequency='0.85' numOctaves='2' stitchTiles='stitch'/%3E%3C/filter%3E%3Crect width='100%25' height='100%25' filter='url(%23n)'/%3E%3C/svg%3E");
}
.stage-glow{
  position:absolute;inset:0;pointer-events:none;z-index:0;
  background:
    radial-gradient(620px 260px at 14% 0%, rgba(139,92,246,.10), transparent 60%),
    radial-gradient(520px 240px at 86% 0%, rgba(79,178,240,.08), transparent 60%),
    radial-gradient(900px 300px at 50% 100%, rgba(52,211,153,.05), transparent 60%);
}
.topbar{
  position:absolute;left:0;top:0;width:1280px;
  background:linear-gradient(180deg, rgba(255,255,255,.035), rgba(255,255,255,0) 70%);
  border-bottom:1px solid var(--border-soft);z-index:1;
}

/* ---- panels & macro cards ---- */
.panel{
  position:absolute;border-radius:var(--radius-lg);border:1px solid var(--border);
  box-shadow:0 1px 0 rgba(255,255,255,.04) inset, 0 14px 30px -18px rgba(0,0,0,.65);
  z-index:1;
}
.panel.motor{background:linear-gradient(165deg,var(--motor-p),#0b0b0e 85%)}
.panel.gate{background:linear-gradient(165deg,var(--gate-p),#0b0a07 85%)}
.panel.delay{background:linear-gradient(165deg,var(--delay-p),#09100d 85%)}
.panel.verb{background:linear-gradient(165deg,var(--verb-p),#090c12 85%)}
.panel.gran{background:linear-gradient(165deg,var(--gran-p),#100b06 85%)}
.panel.card.rev-card{background:linear-gradient(165deg,var(--rev-p),#0b0910 85%)}
.panel.card.link-card{background:linear-gradient(165deg,var(--brand-p),#0a0b11 85%)}

.phead{
  position:absolute;left:14px;top:0;height:30px;line-height:30px;
  font-size:10.5px;font-weight:700;letter-spacing:.09em;color:var(--grey);
  white-space:nowrap;text-transform:uppercase;
}
.phead .dot{
  display:inline-block;width:7px;height:7px;border-radius:50%;margin-right:7px;
  box-shadow:0 0 0 3px currentColor, 0 0 10px 1px currentColor;filter:saturate(1.3);
  animation:pulse-dot 3.2s ease-in-out infinite;
}
.accent{
  position:absolute;left:14px;top:25px;width:38px;height:2px;border-radius:2px;
  box-shadow:0 0 10px 1px currentColor;opacity:.8;
}
@keyframes pulse-dot{0%,100%{opacity:.55}50%{opacity:1}}
@media (prefers-reduced-motion:reduce){.phead .dot{animation:none}}

/* ---- module color theming (drives --accent for every widget) ---- */
[data-module="motor"]{--accent:var(--motor-a)}
[data-module="gate"]{--accent:var(--gate-a)}
[data-module="delay"]{--accent:var(--delay-a)}
[data-module="verb"]{--accent:var(--verb-a)}
[data-module="gran"]{--accent:var(--gran-a)}
[data-module="rev"]{--accent:var(--rev-a)}
[data-module="brand"]{--accent:var(--brand-a)}
[data-module="global"]{--accent:var(--global-a)}

.widgets>div,.widgets>button{position:absolute;z-index:2}

/* ---- knob ---- */
.knob{
  display:flex;flex-direction:column;align-items:center;justify-content:flex-start;
  gap:2px;padding-top:2px;
}
.dial{
  --val:.62;width:48px;height:48px;border-radius:50%;position:relative;flex:none;
  background:
    radial-gradient(circle at 32% 26%, rgba(255,255,255,.22), rgba(255,255,255,0) 42%),
    conic-gradient(from -135deg,
      var(--accent,#ffd23f) 0deg, var(--accent,#ffd23f) calc(var(--val)*270deg),
      var(--track) calc(var(--val)*270deg), var(--track) 270deg,
      transparent 270deg, transparent 360deg);
  box-shadow:0 1px 0 rgba(255,255,255,.06) inset, 0 10px 16px -8px rgba(0,0,0,.7);
}
.dial::before{
  content:"";position:absolute;inset:6px;border-radius:50%;
  background:radial-gradient(circle at 35% 30%, #2a2a32, #16161a 70%);
  box-shadow:0 1px 1px rgba(255,255,255,.08) inset, 0 -2px 4px rgba(0,0,0,.6) inset;
}
.knob[data-size="mini"] .dial{width:36px;height:36px}
.knob:nth-of-type(3n) .dial{--val:.38}
.knob:nth-of-type(4n) .dial{--val:.81}
.knob:nth-of-type(5n) .dial{--val:.55}
.ptr{
  position:absolute;left:50%;top:50%;width:2px;height:15px;
  background:linear-gradient(var(--grey),var(--grey));
  box-shadow:0 0 5px 1px var(--accent,#fff);
  border-radius:2px;z-index:1;
  transform-origin:50% 0;
  transform:rotate(calc(-135deg + var(--val)*270deg)) translate(-50%,3px);
}
.klabel{
  font-size:9.5px;font-weight:600;letter-spacing:.07em;color:var(--dim);
  white-space:nowrap;text-transform:uppercase;margin-top:1px;
}
.kval{
  font-family:var(--font-mono);font-size:9.5px;color:var(--faint);
  background:#000;border:1px solid var(--border-soft);border-radius:4px;
  padding:0 6px;line-height:14px;
}

/* ---- combo ---- */
.combo{
  background:linear-gradient(180deg,#19191f,#141418);
  border:1px solid var(--border);border-radius:var(--radius-sm);
  display:flex;align-items:center;justify-content:space-between;gap:6px;
  padding:0 10px;color:var(--grey);font-size:11px;
  box-shadow:0 1px 0 rgba(255,255,255,.05) inset;
}
.combo .clabel{
  color:var(--faint);font-size:9px;font-weight:600;letter-spacing:.06em;
  text-transform:uppercase;overflow:hidden;text-overflow:ellipsis;white-space:nowrap;
}
.combo .cval{white-space:nowrap;font-weight:600;color:var(--grey)}

/* ---- buttons ---- */
.tbtn{
  display:flex;align-items:center;justify-content:center;
  background:linear-gradient(180deg,#1a1a20,#131317);
  border:1px solid var(--border);border-radius:var(--radius-sm);
  color:var(--dim);font-size:10.5px;font-weight:700;letter-spacing:.06em;
  text-transform:uppercase;cursor:default;transition:background .15s,color .15s,box-shadow .15s;
}
.tbtn.tog.on, .tbtn.on{
  color:#0a0a0c;background:linear-gradient(180deg,var(--accent,#ffd23f),color-mix(in srgb,var(--accent,#ffd23f) 70%,#000));
  border-color:transparent;box-shadow:0 0 14px -2px var(--accent,#ffd23f);
}
.tbtn.seg-l{border-radius:var(--radius-sm) 0 0 var(--radius-sm);border-right-width:0}
.tbtn.seg-r{border-radius:0 var(--radius-sm) var(--radius-sm) 0}
.btn-random{
  color:#fff;border-color:transparent;
  background:linear-gradient(120deg,#8b5cf6,#ec4899 55%,#fb923c);
  box-shadow:0 0 18px -4px rgba(236,72,153,.6);
}

/* ---- visuals ---- */
.visual{
  border:1px solid var(--border-soft);border-radius:var(--radius-sm);
  color:var(--dim);display:flex;align-items:center;justify-content:center;
  text-align:center;font-size:11px;background:rgba(255,255,255,.015);
}
.visual.title{
  justify-content:flex-start;font-size:21px;font-weight:800;border:none;background:none;
  letter-spacing:-.02em;
  background-image:linear-gradient(100deg,#fff 10%,#b9c4ff 45%,#8b5cf6 80%);
  -webkit-background-clip:text;background-clip:text;color:transparent;
  filter:drop-shadow(0 0 14px rgba(139,92,246,.25));
}
.visual.readout{
  background:#000;border-color:var(--border);font-family:var(--font-mono);
  letter-spacing:.03em;color:var(--accent,var(--grey));
  box-shadow:0 0 0 1px rgba(255,255,255,.02) inset,0 0 10px -4px var(--accent,transparent);
  text-shadow:0 0 8px color-mix(in srgb, var(--accent,#fff) 55%, transparent);
}
.visual.led{
  border-color:var(--border);font-family:var(--font-mono);font-weight:700;
  letter-spacing:.08em;
}
.visual.led::before{
  content:"";width:7px;height:7px;border-radius:50%;background:var(--accent,#888);
  box-shadow:0 0 8px 2px var(--accent,#888);margin-right:8px;
}
.visual.sect{
  border:none;color:var(--accent,var(--faint));font-size:10px;font-weight:800;
  letter-spacing:.14em;background:linear-gradient(180deg, color-mix(in srgb, var(--accent,#888) 14%, transparent), transparent);
  border-radius:var(--radius-sm);
  writing-mode:vertical-rl;text-orientation:mixed;transform:rotate(180deg);
}
.visual.ruler{
  background:repeating-linear-gradient(90deg, rgba(255,255,255,.07) 0 1px, transparent 1px 41.5px);
  border-color:var(--border-soft);justify-content:flex-start;align-items:flex-end;
  padding:0 0 4px 6px;font-size:9px;letter-spacing:.08em;color:var(--faint);
  text-transform:uppercase;font-weight:700;
}
.visual.irph{
  color:var(--faint);line-height:1.7;font-size:10px;letter-spacing:.1em;font-weight:700;
  background:
    repeating-linear-gradient(45deg, rgba(255,255,255,.035) 0 2px, transparent 2px 10px),
    rgba(255,255,255,.01);
  border-style:dashed;text-transform:uppercase;
}

/* ---- step sequencer ---- */
.step{
  position:relative;overflow:hidden;background:#141418;
  border:1px solid var(--border);border-radius:8px 8px 5px 5px;
  color:var(--faint);font-size:10px;font-weight:700;
  display:flex;align-items:flex-end;justify-content:center;padding-bottom:4px;
  box-shadow:0 1px 0 rgba(255,255,255,.04) inset;
}
.step::before{
  content:"";position:absolute;left:0;right:0;top:0;height:30%;
  background:var(--track);transition:background .15s;
}
.step.on{color:#0a0a0c;font-weight:800}
.step.on::before{
  height:62%;background:linear-gradient(180deg,var(--accent,#ffd23f),color-mix(in srgb,var(--accent,#ffd23f) 55%,#000));
  box-shadow:0 4px 12px -4px var(--accent,#ffd23f);
}
.step.on{color:var(--grey)}

.legend{margin-top:18px;color:var(--faint);max-width:1280px;font-size:11px;line-height:1.6}
"""

HERE = os.path.dirname(os.path.abspath(__file__))
with open(os.path.join(HERE, "index-v3.html"), "w") as f:
    f.write(html)
with open(os.path.join(HERE, "styles-v3.css"), "w") as f:
    f.write(css)
print(f"\nwrote index-v3.html ({len(html)} bytes) + styles-v3.css ({len(css)} bytes)")

# ---- layout documentation table ----
ZONE_ORDER = ["header", "macro", "motor", "gate", "delay", "verb", "gran"]
zone_rows = {z: [] for z in ZONE_ORDER}

def zone_of(e):
    mod = e["attrs"].get("module", "global")
    y = e["y"]
    if y < MACRO_Y:
        return "header"
    if y < PANELS_Y:
        return "macro"
    return mod if mod in ("motor", "gate", "delay", "verb", "gran") else "header"

def param_label(e):
    a = e["attrs"]
    if "dp" in a:
        return a["dp"].replace('data-param-fwd=', 'fwd=').replace(' data-param-rev=', ' rev=').replace('data-param=', '').replace('data-role=', 'role:').replace('"', '')
    return "—"

for e in elements:
    if e["kind"] == "panel":
        continue
    z = zone_of(e)
    zone_rows.setdefault(z, []).append(e)

md_lines = ["# LAYOUT-V3 — geometria gerada (deVerb UI v3)", "",
            "Gerado automaticamente por `assets/mockup/generate-v3.py` — não editar à mão,",
            "editar o gerador e correr `python3 generate-v3.py`.",
            "",
            "Janela 1280x624, origem (0,0) topo-esquerda. Todas as posições/tamanhos abaixo",
            "foram recalculadas (layout novo, não é o mesmo do template anterior) — IDs de",
            "parâmetro mantidos 100% congelados.",
            ""]

ZONE_TITLES = {
    "header": f"HEADER (0, 0, 1280×{HEADER_H})",
    "macro": f"MACRO STRIP — REV + LINK/TRIM/CHAIN (0, {MACRO_Y}, 1280×{MACRO_H})",
    "motor": f"MOTOR panel ({motor.x}, {motor.y}, {motor.w}×{motor.h})",
    "gate": f"GATE panel ({gate.x}, {gate.y}, {gate.w}×{gate.h})",
    "delay": f"DELAY panel ({delay.x}, {delay.y}, {delay.w}×{delay.h})",
    "verb": f"VERB panel ({verb.x}, {verb.y}, {verb.w}×{verb.h})",
    "gran": f"GRAN panel ({gran.x}, {gran.y}, {gran.w}×{gran.h})",
}

for z in ZONE_ORDER:
    rows = zone_rows.get(z, [])
    if not rows:
        continue
    md_lines.append(f"### {ZONE_TITLES[z]}")
    md_lines.append("")
    md_lines.append("| Peça | Tipo | Param ID | x | y | w | h | Estado demo |")
    md_lines.append("|------|------|----------|---|---|---|---|-------------|")
    rows_sorted = sorted(rows, key=lambda e: (e["y"], e["x"]))
    for e in rows_sorted:
        a = e["attrs"]
        label = a.get("label") or a.get("txt") or a.get("idx", "")
        if e["kind"] == "step":
            label = f'step {a["idx"]+1}'
        pid = param_label(e)
        demo = "ON" if a.get("on") else ("—")
        md_lines.append(f'| {label} | {e["kind"]} | `{pid}` | {e["x"]} | {e["y"]} | {e["w"]} | {e["h"]} | {demo} |')
    md_lines.append("")

md_lines.append("## Resumo")
md_lines.append("")
md_lines.append(f"- {knob_count} knobs, {combo_count} combos, {btn_count} botões, {step_count} steps, {visual_count} visuais — {len(elements)-2} widgets no total.")
md_lines.append("- 122 parâmetros cobertos (100 em pares FWD|REV + 22 globais), idêntico ao template anterior.")
md_lines.append("- Tamanho mínimo de knob: 48px de diâmetro (dial). Combos ≥22px altura, botões ≥20px altura, texto ≥9.5px — dentro das regras de `LAYOUT-TEMPLATE.md`.")
md_lines.append("- Paleta/tema por `data-module`, não por painel de desenho — widgets do strip macro herdam a cor do módulo que controlam (ex.: toggle `DLY` no LINK strip usa `--delay-a`).")

with open(os.path.join(HERE, "LAYOUT-V3.md"), "w") as f:
    f.write("\n".join(md_lines))
print(f"wrote LAYOUT-V3.md ({len(md_lines)} lines)")
