# DESIGN-V4 — "Espelho de Água" (brief de hand-off)

Mockup 1:1 em px da UI v4 do deVerb. **FWD é o objeto, REV é o reflexo.** Esta nota é para quem vai
adaptar o HTML ao plugin (JUCE): o que é conceito, o que é regra, o que é só mockup.

Ficheiros (todos em `assets/mockup/`; v1–v3 intocados):

| Ficheiro | Papel | Editar à mão? |
|---|---|---|
| `generate-v4.py` | geometria + validador + emite `index-v4.html` e `LAYOUT-V4.md` | sim (fonte-verdade da geometria) |
| `styles-v4.css` | tokens e componentes (lido pelo gerador p/ validar contraste/mínimos) | sim |
| `app-v4.js` | interação do mockup + modo `?qa` (overflow de texto + 17 testes de fumo) | sim |
| `index-v4.html`, `LAYOUT-V4.md` | gerados | **não** |
| `shots-v4/*.png` | os 4 estados renderizados (Chromium headless, referência visual) | regenerar |

Correr: `python3 generate-v4.py` (falha se houver sobreposição, elemento fora da zona, mínimos violados,
contraste de tokens abaixo do alvo ou IDs diferentes dos 122 de `index.html`).
QA no browser: abrir `index-v4.html?qa`. Estados iniciais: `?sel=verb&unlink=gate,delay&morph=0.8&linkall=0&order=1`.

---

## 1. O conceito (para decidir o que não está escrito aqui)

*Reverb* ← latim *reverberare*, "bater de volta". A janela é um espelho horizontal; a **linha de água**
(meridiano, y = 334) separa **FWD** (positivo: papel, tinta escura, em cima) de **REV** (negativo: tinta,
papel claro, em baixo). Dois espelhos em simultâneo:

- **no tempo** — REV lê a janela capturada ao contrário (leitor ◂, cabeça de reprodução do gate a correr ao contrário);
- **dos ajustes** — LINK/MORPH/TRIM herdam FWD→REV. **MORPH = quão fiel é o reflexo.**

Frase-guia: *cada controlo REV conhece o seu gémeo FWD.* (`data-twin`.) Espelho = polaridade, posição e
movimento; **nunca texto invertido**. Os layouts de FWD e REV são idênticos (translação), por usabilidade.

Elementos que carregam o conceito (não os "limpar"):
dial do meridiano **cortado pela linha de água** (disco tinta em cima / papel em baixo; etiqueta acima, valor abaixo);
**pedras** com objeto + reflexo (glifo Bauhaus ■ ◖ ● ▲); THROW = pedra atirada à água (aneis achatados no meridiano);
laço LINK (dois anéis entrelaçados = ligado); selecionada = troço da linha de água na cor do módulo.

## 2. Grelha (1280×624) — valores em `LAYOUT-V4.md`

Cabeçalho 44 · banda FWD 238 · **meridiano 104** · banda REV 238. Barra lateral 336 · goteira 24 · área de módulo 896.
Pitch dos dials 72 (célula 64×86); **fila de dials em y local 112 nas barras e nos módulos** (mesma linha de base).
Meridiano, esq→dir = fluxo do sinal: `INPUT → 4 pedras (cadeia) → MORPH+ALL → THROW → T-DLY/T-DEC → X-MODE/ORDER → MASTER`.

Cada módulo existe **2×** (FWD/REV) em coordenadas locais da área de módulo (somar (360,64) ou (360,406)).
Só o módulo selecionado é visível; os 4 estados estão validados sem sobreposições.

## 3. Tokens (`styles-v4.css`, `:root`)

| Token | Valor | Nota |
|---|---|---|
| `--paper / -2 / -3` | `#ECE9E2 / #E3DFD6 / #D6D1C5` | superfície, barra lateral, passos alternados |
| `--ink / -2 / -3` | `#16171A / #1F2125 / #2B2D32` | idem, negativo |
| `--lab-p / --lab-i` | `#55544F / #9A9892` | etiquetas (≥4.5:1 em todos os fundos, verificado) |
| `--gate/delay/verb/gran` | `#EC563A / #2E9D6B / #3558C8 / #F0B323` | vermelhão / jade / índigo / mostarda; **só áreas pequenas e planas** |
| `--on-*` | texto sobre o acento | calculado p/ ≥4.5:1 |
| `--disc-m / --disc-xl` | 48px / 76px | disco do dial (escala de ticks fora: 58 / 88) |

