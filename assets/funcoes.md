# deVerb — Manual de Funções por Módulo

> Descrição completa das **124 funções** do plugin, módulo a módulo, em
> linguagem de manual de instruções. Serve de base ao manual do utilizador
> e de referência ao designer (cada função indica o tipo de controlo ideal).
>
> Convenção: cada função tem **nome UI** (o que se vê no ecrã), **ID interno**,
> **tipo/gama/default**. Quase tudo existe em dose dupla **FWD** (sinal normal)
> e **REV** (sinal reverso); o seletor global **FWD|REV** na topbar escolhe
> qual dos dois lados se está a editar.

---

## 1. MOTOR — PERFORM (coluna + topbar)

O "painel de voo": ganhos gerais e mistura dos dois motores. É onde se atua
ao vivo.

| Função (UI) | ID | Tipo / Gama | Default | O que faz |
|---|---|---|---|---|
| INPUT | `input_gain` | knob 0.0–2.0 | 1.00 | Volume de entrada do plugin. 1.0 = sem alteração. |
| FWD MIX | `fwd_mix` | knob 0–1 | 1.00 | Quanto do motor FWD (efeitos normais) se ouve. |
| REV MIX | `rev_mix` | knob 0–1 | 0.40 | Quanto do motor REV (efeitos reversos) se ouve. |
| MORPH | `morph` | knob 0–1 | 0.00 | **A macro estrela.** 0% = o REV copia o FWD (nos módulos com link ligado); 100% = o REV usa os seus valores próprios. Automatizar isto cria transições. |
| MASTER | `master` | knob 0–1 | 0.80 | Volume geral de saída. |
| SIZE | `dim_size` | knob 0–1 | 0.35 | Dimension Expander (OUT): tamanho das reflexões (0 = colado, 1 = slap/room pequena). |
| WIDTH | `dim_mix` | knob 0–1 (mostra %) | 0% | Dimension Expander (OUT): quantidade de largura stereo. Soma-mono preservada por construção (mono-compatível). |
| Preset... | — | combo | — | 11 presets de fábrica (Init, Trance Gate 16, Offbeat Chop, Big Hall Space, Reverse Tail, Reverse Throw, Beat Repeat, Stutter Brk, Dub Echo, Shimmer Pad, Build Up). |
| RANDOM | — | botão | — | Gera um preset aleatório mas sempre audível (gamas musicais, nunca silêncio). |
| FWD/REV | — | segmentado | FWD | Escolhe qual dos dois motores se edita nas colunas. Não é som, é só a "página" que se vê. |
| BPM readout | `tempo_bpm` | knob 40–240 | 120.0 | Tempo interno. O seletor SYNC (fora dos params) escolhe HOST (segue a DAW, fallback manual) ou MAN (sempre o knob); a caixa esbate quando o knob não conta. |
| GATE passo/total | — | readout | — | Mostra o passo atual do gater (ex. 07/16). |
| GRAB!/IDLE | — | LED | — | Acende quando o granular está a agarrar som. |
| G-D-V-G / Add | `chain_order` / `x_mode` | combos | G-D-V-G / Add | Ordem da cadeia e modo de mistura (ver §8 Routing). |

**Limiter de segurança:** sempre ligado no master, teto -1 dBFS. Podes puxar
tudo sem medo de estourar as colunas. A caixinha `GR` ao lado mostra a
redução em dB (0 = limpo); se viveres acima de −6 dB, baixa o MASTER ou
os mixes — o limiter protege, mas comprime.
**Mudanças bulk** (presets, RANDOM, restore) fazem fade de 40 ms para não
estalarem.

---

## 2. GATE — TRANCE (coluna amarela)

Corta o som em ritmo, como uma faca rítmica — o efeito clássico de trance
em pads. É o primeiro módulo da cadeia por defeito (para as caudas do delay
e do reverb não serem cortadas).

