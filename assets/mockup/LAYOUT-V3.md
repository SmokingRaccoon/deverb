# LAYOUT-V3 — geometria gerada (deVerb UI v3)

Gerado automaticamente por `assets/mockup/generate-v3.py` — não editar à mão,
editar o gerador e correr `python3 generate-v3.py`.

Janela 1280x624, origem (0,0) topo-esquerda. Todas as posições/tamanhos abaixo
foram recalculadas (layout novo, não é o mesmo do template anterior) — IDs de
parâmetro mantidos 100% congelados.

### HEADER (0, 0, 1280×60)

| Peça | Tipo | Param ID | x | y | w | h | Estado demo |
|------|------|----------|---|---|---|---|-------------|
| title | visual | `—` | 8 | 12 | 170 | 36 | — |
| Preset fabrica | combo | `role:factory-preset` | 658 | 14 | 240 | 32 | — |
| RANDOM | btn | `role:random` | 914 | 14 | 78 | 32 | — |
| FWD | btn | `role:mode-fwdrev` | 1008 | 14 | 72 | 32 | ON |
| REV | btn | `role:mode-fwdrev` | 1080 | 14 | 72 | 32 | — |
| gran-led | visual | `—` | 1184 | 14 | 88 | 32 | — |
| bpm-readout | visual | `—` | 202 | 16 | 120 | 28 | — |
| gate-readout | visual | `—` | 328 | 16 | 150 | 28 | — |

### MACRO STRIP — REV + LINK/TRIM/CHAIN (0, 66, 1280×88)

| Peça | Tipo | Param ID | x | y | w | h | Estado demo |
|------|------|----------|---|---|---|---|-------------|
| sect-rev | visual | `—` | 16 | 72 | 44 | 76 | — |
| RATE | knob | `rev_rate` | 348 | 72 | 60 | 76 | — |
| LFO | knob | `rev_lfo` | 412 | 72 | 60 | 76 | — |
| DUCK | knob | `rev_duck` | 476 | 72 | 60 | 76 | — |
| sect-link | visual | `—` | 642 | 72 | 36 | 76 | — |
| T-DLY | knob | `trim_delay` | 932 | 72 | 60 | 76 | — |
| T-DEC | knob | `trim_decay` | 996 | 72 | 60 | 76 | — |
| REV Mode | combo | `rev_mode` | 64 | 94 | 96 | 32 | — |
| REV Source | combo | `rev_source` | 164 | 94 | 88 | 32 | — |
| REV Capture | combo | `rev_capture` | 256 | 94 | 88 | 32 | — |
| THROW | btn | `rev_throw` | 540 | 94 | 80 | 32 | — |
| ALL | btn | `link_master` | 682 | 94 | 46 | 32 | ON |
| GATE | btn | `link_gate` | 732 | 94 | 46 | 32 | — |
| DLY | btn | `link_delay` | 782 | 94 | 46 | 32 | — |
| VRB | btn | `link_verb` | 832 | 94 | 46 | 32 | — |
| GRN | btn | `link_gran` | 882 | 94 | 46 | 32 | — |
| Chain Order | combo | `chain_order` | 1060 | 94 | 80 | 32 | — |
| X Mode | combo | `x_mode` | 1144 | 94 | 72 | 32 | — |

### MOTOR panel (8, 160, 142×456)

| Peça | Tipo | Param ID | x | y | w | h | Estado demo |
|------|------|----------|---|---|---|---|-------------|
| INPUT | knob | `input_gain` | 49 | 198 | 60 | 76 | — |
| FWD MIX | knob | `fwd_mix` | 16 | 280 | 60 | 76 | — |
| REV MIX | knob | `rev_mix` | 82 | 280 | 60 | 76 | — |
| MORPH | knob | `morph` | 31 | 362 | 96 | 114 | — |
| MASTER | knob | `master` | 31 | 482 | 96 | 114 | — |

### GATE panel (156, 160, 340×456)