Polaridade por `data-pol` (`p`/`x` = papel, `n` = tinta) → define `--bg --fg --lab --line`. Acento por `data-acc`.
Tipografia: Helvetica-class (stack no CSS) para UI, mono tabular para valores; **≥10 px sempre**; **só ASCII**
no texto pintado (`UI.md` regra 5) — setas e anéis são desenhados, não caracteres.

Avisos conhecidos do gerador (aceites): mostarda/vermelhão/jade sobre papel e índigo sobre tinta ficam <3:1 como
*gráfico*. Por isso o acento nunca é o único portador de informação (há forma, posição e etiqueta) e, em teclas ON,
o texto é escolhido para ≥4.5:1 contra o acento.

## 4. Componentes → JUCE

| HTML | Componente | Comportamento |
|---|---|---|
| `.dial.m` | rotary | 21 ticks em 270° a partir de −135°, "acesos" até ao valor **efetivo**; disco ø48 + índice; **ponto de acento sobre a escala = valor efetivo**. Arrasto vertical (Shift = fino), roda, duplo clique = default. |
| `.dial.m` REV (`.linked`) | rotary de **duas agulhas** | + agulha-fantasma oca = valor herdado (`FWD × trim`), agulha sólida = valor próprio, ponto = efetivo. Reutilizar a mesma conta de `RevLinker::resolveContinuous` (`lerp(linked, próprio, MORPH)`; trims só em `delay_time` e `verb_decay`). Link OFF → só uma agulha. |
| `.dial.mer`, `.dial.xl` | rotary cortado | pintar o disco em 2 passes com clip acima/abaixo de y=334 (cores trocadas); agulha em *difference* = pintar 2× com clip. |
| `.seg` | **novo** `SegmentedChoice` | todas as opções visíveis (≤6). Liga a um parâmetro *choice* via `juce::ParameterAttachment` (mesma semântica do `ComboBoxAttachment`). Opção ativa = fundo `fg` + sublinhado de acento. |
| `.stepper` | **novo** `StepperChoice` | ◂ valor ▸ + roda; notas (12) e presets. |
| `.numbox` | número arrastável | `tempo_bpm`; só tem efeito sem host (mostrar `HOST`/`INT`). |
| `.key` | toggle | `.on` = acento; `.pwr` = seta do motor + lâmpada (FWD ▸, REV ◂); `.link` = anéis. |
| `.stp` | passo | 16 em grupos de 4, tons alternados por beat; ON = acento; cabeça = barra de 3px. REV: índice `15−i` (corre ao contrário). 8 passos → 9–16 esbatidos. |
| `.tile` + 3 teclas | pedra | seleciona módulo; teclas `data-tile`: PWR FWD (cima), LINK (sobre a linha), PWR REV (baixo). **PWR vive só aqui** (fonte única). |
| `.viz.*` | visuais | ver §6 (quais precisam de dados do DSP). |

### Tether (a regra de UX mais importante)
Com `link_master && link_<módulo>`: controlos **discretos** do REV (segmentados, steppers, passos, PWR, freeze/grab/interrupt)
só *seguem* o FWD (`resolveDiscrete/Bool`) → mostram o valor FWD, estilo tracejado + 50% opacidade, e um clique
abre aviso "LINK X ON — REV FOLLOWS FWD [UNLINK X]" (`juce::CallOutBox`) em vez de não fazer nada. Os **contínuos** continuam
editáveis (o valor próprio pesa `MORPH`). `link_master` OFF liberta tudo.

### Estado de UI (não é parâmetro)
`selectedModule` (gate|delay|verb|gran). Pode ir para uma propriedade da árvore de estado, fora dos 122 IDs.

## 5. Arquitetura sugerida (elimina a classe de bugs de `UI.md` regra 2)