| Função (UI) | ID (`fwd_`/`rev_`) | Tipo / Gama | Default | O que faz |
|---|---|---|---|---|
| Régua + 16 steps | `gate_pattern` | 16 botões on/off | Porta¹ | O padrão de corte: quadrado aceso = som passa, apagado = som corta. |
| Pattern... | — | stepper | — | 10 padrões de fábrica: Porta 4/4, Offbeat, 8ths, 16ths, Saw Up, Euclid 5, Tresillo, Cinquillo, Euclid 3, Euclid 7. |
| Rate | `gate_rate` | combo de notas | 1/16 | Velocidade do corte, em notas musicais (Free–1/1). |
| STEPS | `gate_steps` | stepper | 16 | Tamanho do pattern: 1–16 passos livres (8/16 primeiro por compatibilidade; tercinas: 12/6/3 + rate T). |
| Trigger | `gate_trig` | combo | Host | O que reinicia o padrão: **Host** (segue a DAW), **Midi** (cada nota recomeça), **Transient** (recomeça nos ataques do som), **Free** (corre livre). |
| SMOOTH | `gate_smooth` | knob 0–1 | 0.15 | Suavidade do corte. 0 = faca (agressivo); alto = fundido suave, sem cliques. |
| DEPTH | `gate_depth` | knob 0–1 | 1.00 | Profundidade: 1.0 = silêncio total nos passos fechados; baixa para um efeito subtil. |
| MIX | `gate_mix` | knob 0–1 | 1.00 | Mistura som cortado vs. original. |
| PAN | `gate_pan` | knob 0–1 | 0.00 | Pan alternado por passo (esquerda/direita/esquerda...). Efeito ping-pong rítmico. |
| THR | `gate_env_thr` | knob −60–0 dB | −18.0 | Sensibilidade do trigger Transient. |
| PWR | `gate_on` | toggle | ON | Liga/desliga o módulo (limpa buffers, sem tails penduradas). |

¹ Porta 4/4 = passos 1, 5, 9, 13 acesos (`0x1111`).

**Notas disponíveis** (rate e todos os seletores de nota do plugin):
Free, 1/32, 1/16T, 1/16, 1/16D, 1/8T, 1/8, 1/8D, 1/4, 1/4D, 1/2, 1/1
(T = tercina, D = pontuada).

---

## 3. DELAY — ECHO (coluna verde)

Repete o som no tempo, com 5 sabores. Cada algoritmo muda o caráter das
repetições.

| Função (UI) | ID (`fwd_`/`rev_`) | Tipo / Gama | Default | O que faz |
|---|---|---|---|---|
| Algoritmo | `delay_algo` | combo | Digital | **Digital** (limpo), **Tape** (fita vintage), **PingPong** (saltita L/R), **MultiTap** (4 repetições rítmicas), **Reverse** (repete ao contrário). |
| BPM | `tempo_bpm` | knob 40–240 | 120.0 | Base de tempo manual — conta em MAN ou sem host; em HOST manda a DAW. Setas ↑↓ afinam (±1, Shift ±0.1); duplo clique repõe 120. |
| DIV | `delay_note` | combo de notas | 1/8D | **O controlo principal do tempo**: divisões musicais que seguem o BPM (Free, 1/32…1/1; T = tercina, D = pontuada). |
| MS | `delay_time` | knob 1–2000 ms | 375 | Ajuste fino em ms — **só mexe quando DIV = Free** (o knob fica esbatido caso contrário). O readout mostra sempre o resultado: `375 ms - 1/8D @ HOST`. |
| FB | `delay_fb` | knob 0–0.95 | 0.35 | Feedback: quantas vezes repete. Alto = repetições infinitas. |
| DAMP | `delay_damp` | knob 200–18000 Hz | 6000 | Escurece cada repetição (agudos perdidos = distância). |
| MIX | `delay_mix` | knob 0–1 | 0.25 | Quantidade de delay no som. |
| DRIVE | `delay_drive` | knob 0–1 | 0.30 | Saturação da fita (só Tape): aquece e comprime as repetições. |
| WOW RT | `delay_wow_rate` | knob 0.1–5 Hz | 1.0 | Velocidade da flutuação da fita (só Tape). |
| WOW DP | `delay_wow_depth` | knob 0–20 ms | 4.0 | Profundidade da flutuação — desafinação vintage (só Tape). |
| SPREAD | `delay_spread` | knob 0–1 | 1.00 | Abertura stereo do PingPong. |
| Freeze | `delay_freeze` | toggle | OFF | Congela o buffer: repete o último fragmento em loop. |
| PWR | `delay_on` | toggle | ON | Liga/desliga o módulo. |

---

## 4. VERB — SPACE (coluna azul)

Põe o som dentro de uma sala — do quarto pequeno à catedral. 4 algoritmos.

