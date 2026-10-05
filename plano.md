# deVerb — plano de desenvolvimento

> Multi-efeito criativo dual-engine (FWD + REV): trance gater com pattern
> editor e triggers, delay multi-algoritmo com reverse, reverb algorítmico
> (room/hall/plate/shimmer), e granular/glitch com beat-repeat — tudo
> sincronizado ao tempo do host, com sistema de inherit/morph entre motores.
>
> Estado: v0.2 em desenvolvimento (fases 0–7 feitas). Stack: JUCE 9.0.3
> (shallow clone, commit be29c81) em C++17, CMake + Ninja + ccache,
> formatos Standalone + VST3 + LV2 do mesmo código. Licença JUCE Starter
> (grátis até $20k de receita/funding em 12 meses).
> Workspace: `/home/pedro/audio-dev/` (template de referência: `HelloSine/`).

---

## 0. Sumário executivo

1. **O que é**: um plugin de efeitos para transformar loops/pads/vozes com
   movimento rítmico e espaço — primeiro como **standalone** (para brincar sem
   DAW), depois o **mesmo código** gera VST3/LV2.
2. **O que o distingue**: o **motor REV paralelo** — uma cópia do sinal
   capturada em janelas de 2–4 beats, lida de trás para a frente com controlo
   de velocidade (varispeed), a passar pela sua própria cadeia dos mesmos
   4 efeitos, com herança/morph dos parâmetros do motor normal (FWD).
3. **Estratégia de construção**: fundação primeiro (tempo + delay), depois um
   módulo de cada vez no FWD, só depois o REV, só no fim UI final e packaging.
   Cada fase termina com um binário testável e validado.
4. **Restrições assumidas**: PC de desenvolvimento modesto (i7-4500U, 7.6 GB
   RAM) → builds Release com Ninja + ccache, algoritmos com modo lite,
   nada de processamento pesado no MVP (sem convolução, sem timestretch real).

---

## 1. Visão e identidade do produto

### 1.1 Para quem é

Produtores de eletrónica (trance, techno, ambient, drum & bass) e sound
designers que querem:

- dar movimento rítmico a pads/strings/vozes (gater);
- criar espaço e profundidade sem sair do plugin (delay + reverb);
- gerar variações, fills e breakdowns ao vivo (granular/glitch + REV);
- atuar com 2–3 controlos macro em vez de programar 40 parâmetros.

### 1.2 Personalidade sonora

- **Limpo quando parado**: com tudo a 0/bypass, o plugin é bit-transparente
  (testado — ver §12). Efeitos são 100% opt-in por módulo e por engine.
- **Musical por defeito**: todos os tempos oferecem modo sync (notas
  musicais) e os defaults de fábrica estão em valores "que funcionam"
  (ex: delay 1/8 dotted, gate 1/16, hall médio).
- **Extremo quando puxado**: feedbacks altos, rates 0.25x, shimmer e glitch
  com Chance a 100% existem para quem quer destruir som de propósito —
  sempre com limitador de segurança na saída (ver §7.6).

### 1.3 Nome e módulos

`deVerb` = delay + reverb no nome, mas o produto são 4 módulos × 2 motores:

| Módulo   | Papel numa frase                                            |
|----------|-------------------------------------------------------------|
| GATER    | corta o som em ritmo (trance gate)                          |
| DELAY    | repete no tempo (5 sabores, incluindo reverse)              |
| REVERB   | põe o som numa sala (4 sabores algorítmicos)                |
| GRANULAR | parte e recompõe o som (beat-repeat + glitch)               |

---

## 2. Conceito dual-engine (FWD + REV)

### 2.1 Ideia central

```
                        ┌──────────────── ENGINE FWD ────────────────┐
                        │ Gate → Delay → Reverb → Glitch (ord. cfg.) │
input ──┬── DRY ────────┤                                            ├─┬──→ output
        │               └────────────────────────────────────────────┘ │
        │               ┌──────────────── ENGINE REV ────────────────┐ │
        └── CAPTURA ───►│ Reverse-reader (rate) → G'→D'→R'G' (própr.)│─┘
            (2–4 beats) └────────────────────────────────────────────┘
```

- **DRY** sai sempre na hora (latência zero do plugin = latência do host).
- **FWD** processa o presente.
- **REV** processa o passado recente (janela de 2–4 beats), lido ao contrário
  e com velocidade ajustável — é um **tail decorativo**, não uma correção.
- Cada engine tem **mix, mute/solo e bypass total** próprios. Bypass total =
  o motor nem sequer processa (poupa CPU — importante no i7).

### 2.2 Modos de combinação (estratégia, não só soma)

Para além da soma simples (FWD + REV), o plano prevê estes modos de
performance, implementados como opções baratas sobre a mesma arquitetura:

1. **ADD (defeito)**: `out = dry + fwd*mixF + rev*mixR`. Simples, previsível.
2. **XFADE por beat**: beats pares → FWD, ímpares → REV (crossfade de ~10 ms
   na troca). Um toggle, muito trance. Implementação: ganho alternado com
   rampa, zero custo extra de DSP.
3. **THROW (disparo manual)**: o REV normalmente calado; o botão THROW
   captura os próximos N beats e dispara **um** tail. Ideal para transições
   e fills. Implementação: máquina de estados IDLE → CAPTURING → PLAYING_ONCE.
4. **DUCK do REV pelo dry** (recomendado no MVP): seguidor de envelope do
   input baixa o REV quando há sinal novo e deixa-o subir nos espaços.
   Parâmetros: threshold, attack, release, depth. É um compressor de 1 linha
   de código sobre o bus do REV.
