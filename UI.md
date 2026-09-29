# deVerb UI — spec do tema e do layout (v2)

Janela fixa **1280×624**. Uma página horizontal, sem tabs nem scroll.
Tema escuro contrastado, um acento por módulo.

## Cores (`ui/AbletonLnF.h`)

| Uso | Cor |
|-----|-----|
| fundo | `#0e0e0e` |
| MOTOR acento/panel | `#b5b5b5` / `#151515` |
| GATE acento/panel | `#fed134` / `#1a1810` |
| DELAY acento/panel | `#58c472` / `#101a12` |
| VERB acento/panel | `#5aa9e6` / `#10141c` |
| GRAN acento/panel | `#ff9034` / `#1c1410` |
| REV strip acento/fundo | `#b48ce8` / `#14141a` |
| texto / dim / faint | `#d4d4d4` / `#8f8f8f` / `#5c5c5c` |
| borda / track | `#2e2e2e` / `#383838` |

Knobs herdam o acento do módulo via
`setColour (rotarySliderFillColourId, …)`; steps/freeze/manual/THROW usam o
amarelo; toggles de link e FWD|REV usam o roxo (`buttonOnColourId` por botão).
Headers: nome em cinzento bold 11px + barra de acento 44×3px.
Toggle desligado por `setEnabled(false)` (steps 9–16 em modo 8) se vê
esbatido — o LnF respeita `isEnabled`.

## Sistema de knobs (3 tamanhos, diâmetros consistentes)

| Classe | Célula | Diâmetro rotary | Uso |
|--------|--------|-----------------|-----|
| BIG | 88–100 × 110 | ~72–80 | MORPH, MASTER, knobs do gate |
| STD | 59–76 × 90 | ~52–60 | todas as filas de módulo |
| MINI | 52–56 × 58 | ~28–34 | faixa REV (rate/lfo/duck/trims), extras do delay |

Nomes pintados em faixa de 14px/10px por baixo de cada knob; values em
textbox por baixo do rotary (duplo clique = entrada numérica). Combos 11px,
toggles com texto centrado. Grelha base 8px; labels de secção em dim 11px.

## Mapa da janela

- **Topbar linha 1** (36px): título, BPM, GATE passo/total, preset global,
  segmentado **FWD|REV** (modo global de edição), LED GRAB|IDLE.
- **Topbar linha 2** (64px): faixa REV — mode/source/capture, rate/lfo/duck,
  THROW, links ALL/GATE/DLY/VRB/GRN, trims, order, xfade.
- **Colunas**: MOTOR·PERFORM (macros) / GATE·TRANCE (régua+steps+combos+knobs) /
  DELAY·ECHO (knobs+algo+minis+nota+readout+freeze) / VERB·SPACE (knobs+combos+
  readout+freeze) / GRAN·GLITCH (combos+knobs+interrupt).
- Readouts calculados: `375 ms - 1/8D` (delay), `T60 2.5 s` (verb).
- Ar livre no fundo de DELAY/VERB: reservado (algos Tape+ e loader de IRs).

## Regras para futuros widgets (ler antes de mexer)

1. Novos knobs entram numa das 3 classes (nunca tamanhos avulso).
2. Attachments criam-se **no ctor**; rebinds fazem `.reset()` primeiro —
   o initial update do novo attachment notifica o slider e o velho, ainda
   vivo, escreveria no parâmetro errado (cross-talk FWD|REV, apanhado pelo
   `test_ui`). Ver `bind*Column()`.
3. `resized()` só posiciona — nunca `addItemList` nem cria attachments
   (bug histórico: items duplicados por re-resize, apanhado pelo `test_ui`).
4. IDs de parâmetros congelados no v1; verificar dupes com o gerador da
   tabela do README (grep conta ocorrências).
5. ASCII no texto pintado (o `·` U+00B7 rendeu `Â·` neste sistema).

## Protocolo de sessões heavy-user

- `tests/test_ui.cpp` (correr sob `xvfb-run`): flips FWD>REV>FWD>REV>FWD com
  tweaks, cross-talk, pattern nos dois alvos, 11 presets, restore com UI
  aberta, idempotência de resize, coerência DSP.
- `/tmp/opencode/session{1,2,3}.sh` + `xdotool`: clicks, combos, stepped
  drags (50px de salto único NÃO regista — usar 10×5px) e setas do teclado;
  screenshots por passo em `/tmp/opencode/sess*-*.png`; quit limpo e
  verificação de `~/.config/deVerb.settings` + relaunch.