| Função (UI) | ID (`fwd_`/`rev_`) | Tipo / Gama | Default | O que faz |
|---|---|---|---|---|
| Algoritmo | `verb_algo` | combo | Hall | **Room** (quarto, colado), **Hall** (sala grande), **Plate** (placa metálica, vozes/brilho), **Shimmer** (cauda etérea oitavada, pads infinitos). |
| SIZE | `verb_size` | knob 0–1 | 0.50 | Tamanho da sala. |
| DECAY | `verb_decay` | knob 0.2–20 s | 2.5 | Quanto tempo a cauda demora a desaparecer (T60). O readout mostra o valor. |
| DAMP | `verb_damp` | knob 0–1 | 0.30 | Absorção de agudos (paredes moles vs. pedra). |
| WIDTH | `verb_width` | knob 0–1 | 1.00 | Largura stereo: 1 = cheia, 0 = mono. |
| PRE-DLY | `verb_predelay` | knob 0–250 ms | 20 | Atraso antes de a sala "responder" (separa o seco da cauda). |
| Nota pré-delay | `verb_predelay_note` | combo de notas | Free | Pré-delay em notas em vez de ms. |
| LO-CUT | `verb_locut` | knob 20–500 Hz | 80 | Corta graves da cauda (limpa a mistura). |
| HI-CUT | `verb_hicut` | knob 2–20 kHz | 12000 | Corta agudos da cauda (tira aspereza). |
| MIX | `verb_mix` | knob 0–1 | 0.30 | Quantidade de sala no som. |
| Freeze | `verb_freeze` | toggle | OFF | Congela a cauda (drone infinito da sala). |
| PWR | `verb_on` | toggle | ON | Liga/desliga o módulo. |

> **Shimmer estável:** o loop FDN+anel oitavado tinha ganho > 1 no grave
> (modo ≈125 Hz a +11 dB/s — a cauda "renascia" após 2 s). Um HP de 50 Hz
> no caminho da oitava estabiliza em toda a matriz SR×T60×size sem tocar
> nas oitavas musicais (testes 10b/10d: bloom + oitava 880/440 + decaimento).

**Reservado:** o espaço vazio na coluna (painel "CONVOLUTION – FASE 8") vai
receber reverb por convolução com respostas impulsivas reais (fase futura).

---

## 5. GRAN — GLITCH (coluna laranja)

Parte o som em pedacinhos e recompõe-no — stutter, glitch e texturas. É o
módulo da surpresa; trabalha por último, sobre o som já com espaço.

| Função (UI) | ID (`gr_`/`rev_gr_`) | Tipo / Gama | Default | O que faz |
|---|---|---|---|---|
| Modo | `gr_mode` | combo | Off | **Off** (desligado), **BeatRepeat** (repete um fragmento N vezes a decair), **Slice** (fatia em loop cortado), **Reverse** (fragmento ao contrário), **Pitch** (repete mudado de tom), **Stutter** (gagueja rápido o oitavo mais recente). |
| Trigger | `gr_trigger` | combo | Chance | O que dispara: **Chance** (probabilidade), **Envelope** (ataques do som), **Manual** (botão GRAB). |
| GRAB | `gr_manual` | botão | OFF | Disparo manual (prime para agarrar som). |
| Fragmento | `gr_len_note` | combo de notas | 1/16 | Tamanho do pedaço agarrado. |
| CHANCE | `gr_chance` | knob 0–1 | 0.20 | Probabilidade de disparo (modo Chance). |
| THR | `gr_env_thr` | knob −60–0 dB | −18.0 | Sensibilidade (modo Envelope). |
| REPEATS | `gr_repeats` | knob 1–16 | 4 | Número de repetições (só BeatRepeat; Stutter/Slice duram o TIME). |
| DECAY | `gr_decay` | knob 0.5–0.99 | 0.85 | Quanto cada repetição perde de volume. |
| TIME | `gr_time` / nota | knob 60–8000 ms + combo | 250 / Free | Duração do glitch (modos não-BeatRepeat). Com nota, o knob esbate e a caixa mostra o tempo locked (ex.: 1/4 a 120 BPM = 500 ms). |
| PITCH | `gr_pitch` | knob −12–+12 st | +12 | Tom das repetições (modo Pitch). Interpolação linear = estética lo-fi assumida (sem anti-alias). |
| FLUX | `gr_flux` | knob 0–1 | 0.30 | Densidade do chop no modo Slice (2–8 fatias; só conta aí). |
| XFADE | `gr_xfade` | knob 1–50 ms | 8.0 | Crossfade das emendas (anti-cliques). |
| MIX | `gr_mix` | knob 0–1 | 0.50 | Quantidade de glitch no som. |
| Interrupt | `gr_interrupt` | toggle | OFF | ON = o glitch pausa o som seco (efeito total). |
| PWR | `gr_on` | toggle | ON | Liga/desliga o módulo. |