5. **FREEZE partilhado**: congela delay + reverb + granular dos dois motores
   de uma vez (feedback→1 com crossfade, granular em loop). Um botão.

Fase 1 implementa ADD + bypass/mix. THROW + DUCK entram na fase do REV
(são baratos e de alto valor). XFADE e FREEZE entram na fase de routing.

### 2.3 O que o REV **não** é

- Não é "undo" nem correção de timing: é um gerador de tails.
- Não reporta latência ao host (ver §2.4): a DAW não deve "compensar" o
  passado — o atraso **é** o efeito.
- Não corre quando desligado: bypass do REV = zero processamento.

### 2.4 Latência — decisão explícita

- Latência reportada ao host (`getLatencySamples`): **0** no MVP.
- Latência real percebida do REV: tamanho da janela de captura
  (ex: 2 beats @ 120 BPM = 1000 ms; 4 beats @ 140 BPM ≈ 1714 ms).
- A UI mostra sempre "tail chega em X beats (Y ms)" calculado do BPM atual.
- Se no futuro houver convolução com latência própria, aí sim reporta-se
  latência — mas só a da convolução, nunca a "musical" do REV.

---

## 3. Especificação por módulo

Convenção de parâmetros: cada parâmetro tem um ID estável (nunca mudar IDs
depois do v1 — quebra presets e automação). Prefixos `fwd_` e `rev_`.
Ex: `fwd_delay_time`, `rev_delay_time`.

Todos os parâmetros contínuos usam **smoothing** (rampas) para não haver
cliques ao automatizar. Tempos têm sempre par **livre (ms/Hz) + sync (nota)**.

### 3.1 GATER — trance gate com pattern e triggers

**Papel**: cortar o som em ritmo. Primeiro módulo da cadeia por defeito
(antes de delay/reverb, para os tails não serem cortados).

**Velocidade e grelha temporal:**

| ID | Nome | Valores | Defeito |
|----|------|---------|---------|
| `gate_rate` | Divisão | 1/4, 1/8, 1/8T, 1/16, 1/16T, 1/32, 1/4D, 1/8D, livre Hz | 1/16 |
| `gate_rate_hz` | Taxa livre | 0.1–20 Hz | 8 Hz |
| `gate_steps` | Passos | 8 / 16 / 32 | 16 |
| `gate_pattern` | Pattern | 32 bits on/off + acento por passo | preset "porta" |
| `gate_smooth` | Attack/release do corte | 0–100 % (0 = faca, 100 = suave) | 15 % |
| `gate_depth` | Profundidade | 0–100 % (0 = sem corte, 100 = silêncio) | 100 % |
| `gate_mix` | Mix | 0–100 % | 100 % |
| `gate_pan_alt` | Pan alternado por passo | 0–100 % | 0 % |

**Marcações temporais (exigência do projeto)**: a UI do gater mostra a régua
de beats (1–4 + subdivisões da divisão atual) com o passo ativo iluminado a
partir da posição do host. Sem isto, programar patterns é às cegas.

**Triggers (extra aprovado)** — como o ciclo recomeça:

- `HOST` (defeito): segue o transporte; o padrão alinha ao beat 1 do compasso.
- `MIDI`: qualquer nota MIDI faz restart do padrão (e pode trocar de pattern
  por tecla em fase 2).
- `TRANSIENT`: detetor de transientes do próprio áudio faz restart
  (seguidor de envelope + threshold; 2 modos: suave e percussivo).
- `FREE`: corre livre, sem alinhar.

**Presets de pattern de fábrica** (mínimo): porta 4/4, offbeat, serra
ascendente, build-up 1/32, euclidiano 5/16, aleatório musical (com seed fixa
para ser repetível).

**Implementação**: ganho por amostra a partir de fase de beat (não LFO
livre — LFO desafina do host). Curvas por passo pré-calculadas em tabela;
interpolação linear chega. Custo: irrisório.

### 3.2 DELAY — cinco algoritmos

**Papel**: repetições musicais e espaço rítmico. É o primeiro módulo a
implementar (fundação: contém o `TempoInfo` e as linhas de delay que outros
módulos reutilizam concetualmente).

**Parâmetros comuns:**

| ID | Nome | Valores | Defeito |
|----|------|---------|---------|
| `delay_algo` | Algoritmo | Digital, Tape, PingPong, Multitap, Reverse | Digital |
| `delay_time` | Tempo | 1–2000 ms / notas 1/32–1/1 + D/T | 1/8D |
| `delay_fb` | Feedback | 0–95 % (capado por segurança) | 35 % |
| `delay_damping` | Escurecimento por repetição (lowpass no loop) | 20–20000 Hz | 6000 Hz |
| `delay_width` | Abertura stereo | 0–100 % | 60 % |
| `delay_mix` | Mix | 0–100 % | 25 % |
| `delay_freeze` | Congela o buffer | toggle | off |
| `delay_duck` | Duck pelo dry | 0–100 % | 0 % |

**Algoritmos em detalhe:**

1. **Digital**: 1 linha de delay por canal + feedback + damping. A referência
   limpa. Serve de fallback se outro algoritmo falhar.
2. **Tape**: digital + saturação suave no loop (tanh barato) + wow/flutter
   (LFO 0.1–5 Hz a modular ±0–20 ms) + perda progressiva de agudos.
   Parâmetros extra: `drive`, `wow_rate`, `wow_depth`.
3. **PingPong**: duas linhas cruzadas L→R→L. Parâmetros extra: `spread`
   (diferença L/R) e `xfb` (feedback cruzado vs direto).
4. **Multitap**: 4 saídas da mesma linha com tempos/níveis/pans próprios
   (ex: 1/8, 1/8D, 1/4, 1/2). É o som "trance arp delay". UI: 4 mini-sliders.
   (8 taps fica para fase 2.)
