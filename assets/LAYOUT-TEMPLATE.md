# Template de layout — como propor mudanças de posicionamento

> Preenche este ficheiro (ou copia a estrutura para um novo) quando quiseres
> mudar o layout. Quanto mais exato, menos perguntas faço e mais à primeira
> fica pronto. Não precisas de perceber código — só de medir caixas.

## Regras de ouro (o resto trato eu)

1. **Janela fixa: 1280×624.** Tudo tem de caber aqui dentro. Origem (0,0)
   no canto superior esquerdo; X cresce para a direita, Y para baixo.
2. **Não mudes IDs nem nomes de parâmetros** (estão congelados no
   `funcoes.md`). Podes mover, agrupar, esconder atrás de tabs e redimensionar
   à vontade.
3. **Tamanhos mínimos de usabilidade**: knobs ≥ 48px de diâmetro, botões com
   altura ≥ 20px, texto ≥ 10px, combos com altura ≥ 22px.
4. **Novos widgets têm de mapear para parâmetros existentes** (ou dizer
   claramente "isto é só visual", ex. LEDs, régua, placeholders).

## Formato: uma tabela por zona

Copia e preenche. `x,y,w,h` em pixels a partir do canto da janela.

### Zona: TOPBAR (0, 0, 1280×100) — exemplo preenchido

| # | Peça | Param ID (`funcoes.md`) | x | y | w | h | Notas |
|---|------|-------------------------|---|---|---|---|-------|
| T1 | título "deVerb" | — (visual) | 12 | 0 | 200 | 36 | |
| T2 | preset global | — (combo especial) | 560 | 7 | 240 | 22 | |
| T3 | botão RANDOM | — (ação) | 806 | 7 | 68 | 22 | |
| T4 | segmentado FWD/REV | — (modo edição) | 880 | 7 | 182 | 22 | 2 botões 90+90 |
| T5 | LED GRAB | — (visual) | 1148 | 7 | 120 | 22 | |

### Zona: NOME_DA_ZONA (x, y, w×h)

| # | Peça | Param ID (`funcoes.md`) | x | y | w | h | Notas |
|---|------|-------------------------|---|---|---|---|-------|
| 1 | | | | | | | |
| 2 | | | | | | | |

*(duplica esta tabela por zona: MOTOR, GATE, DELAY, VERB, GRAN, ou zonas novas)*

## O que enviar junto (por ordem de utilidade)

1. **Este ficheiro preenchido** — é o que eu implemento linha a linha. ✅ essencial
2. **Mockup PNG 1280×624** (podes montar por cima de
   `reference-ui-1280x624.png` ou dos `placeholders/`) — para validar o
   aspeto, não para medir. ✅ muito útil
3. **Assets finais em PNG com fundo transparente**, com o nome do placeholder
   que substituem (ex. desenhaste o knob final? chama-lhe
   `knob-96x110-d74.png` e diz-me) — ligo ao código via `BinaryData`.
4. **Texto livre** com a intenção ("quero o MORPH gigante ao centro porque
   é o gesto principal") — ajuda-me a decidir os detalhes que a tabela
   não cobre.

## O que NÃO funciona bem

- Só descrição verbal sem medidas ("põe o delay mais para a esquerda") —
  vou ter de adivinhar pixels e errar.
- Screenshots de outros plugins sem tabela ("tipo este") — serve como
  inspiração, mas preciso sempre da tabela para implementar.
- Mudar nomes/IDs de parâmetros no mockup — os IDs estão congelados;
  se vires um nome novo, pergunto antes de fazer.
