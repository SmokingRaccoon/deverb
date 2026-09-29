# deVerb v0.1.0 — multi-efeito dual-engine (FWD + REV)

Trance gater com pattern editor · delay multi-modo · 4 reverbs algorítmicos ·
granular/glitch com beat-repeat · motor REV paralelo (reverse varispeed com
a sua própria cadeia de efeitos, inherit/morph do FWD).

Formatos do mesmo código: **Standalone, VST3, LV2** (Linux; Win/Mac por vir).

## Instalar (Linux)

Pré-compilado neste repo? Não — compila em 2 comandos (primeira vez demora
uns minutos neste i7; depois é segundos):

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
cp -r build/deVerb_artefacts/Release/VST3/deVerb.vst3 ~/.vst3/
cp -r build/deVerb_artefacts/Release/LV2/deVerb.lv2 ~/.lv2/
```

Dependências: `g++`/`clang`, `cmake`, `ninja`, `ccache` (opcional) +
dev libs JUCE (`libasound2-dev libjack-jackd2-dev ladspa-sdk
libcurl4-openssl-dev libfreetype6-dev libfontconfig1-dev libx11-dev
libxcomposite-dev libxcursor-dev libxext-dev libxinerama-dev libxrandr-dev
libxrender-dev libxi-dev libglu1-mesa-dev mesa-common-dev libegl-dev`).
O JUCE vem como submodule em `../JUCE`; VST3 SDK incluído no JUCE.

Testado em: Ubuntu 26.04, ecrã 1366×768 (a janela tem 1280×624 fixos).

## Começar em 2 minutos (standalone)

1. Corre `build/deVerb_artefacts/Release/Standalone/deVerb`, escolhe o
   preset **"Trance Gate 16"** na topbar.
2. Mete um loop a tocar (ou usa o teclado MIDI: cada nota faz restart ao gate).
3. Abre o motor REV: na faixa REV escolhe **Loop**, sobe o REV MIX. Prime
   **THROW** em modo Throw para disparar caudas únicas.
4. Macro **MORPH**: 0% = REV espelha o FWD, 100% = REV com valores próprios
   (muda de coluna FWD|REV nos headers para editar cada lado).

## Presets de fábrica (topbar)

Init · Trance Gate 16 · Offbeat Chop · Big Hall Space · Reverse Tail ·
Reverse Throw · Beat Repeat · Stutter Brk · Dub Echo · Shimmer Pad · Build Up.
Escolher um preset escreve nos parâmetros (podes afinar por cima).
Botão **RANDOM** na topbar: preset totalmente aleatório (poupa master/input).

## Notas de release v0.1.0

- FWD completo: gate (pattern 8/16, triggers Host/MIDI/Transient/Free) →
  delay digital com sync → reverb Room/Hall/Plate/Shimmer → granular
  (BeatRepeat/Slice/Reverse/Pitch/Stutter).
- REV: captura 2/3/4 beats, rate 0.25–2× varispeed, LFO, THROW (botão/MIDI),
  DUCK, fonte Dry/PostFWD, inherit/morph por módulo + trims.
- Routing: 4 ordens de cadeia (FWD e REV partilham) + modo XFADE
  (beats pares FWD, ímpares REV).
- Validado: `pluginval --strictness-level 10` → SUCCESS; 58 testes DSP
  offline + 7 ponta-a-ponta + 27 de sessão UI; scan Carla VST3+LV2 OK.
- Limites conhecidos: sem convolução com IRs (fase futura), sem timestretch
  real (varispeed), sem user-presets em ficheiro (só fábrica), 32 passos do
  gate e builds Win/Mac para depois. UI fixa 1280×620 (fase 6 de design).
- Dica de dev: o standalone guarda estado em `~/.config/deVerb.settings`.
  Se a UI abrir com valores estranhos depois de mudares parâmetros no código,
  apaga esse ficheiro (estado velho de outro layout).

## Para developers

- `plano.md` — o plano completo (conceito, DSP, fases).
- `Source/` — `core/` (TempoInfo, RevLinker, FactoryPresets), `dsp/`
  (Delay, Gater, ReverbEngine, Granular, ReverseEngine), `ui/`
  (AbletonLnF, BeatRuler).
- `tests/` — `test_dsp` (58 checks: impulso, T60 Schroeder, determinismo,
  5 algos de delay) + `test_chain` (processador real: transparência, tails,
  ordem/XFADE, fuzz próprio) + `test_ui` (27 checks de sessão heavy-user:
  flips FWD|REV, cross-talk, resizes, presets, restore; correr sob xvfb).
  Correr: `cmake --build tests/build --target test_dsp test_chain test_ui`.

## Tabela de parâmetros (v0.2, 104 IDs únicos)

Gerada do `createParams` — se algum ID se repetir, o teste de geração acusa.
`fwd_*` = cadeia normal, `rev_*` = cadeia reversa (editável com o interruptor
global FWD|REV da topbar), `gr_*` = granular FWD, `rev_gr_*` = granular REV.

### Semânticas especiais (não óbvias pelos nomes)

- `morph` 0 = REV espelha FWD (nos módulos com link), 1 = valores próprios.
- `link_master` desliga todos os links; `link_gate/delay/verb/gran` por módulo.
- `trim_delay` multiplica o tempo do delay REV; `trim_decay` o T60 do reverb REV.
- `rev_throw`/`gr_manual`/`rev_gr_manual`: toggles latching — ligar dispara
  (flanco); desligar+ligar redispara. THROW também dispara por nota MIDI.
- `gr_interrupt`: em vez de misturar, o glitch pausa o dry.
- `x_mode` XFade: beats pares FWD, ímpares REV (crossfade 10 ms); sem REV = Add.
- `chain_order`: mesma ordem nas duas cadeias.
- `rev_source` Dry = pós input-gain; PostFWD = saída da cadeia FWD.
- Duck do REV: threshold fixo -24 dB, attack 5 ms, release 250 ms.
- LFO do rate REV: 0.1 Hz fixo, ±15 % × depth. Shimmer interno: 0.35 fixo.
- Delay Reverse ignora feedback/damping; Tape sem drive = Digital.
- Multitap: taps 1, 3/4, 1/2, 1/4 com ganhos 1/.7/.5/.35 (fixos no v1).

| `input_gain` | Input Gain | float | 0.f, 2.f, 0.01f | 1.f |
| `fwd_mix` | FWD Mix | float | 0.f, 1.f, 0.01f | 1.f |
| `morph` | Morph | float | 0.f, 1.f, 0.01f | 0.f |
| `master` | Master | float | 0.f, 1.f, 0.01f | 0.8f |
| `tempo_bpm` | Tempo BPM | float | 40.f, 240.f, 0.1f | 120.f |
| `fwd_delay_time` | FWD Delay Ms | float | 1.f, 2000.f, 0.1f | 375.f |
| `fwd_delay_fb` | FWD Delay FB | float | 0.f, 0.95f, 0.01f | 0.35f |
| `fwd_delay_damp` | FWD Delay Damp | float | 200.f, 18000.f, 1.f, 0.35f | 6000.f |
| `fwd_delay_mix` | FWD Delay Mix | float | 0.f, 1.f, 0.01f | 0.25f |
| `fwd_delay_drive` | FWD Delay Drive | float | 0.f, 1.f, 0.01f | 0.3f |
| `fwd_delay_wow_rate` | FWD Delay WowRate | float | 0.1f, 5.f, 0.01f, 0.5f | 1.f |
| `fwd_delay_wow_depth` | FWD Delay WowDepth | float | 0.f, 20.f, 0.1f | 4.f |
| `fwd_delay_spread` | FWD Delay Spread | float | 0.f, 1.f, 0.01f | 1.f |
| `fwd_gate_smooth` | FWD Gate Smooth | float | 0.f, 1.f, 0.01f | 0.15f |
| `fwd_gate_depth` | FWD Gate Depth | float | 0.f, 1.f, 0.01f | 1.f |
| `fwd_gate_mix` | FWD Gate Mix | float | 0.f, 1.f, 0.01f | 1.f |
| `fwd_gate_pan` | FWD Gate PanAlt | float | 0.f, 1.f, 0.01f | 0.f |
| `fwd_gate_env_thr` | FWD Gate EnvThr | float | -60.f, 0.f, 0.5f | -18.f |
| `fwd_verb_size` | FWD Verb Size | float | 0.f, 1.f, 0.01f | 0.5f |
| `fwd_verb_decay` | FWD Verb Decay | float | 0.2f, 20.f, 0.01f, 0.4f | 2.5f |
| `fwd_verb_damp` | FWD Verb Damp | float | 0.f, 1.f, 0.01f | 0.3f |
| `fwd_verb_width` | FWD Verb Width | float | 0.f, 1.f, 0.01f | 1.f |
| `fwd_verb_predelay` | FWD Verb PreDelay | float | 0.f, 250.f, 0.1f | 20.f |
| `fwd_verb_mix` | FWD Verb Mix | float | 0.f, 1.f, 0.01f | 0.3f |
| `fwd_verb_locut` | FWD Verb LoCut | float | 20.f, 500.f, 1.f, 0.5f | 80.f |
| `fwd_verb_hicut` | FWD Verb HiCut | float | 2000.f, 20000.f, 1.f, 0.5f | 12000.f |
| `gr_chance` | Gran Chance | float | 0.f, 1.f, 0.01f | 0.2f |
| `gr_env_thr` | Gran EnvThr | float | -60.f, 0.f, 0.5f | -18.f |
| `gr_decay` | Gran Decay | float | 0.5f, 0.99f, 0.01f | 0.85f |
| `gr_time` | Gran Time Ms | float | 60.f, 8000.f, 1.f, 0.4f | 250.f |
| `gr_pitch` | Gran Pitch | float | -12.f, 12.f, 0.5f | 12.f |
| `gr_flux` | Gran Flux | float | 0.f, 1.f, 0.01f | 0.3f |
| `gr_xfade` | Gran Xfade | float | 1.f, 50.f, 0.5f | 8.f |
| `gr_mix` | Gran Mix | float | 0.f, 1.f, 0.01f | 0.5f |
| `rev_rate` | REV Rate | float | 0.25f, 2.f, 0.01f | 1.f |
| `rev_lfo` | REV LFO | float | 0.f, 1.f, 0.01f | 0.f |
| `rev_mix` | REV Mix | float | 0.f, 1.f, 0.01f | 0.4f |
| `rev_duck` | REV Duck | float | 0.f, 1.f, 0.01f | 0.3f |
| `trim_delay` | Trim Delay | float | 0.25f, 4.f, 0.01f | 1.f |
| `trim_decay` | Trim Decay | float | 0.25f, 2.f, 0.01f | 1.f |
| `rev_gate_smooth` | REV Gate Smooth | float | 0.f, 1.f, 0.01f | 0.15f |
| `rev_gate_depth` | REV Gate Depth | float | 0.f, 1.f, 0.01f | 1.f |
| `rev_gate_mix` | REV Gate Mix | float | 0.f, 1.f, 0.01f | 1.f |
| `rev_gate_pan` | REV Gate PanAlt | float | 0.f, 1.f, 0.01f | 0.f |
| `rev_gate_env_thr` | REV Gate EnvThr | float | -60.f, 0.f, 0.5f | -18.f |
| `rev_delay_time` | REV Delay Ms | float | 1.f, 2000.f, 0.1f | 375.f |
| `rev_delay_fb` | REV Delay FB | float | 0.f, 0.95f, 0.01f | 0.35f |
| `rev_delay_damp` | REV Delay Damp | float | 200.f, 18000.f, 1.f, 0.35f | 6000.f |
| `rev_delay_mix` | REV Delay Mix | float | 0.f, 1.f, 0.01f | 0.25f |
| `rev_delay_drive` | REV Delay Drive | float | 0.f, 1.f, 0.01f | 0.3f |
| `rev_delay_wow_rate` | REV Delay WowRate | float | 0.1f, 5.f, 0.01f, 0.5f | 1.f |
| `rev_delay_wow_depth` | REV Delay WowDepth | float | 0.f, 20.f, 0.1f | 4.f |
| `rev_delay_spread` | REV Delay Spread | float | 0.f, 1.f, 0.01f | 1.f |
| `rev_verb_size` | REV Verb Size | float | 0.f, 1.f, 0.01f | 0.5f |
| `rev_verb_decay` | REV Verb Decay | float | 0.2f, 20.f, 0.01f, 0.4f | 2.5f |
| `rev_verb_damp` | REV Verb Damp | float | 0.f, 1.f, 0.01f | 0.3f |
| `rev_verb_width` | REV Verb Width | float | 0.f, 1.f, 0.01f | 1.f |
| `rev_verb_predelay` | REV Verb PreDelay | float | 0.f, 250.f, 0.1f | 20.f |
| `rev_verb_mix` | REV Verb Mix | float | 0.f, 1.f, 0.01f | 0.3f |
| `rev_verb_locut` | REV Verb LoCut | float | 20.f, 500.f, 1.f, 0.5f | 80.f |
| `rev_verb_hicut` | REV Verb HiCut | float | 2000.f, 20000.f, 1.f, 0.5f | 12000.f |
| `rev_gr_chance` | REV Gran Chance | float | 0.f, 1.f, 0.01f | 0.2f |
| `rev_gr_env_thr` | REV Gran EnvThr | float | -60.f, 0.f, 0.5f | -18.f |
| `rev_gr_decay` | REV Gran Decay | float | 0.5f, 0.99f, 0.01f | 0.85f |
| `rev_gr_time` | REV Gran Time Ms | float | 60.f, 8000.f, 1.f, 0.4f | 250.f |
| `rev_gr_pitch` | REV Gran Pitch | float | -12.f, 12.f, 0.5f | 12.f |
| `rev_gr_flux` | REV Gran Flux | float | 0.f, 1.f, 0.01f | 0.3f |
| `rev_gr_xfade` | REV Gran Xfade | float | 1.f, 50.f, 0.5f | 8.f |
| `rev_gr_mix` | REV Gran Mix | float | 0.f, 1.f, 0.01f | 0.5f |
| `fwd_delay_algo` | FWD Delay Algo | choice | Digital|Tape|PingPong|MultiTap|Reverse | Digital |
| `fwd_gate_steps` | FWD Gate Steps | choice | 8|16 | 16 |
| `fwd_gate_trig` | FWD Gate Trig | choice | Host|Midi|Transient|Free | Host |
| `fwd_verb_algo` | FWD Verb Algo | choice | Room|Hall|Plate|Shimmer | Hall |
| `gr_mode` | Gran Mode | choice | Off|BeatRepeat|Slice|Reverse|Pitch|Stutter | Off |
| `gr_trigger` | Gran Trig | choice | Chance|Envelope|Manual | Chance |
| `rev_mode` | REV Mode | choice | Off|Loop|Throw | Off |
| `rev_source` | REV Source | choice | Dry|PostFWD | Dry |
| `rev_capture` | REV Capture | choice | 2 beats|3 beats|4 beats | 2 beats |
| `chain_order` | Chain Order | choice | G-D-V-Gr|G-V-D-Gr|D-G-V-Gr|V-D-G-Gr | G-D-V-Gr |
| `x_mode` | X Mode | choice | Add|XFade | Add |
| `rev_gate_steps` | REV Gate Steps | choice | 8|16 | 16 |
| `rev_gate_trig` | REV Gate Trig | choice | Host|Midi|Transient|Free | Host |
| `rev_delay_algo` | REV Delay Algo | choice | Digital|Tape|PingPong|MultiTap|Reverse | Digital |
| `rev_verb_algo` | REV Verb Algo | choice | Room|Hall|Plate|Shimmer | Hall |
| `rev_gr_mode` | REV Gran Mode | choice | Off|BeatRepeat|Slice|Reverse|Pitch|Stutter | Off |
| `rev_gr_trigger` | REV Gran Trig | choice | Chance|Envelope|Manual | Chance |
| `fwd_delay_freeze` | FWD Delay Freeze | bool | - | false |
| `fwd_verb_freeze` | FWD Verb Freeze | bool | - | false |
| `gr_manual` | Gran Manual | bool | - | false |
| `gr_interrupt` | Gran Interrupt | bool | - | false |
| `rev_throw` | REV Throw | bool | - | false |
| `link_master` | Link Master | bool | - | true |
| `link_gate` | Link Gate | bool | - | true |
| `link_delay` | Link Delay | bool | - | true |
| `link_verb` | Link Verb | bool | - | true |
| `link_gran` | Link Gran | bool | - | true |
| `rev_delay_freeze` | REV Delay Freeze | bool | - | false |
| `rev_verb_freeze` | REV Verb Freeze | bool | - | false |
| `rev_gr_manual` | REV Gran Manual | bool | - | false |
| `rev_gr_interrupt` | REV Gran Interrupt | bool | - | false |
| `fwd_gate_pattern` | FWD Gate Pattern | int | 0..65535 | 0x1111 |
| `gr_repeats` | Gran Repeats | int | 1..16 | 4 |
| `rev_gate_pattern` | REV Gate Pattern | int | 0..65535 | 0x1111 |
| `rev_gr_repeats` | REV Gran Repeats | int | 1..16 | 4 |