5. **Reverse (na FWD, versão simples)**: captura 1 beat e lê ao contrário em
   loop enquanto ativo. A versão "a sério" vive no motor REV (§4).

**Anti-clique**: mudanças de tempo fazem crossfade de ~20 ms entre
leituras antiga e nova; mudanças de feedback são rampadas.

**Implementação**: `juce::dsp::DelayLine<float, Lagrange3rd>` (interpolação
de 3ª ordem para modular sem zipper noise), buffers até 5 s stereo.
Prototipagem rápida em Faust antes do C++ final.

### 3.3 REVERB — algorítmico primeiro (convolução é fase 3)

**Papel**: colocar o som numa sala. Decisão tomada: **só algorítmico no MVP**;
convolução com IRs (embutidos + load de ficheiro) entra numa fase posterior
com o seu próprio plano de latência/gestão de ficheiros.

**Sabores do MVP e de onde vêm:**

| Sabor | Base técnica | Caráter | Custo |
|-------|--------------|---------|-------|
| ROOM | Freeverb curto (combs curtos, decay baixo) | pequeno, colado | baixo |
| HALL | FDN 4–8 linhas, matriz Hadamard | grande, denso | médio |
| PLATE | Topologia Dattorro (tank modulado) | brilhante, vocal/synth | médio-alto |
| SHIMMER | FDN + pitch-shift +12 st no feedback | etéreo, pad infinito | alto |

**Parâmetros comuns:**

| ID | Nome | Valores | Defeito |
|----|------|---------|---------|
| `verb_algo` | Algoritmo | Room, Hall, Plate, Shimmer | Hall |
| `verb_size` | Tamanho | 0–100 % (escala delays internos) | 50 % |
| `verb_decay` | Cauda (T60 aprox.) | 0.2–20 s | 2.5 s |
| `verb_damping` | Absorção de agudos | 0–100 % | 30 % |
| `verb_predelay` | Pré-atraso | 0–250 ms / notas até 1/4 | 20 ms |
| `verb_width` | Largura stereo | 0–100 % | 100 % |
| `verb_freeze` | Congela a cauda | toggle | off |
| `verb_mix` | Mix | 0–100 % | 30 % |
| `verb_locut` / `verb_hicut` | Filtros na saída wet | 20–500 Hz / 2–20 kHz | 80 Hz / 12 kHz |

**Estratégia de construção**: começar pelo ROOM com o `juce::dsp::Reverb`
(stock, da família Schroeder) como placeholder funcional no dia 1; depois
substituir por Freeverb próprio; depois FDN; depois Plate; Shimmer por fim.
Cada sabor é uma classe com a mesma interface — trocar não parte presets
(guardar `verb_algo` como int com mapa estável).

**Prototipagem**: o Faust instalado tem `reverbs.lib` com `freeverb`,
`zita_rev_fdn`, `jpverb` e `dattorro_rev` — ouvir cada topologia em minutos
antes de escrever C++.

### 3.4 GRANULAR / GLITCH — beat-repeat e destruição controlada

**Papel**: variação e surpresa. Último módulo da cadeia por defeito (trabalha
sobre o som já com espaço).

**3.4.1 Beat-repeat:**

| ID | Nome | Valores | Defeito |
|----|------|---------|---------|
| `gr_repeat_len` | Tamanho do fragmento | 1/32–1/2 | 1/16 |
| `gr_repeats` | Nº de repetições | 1–16 | 4 |
| `gr_decay` | Decaimento (× por repetição) | 0.5–0.99 | 0.85 |
| `gr_xfade` | Crossfade anti-clique | 1–50 ms | 8 ms |

Captura o último fragmento do tamanho escolhido e repete-o com decaimento
exponencial, quantizado à grelha. É o "efeito por beat" pedido.

**3.4.2 Glitch:**

| ID | Nome | Valores | Defeito |
|----|------|---------|---------|
| `gr_mode` | Tipo | Slice, Reverse, Pitch, Stutter, All | Slice |
| `gr_trigger` | Disparo | Chance, Envelope, Manual | Chance |
| `gr_chance` | Probabilidade (modo Chance) | 0–100 % | 20 % |
| `gr_env_thr` | Threshold (modo Envelope) | -60–0 dB | -18 dB |
| `gr_time` | Duração do glitch | 60 ms–8 s / notas | 1/8 |
| `gr_pitch` | Pitch (modo Pitch) | -12–+12 st | +12 |
| `gr_flux` | Re-randomização interna | 0–100 % | 30 % |
| `gr_interrupt` | Glitch pausa o dry? | toggle | off |

**Grão (base comum):** `gr_grain` (1–200 ms ou nota), `gr_rate/speed`
(como no REV mas em micro-escala), `gr_fb` curto para repetições metálicas.

**Implementação**: ring buffer de 2 compassos + leitores com crossfade;
grain scheduler simples (1 voz no MVP, polifonia de grãos em fase 2).
Zero-crossing na captura quando possível para menos cliques.

---

## 4. Engine REV em detalhe

### 4.1 Captura

- Ring buffer stereo dimensionado para **4 beats ao BPM mínimo suportado**
  (40 BPM → 4 beats = 6 s; a 48 kHz stereo float ≈ 2.3 MB — irrelevante).
- `rev_capture_len`: 2 / 3 / 4 beats (defeito 2). Quantizado ao transporte.
- Modos: `LOOP` (repete a janela), `ONCE` (toca uma vez e cala),
  `THROW` (armado por botão — ver §2.2).

### 4.2 Leitor reverso + rate (varispeed)