O LED **GRAB!/IDLE** na topbar acende quando está a agarrar som.

---

## 6. REV ENGINE — o motor reverso (faixa roxa na topbar)

Enquanto o FWD processa o presente, o REV processa o **passado recente lido
ao contrário**: captura os últimos N beats e toca-os de trás para a frente
com velocidade ajustável, através da sua própria cadeia dos mesmos 4 efeitos.
O resultado soma-se como uma cauda decorativa (reverse throws).

| Função (UI) | ID | Tipo / Gama | Default | O que faz |
|---|---|---|---|---|
| Modo | `rev_mode` | combo | Off | **Off** (desligado, custo zero), **Loop** (cauda contínua), **Throw** (dispara UMA cauda por botão/MIDI e cala-se). |
| Fonte | `rev_source` | combo | Dry | O que captura: **Dry** (som limpo pós-ganho) ou **PostFWD** (já com efeitos FWD). |
| Janela | `rev_capture` | combo | 2 beats | Tamanho da captura: 2, 3 ou 4 beats. |
| RATE | `rev_rate` | knob 0.25–2× | 1.00 | Velocidade do reverso (varispeed, pitch acompanha): <1 atrasa e agrava, >1 acelera e aguça. |
| LFO | `rev_lfo` | knob 0–1 | 0.00 | Vagueio lento do rate (reverse orgânico, quase-chorus). |
| THROW | `rev_throw` | botão | OFF | Dispara uma cauda (modo Throw; também dispara por nota MIDI). O preset Reverse Throw fá-lo disparar ao carregar (audition intencional). |
| DUCK | `rev_duck` | knob 0–1 | 0.30 | Baixa a cauda quando há sinal novo (limpeza automática). |
| T-DLY | `trim_delay` | knob 0.25–4× | 1.00 | Multiplica o tempo do delay REV vs. FWD. |
| T-DEC | `trim_decay` | knob 0.25–2× | 1.00 | Multiplica a cauda do reverb REV vs. FWD. |

---

## 7. LINKS — herança FWD→REV (faixa roxa)

| Função (UI) | ID | Default | O que faz |
|---|---|---|---|
| ALL | `link_master` | ON | Mestre: desligar liberta todos os módulos de uma vez. |
| GATE/DLY/VRB/GRN | `link_gate/delay/verb/gran` | ON | Cada motor REV segue (ON) ou não (OFF) os valores do FWD. |
| MORPH | `morph` | 0.00 | 0% = REV espelha FWD; 100% = valores próprios. Automatizável: morphs em breakdowns. |

**Não confundir:** os links dizem como o REV **soa**; o seletor FWD|REV diz
o que se está a **editar**. Conceitos separados de propósito.

---

## 8. ROUTING — ordem e mistura (faixa roxa)

| Função (UI) | ID | Default | O que faz |
|---|---|---|---|
| Ordem | `chain_order` | G-D-V-Gr | Ordem dos módulos (igual nas 2 cadeias): Gate·Delay·Verb·Gran, G·V·D·Gr, D·G·V·Gr ou V·D·G·Gr. |
| Add/XFade | `x_mode` | Add | **Add** = soma FWD+REV. **XFade** = beats pares levam FWD, ímpares REV (sem REV, comporta-se como Add). |

---

## 9. ON/OFF por módulo — botões PWR (headers das colunas)

Cada um dos 4 módulos tem o seu interruptor, em dose dupla FWD+REV (8 no
total). Permitem brincar com cada módulo independentemente: desliga o que
não queres ouvir sem mexer em mais nada.

| Função (UI) | ID | Default | O que faz |
|---|---|---|---|
| PWR (GATE) | `fwd_gate_on` / `rev_gate_on` | ON | Liga/desliga o gater. |
| PWR (DELAY) | `fwd_delay_on` / `rev_delay_on` | ON | Liga/desliga o delay. |
| PWR (VERB) | `fwd_verb_on` / `rev_verb_on` | ON | Liga/desliga o reverb. |
| PWR (GRAN) | `gr_on` / `rev_gr_on` | ON | Liga/desliga o granular. |

