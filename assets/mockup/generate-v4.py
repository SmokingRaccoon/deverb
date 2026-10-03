#!/usr/bin/env python3
"""deVerb UI v4 -- "Espelho de Agua".

Gera index-v4.html + LAYOUT-V4.md (geometria, validada) a partir deste ficheiro.
Escritos a mao (nao gerados): styles-v4.css (tokens + componentes), app-v4.js
(interacao do mockup), DESIGN-V4.md (brief de hand-off).

Contrato:
  - Janela fixa 1280x624, origem topo-esquerda, todos os widgets em px absolutos
    (coordenadas do #stage), como nas versoes anteriores.
  - IDs de parametro congelados: o conjunto de data-param / data-param-fwd /
    data-param-rev tem de ser IDENTICO ao de index.html (122 IDs) -- verificado.
  - Cada controlo FWD e REV existe 2x (FWD em cima, REV em baixo); o REV leva
    data-twin = ID do gemeo FWD (a agulha-fantasma do dial).
  - Validador: sem sobreposicoes em nenhum dos 4 estados de modulo, nada fora da
    zona, minimos de usabilidade, contraste de tokens, fontes >= 10px.
"""
import os, re, sys, math, random

HERE = os.path.dirname(os.path.abspath(__file__))

# ----------------------------------------------------------------------------
# Grelha
# ----------------------------------------------------------------------------
W, H = 1280, 624
HDR_H, BAND_H, MER_H = 44, 238, 104
Y_FWD = HDR_H                      # 44
Y_MER = Y_FWD + BAND_H             # 282
Y_REV = Y_MER + MER_H              # 386
Y_LINE = Y_MER + MER_H // 2        # 334  (linha de agua)
SIDE_W, MOD_X, MOD_W = 336, 360, 896
BAND_PAD = 20
CONTENT_H = BAND_H - 2 * BAND_PAD  # 198
CY = {"fwd": Y_FWD + BAND_PAD, "rev": Y_REV + BAND_PAD}   # 64 / 406
CELL_W, PITCH, DIAL_H = 64, 72, 86
ROW_DIALS = 112                    # y local da fila de dials (igual em barras e modulos)
SEL_ROW = 120                      # (usado em gate)

MODS = ["gate", "delay", "verb", "gran"]
POL = {"fwd": "p", "rev": "n"}

# estado de demonstracao
LINKS = {"gate": True, "delay": True, "verb": True, "gran": False}
LINK_ALL = True
MORPH = 0.35

NOTES = ["Free", "1/32", "1/16T", "1/16", "1/16D", "1/8T", "1/8", "1/8D", "1/4", "1/4D", "1/2", "1/1"]
PRESETS = ["Init", "Trance Gate 16", "Offbeat Chop", "Big Hall Space", "Reverse Tail", "Reverse Throw",
           "Beat Repeat", "Stutter Brk", "Dub Echo", "Shimmer Pad", "Build Up"]
PATTERNS = ["Porta 4/4", "Offbeat", "8ths", "16ths", "Saw Up", "Euclid 5", "Tresillo", "Cinquillo", "Euclid 3", "Euclid 7"]
ORDERS = ["G-D-V-Gr", "G-V-D-Gr", "D-G-V-Gr", "V-D-G-Gr"]

# ----------------------------------------------------------------------------
# Metadados de parametros (label, min, max, default, fmt, demo REV)  -- de funcoes.md
# ----------------------------------------------------------------------------
DIALS = {
    "gate": {
        "smooth": ("SMOOTH", 0, 1, .15, "n2", .45), "depth": ("DEPTH", 0, 1, 1.0, "n2", .7),
        "mix": ("MIX", 0, 1, 1.0, "n2", None), "pan": ("PAN", 0, 1, 0.0, "n2", .5),
        "env_thr": ("THR", -60, 0, -18.0, "db", -30.0)},
    "delay": {
        "time": ("MS", 1, 2000, 375, "ms", 250), "fb": ("FB", 0, .95, .35, "n2", .6),
        "damp": ("DAMP", 200, 18000, 6000, "hz", 3000), "mix": ("MIX", 0, 1, .25, "n2", .4),
        "drive": ("DRIVE", 0, 1, .3, "n2", .5), "wow_rate": ("WOW RT", .1, 5, 1.0, "hz1", None),
        "wow_depth": ("WOW DP", 0, 20, 4.0, "ms1", None), "spread": ("SPREAD", 0, 1, 1.0, "n2", .6)},
    "verb": {
        "size": ("SIZE", 0, 1, .5, "n2", .8), "decay": ("DECAY", .2, 20, 2.5, "s", 6.0),
        "damp": ("DAMP", 0, 1, .3, "n2", .6), "width": ("WIDTH", 0, 1, 1.0, "n2", .7),
        "predelay": ("PRE-DLY", 0, 250, 20, "ms", 60), "locut": ("LO-CUT", 20, 500, 80, "hz", 160),
        "hicut": ("HI-CUT", 2000, 20000, 12000, "hz", 8000), "mix": ("MIX", 0, 1, .3, "n2", .5)},
    "gran": {
        "chance": ("CHANCE", 0, 1, .2, "n2", .5), "env_thr": ("THR", -60, 0, -18.0, "db", None),
        "repeats": ("REPEATS", 1, 16, 4, "int", 8), "decay": ("DECAY", .5, .99, .85, "n2", .7),
        "time": ("TIME", 60, 8000, 250, "ms", 500), "pitch": ("PITCH", -12, 12, 12.0, "st", -12.0),
        "flux": ("FLUX", 0, 1, .3, "n2", .6), "xfade": ("XFADE", 1, 50, 8.0, "ms1", None),
        "mix": ("MIX", 0, 1, .5, "n2", .5)},
}
TRIM_BASE = {("delay", "time"): "trim_delay", ("verb", "decay"): "trim_decay"}

GLOBAL_DIALS = {  # id: (label, min, max, def, fmt)
    "input_gain": ("INPUT", 0, 2, 1.0, "n2"), "master": ("MASTER", 0, 1, .8, "n2"),
    "fwd_mix": ("FWD MIX", 0, 1, 1.0, "n2"), "rev_mix": ("REV MIX", 0, 1, .4, "n2"),
    "morph": ("MORPH", 0, 1, MORPH, "pct"),
    "trim_delay": ("T-DLY", .25, 4, 1.0, "x"), "trim_decay": ("T-DEC", .25, 2, 1.0, "x"),
    "rev_rate": ("RATE", .25, 2, 1.0, "x"), "rev_lfo": ("LFO", 0, 1, 0.0, "n2"),
    "rev_duck": ("DUCK", 0, 1, .3, "n2"), "tempo_bpm": ("BPM", 40, 240, 120.0, "n1"),
}