- Leitura de trás para a frente com interpolação Lagrange3rd.
- `rev_rate`: 0.25x–2.0x contínuo + presets musicais (0.5x, 1x, 2x).
  **Rate < 1 = mais longo e grave; rate > 1 = mais curto e agudo**
  (pitch acompanha — efeito tape, assumido e desejado no MVP).
- `rev_lfo`: LFO opcional 0.05–1 Hz a vaguear o rate ±0–10 %
  (reverse orgânico/quase-chorus — ideia §5 da conversa).
- Crossfades de ~25 ms nos limites do loop. Qualquer mudança de rate é
  rampada em ~50 ms.

### 4.3 Cadeia própria

O sinal reverso passa pelas **mesmas 4 classes DSP** em instâncias próprias
(`Gater_rev`, `Delay_rev`, …). Parâmetros com prefixo `rev_`.
Isto dá: reverse gated, reverse delay ping-pong, reverse shimmer —
combinações que nenhum preset normal alcança.

### 4.4 Timestretch real (fase 3, fora do MVP)

Caminho previsto: integrar RubberBand (ou phase-vocoder próprio minimalista)
como **modo alternativo** do leitor (`rev_timemode`: Varispeed | Stretch),
opcional em build e desligado por defeito. Não desenhar UI final para isto
agora — reservar o espaço e o ID do parâmetro.

---

## 5. Sistema inherit / morph / link

### 5.1 Regras

- **Granularidade**: toggle global `link_master` + toggle por módulo
  (`link_gate`, `link_delay`, `link_reverb`, `link_granular`).
- **Trims**: por módulo, `rev_X = fwd_X × trim_mult + trim_add` para os 2–3
  parâmetros-chave (ex: delay time, reverb decay, gate rate). Trims guardados
  no preset.
- **Macro MORPH** (`morph`, 0–100 %, automatizável): 0 % = REV espelha FWD
  (nos módulos com link ligado); 100 % = REV usa os seus valores próprios.
  Valores intermédios interpolam linearmente. É o knob de performance do
  plugin — grande, central, mapeável a MIDI CC.
- **Cópia manual**: botões "FWD→REV" e "REV→FWD" por módulo (snapshot
  pontual, sem link contínuo).

### 5.2 Implementação

- APVTS com IDs separados (`fwd_delay_time`, `rev_delay_time`, …).
- Classe `RevLinker`: observa parâmetros FWD (via `ParameterAttachment` /
  listeners) e escreve nos REV com as regras ativas. Escritas sempre com
  `setValueNotifyingHost` fora do audio thread ou via fila lock-free para o
  audio thread aplicar com rampa — nunca tocar parâmetros dentro do
  `processBlock` sem ser por variável atómica.
- Presets guardam: valores FWD, valores REV, estados de link, trims, morph.
  **IDs de parâmetros congelados no v1.**

---

## 6. Tempo e sync (fundação)

- Classe central `TempoInfo`: cada bloco lê o `PlayHead` do host
  (BPM, posição em beats/ppq, time signature, a tocar/parado) e publica
  para os 4 módulos + REV. Sem PlayHead (standalone sem sync externo):
  **BPM manual** (40–240, defeito 120) + transporte interno play/stop.
- Conversões únicas e testadas: `beats→amostras`, `nota→beats`
  (incl. dotted/triplet), `ppq→fase de pattern`.
- **Marcações temporais na UI**: régua de beats com subdivisão atual,
  passo/beat ativo iluminado, display "BPM 128 · 4/4 · beat 2.3".
- Mudanças de BPM: rampadas internamente (tempos em amostras recalculados
  com smoothing; gater re-quantiza no próximo beat, nunca a meio).

---

## 7. Arquitetura de software

### 7.1 Layout de ficheiros proposto

```
deVerb/
  plano.md                 (este ficheiro)
  CMakeLists.txt           (juce_add_plugin FORMATS Standalone VST3 LV2)
  Source/
    PluginProcessor.{h,cpp}   (AudioProcessor, buses stereo, state)
    PluginEditor.{h,cpp}      (shell da UI; painéis por módulo)
    core/
      TempoInfo.{h,cpp}       (BPM/posição, conversões)
      Engine.{h,cpp}          (cadeia Gate→Delay→Reverb→Glitch + mix)
      ReverseEngine.{h,cpp}   (captura + leitor varispeed + cadeia própria)
      RevLinker.{h,cpp}       (inherit/morph/trims)
      PresetManager.{h,cpp}   (fábrica + user, versionamento)
    dsp/
      Gater.{h,cpp}  Delay.{h,cpp}  Reverb.{h,cpp}  Granular.{h,cpp}
      FdnReverb.{h,cpp}  PlateReverb.{h,cpp}  Shimmer.{h,cpp}  (fase 1b+)
      ReverseReader.{h,cpp}  GrainBuffer.{h,cpp}
    ui/
      GatePanel.{h,cpp}  DelayPanel.{h,cpp}  VerbPanel.{h,cpp}
      GlitchPanel.{h,cpp}  EngineStrip.{h,cpp}  BeatRuler.{h,cpp}
      MorphKnob.{h,cpp}  LookAndFeel.{h,cpp}
  presets/                 (fábrica .xml/.preset)
  faust/                   (protótipos .dsp por algoritmo)
  tests/                   (testes offline, ver §12)
```

### 7.2 Classes DSP — contrato comum

Cada módulo implementa: `prepare(spec, maxBlock)`, `process(block, ctx)`
(onde `ctx` = TempoInfo + flags throw/freeze), `reset()`,
`setParam(id, valorNormalizado)` sem alocação, e `getLatency()` (0 no MVP).
Construtor nunca aloca no audio thread; buffers pré-alocados no `prepare`.