**Comportamento** (igual nos 8): ao desligar, o som desvanece em ~5 ms (sem
cliques), os buffers limpam-se (sem tails penduradas ao religar) e o módulo
deixa de gastar CPU. Ao religar, começa sempre do zero. O lado REV segue o
FWD quando o link do módulo está ligado (ver §7); com link desligado cada
lado tem o seu PWR independente. O botão mostra sempre o lado que se está
a editar (seletor global FWD|REV).

---

## 10. Dicas rápidas de manual

1. **Sem som?** Verifica PWR dos módulos, mixes a zero, pattern todo apagado,
   granular em Interrupt sem trigger, ou REV a 0 com FWD cortado.
2. **Primeiros passos:** preset "Trance Gate 16" → prime THROW em modo Throw
   → mexe no MORPH.
3. **Breakdown:** XFade + morph automatizado 0→1 + Shimmer com decay longo.
4. **Tails reversas musicais:** REV Loop, 2 beats, rate 1.0, duck 0.3–0.5,
   reverb Hall atrás.
5. **Glitch controlado:** BeatRepeat 1/16 ×4, chance 20–40%, mix 0.5.

Total: **124 parâmetros** (IDs congelados — nunca mudam de nome).
## Apêndice A — Tabela completa (124 parâmetros, gerada do código)

> Gerada automaticamente de `createParams()` — se algum ID se repetir,
> o teste de geração acusa. Última verificação: 124 únicos, 0 duplicados.

### motor (7)

| ID | Nome UI | Tipo | Gama / Opções | Default |
|----|---------|------|---------------|---------|
| `input_gain` | Input Gain | float | 0.f, 2.f, 0.01f | 1.f |
| `fwd_mix` | FWD Mix | float | 0.f, 1.f, 0.01f | 1.f |
| `morph` | Morph | float | 0.f, 1.f, 0.01f | 0.f |
| `master` | Master | float | 0.f, 1.f, 0.01f | 0.8f |
| `tempo_bpm` | Tempo BPM | float | 40.f, 240.f, 0.1f | 120.f |
| `chain_order` | Chain Order | choice | G-D-V-Gr | G-V-D-Gr | D-G-V-Gr | V-D-G-Gr | G-D-V-Gr |
| `x_mode` | X Mode | choice | Add | XFade | Add |

### fwd_delay (12)

| ID | Nome UI | Tipo | Gama / Opções | Default |
|----|---------|------|---------------|---------|
| `fwd_delay_note` | FWD Delay Note | choice-nota | Free | 1/32 | 1/16T | 1/16 | 1/16D | 1/8T | 1/8 | 1/8D | 1/4 | 1/4D | 1/2 | 1/1 | 1/8D |
| `fwd_delay_time` | FWD Delay Ms | float | 1.f, 2000.f, 0.1f | 375.f |
| `fwd_delay_fb` | FWD Delay FB | float | 0.f, 0.95f, 0.01f | 0.35f |
| `fwd_delay_damp` | FWD Delay Damp | float | 200.f, 18000.f, 1.f, 0.35f | 6000.f |
| `fwd_delay_mix` | FWD Delay Mix | float | 0.f, 1.f, 0.01f | 0.25f |
| `fwd_delay_freeze` | FWD Delay Freeze | on/off | - | OFF |
| `fwd_delay_algo` | FWD Delay Algo | choice | Digital | Tape | PingPong | MultiTap | Reverse | Digital |
| `fwd_delay_drive` | FWD Delay Drive | float | 0.f, 1.f, 0.01f | 0.3f |
| `fwd_delay_wow_rate` | FWD Delay WowRate | float | 0.1f, 5.f, 0.01f, 0.5f | 1.f |
| `fwd_delay_wow_depth` | FWD Delay WowDepth | float | 0.f, 20.f, 0.1f | 4.f |
| `fwd_delay_spread` | FWD Delay Spread | float | 0.f, 1.f, 0.01f | 1.f |
| `fwd_delay_on` | FWD Delay On | on/off | - | ON |

### fwd_gate (10)

