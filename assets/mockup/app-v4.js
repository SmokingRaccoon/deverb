/* deVerb UI v4 -- interacao do mockup (JS puro, sem dependencias).
   Nao e logica de plugin: demonstra a gramatica (pedras, link/morph, agulhas, tether, passos, ripple).
   Estado inicial controlavel por URL: ?sel=verb&unlink=gate,delay&morph=0.8&linkall=0   |   ?qa = verificacoes de texto */
(() => {
  'use strict';
  const errs = []; window.addEventListener('error', e => errs.push('JS ERROR: ' + e.message + ' @' + e.lineno));
  const $ = (s, r = document) => r.querySelector(s);
  const $$ = (s, r = document) => Array.from(r.querySelectorAll(s));
  const clamp = (v, a, b) => Math.min(b, Math.max(a, v));
  const stage = $('#stage');
  const q = new URLSearchParams(location.search);
  const reduce = matchMedia('(prefers-reduced-motion: reduce)').matches || q.has('qa');
  const MODS = ['gate', 'delay', 'verb', 'gran'];
  const LETTER = { G: 'gate', D: 'delay', V: 'verb', Gr: 'gran' };

  // formatadores (espelham generate-v4.py::fmtv)
  const FMT = {
    n2: v => v.toFixed(2), n1: v => v.toFixed(1), int: v => String(Math.round(v)),
    ms: v => Math.round(v) + ' ms', ms1: v => v.toFixed(1) + ' ms',
    hz: v => v >= 1000 ? (v / 1000).toFixed(1) + ' kHz' : Math.round(v) + ' Hz',
    hz1: v => v.toFixed(1) + ' Hz', s: v => v.toFixed(1) + ' s', db: v => v.toFixed(1) + ' dB',
    st: v => (v >= 0 ? '+' : '') + v.toFixed(1) + ' st', x: v => v.toFixed(2) + 'x', pct: v => Math.round(v * 100) + '%'
  };
  const pidOf = el => el.dataset.paramFwd || el.dataset.paramRev || el.dataset.param;
  const isRev = el => !!el.dataset.paramRev;
  const TRIM_OF = { fwd_delay_time: 'delay', fwd_verb_decay: 'verb' };

  const state = { morph: 0.35, linkAll: true, link: { gate: true, delay: true, verb: true, gran: false }, trim: { delay: 1, verb: 1 } };
  const linked = mod => state.linkAll && !!state.link[mod];

  // ---------------------------------------------------------------- dials
  const dials = $$('.dial').map(el => ({
    el, id: pidOf(el), rev: isRev(el), twin: el.dataset.twin, mod: el.dataset.link || el.dataset.mod,
    mn: +el.dataset.min, mx: +el.dataset.max, df: +el.dataset.def, fmt: el.dataset.fmt, val: +el.dataset.val
  }));
  const dById = {}; dials.forEach(d => dById[d.id] = d);
  const revDials = dials.filter(d => d.rev);

  function inherited(d) { // FWD * trim (RevLinker: linked = fwd*trimMult + trimAdd)
    const f = dById[d.twin]; if (!f) return d.val;
    const t = TRIM_OF[d.twin]; const mult = t ? state.trim[t] : 1;
    return clamp(f.val * mult, d.mn, d.mx);
  }
  function effective(d) {
    if (!d.rev || !linked(d.mod)) return d.val;
    const l = inherited(d);
    return clamp(l + (d.val - l) * state.morph, d.mn, d.mx);
  }
  function renderDial(d) {
    const n = v => (v - d.mn) / (d.mx - d.mn), eff = effective(d), s = d.el.style;
    s.setProperty('--v', n(d.val)); s.setProperty('--ve', n(eff));
    if (d.rev) {
      s.setProperty('--vf', n(inherited(d)));
      d.el.classList.toggle('linked', linked(d.mod));
    }
    const kv = $('.kval', d.el); if (kv) kv.textContent = FMT[d.fmt](eff);
    if (d.id === 'morph') { const r = $('.morph-read .kval'); if (r) r.textContent = FMT.pct(d.val); }
  }
  function setDial(d, v) {
    d.val = clamp(d.fmt === 'int' ? Math.round(v) : v, d.mn, d.mx);
    renderDial(d);
    if (d.id === 'morph') { state.morph = d.val; setAmp(); revDials.forEach(renderDial); }
    else if (d.id === 'trim_delay') { state.trim.delay = d.val; revDials.forEach(renderDial); }
    else if (d.id === 'trim_decay') { state.trim.verb = d.val; revDials.forEach(renderDial); }
    else if (!d.rev) { const t = revDials.find(r => r.twin === d.id); if (t) renderDial(t); }
  }
  function setAmp() { stage.style.setProperty('--amp', (0.6 + state.morph * 6).toFixed(2) + 'deg'); }
  dials.forEach(d => {
    const el = d.el; let y0 = 0, n0 = 0, drag = false;
    el.addEventListener('pointerdown', e => { drag = true; y0 = e.clientY; n0 = (d.val - d.mn) / (d.mx - d.mn); try { el.setPointerCapture(e.pointerId); } catch (_) {} e.preventDefault(); });
    el.addEventListener('pointermove', e => { if (!drag) return; const k = e.shiftKey ? 700 : 170; setDial(d, d.mn + clamp(n0 + (y0 - e.clientY) / k, 0, 1) * (d.mx - d.mn)); });
    ['pointerup', 'pointercancel'].forEach(t => el.addEventListener(t, () => { drag = false; }));
    el.addEventListener('dblclick', () => setDial(d, d.df));
    el.addEventListener('wheel', e => { e.preventDefault(); const n = clamp((d.val - d.mn) / (d.mx - d.mn) - Math.sign(e.deltaY) * 0.02, 0, 1); setDial(d, d.mn + n * (d.mx - d.mn)); }, { passive: false });
  });

  // ---------------------------------------------------------------- discretos (seg / stepper)
  const discs = $$('.seg, .stepper').map(el => ({
    el, id: pidOf(el), rev: isRev(el), twin: el.dataset.twin, mod: el.dataset.link || el.dataset.mod,
    opts: el.dataset.options.split('|'), i: +el.dataset.i, seg: el.classList.contains('seg')
  }));
  const cById = {}; discs.forEach(c => cById[c.id] = c);
  const effI = c => (c.rev && c.twin && linked(c.mod)) ? cById[c.twin].i : c.i;
  function renderDisc(c) {
    const i = effI(c);
    if (c.seg) $$('button', c.el).forEach((b, k) => b.classList.toggle('on', k === i));
    else $('.val', c.el).textContent = c.opts[i];
    c.el.classList.toggle('tether', !!(c.rev && c.twin && linked(c.mod)));
  }
  function setDisc(c, i) {
    if (c.rev && c.twin && linked(c.mod)) return toast(c.mod, c.el);
    c.i = (i + c.opts.length) % c.opts.length; renderDisc(c); onDisc(c);
    if (!c.rev) discs.filter(r => r.twin === c.id).forEach(renderDisc);
    syncDerived();
  }
  discs.forEach(c => {
    if (c.seg) $$('button', c.el).forEach((b, k) => b.addEventListener('click', () => setDisc(c, k)));
    else {
      $('.prev', c.el).addEventListener('click', () => setDisc(c, c.i - 1));
      $('.next', c.el).addEventListener('click', () => setDisc(c, c.i + 1));
      c.el.addEventListener('wheel', e => { e.preventDefault(); setDisc(c, c.i + Math.sign(e.deltaY)); }, { passive: false });
    }
  });

  const PATTERNS = [[0, 4, 8, 12], [2, 6, 10, 14], [0, 2, 4, 6, 8, 10, 12, 14], Array.from({ length: 16 }, (_, i) => i),
    [0, 1, 2, 3, 5, 6, 7, 10, 11, 15], [0, 3, 6, 9, 12]];
  const pat = { fwd: new Set([0, 4, 8, 12]), rev: new Set([2, 6, 10, 14]) };

  function onDisc(c) {
    if (c.id === 'chain_order') applyOrder(c.i);
    if (c.id === 'rev_capture') applyCapture(c.i + 2);
    if (/_gate_pattern$/.test(c.id)) { pat[c.rev ? 'rev' : 'fwd'] = new Set(PATTERNS[c.i]); renderSteps(); }
  }
  function syncDerived() {
    // MS so conta com DIV = Free (o knob fica esbatido caso contrario)
    ['fwd', 'rev'].forEach(e => {
      const note = cById[`${e}_delay_note`], dial = dById[`${e}_delay_time`];
      if (note && dial) dial.el.classList.toggle('muted', note.opts[effI(note)] !== 'Free');
    });
    // gate em 8 passos: 9-16 esbatidos
    ['fwd', 'rev'].forEach(e => {
      const st = cById[`${e}_gate_steps`]; if (!st) return;
      const eight = st.opts[effI(st)] === '8';
      $$(`.stp[data-engine="${e}"]`).forEach(s => s.classList.toggle('dis', eight && +s.dataset.step >= 8));
    });
  }

  // ---------------------------------------------------------------- passos
  const stepEls = $$('.stp');
  function renderSteps() {
    stepEls.forEach(s => {
      const rev = s.dataset.engine === 'rev', i = +s.dataset.step, L = rev && linked('gate');
      const on = (rev && !L ? pat.rev : pat.fwd).has(i);
      s.classList.toggle('on', on); s.classList.toggle('tether', L);
    });
  }
  stepEls.forEach(s => s.addEventListener('click', () => {
    const rev = s.dataset.engine === 'rev', i = +s.dataset.step;
    if (rev && linked('gate')) return toast('gate', s);
    const set = pat[rev ? 'rev' : 'fwd']; set.has(i) ? set.delete(i) : set.add(i); renderSteps();
  }));

  // ---------------------------------------------------------------- teclas
  function renderKeys() {
    $$('.key.link').forEach(k => k.classList.toggle('on', !!state.link[k.dataset.param.replace('link_', '')]));
    const all = $('.key.link-all'); if (all) all.classList.toggle('on', state.linkAll);
    $$('.key.pwr.rev').forEach(k => {
      const L = linked(k.dataset.link); k.classList.toggle('tether', L);
      if (L) k.classList.toggle('on', $(`.key.pwr.fwd[data-tile="${k.dataset.link}"]`).classList.contains('on'));
    });
    $$('.key[data-twin]:not(.pwr)').forEach(k => { k.classList.toggle('tether', linked(k.dataset.link)); });
  }
  $$('.key').forEach(k => {
    k.addEventListener('click', () => {
      if (k.classList.contains('link')) { const m = k.dataset.param.replace('link_', ''); state.link[m] = !state.link[m]; return renderAll(); }
      if (k.classList.contains('link-all')) { state.linkAll = !state.linkAll; return renderAll(); }
      if (k.classList.contains('throw')) return fireThrow(k);
      if (k.classList.contains('solid')) return randomize();
      if (k.dataset.twin && linked(k.dataset.link)) return toast(k.dataset.link, k);
      k.classList.toggle('on'); k.setAttribute('aria-pressed', k.classList.contains('on'));
      if (k.dataset.paramFwd === 'gr_manual' || k.dataset.paramRev === 'rev_gr_manual') {
        const led = $(`.viz.led[data-engine="${k.dataset.engine}"]`);
        if (led) { led.classList.toggle('hot', k.classList.contains('on')); $('span', led).textContent = k.classList.contains('on') ? 'GRAB' : 'IDLE'; }
      }
      renderKeys();
    });
  });

  // ---------------------------------------------------------------- pedras + ordem da cadeia
  const tiles = $$('.tile');
  const slotX = tiles.map(t => parseFloat(t.style.left));
  function select(mod) {
    if (stage.dataset.sel === mod) return;
    stage.dataset.sel = mod;
    tiles.forEach(t => t.classList.toggle('sel', t.dataset.select === mod));
    if (reduce) return;
    ['fwd', 'rev'].forEach(e => $$(`.w[data-mod="${mod}"][data-engine="${e}"]`).forEach(w => {
      w.classList.remove('enter-fwd', 'enter-rev'); void w.offsetWidth; w.classList.add('enter-' + e);
      setTimeout(() => w.classList.remove('enter-' + e), 260);
    }));
  }
  tiles.forEach(t => t.addEventListener('click', () => select(t.dataset.select)));
  function applyOrder(i) {
    const order = $('.stepper[data-param="chain_order"]').dataset.options.split('|')[i].split('-').map(l => LETTER[l]);
    order.forEach((mod, k) => {
      $$(`[data-tile="${mod}"], .tile[data-select="${mod}"]`).forEach(el => {
        const dx = el.classList.contains('tile') ? 0 : 8;
        if (!reduce) el.classList.add('moving');
        el.style.left = (slotX[k] + dx) + 'px';
      });
    });
  }

  // ---------------------------------------------------------------- janela de captura (REV)
  function applyCapture(beats) {
    const v = $('.viz.capture'); if (!v) return;
    v.style.setProperty('--capw', beats * 72 + 'px');
    const win = $('.win', v); win.setAttribute('x', 288 - beats * 72); win.setAttribute('width', beats * 72);
    $('.readhead', v).style.animationDuration = (beats * 0.5) + 's';
  }

  // ---------------------------------------------------------------- THROW: pedra atirada -> aneis na linha de agua
  function fireThrow(k) {
    k.classList.add('fire'); setTimeout(() => k.classList.remove('fire'), 160);
    if (reduce) return;
    const r = k.getBoundingClientRect(), s = stage.getBoundingClientRect(), host = $('.ripples');
    for (let n = 0; n < 3; n++) {
      const d = document.createElement('i'); d.className = 'ripple';
      d.style.left = (r.left - s.left + r.width / 2) + 'px'; d.style.animationDelay = (n * 0.18) + 's';
      host.appendChild(d); setTimeout(() => d.remove(), 2200);
    }
  }

  // ---------------------------------------------------------------- toast: o link explica-se
  const toastEl = $('.toast'); let toastT = 0;
  function toast(mod, anchor) {
    const name = mod.toUpperCase();
    toastEl.innerHTML = `<span>LINK ${name} ON - REV FOLLOWS FWD</span><button>UNLINK ${name}</button>`;
    toastEl.hidden = false;
    const r = anchor.getBoundingClientRect(), s = stage.getBoundingClientRect();
    const top = (r.top - s.top) > 70 ? r.top - s.top - 44 : r.bottom - s.top + 8;
    toastEl.style.left = clamp(r.left - s.left, 8, 1280 - 330) + 'px'; toastEl.style.top = top + 'px';
    $('button', toastEl).onclick = () => { state.link[mod] = false; toastEl.hidden = true; renderAll(); };
    clearTimeout(toastT); toastT = setTimeout(() => { toastEl.hidden = true; }, 3800);
  }

  // ---------------------------------------------------------------- RANDOM (demo): dials aleatorios
  function randomize() {
    dials.forEach(d => {
      if (/^(input_gain|master|fwd_mix|rev_mix|morph|tempo_bpm)$/.test(d.id) || d.id.startsWith('trim')) return;
      setDial(d, d.mn + Math.random() * (d.mx - d.mn) * (d.fmt === 'int' ? 0.5 : 1));
    });
  }

  // ---------------------------------------------------------------- BPM numbox + playhead
  const nb = $('.numbox'); if (nb) {
    let y0 = 0, v0 = 120, drag = false; const val = $('.val', nb);
    nb.addEventListener('pointerdown', e => { drag = true; y0 = e.clientY; v0 = +val.textContent; try { nb.setPointerCapture(e.pointerId); } catch (_) {} });
    nb.addEventListener('pointermove', e => { if (drag) val.textContent = clamp(v0 + (y0 - e.clientY) * 0.5, 40, 240).toFixed(1); });
    ['pointerup', 'pointercancel'].forEach(t => nb.addEventListener(t, () => { drag = false; }));
  }
  let ph = 0;
  if (!reduce) setInterval(() => {
    if (document.hidden) return;
    ph = (ph + 1) % 16;
    stepEls.forEach(s => { const i = +s.dataset.step, idx = s.dataset.engine === 'fwd' ? ph : 15 - ph; s.classList.toggle('play', i === idx); });
    $$('.viz.gate-big').forEach(v => { const idx = v.dataset.engine === 'fwd' ? ph : 15 - ph; $('.kval', v).textContent = String(idx + 1).padStart(2, '0'); });
  }, 125);

  // ---------------------------------------------------------------- render global
  function renderAll() {
    dials.forEach(renderDial); discs.forEach(renderDisc); renderSteps(); renderKeys(); syncDerived(); setAmp();
  }

  // ---------------------------------------------------------------- estado inicial (DOM + URL)
  state.morph = dById.morph.val;
  $$('.key.link').forEach(k => { state.link[k.dataset.param.replace('link_', '')] = k.classList.contains('on'); });
  state.linkAll = $('.key.link-all').classList.contains('on');
  if (q.has('morph')) { dById.morph.val = clamp(+q.get('morph'), 0, 1); state.morph = dById.morph.val; }
  if (q.has('linkall')) state.linkAll = q.get('linkall') !== '0';
  if (q.has('unlink')) q.get('unlink').split(',').forEach(m => { state.link[m] = false; });
  if (q.has('link')) q.get('link').split(',').forEach(m => { state.link[m] = true; });
  pat.fwd = new Set(PATTERNS[cById.fwd_gate_pattern.i]);
  renderAll();
  if (MODS.includes(q.get('sel'))) { stage.dataset.sel = q.get('sel'); tiles.forEach(t => t.classList.toggle('sel', t.dataset.select === q.get('sel'))); }
  if (q.has('order')) { const c = cById.chain_order; c.i = +q.get('order'); renderDisc(c); applyOrder(c.i); }


  // ---------------------------------------------------------------- testes de fumo (?qa): interacoes por script
  function smoke() {
    const res = []; const ok = (c, m) => res.push((c ? 'ok   ' : 'FAIL ') + m);
    const click = el => el.dispatchEvent(new MouseEvent('click', { bubbles: true }));
    const css = (el, v) => parseFloat(el.style.getPropertyValue(v));
    // 1 selecao de pedra
    click($('.tile[data-select="verb"]')); ok(stage.dataset.sel === 'verb' && $('.tile.sel').dataset.select === 'verb', 'pedra VERB seleciona o modulo nas 2 bandas');
    ok($$('.w[data-mod="verb"]').some(w => w.offsetParent) && !$$('.w[data-mod="gate"]').some(w => w.offsetParent), 'so o modulo selecionado existe (verb sim, gate nao)');
    click($('.tile[data-select="gran"]'));
    // 2 link GRAN: REV passa a seguir FWD
    const rGr = dById.rev_gr_chance, fGr = dById.gr_chance, modeR = cById.rev_gr_mode;
    ok(!rGr.el.classList.contains('linked') && !modeR.el.classList.contains('tether'), 'GRAN comeca desligado (REV proprio, sem tether)');
    click($('.key.link[data-param="link_gran"]'));
    ok(rGr.el.classList.contains('linked') && modeR.el.classList.contains('tether'), 'ligar LINK GRAN -> dials REV com agulha-fantasma e discretos amarrados');
    ok(modeR.opts[effI(modeR)] === cById.gr_mode.opts[cById.gr_mode.i], 'discreto REV linkado espelha o valor FWD (mode)');
    // 3 MORPH: 0 -> espelha FWD; 1 -> valor proprio
    setDial(dById.morph, 0); ok(Math.abs(css(rGr.el, '--ve') - (fGr.val - fGr.mn) / (fGr.mx - fGr.mn)) < 1e-6, 'MORPH 0 -> efetivo = FWD');
    setDial(dById.morph, 1); ok(Math.abs(css(rGr.el, '--ve') - (rGr.val - rGr.mn) / (rGr.mx - rGr.mn)) < 1e-6, 'MORPH 1 -> efetivo = proprio REV');
    setDial(dById.morph, 0.5); const mid = (fGr.val + rGr.val) / 2; ok(Math.abs(effective(rGr) - mid) < 1e-6, 'MORPH 0.5 -> ponto medio (RevLinker)');
    // 4 tether: clicar discreto amarrado abre o aviso com UNLINK
    click($$('button', modeR.el)[3]); ok(!toastEl.hidden && /UNLINK GRAN/.test(toastEl.textContent), 'clicar controlo REV amarrado explica o link (toast UNLINK)');
    click($('button', toastEl)); ok(!state.link.gran && !modeR.el.classList.contains('tether') && toastEl.hidden, 'UNLINK GRAN liberta o REV');
    // 5 trim delay (so afeta MS do delay)
    click($('.key.link[data-param="link_delay"]')); click($('.key.link[data-param="link_delay"]'));
    const eBefore = effective(dById.rev_delay_time); setDial(dById.trim_delay, 2);
    ok(Math.abs(effective(dById.rev_delay_time) - eBefore) > 1, 'T-DLY altera o tempo REV do delay');
    setDial(dById.trim_delay, 1);
    // 6 MS esbatido com DIV != Free
    ok(dById.fwd_delay_time.el.classList.contains('muted'), 'MS esbatido enquanto DIV != Free');
    setDisc(cById.fwd_delay_note, 0); ok(!dById.fwd_delay_time.el.classList.contains('muted'), 'MS ativo com DIV = Free');
    // 7 passos: FWD edita, REV linkado espelha
    const f0 = $('.stp[data-engine="fwd"][data-step="1"]'), r0 = $('.stp[data-engine="rev"][data-step="1"]');
    click(f0); ok(f0.classList.contains('on') && r0.classList.contains('on') && r0.classList.contains('tether'), 'passo FWD acende e o REV linkado espelha');
    click(f0);
    // 8 ordem da cadeia reordena pedras
    const gx0 = parseFloat($('.tile[data-select="gate"]').style.left); click($('.next', $('.stepper[data-param="chain_order"]')));
    ok(parseFloat($('.tile[data-select="gate"]').style.left) === gx0 && parseFloat($('.tile[data-select="verb"]').style.left) === slotX[1], 'ordem G-V-D-Gr: VERB passa ao 2o lugar');
    click($('.prev', $('.stepper[data-param="chain_order"]')));
    // 9 ALL desliga todos os links
    click($('.key.link-all')); ok(!state.linkAll && !$$('.dial.linked').length, 'ALL desligado liberta todos os modulos');
    click($('.key.link-all'));
    // 10 reset por duplo clique
    const d = dById.fwd_gate_smooth; setDial(d, .9); d.el.dispatchEvent(new MouseEvent('dblclick', { bubbles: true })); ok(d.val === d.df, 'duplo clique repoe o default');
    return res;
  }

  // ---------------------------------------------------------------- QA: overflow de texto por estado de modulo
  function qa() {
    const out = [], keep = stage.dataset.sel;
    for (const m of MODS) {
      stage.dataset.sel = m;
      $$('.w').forEach(w => {
        if (w.offsetParent === null) return;
        const name = w.dataset.label || w.dataset.visual || w.className, wr = w.getBoundingClientRect();
        const walk = document.createTreeWalker(w, NodeFilter.SHOW_TEXT); let n;
        while ((n = walk.nextNode())) {
          const txt = n.textContent.trim(); if (!txt || n.parentElement instanceof SVGElement) continue;
          const rg = document.createRange(); rg.selectNodeContents(n); const tr = rg.getBoundingClientRect();
          const hb = n.parentElement.getBoundingClientRect();
          if (tr.width > hb.width + 1.5) out.push(`[TEXTO/${m}] "${txt}" em ${name}: texto ${tr.width.toFixed(0)}px > caixa ${hb.width.toFixed(0)}px`);
          else if (tr.left < wr.left - 1.5 || tr.right > wr.right + 1.5) out.push(`[FORA/${m}] "${txt}" sai de ${name} (${wr.width.toFixed(0)}px)`);
        }
      });
    }
    stage.dataset.sel = keep;
    // largura natural (max-content) de segmentados/steppers vs. largura atribuida
    for (const m of MODS) {
      stage.dataset.sel = m;
      $$('.seg,.stepper').forEach(el => {
        if (el.offsetParent === null) return;
        const had = el.style.width; el.style.width = 'max-content';
        const need = el.getBoundingClientRect().width; el.style.width = had;
        const have = parseFloat(had);
        if (need > have + 0.5) out.push(`[NEED/${m}] ${el.dataset.label}: natural ${need.toFixed(1)}px > atribuido ${have}px`);
      });
    }
    stage.dataset.sel = keep;
    const sm = smoke(); renderAll(); stage.dataset.sel = keep;
    const pre = $('#qa'); pre.hidden = false;
    pre.textContent = (out.length ? out.join('\n') : 'QA OK: sem overflow de texto nos 4 estados') + '\n--- interacoes ---\n' + sm.join('\n') + '\n' + errs.join('\n');
  }
  if (q.has('qa')) (document.fonts && document.fonts.ready ? document.fonts.ready : Promise.resolve()).then(() => setTimeout(qa, 50));
})();