| Peça | Tipo | Param ID | x | y | w | h | Estado demo |
|------|------|----------|---|---|---|---|-------------|
| PWR | btn | `fwd=fwd_gate_on rev=rev_gate_on` | 448 | 168 | 40 | 22 | ON |
| ruler | visual | `—` | 164 | 198 | 324 | 30 | — |
| step 1 | step | `—` | 169 | 238 | 34 | 50 | ON |
| step 2 | step | `—` | 209 | 238 | 34 | 50 | — |
| step 3 | step | `—` | 249 | 238 | 34 | 50 | — |
| step 4 | step | `—` | 289 | 238 | 34 | 50 | — |
| step 5 | step | `—` | 329 | 238 | 34 | 50 | ON |
| step 6 | step | `—` | 369 | 238 | 34 | 50 | — |
| step 7 | step | `—` | 409 | 238 | 34 | 50 | — |
| step 8 | step | `—` | 449 | 238 | 34 | 50 | — |
| step 9 | step | `—` | 169 | 294 | 34 | 50 | ON |
| step 10 | step | `—` | 209 | 294 | 34 | 50 | — |
| step 11 | step | `—` | 249 | 294 | 34 | 50 | — |
| step 12 | step | `—` | 289 | 294 | 34 | 50 | — |
| step 13 | step | `—` | 329 | 294 | 34 | 50 | ON |
| step 14 | step | `—` | 369 | 294 | 34 | 50 | — |
| step 15 | step | `—` | 409 | 294 | 34 | 50 | — |
| step 16 | step | `—` | 449 | 294 | 34 | 50 | — |
| Pattern fabrica | combo | `role:pattern-preset fwd=fwd_gate_pattern rev=rev_gate_pattern` | 164 | 362 | 159 | 32 | — |
| Gate Rate | combo | `fwd=fwd_gate_rate rev=rev_gate_rate` | 329 | 362 | 159 | 32 | — |
| Gate Steps | combo | `fwd=fwd_gate_steps rev=rev_gate_steps` | 164 | 404 | 159 | 32 | — |
| Gate Trig | combo | `fwd=fwd_gate_trig rev=rev_gate_trig` | 329 | 404 | 159 | 32 | — |
| SMOOTH | knob | `fwd=fwd_gate_smooth rev=rev_gate_smooth` | 164 | 456 | 60 | 76 | — |
| DEPTH | knob | `fwd=fwd_gate_depth rev=rev_gate_depth` | 230 | 456 | 60 | 76 | — |
| MIX | knob | `fwd=fwd_gate_mix rev=rev_gate_mix` | 296 | 456 | 60 | 76 | — |
| PAN | knob | `fwd=fwd_gate_pan rev=rev_gate_pan` | 362 | 456 | 60 | 76 | — |
| THR | knob | `fwd=fwd_gate_env_thr rev=rev_gate_env_thr` | 428 | 456 | 60 | 76 | — |

### DELAY panel (502, 160, 208×456)

| Peça | Tipo | Param ID | x | y | w | h | Estado demo |
|------|------|----------|---|---|---|---|-------------|
| PWR | btn | `fwd=fwd_delay_on rev=rev_delay_on` | 662 | 168 | 40 | 22 | ON |
| BPM | knob | `tempo_bpm` | 510 | 198 | 60 | 76 | — |
| MS | knob | `fwd=fwd_delay_time rev=rev_delay_time` | 576 | 198 | 60 | 76 | — |
| FB | knob | `fwd=fwd_delay_fb rev=rev_delay_fb` | 642 | 198 | 60 | 76 | — |
| DAMP | knob | `fwd=fwd_delay_damp rev=rev_delay_damp` | 510 | 284 | 60 | 76 | — |
| MIX | knob | `fwd=fwd_delay_mix rev=rev_delay_mix` | 576 | 284 | 60 | 76 | — |
| DRIVE | knob | `fwd=fwd_delay_drive rev=rev_delay_drive` | 642 | 284 | 60 | 76 | — |
| WOW RT | knob | `fwd=fwd_delay_wow_rate rev=rev_delay_wow_rate` | 510 | 370 | 60 | 76 | — |
| WOW DP | knob | `fwd=fwd_delay_wow_depth rev=rev_delay_wow_depth` | 576 | 370 | 60 | 76 | — |
| SPREAD | knob | `fwd=fwd_delay_spread rev=rev_delay_spread` | 642 | 370 | 60 | 76 | — |
| Delay Algo | combo | `fwd=fwd_delay_algo rev=rev_delay_algo` | 510 | 462 | 192 | 32 | — |
| Delay Note | combo | `fwd=fwd_delay_note rev=rev_delay_note` | 510 | 504 | 192 | 32 | — |
| Freeze | btn | `fwd=fwd_delay_freeze rev=rev_delay_freeze` | 610 | 548 | 92 | 30 | — |
| delay-ms | visual | `—` | 510 | 549 | 94 | 28 | — |

### VERB panel (716, 160, 274×456)