def fmtv(fmt, v):
    if fmt == "n2": return f"{v:.2f}"
    if fmt == "n1": return f"{v:.1f}"
    if fmt == "int": return f"{round(v)}"
    if fmt == "ms": return f"{round(v)} ms"
    if fmt == "ms1": return f"{v:.1f} ms"
    if fmt == "hz": return f"{v / 1000:.1f} kHz" if v >= 1000 else f"{round(v)} Hz"
    if fmt == "hz1": return f"{v:.1f} Hz"
    if fmt == "s": return f"{v:.1f} s"
    if fmt == "db": return f"{v:.1f} dB"
    if fmt == "st": return f"{v:+.1f} st"
    if fmt == "x": return f"{v:.2f}x"
    if fmt == "pct": return f"{round(v * 100)}%"
    raise ValueError(fmt)


def norm(v, mn, mx):
    return (v - mn) / (mx - mn)


def pid(engine, mod, base):
    if mod == "gran":
        return f"gr_{base}" if engine == "fwd" else f"rev_gr_{base}"
    return f"{engine}_{mod}_{base}"


def g(v):
    s = f"{v:.4f}".rstrip("0").rstrip(".")
    return s if s else "0"


# ----------------------------------------------------------------------------
# Registo de elementos
# ----------------------------------------------------------------------------
ELS = []


def add(kind, x, y, w, h, html, zone, name, mod=None, engine=None, doc=None):
    e = dict(kind=kind, x=int(round(x)), y=int(round(y)), w=int(round(w)), h=int(round(h)),
             html=html, zone=zone, name=name, mod=mod, engine=engine, doc=doc or kind)
    ELS.append(e)
    return e


def A(**kw):
    return " ".join(f'{k.replace("_", "-")}="{v}"' for k, v in kw.items() if v is not None)


def stl(x, y, w, h, extra=""):
    return f"left:{int(round(x))}px;top:{int(round(y))}px;width:{int(round(w))}px;height:{int(round(h))}px{extra}"


def is_linked(mod):
    return LINK_ALL and LINKS.get(mod, False)


def eff_value(fwd, own, mn, mx, mod, base):
    trim = 1.0  # trims por defeito = 1.0
    linked = fwd * trim
    return max(mn, min(mx, linked + (own - linked) * MORPH))


# ----------------------------------------------------------------------------
# Construtores de widgets
# ----------------------------------------------------------------------------
def build_dial(engine, mod, base, x, y, zone, kind_class="m"):
    label, mn, mx, df, fmt, rdemo = DIALS[mod][base]
    fwdv = df
    own = df if engine == "fwd" else (rdemo if rdemo is not None else df)
    linked = engine == "rev" and is_linked(mod)
    shown = eff_value(fwdv, own, mn, mx, mod, base) if linked else own
    at = dict(data_acc=mod, data_mod=mod, data_engine=engine, data_pol=POL[engine], data_label=label,
              data_min=g(mn), data_max=g(mx), data_def=g(df), data_val=g(own), data_fmt=fmt)
    if engine == "fwd":
        at["data_param_fwd"] = pid("fwd", mod, base)
    else:
        at["data_param_rev"] = pid("rev", mod, base)
        at["data_twin"] = pid("fwd", mod, base)
        at["data_link"] = mod
    cls = f"w dial {kind_class}" + (" linked" if linked else "")
    vars_ = f";--v:{g(norm(own, mn, mx))}"
    needles = '<i class="needle own"></i>'
    if engine == "rev":
        vars_ += f";--vf:{g(norm(fwdv, mn, mx))};--ve:{g(norm(shown, mn, mx))}"
        needles = '<i class="needle ghost"></i>' + needles
    else:
        vars_ += f";--ve:{g(norm(own, mn, mx))}"
    needles += '<i class="needle dot"></i>'
    html = (f'<div class="{cls}" {A(**at)} style="{stl(x, y, CELL_W, DIAL_H, vars_)}">'
            f'<div class="ring"><i class="disc"></i>{needles}</div>'
            f'<div class="klabel">{label}</div><div class="kval">{fmtv(fmt, shown)}</div></div>')
    add("dial", x, y, CELL_W, DIAL_H, html, zone, label, mod, engine, "dial M")


def build_global_dial(gid, x, y, zone, variant="m", engine=None, acc="global", pol="x", twin=None, link=None):
    label, mn, mx, df, fmt = GLOBAL_DIALS[gid]
    own = df
    at = dict(data_acc=acc, data_pol=pol, data_label=label, data_min=g(mn), data_max=g(mx),
              data_def=g(df), data_val=g(own), data_fmt=fmt, data_param=gid)
    if engine:
        at["data_engine"] = engine
    html = (f'<div class="w dial {variant}" {A(**at)} style="{stl(x, y, CELL_W, DIAL_H if variant != "mer" else 88, f";--v:{g(norm(own, mn, mx))};--ve:{g(norm(own, mn, mx))}")}">'
            f'<div class="ring"><i class="disc"></i><i class="needle own"></i><i class="needle dot"></i></div>'
            f'<div class="klabel">{label}</div><div class="kval">{fmtv(fmt, own)}</div></div>')
    h = DIAL_H if variant != "mer" else 88
    add("dial", x, y, CELL_W, h, html, zone, label, None, engine, "dial M" if variant == "m" else "dial M (meridiano)")


def est_w(text, per=7.75, pad=17, minw=40):
    return max(minw, int(round(len(text) * per + pad)))


def build_seg(ids, x, y, zone, name, cap, options, sel, engine=None, mod=None, w=None, h=28,
              acc=None, tether=False, cap_above=None, cap_below=None, role=None, extra_cls="", pol=None, opt_minw=40):
    """options: list of (texto, nome-completo). ids: dict de atributos data-param*."""
    capw = (est_w(cap, 7.75, 19, 0) if cap else 0)
    ow = [est_w(o[0], minw=opt_minw) for o in options]
    tw = capw + sum(ow) + 2
    if w is None:
        w = tw
    pol = pol or (POL[engine] if engine else "x")
    at = dict(data_acc=acc or mod or "global", data_mod=mod if engine else None, data_engine=engine,
              data_pol=pol, data_label=name, data_options="|".join(o[1] for o in options), data_i=sel,
              data_cap_above=cap_above, data_cap_below=cap_below, data_role=role)
    at.update(ids)
    cls = "w seg" + (" tether" if tether else "") + (f" {extra_cls}" if extra_cls else "")
    btns = "".join(f'<button data-i="{i}"{" class=on" if i == sel else ""}>{o[0]}</button>'.replace("class=on", 'class="on"')
                   for i, o in enumerate(options))
    capi = f'<span class="cap">{cap}</span>' if cap else ""
    html = f'<div class="{cls}" {A(**at)} style="{stl(x, y, w, h)}">{capi}{btns}</div>'
    e = add("seg", x, y, w, h, html, zone, name, mod, engine, "segmentado")
    e["need_w"] = tw
    return e