### 7.3 Buses e MIDI

- Audio: stereo in → stereo out (mono compatível por downmix interno).
- MIDI in: aceite (restart de gater/patterns, THROW por nota, morph por CC).
  MIDI out: não. `IS_SYNTH FALSE`, `NEEDS_MIDI_INPUT TRUE`.
- Tail: `getTailLengthSeconds()` realista (ex: 8 s) para o host não cortar tails.

### 7.4 State e presets

- State via APVTS (`copyState/replaceState`) + `version` int para migração.
- Presets de fábrica em 3 níveis: **globais** (tudo), **por engine**,
  **por módulo**. Categorias: Trance Gate, Space, Reverse Tails, Glitch,
  Breakdown, Ambient.
- Regra: presets antigos carregam sempre (migração com defaults para
  parâmetros novos).

### 7.5 Threading e realtime-safety

- Audio thread: só DSP + leitura de atómicos. Zero `new/malloc`, zero locks,
  zero I/O de ficheiros, zero chamadas ao host.
- Message thread: UI, gestão de presets, load de IRs (futuro), `RevLinker`.
- Comunicação UI→DSP: APVTS (thread-safe) + FIFOs lock-free para eventos
  (throw, freeze, pattern edit).

### 7.6 Segurança sonora

- Cap de feedback a 95 %, cap de output com **limiter simples** no master
  (ceiling -1 dBFS, sempre ligado, sem parâmetros no MVP).
- `ScopedNoDenormals` em todo o lado; filtros com snap-to-zero.
- kill-switch: bypass total = passthrough bit-transparente.

---

## 8. Orçamento de CPU e memória (i7-4500U, 2c/4t)

| Bloco | Custo aprox. por instância | Nota |
|-------|----------------------------|------|
| Gater | ~0 % | só ganhos |
| Delay digital/tape/pingpong | < 1 % | + LFO no tape |
| Multitap 4 | < 1 % | mesma linha |
| Freeverb/Room | 1–2 % | |
| FDN Hall | 2–4 % | 4–8 linhas |
| Plate Dattorro | 3–5 % | modo lite: menos modulação |
| Shimmer | 4–6 % | pitch-shift é o custo |
| Granular 1 voz | 1–2 % | |
| Leitor REV varispeed | < 1 % | resampling Lagrange |
| **Total típico (FWD+REV médios)** | **5–12 % de um core** | OK |
| **Total extremo (tudo ao máximo)** | 20–35 % | aceitável; modos lite ajudam |

Memória: captura REV 4 beats @40 BPM ≈ 2.3 MB; delays 5 s ≈ 2 MB;
granular 2 compassos ≈ 4 MB. Total < 15 MB. Irrelevante.

Estratégia: medir com `perf`/medidor interno desde a fase 1; qualquer módulo
que passe 6 % ganha modo lite; REV sempre desligável.

---

## 9. UI/UX

### 9.1 Layout (standalone = plugin, mesma janela)

```
┌──────────────────────────────────────────────────────────┐
│ TRANSPORTE: BPM [HOST 128] 4/4 beat ●2.3 │ PRESET │ MORPH │
├────────────┬────────────┬────────────┬───────────────────┤
│ GATER      │ DELAY      │ REVERB     │ GRANULAR          │
│ pattern+   │ algo+tempo │ algo+size/ │ repeat+glitch     │
│ régua beats│舵 taps     │ decay+damp │ + GRÃO            │
├────────────┴────────────┴────────────┴───────────────────┤
│ ENGINE FWD [mix][mute] │ ENGINE REV [mix][mute][RATE][TAIL]│
│ THROW │ FREEZE │ DUCK │ LINK/MORPH por módulo           │
└──────────────────────────────────────────────────────────┘
```

- 4 painéis de módulo (FWD em cima por defeito; seletor FWD/REV por painel
  em vez de duplicar a janela — menos trabalho, mesma potência).
- Faixa de engines em baixo com os controlos de performance grandes.
- Régua de beats (`BeatRuler`) partilhada no topo do gater e do granular.

### 9.2 Componentes JUCE

`AudioProcessorValueTreeState` + Attachments em tudo; `LookAndFeel` custom
escuro com acento por módulo; knobs rotativos + sliders de pattern;
`BeatRuler` com repaint a 30 fps max (nunca no audio thread);
redimensionável com layout proporcional (guardar tamanho no state).

### 9.3 Workflow de design

Fase 1: UI funcional feia (sliders stock) — o som valida-se sem design.
Fase 2: mockup (Figma/grelha no papel) → LookAndFeel → knobs custom →
pattern editor desenhado à mão. Não inverter esta ordem.

### 9.4 Acessibilidade e detalhe

- Todos os knobs com entrada numérica (duplo clique) e reset a duplo-alt.
- Tooltips com unidade e valor musical ("1/8D @128 BPM = 351 ms").
- Tamanhos de texto legíveis a 100 % e 200 % (HiDPI).

---

## 10. Presets e state (estratégia)

- **Fábrica mínima viável** (15–20): 4 trance gates, 3 delays espaço,
  3 reverbs, 3 reverse tails, 3 glitch/breakdown. Cada preset com descrição
  de 1 linha (ensina o utilizador).
- **Categorias por uso**, não por módulo: Gate, Space, Reverse, Glitch,
  Breakdown, Ambient — o utilizador pensa em resultado, não em DSP.
- **User presets**: pasta ao lado do plugin + import/export `.deverb`.
- **Automação**: todos os parâmetros contínuos expostos com nomes e unidades;
  pattern e algoritmo como choice automatizável (cuidado: trocar algoritmo
  faz crossfade interno, nunca clique).

---