| ID | Nome UI | Tipo | Gama / Opções | Default |
|----|---------|------|---------------|---------|
| `fwd_gate_rate` | FWD Gate Rate | choice-nota | Free | 1/32 | 1/16T | 1/16 | 1/16D | 1/8T | 1/8 | 1/8D | 1/4 | 1/4D | 1/2 | 1/1 | 1/16 |
| `fwd_gate_steps` | FWD Gate Steps | choice 1–16 | 8 | 16 | 12 | 6 | 4 | 3 | 2 | 1 | 5 | 7 | 9 | 10 | 11 | 13 | 14 | 15 | 16 |
| `fwd_gate_pattern` | FWD Gate Pattern | int | 0..65535 | 0x1111 (4369) |
| `fwd_gate_smooth` | FWD Gate Smooth | float | 0.f, 1.f, 0.01f | 0.15f |
| `fwd_gate_depth` | FWD Gate Depth | float | 0.f, 1.f, 0.01f | 1.f |
| `fwd_gate_mix` | FWD Gate Mix | float | 0.f, 1.f, 0.01f | 1.f |
| `fwd_gate_pan` | FWD Gate PanAlt | float | 0.f, 1.f, 0.01f | 0.f |
| `fwd_gate_trig` | FWD Gate Trig | choice | Host | Midi | Transient | Free | Host |
| `fwd_gate_env_thr` | FWD Gate EnvThr | float | -60.f, 0.f, 0.5f | -18.f |
| `fwd_gate_on` | FWD Gate On | on/off | - | ON |

### fwd_verb (12)

| ID | Nome UI | Tipo | Gama / Opções | Default |
|----|---------|------|---------------|---------|
| `fwd_verb_algo` | FWD Verb Algo | choice | Room | Hall | Plate | Shimmer | Hall |
| `fwd_verb_size` | FWD Verb Size | float | 0.f, 1.f, 0.01f | 0.5f |
| `fwd_verb_decay` | FWD Verb Decay | float | 0.2f, 20.f, 0.01f, 0.4f | 2.5f |
| `fwd_verb_damp` | FWD Verb Damp | float | 0.f, 1.f, 0.01f | 0.3f |
| `fwd_verb_width` | FWD Verb Width | float | 0.f, 1.f, 0.01f | 1.f |
| `fwd_verb_predelay` | FWD Verb PreDelay | float | 0.f, 250.f, 0.1f | 20.f |
| `fwd_verb_predelay_note` | FWD Verb PreDelay Note | choice-nota | Free | 1/32 | 1/16T | 1/16 | 1/16D | 1/8T | 1/8 | 1/8D | 1/4 | 1/4D | 1/2 | 1/1 | Free |
| `fwd_verb_freeze` | FWD Verb Freeze | on/off | - | OFF |
| `fwd_verb_mix` | FWD Verb Mix | float | 0.f, 1.f, 0.01f | 0.3f |
| `fwd_verb_locut` | FWD Verb LoCut | float | 20.f, 500.f, 1.f, 0.5f | 80.f |
| `fwd_verb_hicut` | FWD Verb HiCut | float | 2000.f, 20000.f, 1.f, 0.5f | 12000.f |
| `fwd_verb_on` | FWD Verb On | on/off | - | ON |

### gr (16)

| ID | Nome UI | Tipo | Gama / Opções | Default |
|----|---------|------|---------------|---------|
| `gr_mode` | Gran Mode | choice | Off | BeatRepeat | Slice | Reverse | Pitch | Stutter | Off |
| `gr_trigger` | Gran Trig | choice | Chance | Envelope | Manual | Chance |
| `gr_manual` | Gran Manual | on/off | - | OFF |
| `gr_chance` | Gran Chance | float | 0.f, 1.f, 0.01f | 0.2f |
| `gr_env_thr` | Gran EnvThr | float | -60.f, 0.f, 0.5f | -18.f |
| `gr_len_note` | Gran Len Note | choice-nota | Free | 1/32 | 1/16T | 1/16 | 1/16D | 1/8T | 1/8 | 1/8D | 1/4 | 1/4D | 1/2 | 1/1 | 1/16 |
| `gr_repeats` | Gran Repeats | int | 1..16 | 4 |
| `gr_decay` | Gran Decay | float | 0.5f, 0.99f, 0.01f | 0.85f |
| `gr_time` | Gran Time Ms | float | 60.f, 8000.f, 1.f, 0.4f | 250.f |
| `gr_time_note` | Gran Time Note | choice-nota | Free | 1/32 | 1/16T | 1/16 | 1/16D | 1/8T | 1/8 | 1/8D | 1/4 | 1/4D | 1/2 | 1/1 | Free |
| `gr_pitch` | Gran Pitch | float | -12.f, 12.f, 0.5f | 12.f |
| `gr_flux` | Gran Flux | float | 0.f, 1.f, 0.01f | 0.3f |
| `gr_xfade` | Gran Xfade | float | 1.f, 50.f, 0.5f | 8.f |
| `gr_mix` | Gran Mix | float | 0.f, 1.f, 0.01f | 0.5f |
| `gr_interrupt` | Gran Interrupt | on/off | - | OFF |
| `gr_on` | Gran On | on/off | - | ON |

