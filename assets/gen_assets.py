"""Gera os templates de fundo do deVerb (procedural, sem assets externos).
Saída em assets/: fundo neutro + variante com glow âmbar. 1280x624 = janela.
Correr: python3 gen_assets.py  (a partir da pasta assets/)
"""
import numpy as np
from PIL import Image, ImageDraw

W, H = 1280, 624

def base_array():
    y = np.linspace(0, 1, H)[:, None]
    x = np.linspace(0, 1, W)[None, :]
    # gradiente vertical subtil: topo #171717 -> fundo #0b0b0b
    g = np.broadcast_to(23 - 12 * y, (H, W))
    img = np.stack([g, g, g], axis=-1)
    # grão de filme fino (±3.5 níveis)
    rng = np.random.default_rng(0xDEADBEEF)
    img += rng.normal(0, 3.5, (H, W, 1))
    # vinheta radial suave (~22% nos cantos)
    cx, cy = 0.5, 0.55
    r = np.sqrt(((x - cx) * 1.15) ** 2 + ((y - cy) * 1.6) ** 2)
    img *= (1.0 - 0.22 * np.clip(r, 0, 1) ** 2)[..., None]
    return np.clip(img, 0, 255).astype(np.uint8)

def add_grid(pil):
    d = ImageDraw.Draw(pil, "RGBA")
    for gx in range(0, W, 64):
        d.line([(gx, 0), (gx, H)], fill=(255, 255, 255, 7))
    for gy in range(0, H, 64):
        d.line([(0, gy), (W, gy)], fill=(255, 255, 255, 7))
    return pil

def add_glow(arr):
    y = np.linspace(0, 1, H)[:, None]
    x = np.linspace(0, 1, W)[None, :]
    # brilho âmbar (#fed134) a nascer de baixo-esquerda, muito subtil
    r = np.sqrt(((x - 0.28) * 1.1) ** 2 + ((y - 1.05) * 1.9) ** 2)
    glow = np.clip(1 - r, 0, 1) ** 2.2 * 0.16
    amber = np.array([254, 209, 52], dtype=float)
    out = arr.astype(float) * (1 - glow[..., None]) + amber * glow[..., None]
    # fio de luz ténue no topo
    top = np.clip(1 - np.abs(y - 0.015) * 40, 0, 1) * 6
    out += top[..., None]
    return np.clip(out, 0, 255).astype(np.uint8)

if __name__ == "__main__":
    import os
    os.chdir(os.path.dirname(os.path.abspath(__file__)))
    neutral = add_grid(Image.fromarray(base_array()))
    neutral.save("bg-dark-texture.png")
    print("bg-dark-texture.png", neutral.size)

    accent = add_grid(Image.fromarray(add_glow(base_array())))
    accent.save("bg-accent-glow.png")
    print("bg-accent-glow.png", accent.size)