## 11. Estratégia de prototipagem (Faust + standalone)

Antes de escrever cada algoritmo em C++ final:

1. Protótipo `.dsp` em `deVerb/faust/` (ex: `multitap.dsp`, `freeverb_test.dsp`).
2. Ouvir com `faust2alsa`/`faust2jack` ou gerar C++ e inspecionar.
3. Congelar parâmetros que soam bem → transcrever para a classe JUCE.
4. Teste A/B: render offline do Faust vs JUCE com o mesmo input
   (ver §12.3) — diferenças > 1 dB investigam-se.

Isto separa "descobrir o som" (rápido, divertido) de "engenharia final"
(lento, rigoroso) — e poupa horas de compilação JUCE neste PC.

---

## 12. Testes e validação

### 12.1 Validação de formato (sempre)

- `pluginval --strictness-level 10` no VST3 a cada fase. Nível 5 durante o
  desenvolvimento diário, 10 antes de qualquer release/teste em DAW.
- Carregar em **Carla** (leve, já instalada) + pelo menos uma DAW real
  (Reaper trial ou Ardour) antes de declarar qualquer fase feita.

### 12.2 Testes funcionais por fase (checklist)

- Passthrough bit-transparente com tudo bypassed (null-test).
- Clicks/pops: varrer cada parâmetro de extremo a extremo a ouvir + medir
  (picos > 0 dBFS inesperados = falha).
- Sync: a 90/120/140/174 BPM, confirmar tempos com medição (não "a ouvido").
- MIDI: notas fazem restart/throw; CC move o morph.
- State: guardar preset, fechar, reabrir, comparar (hash do state).
- Estabilidade: 10 min a correr + abrir/fechar UI 20× + trocar presets
  depressa (apanha use-after-free e leaks).

### 12.3 Testes DSP offline

Pequena harness (executável separado ou testes no próprio repo) que corre
buffers sintéticos (impulso, seno, ruído) pelos módulos e verifica energia,
decaimento e invariantes (ex: reverb T60 dentro de ±20 % do pedido).
Golden files `.wav` de referência para regressões.

### 12.4 Teste de ouvido (sessão semanal)

Playlist fixa de 3 loops (pad, bateria, voz) + presets de referência;
ouvir no standalone com os olhos na UI e depois sem olhar (a UI não pode
enganar o ouvido).

---

## 13. Build, packaging e CI

- `juce_add_plugin(PRODUCT_NAME deVerb FORMATS Standalone VST3 LV2 ...)`,
  `PLUGIN_CODE` único de 4 chars, `LV2URI https://pedrolabs.example/deverb`.
- Flags do template HelloSine reutilizadas (`JUCE_WEB_BROWSER=0`,
  `JUCE_USE_CURL=0`, ccache, `CMAKE_EXPORT_COMPILE_COMMANDS`).
- Instalação local: VST3 → `~/.vst3/`, LV2 → `~/.lv2/` (Linux).
- CI (quando houver conta GitHub pronta): build Release Linux + pluginval
  em actions; Win/Mac só quando houver máquina para testar — nunca
  distribuir binário que não foi aberto pelo menos uma vez.
- Versionamento semver; cada release com notas + lista de presets novos.

---

## 14. Licenças e legal

- JUCE Starter: grátis até $20k/12m; se o deVerb vender acima disso, migrar
  para Indie antes de distribuir mais (custo conhecido, sem surpresas).
- Faust: os protótipos `.dsp` próprios são nossos; atenção às licenças se
  copiares código de exemplos de terceiros.
- IRs futuros (convolução): só IRs próprios ou com licença explícita
  (CC0/comprados) — nunca IRs sacados de packs comerciais.
- Nome/marca: verificar que "deVerb" não colide com plugin existente antes
  do v1 (pesquisa KVR + marcas).

---

## 15. Roadmap com critérios de aceitação

**Fase 0 — esqueleto** (1–2 tardes)
Projeto compila Standalone+VST3+LV2; passthrough transparente; APVTS com
5 parâmetros dummy; pluginval nível 5 passa. Critério: abrir o standalone
e ouvir o input igual ao output.

**Fase 1 — Delay + TempoInfo** (núcleo)
Digital + sync (ms/notas/D/T) + damping + mix; TempoInfo com BPM manual e
leitura do host; 3 presets. Critério: a 120 BPM, delay 1/8D medido = 375 ms.

**Fase 2 — Gater** (o "trance")
Pattern 16 passos + 6 presets + smooth + triggers HOST/MIDI/TRANSIENT +
régua de beats. Critério: pattern offbeat a 128 BPM alinha ao kick.

**Fase 3 — Reverb algorítmico**
Room (stock→Freeverb) → Hall (FDN) → Plate → Shimmer; pre-delay sync;
freeze. Critério: T60 pedido vs medido ±20 %; sem artefactos metálicos
óbvios no Room.

**Fase 4 — Granular/Glitch**
Beat-repeat + glitch Slice/Reverse/Pitch/Stutter + Chance/Env/Manual.
Critério: repeat 1/16 ×4 com decay soa quantizado, sem cliques.

**Fase 5 — Motor REV + link/morph**
Captura 2–4 beats + varispeed + cadeia própria + link/morph/trims +
THROW + DUCK. Critério: preset "reverse tail" reconhecível + morph
automatizável sem cliques.

**Fase 6 — Routing final + UI + release**
Ordem configurável, XFADE, presets de fábrica completos, LookAndFeel final,
pluginval nível 10, testes em 2 hosts, notas de release. Critério: outra
pessoa instala e usa sem explicações.

**Fase 7+ (futuro)**: convolução com IRs, sidechain externo, timestretch,
multitap 8, polifonia de grãos, builds Win/Mac.