### rev_gate (10)

| ID | Nome UI | Tipo | Gama / Opções | Default |
|----|---------|------|---------------|---------|
| `rev_gate_on` | REV Gate On | on/off | - | ON |
| `rev_gate_rate` | REV Gate Rate | choice-nota | Free | 1/32 | 1/16T | 1/16 | 1/16D | 1/8T | 1/8 | 1/8D | 1/4 | 1/4D | 1/2 | 1/1 | 1/16 |
| `rev_gate_steps` | REV Gate Steps | choice 1–16 | 8 | 16 | 12 | 6 | 4 | 3 | 2 | 1 | 5 | 7 | 9 | 10 | 11 | 13 | 14 | 15 | 16 |
| `rev_gate_pattern` | REV Gate Pattern | int | 0..65535 | 0x1111 (4369) |
| `rev_gate_smooth` | REV Gate Smooth | float | 0.f, 1.f, 0.01f | 0.15f |
| `rev_gate_depth` | REV Gate Depth | float | 0.f, 1.f, 0.01f | 1.f |
| `rev_gate_mix` | REV Gate Mix | float | 0.f, 1.f, 0.01f | 1.f |
| `rev_gate_pan` | REV Gate PanAlt | float | 0.f, 1.f, 0.01f | 0.f |
| `rev_gate_trig` | REV Gate Trig | choice | Host | Midi | Transient | Free | Host |
| `rev_gate_env_thr` | REV Gate EnvThr | float | -60.f, 0.f, 0.5f | -18.f |

### rev_delay (12)

| ID | Nome UI | Tipo | Gama / Opções | Default |
|----|---------|------|---------------|---------|
| `rev_delay_on` | REV Delay On | on/off | - | ON |
| `rev_delay_note` | REV Delay Note | choice-nota | Free | 1/32 | 1/16T | 1/16 | 1/16D | 1/8T | 1/8 | 1/8D | 1/4 | 1/4D | 1/2 | 1/1 | 1/8D |
| `rev_delay_time` | REV Delay Ms | float | 1.f, 2000.f, 0.1f | 375.f |
| `rev_delay_fb` | REV Delay FB | float | 0.f, 0.95f, 0.01f | 0.35f |
| `rev_delay_damp` | REV Delay Damp | float | 200.f, 18000.f, 1.f, 0.35f | 6000.f |
| `rev_delay_mix` | REV Delay Mix | float | 0.f, 1.f, 0.01f | 0.25f |
| `rev_delay_freeze` | REV Delay Freeze | on/off | - | OFF |
| `rev_delay_algo` | REV Delay Algo | choice | Digital | Tape | PingPong | MultiTap | Reverse | Digital |
| `rev_delay_drive` | REV Delay Drive | float | 0.f, 1.f, 0.01f | 0.3f |
| `rev_delay_wow_rate` | REV Delay WowRate | float | 0.1f, 5.f, 0.01f, 0.5f | 1.f |
| `rev_delay_wow_depth` | REV Delay WowDepth | float | 0.f, 20.f, 0.1f | 4.f |
| `rev_delay_spread` | REV Delay Spread | float | 0.f, 1.f, 0.01f | 1.f |

### rev_verb (12)

| ID | Nome UI | Tipo | Gama / Opções | Default |
|----|---------|------|---------------|---------|
| `rev_verb_on` | REV Verb On | on/off | - | ON |
| `rev_verb_algo` | REV Verb Algo | choice | Room | Hall | Plate | Shimmer | Hall |
| `rev_verb_size` | REV Verb Size | float | 0.f, 1.f, 0.01f | 0.5f |
| `rev_verb_decay` | REV Verb Decay | float | 0.2f, 20.f, 0.01f, 0.4f | 2.5f |
| `rev_verb_damp` | REV Verb Damp | float | 0.f, 1.f, 0.01f | 0.3f |
| `rev_verb_width` | REV Verb Width | float | 0.f, 1.f, 0.01f | 1.f |
| `rev_verb_predelay` | REV Verb PreDelay | float | 0.f, 250.f, 0.1f | 20.f |
| `rev_verb_predelay_note` | REV Verb PreDelay Note | choice-nota | Free | 1/32 | 1/16T | 1/16 | 1/16D | 1/8T | 1/8 | 1/8D | 1/4 | 1/4D | 1/2 | 1/1 | Free |
| `rev_verb_freeze` | REV Verb Freeze | on/off | - | OFF |
| `rev_verb_mix` | REV Verb Mix | float | 0.f, 1.f, 0.01f | 0.3f |
| `rev_verb_locut` | REV Verb LoCut | float | 20.f, 500.f, 1.f, 0.5f | 80.f |
| `rev_verb_hicut` | REV Verb HiCut | float | 2000.f, 20000.f, 1.f, 0.5f | 12000.f |

