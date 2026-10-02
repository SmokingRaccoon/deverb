# deVerb UI — spec do tema e do layout (v4 "Espelho de Agua")

Janela fixa **1280×624**. FWD em cima (papel), REV em baixo (tinta),
meridiano com a relação (LINK/MORPH/TRIM). Sem tabs nem scroll.
Hand-off completo em `assets/mockup/DESIGN-V4.md` (conceito, tokens,
componentes, tether) e `assets/mockup/LAYOUT-V4.md` (geometria gerada,
não editar à mão — fonte-verdade `generate-v4.py`).

## Cores (`ui/WaterLnF.h`)

| Uso | Cor |
|-----|-----|
| papel / papel-2 / papel-3 | `#ECE9E2` / `#E3DFD6` / `#D6D1C5` |
| tinta / tinta-2 / tinta-3 | `#16171A` / `#1F2125` / `#2B2D32` |
| etiquetas papel / tinta | `#55544F` / `#9A9892` (≥4.5:1) |
| GATE acento | `#EC563A` (texto ON `#16171A`) |
| DELAY acento | `#2E9D6B` (texto ON `#16171A`) |
| VERB acento | `#3558C8` (texto ON `#ECE9E2`) |
| GRAN acento | `#F0B323` (texto ON `#16171A`) |
| linha de água | `#8D8A82` |

Acento só em áreas pequenas e planas (nunca único portador de informação).
Fonte embutida: Nimbus Sans Regular/Bold via BinaryData (`Source/ui/fonts/`,
`DeVerbFonts`); mono de valores fica no sistema.

## Componentes (`Source/ui/V4*.h`)

| HTML/mockup | JUCE | Notas |
|---|---|---|
| `.dial.m` | `V4Dial` + `WaterLnF::drawRotarySlider` | 21 ticks 270° desde −135°, disco ø48; REV com link mostra 2 agulhas (fantasma = herdado `FWD×trim`, sólida = próprio, ponto = efetivo `lerp` via `RevLinker`) |
| `.dial.mer/.xl` | `V4Dial("mer"/"xl")` | disco cortado pela linha de água (2 passes + `difference`), M 48 / XL 76; MORPH é o herói |
| `.seg` | `V4Seg` | opções todas visíveis (≤6) via `ParameterAttachment`; REV com link = tether (tracejado, valor FWD, clique abre UNLINK) |
| `.stepper` | `V4Stepper` | prev/valor/next + roda; notas, patterns (bits→fábrica), `chain_order`, preset global (role, sem param) |
| `.numbox` | `V4Num` | BPM arrastável + roda + duplo clique; só conta sem host (`HOST`/`INT` ao lado) |
| `.key` | `V4Key` | PWR (seta+lâmpada), LINK (anéis), THROW (momentâneo), RANDOM |
| `.tile` | `V4Tile` | pedra Bauhaus objeto+reflexo, seleciona módulo; PWR/LINK são `V4Key`s sobre a pedra |
| `.viz.*` | `V4Viz` | scope (FIFO lock-free), capture-window (atomics `revCapBeatsUi/revReadPosUi`), taps/decay/shards de params, IR reservado |

## Mapa da janela

- **Cabeçalho 44**: título, preset stepper, RANDOM, BPM numbox, `HOST`/`INT`.
- **Banda FWD 238** (papel): barra lateral (engine + scope + `FWD MIX`) + área de módulo 896×198.
- **Meridiano 104** (linha y=334): `INPUT → 4 pedras (cadeia = chain_order) → MORPH+ALL → THROW → T-DLY/T-DEC → X-MODE/ORDER → MASTER`.
- **Banda REV 238** (tinta): espelho do FWD (translação, nunca texto invertido).
- 8 `V4ModulePanel` (4 módulos × FWD/REV), attachments criados **uma vez** no ctor; `selectedModule` em `apvts.state/v4ui/selMod` (fora dos 122 IDs); visibilidade + `ComponentAnimator` 180ms (respeita reduced-motion).

## Regras para futuros widgets (ler antes de mexer)

1. Janela e IDs congelados: 1280×624, 122 params (`fwd_*`/`gr_*`, `rev_*`/`rev_gr_*`, 22 globais). Novos widgets mapeiam para params existentes ou são `visual`.
2. Attachments no ctor, nunca rebind (o cross-talk FWD|REV morreu com `bind*Column()`).
3. `resized()` só posiciona — sem `addItemList`, sem criar attachments.
4. Labels curtos nos segs (recalibrado com Nimbus real: `BEAT/SLICE/REV/PITCH/STUT`, `CHANCE/ENV/MANUAL`); texto ≥10px, dial ≥48px, teclas ≥20px, seg/stepper ≥22px.
5. ASCII no texto pintado.
6. Validar: `python3 assets/mockup/generate-v4.py` (122 IDs, 0 overlaps), `test_ui` sob `xvfb-run`, `pluginval --strictness-level 10`.

## Protocolo de sessões heavy-user

- `tests/test_ui.cpp` (sob `xvfb-run`): selMod+ValueTree, tweaks FWD/REV simultâneos sem cross-talk, tether (REV ligado não escreve → UNLINK), steps nos 2 motores, presets, RANDOM, PWR por motor, restore com UI aberta.
- Screenshots: standalone sob Xvfb + `xwd`/`ffmpeg` (ver `/tmp/opencode/v4-*.png`); `pluginval` 1.0.4 strict 10 = SUCCESS.
