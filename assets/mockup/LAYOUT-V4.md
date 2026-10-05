# LAYOUT-V4 - geometria gerada (deVerb UI v4, Espelho de Agua)

Gerado por `generate-v4.py` (nao editar a mao). Janela 1280x624, origem (0,0) topo-esquerda.

## Grelha

| Zona | x | y | w | h |
|---|---|---|---|---|
| Cabecalho (papel) | 0 | 0 | 1280 | 44 |
| Banda FWD (papel) | 0 | 44 | 1280 | 238 |
| &nbsp;&nbsp;barra lateral FWD | 0 | 44 | 336 | 238 |
| &nbsp;&nbsp;area de modulo FWD | 360 | 64 | 896 | 198 |
| Meridiano (linha de agua em y=334) | 0 | 282 | 1280 | 104 |
| Banda REV (tinta) | 0 | 386 | 1280 | 238 |
| &nbsp;&nbsp;barra lateral REV | 0 | 386 | 336 | 238 |
| &nbsp;&nbsp;area de modulo REV | 360 | 406 | 896 | 198 |

Pitch de dials = 72px (celula 64x86); fila de dials em y local 112 nas barras e nos modulos (mesma linha de base). Cada modulo existe 2x (FWD/REV): as coordenadas de modulo abaixo sao **locais** (origem = canto da area de modulo); somar (360,64) para FWD ou (360,406) para REV.

### Cabecalho

| Peca | Tipo | Param ID | x | y | w | h |
|---|---|---|---|---|---|---|
| title | visual | - | 24 | 0 | 140 | 44 |
| Preset | stepper | role:`factory-preset` | 508 | 8 | 272 | 28 |
| RANDOM | tecla (acao) | role:`random` | 788 | 8 | 88 | 28 |
| BPM | caixa numerica (arrastar/escrever) | `tempo_bpm` | 1040 | 8 | 128 | 28 |
| bpm-source | visual | - | 1176 | 8 | 80 | 28 |

### Barra lateral FWD (o presente)

| Peca | Tipo | Param ID | x | y | w | h |
|---|---|---|---|---|---|---|
| engine-fwd | visual | - | 24 | 64 | 288 | 28 |
| scope | visual | - | 24 | 100 | 288 | 68 |
| FWD MIX | dial M | `fwd_mix` | 240 | 176 | 64 | 86 |
| SIZE | dial M | `dim_size` | 24 | 186 | 64 | 86 |
| WIDTH | dial M | `dim_mix` | 96 | 186 | 64 | 86 |

### Barra lateral REV (o passado: o leitor)

| Peca | Tipo | Param ID | x | y | w | h |
|---|---|---|---|---|---|---|
| engine-rev | visual | - | 24 | 406 | 112 | 28 |
| REV Mode | segmentado | `rev_mode` | 144 | 406 | 168 | 28 |
| REV Source | segmentado | `rev_source` | 24 | 440 | 112 | 28 |
| REV Capture | segmentado | `rev_capture` | 144 | 440 | 148 | 28 |
| capture-window | visual | - | 24 | 474 | 288 | 36 |
| RATE | dial M | `rev_rate` | 24 | 518 | 64 | 86 |
| LFO | dial M | `rev_lfo` | 96 | 518 | 64 | 86 |
| DUCK | dial M | `rev_duck` | 168 | 518 | 64 | 86 |
| REV MIX | dial M | `rev_mix` | 240 | 518 | 64 | 86 |

### Meridiano (relacao + roteamento; fluxo do sinal da esquerda para a direita)

| Peca | Tipo | Param ID | x | y | w | h |
|---|---|---|---|---|---|---|
| pedra GATE | pedra (seleciona o modulo) | - | 106 | 290 | 124 | 88 |
| pedra DELAY | pedra (seleciona o modulo) | - | 238 | 290 | 124 | 88 |
| pedra VERB | pedra (seleciona o modulo) | - | 370 | 290 | 124 | 88 |
| pedra GRAN | pedra (seleciona o modulo) | - | 502 | 290 | 124 | 88 |
| MORPH | dial XL (heroi) | `morph` | 644 | 290 | 88 | 88 |
| INPUT | dial M (meridiano) | `input_gain` | 24 | 291 | 64 | 88 |
| T-DLY | dial M (meridiano) | `trim_delay` | 904 | 291 | 64 | 88 |
| T-DEC | dial M (meridiano) | `trim_decay` | 976 | 291 | 64 | 88 |
| MASTER | dial M (meridiano) | `master` | 1192 | 291 | 64 | 88 |
| GATE FWD PWR | PWR FWD | `fwd_gate_on` | 114 | 294 | 40 | 20 |
| DELAY FWD PWR | PWR FWD | `fwd_delay_on` | 246 | 294 | 40 | 20 |
| VERB FWD PWR | PWR FWD | `fwd_verb_on` | 378 | 294 | 40 | 20 |
| GRAN FWD PWR | PWR FWD | `gr_on` | 510 | 294 | 40 | 20 |
| morph-readout | visual | - | 740 | 296 | 56 | 34 |
| X Mode | segmentado | `x_mode` | 1058 | 296 | 116 | 28 |
| THROW | tecla (performance) | `rev_throw` | 814 | 314 | 72 | 40 |
| LINK GATE | LINK (FWD->REV) | `link_gate` | 114 | 324 | 40 | 20 |
| LINK DELAY | LINK (FWD->REV) | `link_delay` | 246 | 324 | 40 | 20 |
| LINK VERB | LINK (FWD->REV) | `link_verb` | 378 | 324 | 40 | 20 |
| LINK GRAN | LINK (FWD->REV) | `link_gran` | 510 | 324 | 40 | 20 |
| LINK ALL | LINK master | `link_master` | 740 | 342 | 52 | 20 |
| Chain Order | stepper | `chain_order` | 1058 | 344 | 116 | 28 |
| GATE REV PWR | PWR REV | `rev_gate_on` | 114 | 354 | 40 | 20 |
| DELAY REV PWR | PWR REV | `rev_delay_on` | 246 | 354 | 40 | 20 |
| VERB REV PWR | PWR REV | `rev_verb_on` | 378 | 354 | 40 | 20 |
| GRAN REV PWR | PWR REV | `rev_gr_on` | 510 | 354 | 40 | 20 |