---

## 16. Riscos e mitigações

| Risco | Impacto | Mitigação |
|-------|---------|-----------|
| Shimmer/Plate pesados no i7 | dropouts | modos lite + medição desde fase 1 + REV desligável |
| Cliques em pattern/rate/algoritmo | som amador | crossfades + rampas como regra, checklist §12.2 |
| IDs de parâmetros mudam e partem presets | suporte | congelar IDs no v1; migração com version |
| Scope creep (8 taps, sidechain, conv já) | nunca acaba | fases com critérios de saída; "fase 7+" é intencional |
| Latência do REV confunde utilizadores | más reviews | display "tail em X beats" + preset que demonstra |
| Colisão de nome/marca | legal | pesquisa antes do v1 (§14) |

---

## 17. Decisões tomadas / em aberto

**Tomadas**: dual-engine paralelo ADD; REV varispeed (timestretch fase 3);
captura 2–4 beats; latência do REV não compensada (é o efeito); reverb só
algorítmico no MVP; morph macro 0–100 % + links por módulo + trims;
routing configurável só na fase 6.

**Em aberto** (responder antes da fase indicada):
1. THROW + DUCK entram no MVP do REV? (recomendação: sim) [fase 5] — **SHIPPED**.
2. XFADE por beat entra na fase 6 ou fica para depois? [fase 6] — **SHIPPED**.
3. LFO no rate do reverse entra no MVP? (barato, alto valor) [fase 5] — **SHIPPED**.
4. MIDI: program-change de patterns por tecla na fase 2 ou depois? [fase 2] — **EM ABERTO**.
5. Nome final e códigos de plugin (4-char + LV2URI) antes da fase 0. — **EM ABERTO**.

---

## 19. Revisão UI v2 + modo global FWD|REV + algoritmos de delay (decisões)

Decidido em sessão de revisão (ecrã 1366×768, janela fixa 1280×624):

### 19.1 Um só interruptor FWD|REV (global, na topbar)

Os 4 toggles por coluna foram um erro de UX: 4 estados independentes =
16 combinações, e o utilizador perde-se ("estou a editar que lado?").
Substituir por **um controlo segmentado FWD|REV na topbar** que religa as
4 colunas de uma vez. Um bool `showRev` no editor; `bind*Column()`,
pattern editor, presets de pattern, readouts e régua seguem-no.
Os toggles de **link** (faixa REV) ficam — são conceito DSP diferente
("o REV *soa* como o FWD"), com tooltips a vincar a diferença vs o
interruptor de *edição*.
Regra de coerência (testada): virar a chave nunca escreve parâmetros;
só muda o que os widgets mostram. Valores FWD e REV nunca se misturam.

### 19.2 Delay com 5 algoritmos (o dropdown que faltava)

A fase 1 só trouxe Digital; a UI nunca teve selector. Estender `Delay`
(uma linha stereo, loop per-sample — estrutura ideal) com `Algo`:
Digital (atual, intocado), **Tape** (tanh drive + wow/flutter por LFO no
tempo de leitura), **PingPong** (cross-feedback na mesma linha, param
spread), **Multitap** (4 taps fixos 1, 3/4, 1/2, 1/4 com ganhos
1/.7/.5/.35, feedback do tap longo), **Reverse** (reusa `ReverseEngine`
com janela = tempo do delay em beats, via `setTempoBpm` por bloco).
Novos params (×2 FWD/REV): `delay_algo`, `delay_drive`, `delay_wow_rate`,
`delay_wow_depth`, `delay_spread`. Multitap/Reverse sem extras no v1.
UI: combo full-width na coluna DELAY + 4 mini-knobs 2×2 (drive, wow rate,
wow depth, spread) — cabe no ar reservado da coluna. Testes: saturação
tape, alternância pingpong, 4 ecos multitap, reverse ao contrário.

### 19.3 Protocolo de sessões heavy-user (FWD>REV>FWD>REV>FWD)