| Peça | Tipo | Param ID | x | y | w | h | Estado demo |
|------|------|----------|---|---|---|---|-------------|
| PWR | btn | `fwd=fwd_verb_on rev=rev_verb_on` | 942 | 168 | 40 | 22 | ON |
| SIZE | knob | `fwd=fwd_verb_size rev=rev_verb_size` | 724 | 198 | 60 | 76 | — |
| DECAY | knob | `fwd=fwd_verb_decay rev=rev_verb_decay` | 790 | 198 | 60 | 76 | — |
| DAMP | knob | `fwd=fwd_verb_damp rev=rev_verb_damp` | 856 | 198 | 60 | 76 | — |
| WIDTH | knob | `fwd=fwd_verb_width rev=rev_verb_width` | 922 | 198 | 60 | 76 | — |
| PRE-DLY | knob | `fwd=fwd_verb_predelay rev=rev_verb_predelay` | 724 | 284 | 60 | 76 | — |
| LO-CUT | knob | `fwd=fwd_verb_locut rev=rev_verb_locut` | 790 | 284 | 60 | 76 | — |
| HI-CUT | knob | `fwd=fwd_verb_hicut rev=rev_verb_hicut` | 856 | 284 | 60 | 76 | — |
| MIX | knob | `fwd=fwd_verb_mix rev=rev_verb_mix` | 922 | 284 | 60 | 76 | — |
| Verb Algo | combo | `fwd=fwd_verb_algo rev=rev_verb_algo` | 724 | 376 | 126 | 32 | — |
| PreDelay Note | combo | `fwd=fwd_verb_predelay_note rev=rev_verb_predelay_note` | 856 | 376 | 126 | 32 | — |
| Freeze | btn | `fwd=fwd_verb_freeze rev=rev_verb_freeze` | 882 | 418 | 100 | 30 | — |
| verb-t60 | visual | `—` | 724 | 419 | 152 | 28 | — |
| ir-placeholder | visual | `—` | 724 | 458 | 258 | 150 | — |

### GRAN panel (996, 160, 274×456)

| Peça | Tipo | Param ID | x | y | w | h | Estado demo |
|------|------|----------|---|---|---|---|-------------|
| PWR | btn | `fwd=gr_on rev=rev_gr_on` | 1222 | 168 | 40 | 22 | — |
| Gran Mode | combo | `fwd=gr_mode rev=rev_gr_mode` | 1004 | 198 | 126 | 32 | — |
| Gran Trig | combo | `fwd=gr_trigger rev=rev_gr_trigger` | 1136 | 198 | 126 | 32 | — |
| GRAB | btn | `fwd=gr_manual rev=rev_gr_manual` | 1004 | 238 | 126 | 30 | — |
| Gran Len | combo | `fwd=gr_len_note rev=rev_gr_len_note` | 1136 | 238 | 126 | 32 | — |
| CHANCE | knob | `fwd=gr_chance rev=rev_gr_chance` | 1004 | 284 | 60 | 76 | — |
| THR | knob | `fwd=gr_env_thr rev=rev_gr_env_thr` | 1070 | 284 | 60 | 76 | — |
| REPEATS | knob | `fwd=gr_repeats rev=rev_gr_repeats` | 1136 | 284 | 60 | 76 | — |
| DECAY | knob | `fwd=gr_decay rev=rev_gr_decay` | 1202 | 284 | 60 | 76 | — |
| TIME | knob | `fwd=gr_time rev=rev_gr_time` | 1037 | 370 | 60 | 76 | — |
| PITCH | knob | `fwd=gr_pitch rev=rev_gr_pitch` | 1103 | 370 | 60 | 76 | — |
| FLUX | knob | `fwd=gr_flux rev=rev_gr_flux` | 1169 | 370 | 60 | 76 | — |
| XFADE | knob | `fwd=gr_xfade rev=rev_gr_xfade` | 1070 | 456 | 60 | 76 | — |
| MIX | knob | `fwd=gr_mix rev=rev_gr_mix` | 1136 | 456 | 60 | 76 | — |
| Interrupt | btn | `fwd=gr_interrupt rev=rev_gr_interrupt` | 1004 | 546 | 126 | 30 | — |
| Gran Time Note | combo | `fwd=gr_time_note rev=rev_gr_time_note` | 1136 | 546 | 126 | 32 | — |

## Resumo

- 41 knobs, 18 combos, 17 botões, 16 steps, 10 visuais — 102 widgets no total.
- 122 parâmetros cobertos (100 em pares FWD|REV + 22 globais), idêntico ao template anterior.
- Tamanho mínimo de knob: 48px de diâmetro (dial). Combos ≥22px altura, botões ≥20px altura, texto ≥9.5px — dentro das regras de `LAYOUT-TEMPLATE.md`.
- Paleta/tema por `data-module`, não por painel de desenho — widgets do strip macro herdam a cor do módulo que controlam (ex.: toggle `DLY` no LINK strip usa `--delay-a`).