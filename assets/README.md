# assets/ — templates de imagery do deVerb

Fundo procedural (PIL + numpy, sem ficheiros externos), à medida da janela
(1280×624), no tema escuro do plugin. Servem de **base para assets finais**:
podes usar como fundo direto, como camada por baixo de painéis, ou como
referência de paleta/textura para um designer.

- `bg-dark-texture.png` — neutro: gradiente carvão + grão fino + grelha
  64px fantasma + vinheta. Seguro por baixo de qualquer UI (texto legível).
- `bg-accent-glow.png` — igual + bloom âmbar `#fed134` subtil em baixo à
  esquerda + fio de luz no topo. Para variante "premium"/hero.
- `gen_assets.py` — gerador; corre `python3 gen_assets.py` aqui para
  regenerar (seed fixa → reproduzível). Ajusta cores/intensidades no script.

Uso futuro no JUCE: `ImageFileFormat::loadFrom (BinaryData/...)` com as
imagens em `juce_add_binary_data`, desenhadas no `paint()` do editor por
baixo dos controlos. Quando houver arte final (AI/designer), substitui estes
ficheiros mantendo os nomes — nada no código precisa de mudar.

---

## Prancheta do designer — screenshot + placeholders por peça

- **`reference-ui-1280x624.png`** — screenshot integral do plugin em defaults
  (janela 1280×624). É a referência: desenha POR CIMA ou ao lado.
- **`placeholders/`** — um PNG por geometria REAL do layout, fundo
  transparente, outline tracejado + etiqueta com dimensões. Gerados por
  `gen_placeholders.py` (corre para regenerar após mudanças de layout).

### Knobs (arco 270°, agulha, nome 10px, value-box; stroke 3px)

| Peça | Tamanho | Arco | Onde é usada |
|------|---------|------|--------------|
| `knob-96x110-d74.png` | 96×110 | ø74 | Gate: smooth/depth/mix/pan/thr |
| `knob-88x110-d74.png` | 88×110 | ø74 | Motor: morph/master |
| `knob-84x90-d54.png` | 84×90 | ø54 | Motor: fwdMix/revMix |
| `knob-72x90-d54.png` | 72×90 | ø54 | Motor: input; Delay: bpm/time/fb/damp/dmix |
| `knob-64x90-d54.png` | 64×90 | ø54 | Verb ×8; Gran ×9 |
| `knob-56x58-d38.png` | 56×58 | ø38 | Topbar: rate/lfo/duck/trims (sem nome) |
| `knob-110x64-d44.png` | 110×64 | ø44 | Delay: drive/wowRate/wowDepth/spread |

Acento por módulo: gate `#fed134`, delay `#58c472`, verb `#5aa9e6`,
gran `#ff9034`, motor cinzento, topbar roxo `#b48ce8`.
Estados a desenhar por knob: arco vazio (track) / arco preenchido 0–100% /
agulha / value-box com texto / disabled (tudo faint).

### Toggles (estados: on / off / disabled)

| Peça | Tamanho | Onde é usada |
|------|---------|--------------|
| `toggle-56x30.png` | 56×30 | Links ALL/GATE/DLY/VRB/GRN |
| `toggle-90x30.png` | 90×30 | THROW |
| `toggle-90x22.png` | 90×22 | FWD/REV (segmentado topbar) |
| `toggle-68x22.png` | 68×22 | RANDOM |
| `toggle-120x32.png` | 120×32 | Freeze (delay) |
| `toggle-124x32.png` | 124×32 | Freeze (verb) |
| `toggle-126x30.png` | 126×30 | GRAB, Interrupt, manual |
| `toggle-48x20.png` | 48×20 | PWR por coluna (gate/delay/verb/gran) |

### Combos (estados: fechada / aberta+lista / disabled; seta à direita)

| Peça | Tamanho | Onde é usada |
|------|---------|--------------|
| `combo-68x22.png` | 68×22 | RANDOM, FWD/REV |
| `combo-90x30.png` | 90×30 | revSource, revCapture |
| `combo-110x30.png` | 110×30 | revModeBox |
| `combo-122x32.png` | 122×32 | algoBox, preNoteBox |
| `combo-126x30.png` | 126×30 | grMode/grTrig/grLen/grTimeNote, manual é toggle |
| `combo-144x32.png` | 144×32 | preset/rate/steps/trig (gate) |
| `combo-224x32.png` | 224×32 | delayAlgo, noteBox |
| `combo-240x22.png` | 240×22 | globalPresetBox (topbar) |
| `combo-76x30.png` | 76×30 | orderBox |
| `combo-72x30.png` | 72×30 | xfadeBox |

### Gate + estrutura

| Peça | Tamanho | Notas |
|------|---------|-------|
| `step-34x46.png` | 34×46 | 16 botões 2×8; estados on/off/disabled |
| `ruler-288x44.png` | 288×44 | régua de beats + passo ativo iluminado |
| `panel-motor-184x508.png` | 184×508 | tints: motor `#151515` |
| `panel-gate-296x508.png` | 296×508 | tint `#1a1810` |
| `panel-delay-232x508.png` | 232×508 | tint `#101a12` |
| `panel-verb-256x508.png` | 256×508 | tint `#10141c` |
| `panel-gran-264x508.png` | 264×508 | tint `#1c1410` |
| `topbar-1280x100.png` | 1280×100 | fundo `#14141a`, com BPM/gate readouts + LEDs |
| `window-1280x624.png` | 1280×624 | janela completa; fundo `#0e0e0e` |

Labels de texto (bpmReadout, gateReadout, readouts ms/T60, LEDs): só tipografia,
sem asset. Fonte UI: 10–11px cinzento, títulos 17px bold.