### Modulo GATE (coords locais; IDs FWD mostrados, REV = `rev_*` com `data-twin` para o gemeo FWD)

| Peca | Tipo | Param ID | x | y | w | h |
|---|---|---|---|---|---|---|
| title-gate | visual | - | 0 | 0 | 300 | 24 |
| ruler | visual | - | 0 | 28 | 660 | 14 |
| gate-readout | visual | - | 684 | 28 | 212 | 74 |
| step 1 | passo | `fwd_gate_pattern` | 0 | 46 | 36 | 56 |
| step 2 | passo | `fwd_gate_pattern` | 40 | 46 | 36 | 56 |
| step 3 | passo | `fwd_gate_pattern` | 80 | 46 | 36 | 56 |
| step 4 | passo | `fwd_gate_pattern` | 120 | 46 | 36 | 56 |
| step 5 | passo | `fwd_gate_pattern` | 168 | 46 | 36 | 56 |
| step 6 | passo | `fwd_gate_pattern` | 208 | 46 | 36 | 56 |
| step 7 | passo | `fwd_gate_pattern` | 248 | 46 | 36 | 56 |
| step 8 | passo | `fwd_gate_pattern` | 288 | 46 | 36 | 56 |
| step 9 | passo | `fwd_gate_pattern` | 336 | 46 | 36 | 56 |
| step 10 | passo | `fwd_gate_pattern` | 376 | 46 | 36 | 56 |
| step 11 | passo | `fwd_gate_pattern` | 416 | 46 | 36 | 56 |
| step 12 | passo | `fwd_gate_pattern` | 456 | 46 | 36 | 56 |
| step 13 | passo | `fwd_gate_pattern` | 504 | 46 | 36 | 56 |
| step 14 | passo | `fwd_gate_pattern` | 544 | 46 | 36 | 56 |
| step 15 | passo | `fwd_gate_pattern` | 584 | 46 | 36 | 56 |
| step 16 | passo | `fwd_gate_pattern` | 624 | 46 | 36 | 56 |
| SMOOTH | dial M | `fwd_gate_smooth` | 0 | 112 | 64 | 86 |
| DEPTH | dial M | `fwd_gate_depth` | 72 | 112 | 64 | 86 |
| MIX | dial M | `fwd_gate_mix` | 144 | 112 | 64 | 86 |
| PAN | dial M | `fwd_gate_pan` | 216 | 112 | 64 | 86 |
| THR | dial M | `fwd_gate_env_thr` | 288 | 112 | 64 | 86 |
| Pattern | stepper | `fwd_gate_pattern` | 372 | 120 | 256 | 28 |
| Gate Rate | stepper | `fwd_gate_rate` | 640 | 120 | 256 | 28 |
| Gate Steps | segmentado | `fwd_gate_steps` | 372 | 156 | 152 | 28 |
| Gate Trig | segmentado | `fwd_gate_trig` | 536 | 156 | 360 | 28 |

### Modulo DELAY (coords locais; IDs FWD mostrados, REV = `rev_*` com `data-twin` para o gemeo FWD)