### rev_gr (16)

| ID | Nome UI | Tipo | Gama / Opções | Default |
|----|---------|------|---------------|---------|
| `rev_gr_on` | REV Gran On | on/off | - | ON |
| `rev_gr_mode` | REV Gran Mode | choice | Off | BeatRepeat | Slice | Reverse | Pitch | Stutter | Off |
| `rev_gr_trigger` | REV Gran Trig | choice | Chance | Envelope | Manual | Chance |
| `rev_gr_manual` | REV Gran Manual | on/off | - | OFF |
| `rev_gr_chance` | REV Gran Chance | float | 0.f, 1.f, 0.01f | 0.2f |
| `rev_gr_env_thr` | REV Gran EnvThr | float | -60.f, 0.f, 0.5f | -18.f |
| `rev_gr_len_note` | REV Gran Len Note | choice-nota | Free | 1/32 | 1/16T | 1/16 | 1/16D | 1/8T | 1/8 | 1/8D | 1/4 | 1/4D | 1/2 | 1/1 | 1/16 |
| `rev_gr_repeats` | REV Gran Repeats | int | 1..16 | 4 |
| `rev_gr_decay` | REV Gran Decay | float | 0.5f, 0.99f, 0.01f | 0.85f |
| `rev_gr_time` | REV Gran Time Ms | float | 60.f, 8000.f, 1.f, 0.4f | 250.f |
| `rev_gr_time_note` | REV Gran Time Note | choice-nota | Free | 1/32 | 1/16T | 1/16 | 1/16D | 1/8T | 1/8 | 1/8D | 1/4 | 1/4D | 1/2 | 1/1 | Free |
| `rev_gr_pitch` | REV Gran Pitch | float | -12.f, 12.f, 0.5f | 12.f |
| `rev_gr_flux` | REV Gran Flux | float | 0.f, 1.f, 0.01f | 0.3f |
| `rev_gr_xfade` | REV Gran Xfade | float | 1.f, 50.f, 0.5f | 8.f |
| `rev_gr_mix` | REV Gran Mix | float | 0.f, 1.f, 0.01f | 0.5f |
| `rev_gr_interrupt` | REV Gran Interrupt | on/off | - | OFF |

### rev-engine (8)

| ID | Nome UI | Tipo | Gama / Opções | Default |
|----|---------|------|---------------|---------|
| `rev_mode` | REV Mode | choice | Off | Loop | Throw | Off |
| `rev_source` | REV Source | choice | Dry | PostFWD | Dry |
| `rev_capture` | REV Capture | choice | 2 beats | 3 beats | 4 beats | 2 beats |
| `rev_rate` | REV Rate | float | 0.25f, 2.f, 0.01f | 1.f |
| `rev_lfo` | REV LFO | float | 0.f, 1.f, 0.01f | 0.f |
| `rev_mix` | REV Mix | float | 0.f, 1.f, 0.01f | 0.4f |
| `rev_throw` | REV Throw | on/off | - | OFF |
| `rev_duck` | REV Duck | float | 0.f, 1.f, 0.01f | 0.3f |

### links (7)

| ID | Nome UI | Tipo | Gama / Opções | Default |
|----|---------|------|---------------|---------|
| `link_master` | Link Master | on/off | - | ON |
| `link_gate` | Link Gate | on/off | - | ON |
| `link_delay` | Link Delay | on/off | - | ON |
| `link_verb` | Link Verb | on/off | - | ON |
| `link_gran` | Link Gran | on/off | - | ON |
| `trim_delay` | Trim Delay | float | 0.25f, 4.f, 0.01f | 1.f |
| `trim_decay` | Trim Decay | float | 0.25f, 2.f, 0.01f | 1.f |

### out (2)

| ID | Nome UI | Tipo | Gama / Opções | Default |
|----|---------|------|---------------|---------|
| `dim_size` | Dim Size | float | 0.f, 1.f, 0.01f | 0.35f |
| `dim_mix` | Dim Mix | float | 0.f, 1.f, 0.01f | 0.f |
