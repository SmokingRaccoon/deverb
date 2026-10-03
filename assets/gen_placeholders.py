"""Gera placeholders por peça para o designer (fundo transparente,
outline tracejado amarelo + etiqueta nome+dimensões).
Correr: python3 gen_placeholders.py  (a partir da pasta assets/)
Saída: placeholders/*.png
"""
import os
from PIL import Image, ImageDraw, ImageFont

ACCENT = (254, 209, 52, 255)
DIM = (140, 140, 140, 255)
FAINT = (70, 70, 70, 255)

PARTS = [
    # (ficheiro, w, h, etiqueta) — geometrias REAIS do layout (PluginEditor.cpp)
    # Knobs: arco Base 270 graus; diametro = min(w-8, area_rotary-4); nome 10px + value-box.
    ("knob-96x110-d74.png", 96, 110, "KNOB 96x110 ARC74"),
    ("knob-88x110-d74.png", 88, 110, "KNOB 88x110 ARC74"),
    ("knob-84x90-d54.png", 84, 90, "KNOB 84x90 ARC54"),
    ("knob-72x90-d54.png", 72, 90, "KNOB 72x90 ARC54"),
    ("knob-64x90-d54.png", 64, 90, "KNOB 64x90 ARC54"),
    ("knob-56x58-d38.png", 56, 58, "KNOB 56x58 ARC38"),
    ("knob-110x64-d44.png", 110, 64, "KNOB 110x64 ARC44"),
    # Toggles (estados: on / off / disabled).
    ("toggle-56x30.png", 56, 30, "TOGGLE 56x30"),
    ("toggle-90x30.png", 90, 30, "TOGGLE 90x30"),
    ("toggle-90x22.png", 90, 22, "TOGGLE 90x22"),
    ("toggle-68x22.png", 68, 22, "TOGGLE 68x22"),
    ("toggle-120x32.png", 120, 32, "TOGGLE 120x32"),
    ("toggle-124x32.png", 124, 32, "TOGGLE 124x32"),
    ("toggle-126x30.png", 126, 30, "TOGGLE 126x30"),
    ("toggle-48x20.png", 48, 20, "PWR 48x20"),
    # Combos (estados: fechada / aberta+lista / disabled). Seta a direita.
    ("combo-68x22.png", 68, 22, "COMBO 68x22"),
    ("combo-90x30.png", 90, 30, "COMBO 90x30"),
    ("combo-110x30.png", 110, 30, "COMBO 110x30"),
    ("combo-122x32.png", 122, 32, "COMBO 122x32"),
    ("combo-126x30.png", 126, 30, "COMBO 126x30"),
    ("combo-144x32.png", 144, 32, "COMBO 144x32"),
    ("combo-224x32.png", 224, 32, "COMBO 224x32"),
    ("combo-240x22.png", 240, 22, "COMBO 240x22"),
    ("combo-76x30.png", 76, 30, "COMBO 76x30"),
    ("combo-72x30.png", 72, 30, "COMBO 72x30"),
    # Gate.
    ("step-34x46.png", 34, 46, "STEP 34x46"),
    ("ruler-288x44.png", 288, 44, "RULER 288x44"),
    # Panels e estrutura.
    ("panel-motor-184x508.png", 184, 508, "PANEL MOTOR 184x508"),
    ("panel-gate-296x508.png", 296, 508, "PANEL GATE 296x508"),
    ("panel-delay-232x508.png", 232, 508, "PANEL DELAY 232x508"),
    ("panel-verb-256x508.png", 256, 508, "PANEL VERB 256x508"),
    ("panel-gran-264x508.png", 264, 508, "PANEL GRAN 264x508"),
    ("topbar-1280x100.png", 1280, 100, "TOPBAR 1280x100"),
    ("window-1280x624.png", 1280, 624, "WINDOW 1280x624"),
]


def dashed_rect(d, x0, y0, x1, y1, fill, dash=6, width=2):
    for x in range(x0, x1, dash * 2):
        d.line([(x, y0), (min(x + dash, x1), y0)], fill=fill, width=width)
        d.line([(x, y1), (min(x + dash, x1), y1)], fill=fill, width=width)
    for y in range(y0, y1, dash * 2):
        d.line([(x0, y), (x0, min(y + dash, y1))], fill=fill, width=width)
        d.line([(x1, y), (x1, min(y + dash, y1))], fill=fill, width=width)


def make_placeholder(w, h, label):
    img = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    # cruz fantasma
    d.line([(0, 0), (w, h)], fill=FAINT, width=1)
    d.line([(0, h), (w, 0)], fill=FAINT, width=1)
    # outline tracejado
    dashed_rect(d, 1, 1, w - 2, h - 2, ACCENT)
    # etiqueta (2 linhas se preciso)
    try:
        font = ImageFont.load_default(size=11)
    except TypeError:
        font = ImageFont.load_default()
    parts = label.split(" ", 1)
    lines = [parts[0], parts[1]] if len(parts) > 1 and w < 150 else [label]
    ty = h // 2 - 8 * len(lines)
    for ln in lines:
        bb = d.textbbox((0, 0), ln, font=font)
        tw = bb[2] - bb[0]
        d.text(((w - tw) // 2, ty), ln, font=font, fill=DIM)
        ty += 14
    return img


if __name__ == "__main__":
    os.chdir(os.path.dirname(os.path.abspath(__file__)))
    os.makedirs("placeholders", exist_ok=True)
    for fname, w, h, label in PARTS:
        make_placeholder(w, h, label).save(f"placeholders/{fname}")
        print(f"placeholders/{fname} ({w}x{h})")