| Peca | Tipo | Param ID | x | y | w | h |
|---|---|---|---|---|---|---|
| title-delay | visual | - | 0 | 0 | 300 | 24 |
| delay-ms | visual | - | 560 | 0 | 336 | 24 |
| Delay Algo | segmentado | `fwd_delay_algo` | 0 | 32 | 400 | 28 |
| Delay Note | stepper | `fwd_delay_note` | 416 | 32 | 176 | 28 |
| Delay Freeze | tecla | `fwd_delay_freeze` | 608 | 32 | 88 | 28 |
| delay-taps | visual | - | 0 | 70 | 560 | 36 |
| MS | dial M | `fwd_delay_time` | 0 | 112 | 64 | 86 |
| FB | dial M | `fwd_delay_fb` | 72 | 112 | 64 | 86 |
| DAMP | dial M | `fwd_delay_damp` | 144 | 112 | 64 | 86 |
| MIX | dial M | `fwd_delay_mix` | 216 | 112 | 64 | 86 |
| DRIVE | dial M | `fwd_delay_drive` | 288 | 112 | 64 | 86 |
| WOW RT | dial M | `fwd_delay_wow_rate` | 360 | 112 | 64 | 86 |
| WOW DP | dial M | `fwd_delay_wow_depth` | 432 | 112 | 64 | 86 |
| SPREAD | dial M | `fwd_delay_spread` | 504 | 112 | 64 | 86 |

### Modulo VERB (coords locais; IDs FWD mostrados, REV = `rev_*` com `data-twin` para o gemeo FWD)

| Peca | Tipo | Param ID | x | y | w | h |
|---|---|---|---|---|---|---|
| title-verb | visual | - | 0 | 0 | 300 | 24 |
| verb-t60 | visual | - | 560 | 0 | 336 | 24 |
| ir-placeholder | visual | - | 600 | 30 | 296 | 76 |
| Verb Algo | segmentado | `fwd_verb_algo` | 0 | 32 | 275 | 28 |
| PreDelay Note | stepper | `fwd_verb_predelay_note` | 291 | 32 | 180 | 28 |
| Verb Freeze | tecla | `fwd_verb_freeze` | 487 | 32 | 88 | 28 |
| verb-decay | visual | - | 0 | 70 | 540 | 36 |
| SIZE | dial M | `fwd_verb_size` | 0 | 112 | 64 | 86 |
| DECAY | dial M | `fwd_verb_decay` | 72 | 112 | 64 | 86 |
| DAMP | dial M | `fwd_verb_damp` | 144 | 112 | 64 | 86 |
| WIDTH | dial M | `fwd_verb_width` | 216 | 112 | 64 | 86 |
| PRE-DLY | dial M | `fwd_verb_predelay` | 288 | 112 | 64 | 86 |
| LO-CUT | dial M | `fwd_verb_locut` | 360 | 112 | 64 | 86 |
| HI-CUT | dial M | `fwd_verb_hicut` | 432 | 112 | 64 | 86 |
| MIX | dial M | `fwd_verb_mix` | 504 | 112 | 64 | 86 |

### Modulo GRAN (coords locais; IDs FWD mostrados, REV = `rev_*` com `data-twin` para o gemeo FWD)

| Peca | Tipo | Param ID | x | y | w | h |
|---|---|---|---|---|---|---|
| title-gran | visual | - | 0 | 0 | 300 | 24 |
| gran-led | visual | - | 790 | 0 | 106 | 24 |
| Gran Mode | segmentado | `gr_mode` | 0 | 30 | 340 | 28 |
| Gran Trig | segmentado | `gr_trigger` | 356 | 30 | 220 | 28 |
| Gran GRAB | tecla | `gr_manual` | 592 | 30 | 72 | 28 |
| Gran Interrupt | tecla | `gr_interrupt` | 672 | 30 | 96 | 28 |
| gran-shards | visual | - | 410 | 64 | 330 | 42 |
| Gran Len | stepper | `gr_len_note` | 0 | 66 | 176 | 28 |
| Gran Time Note | stepper | `gr_time_note` | 192 | 66 | 196 | 28 |
| CHANCE | dial M | `gr_chance` | 0 | 112 | 64 | 86 |
| THR | dial M | `gr_env_thr` | 72 | 112 | 64 | 86 |
| REPEATS | dial M | `gr_repeats` | 144 | 112 | 64 | 86 |
| DECAY | dial M | `gr_decay` | 216 | 112 | 64 | 86 |
| TIME | dial M | `gr_time` | 288 | 112 | 64 | 86 |
| PITCH | dial M | `gr_pitch` | 360 | 112 | 64 | 86 |
| FLUX | dial M | `gr_flux` | 432 | 112 | 64 | 86 |
| XFADE | dial M | `gr_xfade` | 504 | 112 | 64 | 86 |
| MIX | dial M | `gr_mix` | 576 | 112 | 64 | 86 |

## Resumo

- 195 elementos (dial=72, key=23, numbox=1, plate=4, seg=16, step=32, stepper=14, viz=33); 124 IDs `data-param*` identicos a `index.html` (+dim_size/dim_mix no OUT).
- Estados: 4 (modulo selecionado: gate/delay/verb/gran). Validado sem sobreposicoes em nenhum.
- Discos de dial: M = 48px (escala 58px), XL (MORPH) = 76px (escala 88px) -> todos >= 48px. Texto >= 10px. Teclas >= 20px, seg/stepper >= 28px.