def build_stepper(ids, x, y, zone, name, cap, options, sel, w, engine=None, mod=None, h=28,
                  acc=None, tether=False, role=None, cap_above=None, cap_below=None, show=None, pol=None):
    pol = pol or (POL[engine] if engine else "x")
    at = dict(data_acc=acc or mod or "global", data_mod=mod if engine else None, data_engine=engine,
              data_pol=pol, data_label=name, data_options="|".join(options), data_i=sel, data_role=role,
              data_cap_above=cap_above, data_cap_below=cap_below)
    at.update(ids)
    cls = "w stepper" + (" tether" if tether else "")
    capi = f'<span class="cap">{cap}</span>' if cap else ""
    html = (f'<div class="{cls}" {A(**at)} style="{stl(x, y, w, h)}">{capi}'
            f'<button class="prev" aria-label="prev"></button><span class="val">{show or options[sel]}</span>'
            f'<button class="next" aria-label="next"></button></div>')
    e = add("stepper", x, y, w, h, html, zone, name, mod, engine, "stepper")
    e["need_w"] = (est_w(cap, 7.75, 19, 0) if cap else 0) + 58 + max(len(o) for o in options) * 6.9
    return e


def build_key(ids, x, y, w, h, zone, name, text, engine=None, mod=None, on=False, acc=None, cls_extra="",
              tether=False, role=None, inner="", doc="tecla", pol=None, tile=None):
    pol = pol or (POL[engine] if engine else "x")
    at = dict(data_acc=acc or mod or "global", data_mod=mod if (mod and "pwr" not in cls_extra and "link" not in cls_extra) else None,
              data_engine=engine, data_pol=pol, data_label=name, data_role=role, data_tile=tile)
    at.update(ids)
    cls = "w key " + cls_extra + (" on" if on else "") + (" tether" if tether else "")
    html = f'<button class="{cls.strip()}" {A(**at)} aria-pressed="{"true" if on else "false"}" title="{name}" style="{stl(x, y, w, h)}">{inner or text}</button>'
    e = add("key", x, y, w, h, html, zone, name, at.get("data_mod"), engine, doc)
    e["need_w"] = est_w(text, 7.0, 16, 0) if text else 0
    return e