Cada sessão tem objetivo musical e guião (ex: "tail reverso a partir do
trance gate"): edita FWD → vira REV → edita → volta → confirma.
Verificações por viragem: valores mostrados == valores guardados do alvo,
sem cross-talk, DSP contínuo (sem NaN/cliques), presets aplicam no alvo
certo, get/setState com UI aberta, settings guardam ambos os lados e
a reabertura repõe tudo. Automatizado em `test_ui` (headless/Xvfb) +
sessões reais com xdotool e screenshots diff.

### 19.4 Roadmap ajustado

Fase 7 (a seguir): modo global FWD|REV + 5 algos de delay + testes de
sessão + docs. Fase 8 (futuro): convolução IRs, sidechain externo,
timestretch real, multitap editável, builds Win/Mac, user-presets em
ficheiro.

---

## 20. Log de decisões e incidentes

- **rev_mix duplicado**: o dummy da fase 0 sobreviveu à chegada do param real
  da fase 5 → IDs duplicados → `pluginval` 10 chumbava no state-restoration
  só nesse param. Removido o dummy. Lição: o gerador da tabela do README
  conta dupes (hoje: 124 IDs únicos).
- **Cross-talk FWD|REV nos rebinds**: o initial update do NOVO attachment faz
  `slider.setValue (notify)` ainda com o VELHO vivo → o velho escrevia no
  param errado. Apanhado pelo `test_ui` (FWD ficava com o default do REV).
  Regra: `.reset()` antes de recriar, sempre (ver `UI.md` regra 2).
- **Granular Off atenuava o dry** (mix 0.5 aplicado mesmo parado): Off agora
  é bypass transparente (apanhado pelo `test_chain`).
- **Limiter do master adiado**: o §7.6 previa ceiling -1 dBFS; adiado com nota
  (picos medidos < 1.0 nos testes; voltar a avaliar com presets extremos).
  **RESOLVIDO (v0.2)**: limiter implementado (peak follower, release 50 ms,
  ganho comum stereo) + teste que trava picos a -1 dBFS.
- **`getSampleRate()` sem host não é fiável**: o processador guardava o coef
  de release do limiter calculado dele (lixo sem host → limiter morto e picos
  a 2.78 nos testes). Regra: guardar `dspSr` no prepare e usar SEMPRE essa.
- **Wow além do buffer**: delay 2200 ms + wow 20 ms excedia a DelayLine;
  `maxDelaySec` 2.2 → 2.3 s de headroom.
- **Escrita de ficheiros do teste**: `test_ui`/`test_chain` não tocam em disco.
- **xdotool**: salto único de 50px não mexe knobs JUCE; usar 10×5px ou setas.
- **`·` U+00B7 rende `Â·`**: só ASCII em texto pintado.
- **Settings stale**: o standalone guarda estado ao sair; após mudar params
  no código, apagar `~/.config/deVerb.settings` (nota no README).
- **Bypass por módulo (v0.3)**: 8 switches `*_on` com helper `EnableRamp`
  (rampa 5 ms no wet, skip sem CPU, clear único sem tails); REV segue FWD
  via links; botões PWR nos headers ligados ao alvo FWD|REV.
- **Shimmer renascia após 2 s**: o loop FDN+anel tinha ganho > 1 no grave
  (modo ≈150 Hz a +11 dB/s); o teste antigo media o artefacto e passava.
  HP de 50 Hz no caminho da oitava estabiliza tudo (matriz SR×T60×size);
  testes 10b/10d medem bloom/decaimento/oitava real.
- **Botão THROW morto**: `V4Key` fazia 1→0 no mesmo gesto e o DSP nunca via
  o flanco (lê o botão por bloco). Hold de 60 ms via `callAfterDelay`.
- **Duck em escada**: o envelope lia sempre a última amostra gravada
  (constante no bloco); bursts curtos nem duckavam. Cursor por amostra
  sobre `[w-n, w)`; teste 21 trava regressão.
- **Seletor de presets não escolhia**: presets de fábrica são deltas e o
  seletor mudava o nome mas motores ligados (REV Loop, granular) ficavam a
  tocar nos presets seguintes. `applyFactoryPreset` repõe defaults (menos
  `tempo_bpm`) antes dos deltas; testes U9 + chain-preset travam.
- **Glitch com salto temporal**: a gravação pausava a tocar e o grab seguinte
  colava áudio antigo a novo (foto `loopL/R` existia mas ninguém lia dela).
  Grava sempre + foto no grab + costura loop clássica na foto; teste 22.
- **Verificação total (lotes A–D)**: Slice/Stutter/EnvTrig/Pitch−12, delay
  freeze infinito, XFADE por beat, POST, MIDI-throw, orders 1/3, MAN sync,
  RANDOM via tecla, HOST/MAN end-to-end (testes 26–34, U9, C2, routing).
  Bench §8 no i7 (48k/512): módulos ≤2.8%/core, default 3.2%, extremo 19%
  (dentro do orçamento). ASan+UBSan limpo nas 3 suites, 0 relatórios.
- **Granular a fundo**: costura com xf stale no bloco do grab (+3 dB fantasma;
  agora xeff por amostra; teste 37); decay do Stutter por tempo de fragmento
  (antes morria em ~200 ms e o TIME ficava decorativo); TIME mostra o locked com
  nota + DECAY esbate fora de BR/Stutter; LED GRAB! com hold de 250 ms (o flag
  cru aliasava a 30 Hz).
- **Dimension Expander no OUT** (124 IDs): 4 delays ultra-curtos + matriz
  fora-de-fase (wet soma em L, subtrai em R → soma-mono exata). Caça grossa:
  o 1º `popSample` de CADA canal tem de avançar o read pointer da JUCE
  DelayLine, senão o canal parado relê a mesma zona e clica a cada volta do
  anel (período = totalSize). Testes D1–D4 + mainOut + U10.
- **Fix mac `dimSizeAtt/dimMixAtt` no dtor** (EXC_BAD_ACCESS no teardown do
  test_ui/mac; Linux passava por sorte da heap).
- **Lote F–H**: `syncPolicyUi`/`lastPreset` repostos no restore (testes F1/F2),
  `getParamFloat` defensivo, Espaço passa ao host (teste F5), drift 122/104
  limpo, cobertura G38–G60 + C3. O G não achou bugs no DSP (só 1 fix de
  harness: threshold do duck; o resto foi afinar janelas/limiares dos
  testes). Grab da foto em 2 memcpys (1.08 ms → 0.15 ms; cabe a 64
  amostras), scope com drop-tail em vez de `reset()` (race), `stopTimer()`
  no dtor. Re-medição: reset estrutural 0.27 ms, grab 0.15 ms (orçamento
  10.7 ms). ASan+UBSan re-corridos limpos após as mudanças.

## 18. Glossário rápido

- **FWD/REV**: motor normal / motor reverso. **APVTS**: sistema de parâmetros
  do JUCE. **PPQ**: pulsos por semínima (posição musical do host).
- **FDN**: rede de delays com feedback matricial (reverb moderno).
- **Varispeed**: mudar velocidade com pitch a acompanhar (tape).
- **Timestretch**: mudar tempo sem mudar pitch (fase 3).
- **THROW**: disparo único de tail reverso. **FREEZE**: congelar caudas/buffers.
- **pluginval**: validador oficial de plugins (nível 10 = release).