`ModulePanel(engine, module)` instanciado **8× no construtor** (4 módulos × FWD/REV), cada um com os seus attachments
criados **uma vez**; mudar de módulo só altera visibilidade. Desaparecem `bind*Column()`, `.reset()` de attachments e o
cross-talk FWD|REV. `resized()` só posiciona (regra 3 mantém-se). IDs: FWD `fwd_*`/`gr_*`, REV `rev_*`/`rev_gr_*` — cada
elemento do REV tem `data-twin` para o gémeo.

Outros pontos: ordem da cadeia = posições das pedras a partir de `chain_order` (animar com `ComponentAnimator`);
subir/descer espelhado ao trocar de módulo (180 ms ease-out, FWD sobe do meridiano, REV desce); anéis do THROW só
com Timer enquanto ativos; respeitar redução de movimento.

## 6. O que é só mockup / precisa de dados do processador

| Elemento | Estado no mockup | Para o plugin real |
|---|---|---|
| `scope` (FWD) | forma de onda estática | FIFO lock-free do sinal de entrada |
| `capture-window` (REV) | janela 2/3/4 beats + cabeça a varrer ◂ (CSS) | posição de leitura/rate do `ReverseEngine` via atomics |
| `gate-readout`, playhead | simulados a 120 BPM | já existe (`gateReadout`) |
| `gran-led` | GRAB/IDLE por clique | já existe (LED) |
| `delay-taps`, `verb-decay`, `gran-shards` | desenhados **a partir dos parâmetros** (podem ser pintados sem DSP) | em v4 estático; ligar a `fb/time`, `decay`, `chance/repeats` |
| `ir-placeholder` | reservado "CONVOLUTION PHASE 8" | slot do loader de IRs (fase 8) |
| valores iniciais REV ≠ FWD | seeds para ver as agulhas | defaults reais são iguais ao FWD |

## 7. Mapa de vocabulário v3 → v4

`knob` → `dial` (+`.m/.mer/.xl`) · `combo` → `seg` (≤6 opções) ou `stepper` (notas/presets) · `tbtn.tog` → `key` ·
`step` → `stp` · `visual.*` → `viz.*` · `.panel` → zonas (cabeçalho/barras/módulo/meridiano) · `data-module` → `data-acc`
(cor) + `data-mod` (grupo de visibilidade). Novos: `data-engine`, `data-pol`, `data-twin`, `data-link`, `data-tile`,
`data-select`, `data-options`, `data-i`. **`data-param`, `data-param-fwd`, `data-param-rev`, `data-role`, `data-step`
mantêm-se** (nos widgets FWD só `-fwd`; nos REV só `-rev` + `data-twin`).

## 8. Desvios face ao plano aprovado (e porquê)

- **Só 2 tamanhos de dial** (M, XL). O "L ø72" desapareceu: FWD/REV MIX ficaram no slot 4 da fila de dials das barras
  (mesma linha de base que os módulos) — mais limpo e cumpre ≥48 px.
- **THROW passou para o meridiano** (junto ao MORPH): gesto de performance + metáfora (pedra → anéis). A barra REV ficou com o leitor puro.
- **PWR de módulo só nas pedras** (8 lâmpadas sempre visíveis), não repetido nas bandas.
- **CAPTURE** (2/3/4) como segmentado com legenda "BEATS"; a janela de captura desenhada acompanha-o.
- Acento do vermelhão `#E4452B → #EC563A` e do índigo `#2447A8 → #3558C8` para cumprir ≥4.5:1 com o texto sobre a tecla ON.

## 9. Verificação feita

- Gerador: 193 elementos, **122 IDs idênticos** a `index.html`, 0 sobreposições nos 4 estados, tudo dentro da zona.
- Contraste: 12 pares de texto ≥4.5:1 (etiquetas 4.8–6.3, tinta/papel 14.8).
- Chromium 154 headless: os 4 estados renderizados e inspecionados (`shots-v4/`); `?qa`: **sem overflow de texto**
  (larguras de segmentados calibradas contra a medição real) + **17 testes de fumo** (seleção, link, MORPH 0/½/1, tether,
  UNLINK, trims, MS esbatido, passos espelhados, ordem da cadeia, ALL, reset).
- **Não verificado:** fonte final (o render usa Nimbus Sans; recalibrar com `?qa` quando a fonte for embutida), desempenho de
  repintura em JUCE, uso real com DAW.