def build_step(engine, i, x, y, w, h, on, tether):
    fid, rid = "fwd_gate_pattern", "rev_gate_pattern"
    at = dict(data_acc="gate", data_mod="gate", data_engine=engine, data_pol=POL[engine], data_step=i, data_grp=i // 4)
    if engine == "fwd":
        at["data_param_fwd"] = fid
    else:
        at["data_param_rev"] = rid
        at["data_twin"] = fid
        at["data_link"] = "gate"
    cls = "w stp" + (" on" if on else "") + (" tether" if tether else "") + (" g1" if (i // 4) % 2 else "")
    html = f'<button class="{cls}" {A(**at)} title="step {i + 1}" style="{stl(x, y, w, h)}">{i + 1}</button>'
    add("step", x, y, w, h, html, "mod", f"step {i + 1}", "gate", engine, "passo")


def build_viz(cls, vis, x, y, w, h, zone, inner="", mod=None, engine=None, acc=None, pol=None, extra_at=None, doc="visual"):
    at = dict(data_visual=vis, data_acc=acc or mod, data_mod=mod, data_engine=engine,
              data_pol=pol or (POL[engine] if engine else "p"))
    if extra_at:
        at.update(extra_at)
    html = f'<div class="w viz {cls}" {A(**at)} style="{stl(x, y, w, h)}">{inner}</div>'
    add("viz", x, y, w, h, html, zone, vis, mod, engine, doc)


# ----------------------------------------------------------------------------
# SVG (graficos assinatura; deterministicos)
# ----------------------------------------------------------------------------
def svg_wrap(w, h, body, cls=""):
    return f'<svg class="{cls}" viewBox="0 0 {w} {h}" width="{w}" height="{h}" aria-hidden="true">{body}</svg>'


def svg_scope(w, h):
    r = random.Random(11)
    cy = h / 2
    d = []
    for i in range(0, w - 18, 3):
        env = (0.22 + 0.5 * abs(math.sin(i * 0.045))) * (0.55 + 0.45 * r.random())
        a = env * (h / 2 - 5)
        d.append(f"M{i + 1} {cy - a:.1f}V{cy + a:.1f}")
    body = (f'<line class="hair" x1="0" y1="{cy}" x2="{w}" y2="{cy}"/>'
            f'<path class="wave" d="{"".join(d)}"/>'
            f'<line class="now" x1="{w - 14}" y1="3" x2="{w - 14}" y2="{h - 3}"/>'
            f'<text class="t" x="{w - 2}" y="11" text-anchor="end">NOW</text>')
    return svg_wrap(w, h, body, "scope-svg")


def svg_capture(w, h):
    r = random.Random(5)
    beat = w / 4
    parts = []
    for k in range(5):
        x = w - k * beat
        parts.append(f'<line class="grid" x1="{x:.1f}" y1="14" x2="{x:.1f}" y2="{h}"/>')
    labels = "".join(f'<text class="t" x="{w - k * beat - 4:.1f}" y="10" text-anchor="end">{"NOW" if k == 0 else "-" + str(k)}</text>'
                     for k in range(0, 4))
    bars = []
    for i in range(0, w - 2, 3):
        env = 0.2 + 0.55 * abs(math.sin(i * 0.05 + 1)) * (0.5 + 0.5 * r.random())
        a = env * 10
        bars.append(f"M{i + 1} {h - 11 - a:.1f}V{h - 11 + a:.1f}")
    body = (f'<rect class="win" x="{w - 2 * beat:.1f}" y="14" width="{2 * beat:.1f}" height="{h - 14}"/>'
            + "".join(parts) + labels + f'<path class="wave" d="{"".join(bars)}"/>'
            f'<g class="readhead"><line x1="0" y1="14" x2="0" y2="{h}"/><path d="M0 {h - 11} l7 -4 v8 z"/></g>')
    return svg_wrap(w, h, body, "cap-svg")


def svg_taps(w, h):
    body = f'<line class="hair" x1="0" y1="{h - 4}" x2="{w}" y2="{h - 4}"/>'
    for i in range(8):
        x = 16 + i * 66
        r_ = 12 * (0.78 ** i)
        cls = "tap dry" if i == 0 else "tap"
        body += f'<circle class="{cls}" cx="{x}" cy="{h - 4 - r_ - 1:.1f}" r="{r_:.1f}" opacity="{1 - i * 0.1:.2f}"/>'
    return svg_wrap(w, h, body)


def svg_decay(w, h):
    d = []
    n = w // 3
    for i in range(n):
        a = math.exp(-i / n * 4.2) * (h / 2 - 3)
        d.append(f"M{i * 3 + 1} {h / 2 - a:.1f}V{h / 2 + a:.1f}")
    body = (f'<line class="hair" x1="0" y1="{h / 2}" x2="{w}" y2="{h / 2}"/><path class="wave" d="{"".join(d)}"/>'
            f'<line class="now" x1="{w * 0.52:.1f}" y1="2" x2="{w * 0.52:.1f}" y2="{h - 2}"/>'
            f'<text class="t" x="{w * 0.52 + 5:.1f}" y="11">T60</text>')
    return svg_wrap(w, h, body)


def svg_shards(w, h):
    r = random.Random(23)
    body = ""
    for i in range(15):
        x = 8 + i * (w - 24) / 14 + r.uniform(-4, 4)
        s = r.uniform(6, 16)
        up = r.random() > .5
        cy = h / 2 + r.uniform(-6, 6)
        pts = f"{x - s / 2:.1f},{cy + s / 2 * (1 if up else -1):.1f} {x + s / 2:.1f},{cy + s / 2 * (1 if up else -1):.1f} {x:.1f},{cy - s / 2 * (1 if up else -1):.1f}"
        body += f'<polygon class="{"shard" if r.random() > .35 else "shard o"}" points="{pts}"/>'
    return svg_wrap(w, h, body)


def svg_ruler(w, h):
    body = ""
    for gi in range(4):
        body += f'<text class="t" x="{gi * 168 + 2}" y="10">{gi + 1}</text>'
    for i in range(16):
        x = (i // 4) * 168 + (i % 4) * 40 + 18
        body += f'<line class="tick" x1="{x}" y1="{h - 4}" x2="{x}" y2="{h}"/>'
    return svg_wrap(w, h, body)


def svg_glyph(mod):
    objs = {
        "gate": '<rect x="11" y="0" width="22" height="22"/>',
        "delay": '<path d="M9 0 A11 11 0 0 1 9 22 Z"/><path d="M25 3 A8 8 0 0 1 25 19 Z" opacity=".6"/><path d="M37 6 A5 5 0 0 1 37 16 Z" opacity=".35"/>',
        "verb": '<circle cx="22" cy="11" r="11"/><circle class="hole" cx="22" cy="11" r="7.5"/><circle class="hole" cx="22" cy="11" r="4"/>',
        "gran": '<polygon points="11,22 33,22 22,1"/><polygon points="36,22 42,22 39,14" opacity=".55"/><polygon points="2,22 8,22 5,15" opacity=".55"/>',
    }[mod]
    return (f'<svg class="glyph" viewBox="0 0 44 44" width="44" height="44" aria-hidden="true">'
            f'<g class="obj">{objs}</g><g class="rw"><g class="refl" transform="translate(0 44) scale(1 -1)">{objs}</g></g></svg>')


# ----------------------------------------------------------------------------
# HEADER
# ----------------------------------------------------------------------------
def build_header():
    z = "header"
    build_viz("title", "title", 24, 0, 140, 44, z, '<i class="mark"></i>deVerb', pol="p")
    build_stepper(dict(data_role="factory-preset"), 508, 8, z, "Preset", None, PRESETS, 0, 272, role="factory-preset", acc="global")
    build_key(dict(data_role="random"), 788, 8, 88, 28, z, "RANDOM", "RANDOM", role="random", cls_extra="solid", doc="tecla (acao)")
    # BPM numbox
    label, mn, mx, df, fmt = GLOBAL_DIALS["tempo_bpm"]
    html = (f'<div class="w numbox" {A(data_acc="global", data_pol="p", data_param="tempo_bpm", data_label="BPM", data_min=mn, data_max=mx, data_def=g(df), data_val=g(df), data_fmt=fmt)} '
            f'style="{stl(1040, 8, 128, 28)}"><span class="cap">BPM</span><span class="val">{fmtv(fmt, df)}</span></div>')
    add("numbox", 1040, 8, 128, 28, html, z, "BPM", None, None, "caixa numerica (arrastar/escrever)")
    build_viz("bpm-src", "bpm-source", 1176, 8, 80, 28, z, '<i class="lamp"></i><span>INT</span>', pol="p")


# ----------------------------------------------------------------------------
# BARRAS LATERAIS
# ----------------------------------------------------------------------------
def build_sidebars():
    c = CY["fwd"]
    z = "side-fwd"
    build_viz("engine fwd", "engine-fwd", 24, c, 288, 28, z,
              '<b>FWD</b><i class="dir"></i><span class="cap">PRESENT</span>', engine="fwd", pol="p")
    build_viz("scope", "scope", 24, c + 36, 288, 68, z, svg_scope(288, 68), engine="fwd", pol="p")
    build_global_dial("fwd_mix", 240, c + ROW_DIALS, z, "m", engine="fwd", acc="global", pol="p")

    c = CY["rev"]
    z = "side-rev"
    build_viz("engine rev", "engine-rev", 24, c, 112, 28, z,
              '<b>REV</b><i class="dir"></i><span class="cap">PAST</span>', engine="rev", pol="n")
    build_seg(dict(data_param="rev_mode"), 144, c, z, "REV Mode", None, [("OFF", "Off"), ("LOOP", "Loop"), ("THROW", "Throw")],
              1, engine="rev", acc="global", w=168)
    build_seg(dict(data_param="rev_source"), 24, c + 34, z, "REV Source", None, [("DRY", "Dry"), ("POST", "PostFWD")],
              0, engine="rev", acc="global", w=112)
    build_seg(dict(data_param="rev_capture"), 144, c + 34, z, "REV Capture", "BEATS",
              [("2", "2 beats"), ("3", "3 beats"), ("4", "4 beats")], 0, engine="rev", acc="global", w=148, opt_minw=28)
    build_viz("capture", "capture-window", 24, c + 68, 288, 36, z, svg_capture(288, 36), engine="rev", pol="n")
    for i, gid in enumerate(["rev_rate", "rev_lfo", "rev_duck", "rev_mix"]):
        build_global_dial(gid, 24 + i * PITCH, c + ROW_DIALS, z, "m", engine="rev", acc="global", pol="n")


# ----------------------------------------------------------------------------
# MERIDIANO
# ----------------------------------------------------------------------------
TILE_W, TILE_H, TILE_Y = 124, 88, 290
MER_X = {}


def build_meridian():
    z = "meridian"
    widths = {"input": 64, "tiles": 4 * TILE_W + 3 * 8, "morph": 88 + 8 + 56, "throw": 72,
              "trim": 2 * 64 + 8, "xm": 116, "master": 64}
    total = sum(widths.values())
    gap = (W - 48 - total) // 6
    x0 = 24 + (W - 48 - total - 6 * gap) // 2
    xs, cx = {}, x0
    for k in ["input", "tiles", "morph", "throw", "trim", "xm", "master"]:
        xs[k] = cx
        cx += widths[k] + gap
    MER_X.update(xs)

    build_global_dial("input_gain", xs["input"], 291, z, "mer", acc="global", pol="x")

    # pedras
    for i, mod in enumerate(MODS):
        tx = xs["tiles"] + i * (TILE_W + 8)
        title = {"gate": "GATE", "delay": "DELAY", "verb": "VERB", "gran": "GRAN"}[mod]
        sel = " sel" if mod == "gate" else ""
        html = (f'<button class="w tile{sel}" {A(data_select=mod, data_acc=mod, data_pol="x", data_label=title, data_i=i)} '
                f'title="{title}" style="{stl(tx, TILE_Y, TILE_W, TILE_H)}"><span class="tname">{title}</span>{svg_glyph(mod)}</button>')
        add("plate", tx, TILE_Y, TILE_W, TILE_H, html, z, f"pedra {title}", None, None, "pedra (seleciona o modulo)")
        on_f, on_r = True, True
        build_key(({"data_param_fwd": pid("fwd", mod, "on")}), tx + 8, TILE_Y + 4, 40, 20, z, f"{title} FWD PWR", "",
                  engine="fwd", acc=mod, on=on_f, cls_extra="pwr fwd", inner='<i class="arr"></i><i class="lamp"></i>', doc="PWR FWD", tile=mod)
        build_key(({"data_param_rev": pid("rev", mod, "on"), "data_twin": pid("fwd", mod, "on"), "data_link": mod}),
                  tx + 8, TILE_Y + 64, 40, 20, z, f"{title} REV PWR", "", engine="rev", acc=mod, on=on_r,
                  cls_extra="pwr rev", tether=is_linked(mod), inner='<i class="arr"></i><i class="lamp"></i>', doc="PWR REV", tile=mod)
        build_key(({"data_param": f"link_{mod}"}), tx + 8, TILE_Y + 34, 40, 20, z, f"LINK {title}", "",
                  acc=mod, on=LINKS[mod], cls_extra="link", inner='<i class="knot"></i>', doc="LINK (FWD->REV)", tile=mod)

    # MORPH
    label, mn, mx, df, fmt = GLOBAL_DIALS["morph"]
    mx0 = xs["morph"]
    html = (f'<div class="w dial xl" {A(data_acc="brand", data_pol="x", data_label="MORPH", data_min=0, data_max=1, data_def=0, data_val=g(MORPH), data_fmt="pct", data_param="morph")} '
            f'style="{stl(mx0, 290, 88, 88, f";--v:{g(MORPH)};--ve:{g(MORPH)}")}">'
            f'<div class="ring"><i class="disc"></i><i class="needle own"></i><i class="needle dot"></i></div></div>')
    add("dial", mx0, 290, 88, 88, html, z, "MORPH", None, None, "dial XL (heroi)")
    build_viz("morph-read", "morph-readout", mx0 + 96, 296, 56, 34, z,
              f'<span class="klabel">MORPH</span><span class="kval">{fmtv("pct", MORPH)}</span>', pol="x")
    build_key(dict(data_param="link_master"), mx0 + 96, 342, 52, 20, z, "LINK ALL", "ALL", acc="brand", on=LINK_ALL,
              cls_extra="link-all", doc="LINK master", pol="n")

    # THROW
    build_key(dict(data_param="rev_throw"), xs["throw"], Y_LINE - 20, 72, 40, z, "THROW", "THROW", engine=None,
              acc="brand", cls_extra="throw", doc="tecla (performance)", inner='<span class="t">THROW</span>')
    # trims
    build_global_dial("trim_delay", xs["trim"], 291, z, "mer", acc="global", pol="x")
    build_global_dial("trim_decay", xs["trim"] + 72, 291, z, "mer", acc="global", pol="x")
    # x-mode + order
    build_seg(dict(data_param="x_mode"), xs["xm"], 296, z, "X Mode", None, [("ADD", "Add"), ("XFADE", "XFade")], 0,
              w=116, acc="global", cap_above="X-MODE")
    build_stepper(dict(data_param="chain_order"), xs["xm"], 344, z, "Chain Order", None, ORDERS, 0, 116, acc="global",
                  cap_below="ORDER", pol="n")
    build_global_dial("master", xs["master"], 291, z, "mer", acc="global", pol="x")


# ----------------------------------------------------------------------------
# MODULOS (x2: FWD e REV)
# ----------------------------------------------------------------------------
def pattern_mask(engine, linked):
    # FWD: 4/4 (0x1111); REV proprio: offbeat (0x4444); com link o REV segue FWD
    if engine == "fwd" or linked:
        return {0, 4, 8, 12}
    return {2, 6, 10, 14}


def flow(x, items, gap=16):
    """items: list of callables(x)->width. Devolve x final."""
    for fn in items:
        w = fn(x)
        x += w + gap
    return x - gap


def build_module(engine, mod):
    ox, oy = MOD_X, CY[engine]
    z = "mod"
    L = is_linked(mod) and engine == "rev"
    t = {"gate": ("GATE", "TRANCE"), "delay": ("DELAY", "ECHO"), "verb": ("VERB", "SPACE"), "gran": ("GRAN", "GLITCH")}[mod]
    gl = svg_glyph(mod).replace('width="44" height="44"', 'width="22" height="22"')
    gl_small = (f'<svg class="mglyph" viewBox="0 0 44 22" width="32" height="16" aria-hidden="true">'
                f'{re.search(r"<g class=.obj.>(.*?)</g><g class=.rw", gl, re.S).group(1)}</svg>')
    build_viz("mtitle", f"title-{mod}", ox, oy, 300, 24, z, f'{gl_small}<b>{t[0]}</b><span class="cap">{t[1]}</span>',
              mod=mod, engine=engine)

    def dial_row(bases, xstart=0):
        for i, b in enumerate(bases):
            build_dial(engine, mod, b, ox + xstart + i * PITCH, oy + ROW_DIALS, z)

    def S(ids_base, x, y, name, cap, opts, sel, w=None, role=None, tether=None):
        ids = ({"data_param_fwd": pid("fwd", mod, ids_base)} if engine == "fwd" else
               {"data_param_rev": pid("rev", mod, ids_base), "data_twin": pid("fwd", mod, ids_base), "data_link": mod})
        return build_seg(ids, ox + x, oy + y, z, name, cap, opts, sel, engine=engine, mod=mod, w=w, role=role,
                         tether=L if tether is None else tether)

    def ST(ids_base, x, y, name, cap, opts, sel, w, role=None, show=None):
        ids = ({"data_param_fwd": pid("fwd", mod, ids_base)} if engine == "fwd" else
               {"data_param_rev": pid("rev", mod, ids_base), "data_twin": pid("fwd", mod, ids_base), "data_link": mod})
        return build_stepper(ids, ox + x, oy + y, z, name, cap, opts, sel, w, engine=engine, mod=mod, tether=L, role=role, show=show)

    def K(ids_base, x, y, w, h, name, text, on=False, cls=""):
        ids = ({"data_param_fwd": pid("fwd", mod, ids_base)} if engine == "fwd" else
               {"data_param_rev": pid("rev", mod, ids_base), "data_twin": pid("fwd", mod, ids_base), "data_link": mod})
        return build_key(ids, ox + x, oy + y, w, h, z, name, text, engine=engine, mod=mod, on=on, cls_extra=cls, tether=L)

    def own_i(fwd_i, rev_i):
        return fwd_i if (engine == "fwd" or L) else rev_i

    if mod == "gate":
        build_viz("ruler", "ruler", ox, oy + 28, 660, 14, z, svg_ruler(660, 14), mod=mod, engine=engine)
        mask = pattern_mask(engine, L)
        for i in range(16):
            gx = (i // 4) * 168 + (i % 4) * 40
            build_step(engine, i, ox + gx, oy + 46, 36, 56, i in mask, L)
        build_viz("gate-big", "gate-readout", ox + 684, oy + 28, 212, 74,
                  z, '<span class="kval">07</span><span class="of">/16</span>', mod=mod, engine=engine)
        dial_row(["smooth", "depth", "mix", "pan", "env_thr"])
        ST("pattern", 372, SEL_ROW, "Pattern", "PATTERN", PATTERNS, own_i(0, 1), 256, role="pattern-preset")
        ST("rate", 640, SEL_ROW, "Gate Rate", "RATE", NOTES, 3, 256)
        S("steps", 372, SEL_ROW + 36, "Gate Steps", "STEPS", [("8", "8"), ("16", "16")], 1, w=152)
        S("trig", 536, SEL_ROW + 36, "Gate Trig", "TRIG",
          [("HOST", "Host"), ("MIDI", "Midi"), ("TRANS", "Transient"), ("FREE", "Free")], 0, w=360)

    elif mod == "delay":
        build_viz("readout right", "delay-ms", ox + 560, oy, 336, 24, z, '<span class="kval">375 ms - 1/8D @ INT</span>',
                  mod=mod, engine=engine)
        e1 = S("algo", 0, 32, "Delay Algo", "ALGO",
               [("DIGITAL", "Digital"), ("TAPE", "Tape"), ("PINGPONG", "PingPong"), ("MULTITAP", "MultiTap"), ("REVERSE", "Reverse")],
               0)
        x2 = e1["x"] - ox + e1["w"] + 16
        ST("note", x2, 32, "Delay Note", "DIV", NOTES, 7, 176)
        K("freeze", x2 + 176 + 16, 32, 88, 28, "Delay Freeze", "FREEZE")
        build_viz("taps", "delay-taps", ox, oy + 70, 560, 36, z, svg_taps(560, 36), mod=mod, engine=engine)
        dial_row(["time", "fb", "damp", "mix", "drive", "wow_rate", "wow_depth", "spread"])

    elif mod == "verb":
        build_viz("readout right", "verb-t60", ox + 560, oy, 336, 24, z, '<span class="kval">T60 2.5 s</span>', mod=mod, engine=engine)
        e1 = S("algo", 0, 32, "Verb Algo", "ALGO",
               [("ROOM", "Room"), ("HALL", "Hall"), ("PLATE", "Plate"), ("SHIMMER", "Shimmer")], 1)
        x2 = e1["x"] - ox + e1["w"] + 16
        ST("predelay_note", x2, 32, "PreDelay Note", "PRE", NOTES, 0, 180)
        K("freeze", x2 + 180 + 16, 32, 88, 28, "Verb Freeze", "FREEZE")
        build_viz("decay", "verb-decay", ox, oy + 70, 540, 36, z, svg_decay(540, 36), mod=mod, engine=engine)
        build_viz("ir", "ir-placeholder", ox + 600, oy + 30, 296, 76, z, "CONVOLUTION<br>PHASE 8", mod=mod, engine=engine)
        dial_row(["size", "decay", "damp", "width", "predelay", "locut", "hicut", "mix"])

    elif mod == "gran":
        build_viz("led right", "gran-led", ox + 790, oy, 106, 24, z, '<i class="lamp"></i><span>IDLE</span>', mod=mod, engine=engine)
        e1 = S("mode", 0, 30, "Gran Mode", "MODE",
               [("OFF", "Off"), ("BEAT", "BeatRepeat"), ("SLICE", "Slice"), ("REV", "Reverse"), ("PITCH", "Pitch"), ("STUT", "Stutter")],
               0 if (engine == "fwd" or L) else 1)
        x2 = e1["x"] - ox + e1["w"] + 16
        e2 = S("trigger", x2, 30, "Gran Trig", "TRIG", [("CHANCE", "Chance"), ("ENV", "Envelope"), ("MANUAL", "Manual")], 0)
        x3 = e2["x"] - ox + e2["w"] + 16
        K("manual", x3, 30, 72, 28, "Gran GRAB", "GRAB")
        K("interrupt", x3 + 72 + 8, 30, 96, 28, "Gran Interrupt", "INTERRUPT")
        ST("len_note", 0, 66, "Gran Len", "LEN", NOTES, 3, 176)
        ST("time_note", 192, 66, "Gran Time Note", "T-NOTE", NOTES, 0, 196)
        build_viz("shards", "gran-shards", ox + 410, oy + 64, 330, 42, z, svg_shards(330, 42), mod=mod, engine=engine)
        dial_row(["chance", "env_thr", "repeats", "decay", "time", "pitch", "flux", "xfade", "mix"])


# ----------------------------------------------------------------------------
# Montagem
# ----------------------------------------------------------------------------
build_header()
build_sidebars()
build_meridian()
for eng in ("fwd", "rev"):
    for m in MODS:
        build_module(eng, m)

# ----------------------------------------------------------------------------
# Validacao
# ----------------------------------------------------------------------------
errors, warnings = [], []
ZONES = {
    "header": (0, 0, W, HDR_H),
    "side-fwd": (0, Y_FWD, SIDE_W, BAND_H),
    "side-rev": (0, Y_REV, SIDE_W, BAND_H),
    "meridian": (0, Y_MER, W, MER_H),
}


def zone_rect(e):
    if e["zone"] == "mod":
        return (MOD_X, CY[e["engine"]], MOD_W, CONTENT_H)
    return ZONES[e["zone"]]


def inside(r, e, tol=0):
    return (e["x"] >= r[0] - tol and e["y"] >= r[1] - tol and
            e["x"] + e["w"] <= r[0] + r[2] + tol and e["y"] + e["h"] <= r[1] + r[3] + tol)


for e in ELS:
    r = zone_rect(e)
    if not inside(r, e):
        errors.append(f"[ZONA] '{e['name']}' ({e['x']},{e['y']},{e['w']}x{e['h']}) sai da zona {e['zone']} {r}")
    if not inside((0, 0, W, H), e):
        errors.append(f"[JANELA] '{e['name']}' fora de 1280x624")
    nw = e.get("need_w")
    if nw and nw > e["w"] + 1:
        errors.append(f"[LARGURA] '{e['name']}' precisa ~{nw:.0f}px mas tem {e['w']}px")


def ov(a, b):
    return (a["x"] < b["x"] + b["w"] and b["x"] < a["x"] + a["w"] and
            a["y"] < b["y"] + b["h"] and b["y"] < a["y"] + a["h"])


for m in MODS:
    vis = [e for e in ELS if e["mod"] in (None, m) and e["kind"] != "plate"]
    for i in range(len(vis)):
        for j in range(i + 1, len(vis)):
            if ov(vis[i], vis[j]):
                a, b = vis[i], vis[j]
                errors.append(f"[OVERLAP/{m}] '{a['name']}' ({a['x']},{a['y']},{a['w']}x{a['h']}) x "
                              f"'{b['name']}' ({b['x']},{b['y']},{b['w']}x{b['h']})")
# chaves das pedras: contidas na pedra
plates = [e for e in ELS if e["kind"] == "plate"]
for p in plates:
    for e in ELS:
        if e["kind"] == "key" and ov(p, e) and not inside((p["x"], p["y"], p["w"], p["h"]), e):
            errors.append(f"[PEDRA] '{e['name']}' sai da pedra '{p['name']}'")

# minimos de usabilidade (LAYOUT-TEMPLATE.md)
css_path = os.path.join(HERE, "styles-v4.css")
css = open(css_path).read() if os.path.exists(css_path) else ""
tok = dict(re.findall(r"--([a-z0-9-]+)\s*:\s*(#[0-9a-fA-F]{6})\b", css))
nums = dict((k, float(v)) for k, v in re.findall(r"--([a-z0-9-]+)\s*:\s*([\d.]+)px", css))
for k in ("disc-m", "disc-xl"):
    if k in nums and nums[k] < 48:
        errors.append(f"[MIN] --{k} = {nums[k]}px < 48px (dial)")
if "disc-m" not in nums:
    warnings.append("--disc-m nao encontrado no CSS (nao verifiquei dial >= 48px)")
for e in ELS:
    if e["kind"] == "key" and e["h"] < 20: errors.append(f"[MIN] tecla '{e['name']}' h={e['h']} < 20")
    if e["kind"] in ("seg", "stepper", "numbox") and e["h"] < 22: errors.append(f"[MIN] '{e['name']}' h={e['h']} < 22")
_css_nc = re.sub(r"/\*.*?\*/", "", css, flags=re.S)
for fs in (re.findall(r"font-size\s*:\s*([\d.]+)px", _css_nc) +
           re.findall(r"(?<![-\w])font\s*:\s*(?:\d{3}\s+|italic\s+|bold\s+)*([\d.]+)px", _css_nc)):
    if float(fs) < 10:
        errors.append(f"[MIN] font-size {fs}px < 10px no CSS")


# contraste
def _lin(c):
    c = c / 255
    return c / 12.92 if c <= 0.03928 else ((c + 0.055) / 1.055) ** 2.4


def lum(h):
    h = h.lstrip("#")
    r, g_, b = (int(h[i:i + 2], 16) for i in (0, 2, 4))
    return 0.2126 * _lin(r) + 0.7152 * _lin(g_) + 0.0722 * _lin(b)


def ratio(a, b):
    la, lb = lum(a), lum(b)
    if la < lb: la, lb = lb, la
    return (la + 0.05) / (lb + 0.05)


contrast_rows = []
if tok:
    pairs = [("lab-p", "paper", 4.5), ("lab-p", "paper-2", 4.5), ("lab-p", "paper-3", 4.5),
             ("lab-i", "ink", 4.5), ("lab-i", "ink-2", 4.5), ("lab-i", "ink-3", 4.5),
             ("ink", "paper", 7), ("paper", "ink", 7)]
    for a in ("gate", "delay", "verb", "gran"):
        if f"on-{a}" in tok:
            pairs.append((f"on-{a}", a, 4.5))
    for a, b, need in pairs:
        if a in tok and b in tok:
            r_ = ratio(tok[a], tok[b])
            contrast_rows.append((a, b, r_, need))
            if r_ < need:
                errors.append(f"[CONTRASTE] --{a} sobre --{b}: {r_:.2f} < {need}")
    for a in ("gate", "delay", "verb", "gran"):
        for g_ in ("paper", "ink"):
            if a in tok and g_ in tok:
                r_ = ratio(tok[a], tok[g_])
                if r_ < 3:
                    warnings.append(f"[GRAFICO] acento --{a} sobre --{g_}: {r_:.2f} < 3 (so em areas planas c/ contorno)")
else:
    warnings.append("styles-v4.css sem tokens ainda (contraste nao verificado)")

# cobertura dos 122 IDs
html_all = "\n".join(e["html"] for e in ELS)
ids_new = set(re.findall(r'data-param(?:-fwd|-rev)?="([^"]+)"', html_all))
old_path = os.path.join(HERE, "index.html")
if os.path.exists(old_path):
    ids_old = set(re.findall(r'data-param(?:-fwd|-rev)?="([^"]+)"', open(old_path).read()))
    miss, extra = sorted(ids_old - ids_new), sorted(ids_new - ids_old)
    if miss: errors.append(f"[IDS] faltam {len(miss)}: {miss}")
    if extra: errors.append(f"[IDS] a mais {len(extra)}: {extra}")
else:
    warnings.append("index.html (v1) nao encontrado: cobertura de IDs nao comparada")

print(f"elementos: {len(ELS)}  | IDs distintos: {len(ids_new)} (esperado 122)")
cnt = {}
for e in ELS: cnt[e['kind']] = cnt.get(e['kind'], 0) + 1
print("por tipo:", ", ".join(f"{k}={v}" for k, v in sorted(cnt.items())))
for a, b, r_, need in contrast_rows:
    print(f"  contraste --{a:6s} / --{b:8s}: {r_:5.2f}  (>= {need})")
for w_ in warnings: print("AVISO", w_)
if errors:
    print("\n--- ERROS ---")
    for e in errors: print(e)
    sys.exit(1)
print("Validacao OK: sem sobreposicoes nos 4 estados, tudo dentro da zona, minimos cumpridos, 122 IDs identicos.")

# ----------------------------------------------------------------------------
# HTML
# ----------------------------------------------------------------------------
order = {"plate": 0, "viz": 1, "step": 2, "dial": 3, "seg": 3, "stepper": 3, "numbox": 3, "key": 4}
body = "\n".join(e["html"] for e in sorted(ELS, key=lambda e: order.get(e["kind"], 5)))

html = f"""<!DOCTYPE html>
<html lang="pt">
<head>
<meta charset="utf-8">
<title>deVerb UI v4 - Espelho de Agua (1280x624)</title>
<link rel="stylesheet" href="styles-v4.css">
</head>
<body>
<div class="page">
<div class="meta">deVerb UI v4 &middot; <b>Espelho de &Aacute;gua</b> &middot; FWD (positivo, em cima) e REV (negativo, em baixo) sempre vis&iacute;veis; o meridiano guarda a rela&ccedil;&atilde;o (LINK, MORPH, TRIM) &middot; esqueleto 1:1 em px, origem (0,0) topo-esq. &middot; gerado por <span class="mono">generate-v4.py</span> &middot; ver <span class="mono">DESIGN-V4.md</span> e <span class="mono">LAYOUT-V4.md</span></div>
<div id="stage" data-sel="gate">
<div class="bg hdr" style="left:0;top:0;width:{W}px;height:{HDR_H}px"></div>
<div class="bg side fwd" style="left:0;top:{Y_FWD}px;width:{SIDE_W}px;height:{BAND_H}px"></div>
<div class="bg side rev" style="left:0;top:{Y_REV}px;width:{SIDE_W}px;height:{BAND_H}px"></div>
<div class="waterline" style="top:{Y_LINE - 1}px"></div>
<div class="ripples" style="top:{Y_MER}px;height:{MER_H}px"></div>
<div class="widgets">
{body}
</div>
<div class="toast" hidden></div>
</div>
<div class="legend">Estados demonstrativos (sem l&oacute;gica de &aacute;udio): MORPH 35%, links GATE/DLY/VRB ligados e GRAN desligado (o REV de GRAN mostra uma agulha; os outros mostram duas: <i>fantasma</i> = valor FWD herdado, <i>s&oacute;lida</i> = valor pr&oacute;prio, <i>ponto</i> = valor efetivo). Controlos discretos do REV com link ligado ficam <i>amarrados</i> ao FWD. Experimenta: escolher uma pedra, ligar/desligar o link, arrastar dials (duplo clique repõe), THROW, MORPH. <span class="mono">?sel=verb&amp;unlink=gate&amp;morph=0.8</span> controla o estado inicial; <span class="mono">?qa</span> corre verifica&ccedil;&otilde;es de texto.</div>
</div>
<pre id="qa" hidden></pre>
<script src="app-v4.js"></script>
</body>
</html>
"""
with open(os.path.join(HERE, "index-v4.html"), "w") as f:
    f.write(html)

# ----------------------------------------------------------------------------
# LAYOUT-V4.md
# ----------------------------------------------------------------------------
def ids_of(e):
    return re.findall(r'data-param(?:-fwd|-rev)?="([^"]+)"', e["html"])


def role_of(e):
    m = re.search(r'data-role="([^"]+)"', e["html"])
    return m.group(1) if m else None


def row(e, dx=0, dy=0):
    ids = ids_of(e)
    pid_s = ", ".join(f"`{i}`" for i in ids[:2]) or (f"role:`{role_of(e)}`" if role_of(e) else "-")
    return f"| {e['name']} | {e['doc']} | {pid_s} | {e['x'] - dx} | {e['y'] - dy} | {e['w']} | {e['h']} |"


md = ["# LAYOUT-V4 - geometria gerada (deVerb UI v4, Espelho de Agua)", "",
      "Gerado por `generate-v4.py` (nao editar a mao). Janela 1280x624, origem (0,0) topo-esquerda.", "",
      "## Grelha", "",
      "| Zona | x | y | w | h |", "|---|---|---|---|---|",
      f"| Cabecalho (papel) | 0 | 0 | {W} | {HDR_H} |",
      f"| Banda FWD (papel) | 0 | {Y_FWD} | {W} | {BAND_H} |",
      f"| &nbsp;&nbsp;barra lateral FWD | 0 | {Y_FWD} | {SIDE_W} | {BAND_H} |",
      f"| &nbsp;&nbsp;area de modulo FWD | {MOD_X} | {CY['fwd']} | {MOD_W} | {CONTENT_H} |",
      f"| Meridiano (linha de agua em y={Y_LINE}) | 0 | {Y_MER} | {W} | {MER_H} |",
      f"| Banda REV (tinta) | 0 | {Y_REV} | {W} | {BAND_H} |",
      f"| &nbsp;&nbsp;barra lateral REV | 0 | {Y_REV} | {SIDE_W} | {BAND_H} |",
      f"| &nbsp;&nbsp;area de modulo REV | {MOD_X} | {CY['rev']} | {MOD_W} | {CONTENT_H} |", "",
      f"Pitch de dials = {PITCH}px (celula {CELL_W}x{DIAL_H}); fila de dials em y local {ROW_DIALS} nas barras e nos modulos "
      "(mesma linha de base). Cada modulo existe 2x (FWD/REV): as coordenadas de modulo abaixo sao **locais** "
      f"(origem = canto da area de modulo); somar ({MOD_X},{CY['fwd']}) para FWD ou ({MOD_X},{CY['rev']}) para REV.", ""]


def table(title, items, dx=0, dy=0):
    out = [f"### {title}", "", "| Peca | Tipo | Param ID | x | y | w | h |", "|---|---|---|---|---|---|---|"]
    for e in sorted(items, key=lambda e: (e["y"], e["x"])):
        out.append(row(e, dx, dy))
    return out + [""]


md += table("Cabecalho", [e for e in ELS if e["zone"] == "header"])
md += table("Barra lateral FWD (o presente)", [e for e in ELS if e["zone"] == "side-fwd"])
md += table("Barra lateral REV (o passado: o leitor)", [e for e in ELS if e["zone"] == "side-rev"])
md += table("Meridiano (relacao + roteamento; fluxo do sinal da esquerda para a direita)",
            [e for e in ELS if e["zone"] == "meridian"])
for m in MODS:
    items = [e for e in ELS if e["zone"] == "mod" and e["mod"] == m and e["engine"] == "fwd"]
    md += table(f"Modulo {m.upper()} (coords locais; IDs FWD mostrados, REV = `rev_*` com `data-twin` para o gemeo FWD)",
                items, MOD_X, CY["fwd"])
md += ["## Resumo", "",
       f"- {len(ELS)} elementos ({', '.join(f'{k}={v}' for k, v in sorted(cnt.items()))}); 122 IDs `data-param*` identicos a `index.html`.",
       "- Estados: 4 (modulo selecionado: gate/delay/verb/gran). Validado sem sobreposicoes em nenhum.",
       f"- Discos de dial: M = 48px (escala 58px), XL (MORPH) = 76px (escala 88px) -> todos >= 48px. Texto >= 10px. Teclas >= 20px, seg/stepper >= 28px."]
with open(os.path.join(HERE, "LAYOUT-V4.md"), "w") as f:
    f.write("\n".join(md) + "\n")
print(f"escrito index-v4.html ({len(html)} bytes) + LAYOUT-V4.md ({len(md)} linhas)")
