/* AI-GIT · Neural Mission Control */
const $ = s => document.querySelector(s), $$ = s => [...document.querySelectorAll(s)];
const sleep = ms => new Promise(r => setTimeout(r, ms));
const el = (t, c, h) => { const e = document.createElement(t); e.className = c || ''; if (h) e.innerHTML = h; return e; };
let seed = 42; // deterministic fake hashes
const rnd = () => (seed = (seed * 1664525 + 1013904223) >>> 0) / 4294967296;
const hex = n => { let s = ''; while (s.length < n) s += Math.floor(rnd() * 4294967296).toString(16).padStart(8, '0'); return s.slice(0, n); };
const mbf = v => v >= 1000 ? (v / 1000).toFixed(2) + ' GB' : v < 1 ? Math.round(v * 1024) + ' KB' : v.toFixed(v < 10 ? 1 : 0) + ' MB';
const cas = h => `s3://ai-git-cas/${h.slice(0, 2)}/${h.slice(2, 4)}/${h}`;

<button onclick="window.location.href='overview.html'">
  View Profile
</button>

/* ---------- State ---------- */
const S = { raw: 4200, stored: 819, local: 0, remote: 0, chunks: [], run: 0 }; // baseline ≈ 80.5% saved
const FILES = [
  { n: 'model-00001-of-00002.safetensors', s: 'shard 1/2', d: 'model', mb: 96, w: 1 },
  { n: 'model-00002-of-00002.safetensors', s: 'shard 2/2', d: 'model', mb: 80, w: 1 },
  { n: 'config.json', s: 'config', d: 'model', mb: 0.002 },
  { n: 'tokenizer.json', s: 'tokenizer', d: 'tokenizer', mb: 2.1 },
  { n: 'vocab.txt', s: 'vocab', d: 'tokenizer', mb: 0.9 },
];
FILES.forEach(f => f.sha = hex(64));

/* ---------- Inspector ---------- */
function inspect(o) {
  $('#iT').textContent = o.t; $('#iSha').textContent = o.sha;
  $('#iRng').textContent = o.rng || '—'; $('#iLnk').textContent = o.lnk || '—';
  const s = $('#iSt'); s.textContent = o.st || '—'; s.className = 'pill ' + (o.cls || '');
}
const chunkInfo = c => ({
  t: 'Chunk #' + c.i, sha: c.h,
  rng: '0x' + Math.round(c.a * 1048576).toString(16) + ' → 0x' + Math.round(c.b * 1048576).toString(16) + ' (' + mbf(c.s) + ')',
  lnk: cas(c.h), st: c.dup ? 'CAS hit · deduplicated' : 'New · uploaded to MinIO', cls: c.dup ? 'hit' : 'new'
});

/* ---------- Deduplication gauge (eased odometer) ---------- */
const CIRC = 163.36; let tgt = 0, shown = 0, rawS = 0, stS = 0;
function gauge() {
  tgt = (1 - S.stored / S.raw) * 100;
  $('#gArc').style.strokeDashoffset = CIRC * (1 - tgt / 100);
}
(function loop() {
  shown += (tgt - shown) * .1; rawS += (S.raw - rawS) * .1; stS += (S.stored - stS) * .1;
  $('#gPct').textContent = shown.toFixed(1);
  $('#gRaw').textContent = (rawS / 1000).toFixed(3); $('#gStored').textContent = (stS / 1000).toFixed(3);
  requestAnimationFrame(loop);
})();

/* ---------- Buckets ---------- */
function bucket(k) {
  const cap = k === 'local' ? 260 : 120;
  $('#l' + k).style.transform = `scaleY(${Math.min(1, S[k] / cap)})`;
  $('#t' + k).textContent = mbf(S[k]);
}
function drop(p, b, done) {
  const pr = p.getBoundingClientRect(), br = b.getBoundingClientRect();
  const dx = br.left + br.width / 2 - (pr.left + pr.width / 2), dy = br.top + 24 - pr.top;
  p.animate([
    { transform: 'none', opacity: 1, offset: 0 },
    { transform: 'translateY(34px)', opacity: 1, offset: .2 },
    { transform: `translate(${dx}px,${dy}px) scale(.15)`, opacity: .3 }
  ], { duration: 900, easing: 'ease-in' }).onfinish = () => { p.remove(); done(); };
}

/* ---------- FastCDC Laser Slicer ---------- */
function slice(f) {
  if (!f.w) return;
  const id = ++S.run, tr = $('#track'), rest = $('#rest'), las = $('#laser'), W = tr.clientWidth, mb = f.mb, AVG = 2, SPEED = 24;
  $('#fname').textContent = f.n + ' · ' + mbf(mb);
  $$('.tree .f').forEach(li => li.classList.toggle('on', li._f === f));
  tr.querySelectorAll('.piece').forEach(p => p.remove());
  rest.style.clipPath = 'inset(0)'; las.style.opacity = 1;
  $('#matrix').innerHTML = ''; $('#rows').innerHTML = ''; $('#mCt').textContent = 0; S.chunks = [];

  const cut = (a, b) => {
    const s = b - a, dup = Math.random() < .62, c = { i: S.chunks.length, a, b, s, h: hex(64), dup };
    S.chunks.push(c); S.raw += s; if (!dup) S.stored += s; gauge();
    // 1. slice the DOM bar into two unequal pieces
    const x = a / mb * W, w = s / mb * W;
    const p = el('div', 'piece', `<span>${s.toFixed(1)} MB</span>`);
    p.style.cssText = `left:${x}px;width:${w}px;background-size:${W}px 100%;background-position:-${x}px 0`;
    tr.appendChild(p);
    rest.style.clipPath = `inset(0 0 0 ${b / mb * W}px)`;
    las.classList.remove('flash'); void las.offsetWidth; las.classList.add('flash');
    // 2. drop into buckets (dedup hits only fill the local cache)
    if (!dup) { const q = p.cloneNode(true); tr.appendChild(q); drop(q, $('#bremote'), () => { S.remote += s; bucket('remote'); }); }
    drop(p, $('#blocal'), () => { S.local += s; bucket('local'); });
    // 3. matrix tile + chunk row
    const t = el('div', 'tile' + (dup ? ' dup' : ''), '<i></i>');
    t.onmouseenter = () => inspect(chunkInfo(c));
    $('#matrix').appendChild(t); $('#mCt').textContent = S.chunks.length;
    const r = el('tr', '', `<td>${c.i}</td><td>${s.toFixed(2)} MB</td><td>${c.h.slice(0, 14)}…</td><td class="${dup ? 'hit' : 'new'}">${dup ? 'HIT' : 'NEW'}</td>`);
    r.onmouseenter = () => inspect(chunkInfo(c));
    $('#rows').prepend(r);
  };

  let pos = 0, last = 0, tg = 1 + Math.random() * 3, prev = performance.now();
  (function tick(t) {
    if (id !== S.run) return;
    pos = Math.min(mb, pos + (t - prev) / 1000 * SPEED); prev = t;
    if (pos - last >= tg) pos = last + tg;
    las.style.transform = `translateX(${pos / mb * W}px)`;
    const win = pos - last;
    $('#gear').textContent = `gear 0x${(Math.random() * 4294967295 >>> 0).toString(16).padStart(8, '0')} · ${win < AVG ? 'MASK_S (strict)' : 'MASK_L (loose)'} · window ${win.toFixed(2)} MB`;
    if (win >= tg || pos >= mb) { cut(last, pos); last = pos; tg = 1 + Math.random() * 3; }
    if (pos < mb) requestAnimationFrame(tick);
    else { las.style.opacity = 0; $('#gear').textContent = `done · ${S.chunks.length} chunks · ${S.chunks.filter(c => c.dup).length} deduplicated`; }
  })(performance.now());
}

/* ---------- Left rail tree + Merkle forest ---------- */
function buildTree() {
  const ul = $('#tree');
  ['model', 'tokenizer'].forEach(d => {
    const li = el('li', '', '📁 ' + d + '/'), sub = el('ul');
    FILES.filter(f => f.d === d).forEach(f => {
      const r = el('li', 'f' + (f.w ? ' w' : ''), `${f.s}<em>${mbf(f.mb)}</em>`);
      r._f = f; r.onmouseenter = () => inspect(fileInfo(f));
      r.onclick = () => f.w ? slice(f) : inspect(fileInfo(f));
      sub.appendChild(r);
    });
    li.appendChild(sub); ul.appendChild(li);
  });
}
const fileInfo = f => ({ t: 'Blob ' + f.n, sha: f.sha, rng: mbf(f.mb), lnk: cas(f.sha), st: f.w ? 'Click to run slicer' : 'Stored', cls: f.w ? 'new' : 'hit' });

function merkle() {
  const rootSha = hex(64), dirs = { model: { x: 170, sha: hex(64) }, tokenizer: { x: 480, sha: hex(64) } };
  const FX = [70, 170, 270, 420, 540];
  let edges = '', nodes = '';
  const nd = (x, y, r, label, cls, i) => `<g class="nd ${cls}" data-i="${i}" transform="translate(${x} ${y})"><circle r="${r}"/><text y="${r + 14}">${label}</text></g>`;
  const info = [{ t: 'Commit a4f9c…', sha: rootSha, rng: 'root tree', lnk: cas(rootSha), st: 'Merkle root', cls: 'hit' }];
  nodes += nd(300, 34, 16, 'commit a4f9c', 'root', 0);
  Object.entries(dirs).forEach(([d, o]) => {
    edges += `<path class="edge" d="M300 34C300 72 ${o.x} 72 ${o.x} 110"/>`;
    info.push({ t: 'Tree ' + d + '/', sha: o.sha, rng: 'folder', lnk: cas(o.sha), st: 'Merkle tree node', cls: 'hit' });
    nodes += nd(o.x, 110, 11, d + '/', '', info.length - 1);
  });
  FILES.forEach((f, k) => {
    const dx = dirs[f.d].x;
    edges += `<path class="edge" d="M${dx} 110C${dx} 150 ${FX[k]} 150 ${FX[k]} 190"/>`;
    info.push(fileInfo(f));
    nodes += nd(FX[k], 190, f.w ? 13 : 8, f.s, f.w ? 'w' : '', info.length - 1);
  });
  $('#forest').innerHTML = edges + nodes;
  $$('#forest .nd').forEach(g => {
    const i = +g.dataset.i, f = FILES[i - 3];
    g.onmouseenter = () => inspect(info[i]);
    g.onclick = () => f && f.w && slice(f);
  });
}

/* ---------- Model diff ---------- */
[['embed_tokens', 3], ['attn q/k/v · L0-15', 18], ['attn q/k/v · L16-31', 47], ['mlp up/down', 88], ['lm_head', 12]]
  .forEach(([n, p]) => $('#diff').appendChild(el('div', 'dr', `<span>${n}</span><div class="db"><i style="width:${p}%"></i></div><em>${p}% changed</em>`)));

/* ---------- Tabs + dock toggle ---------- */
$$('.tabs button').forEach(b => b.onclick = () => {
  $$('.tabs button').forEach(x => x.classList.toggle('on', x === b));
  $$('.pane').forEach(p => p.classList.toggle('on', p.id === 'p-' + b.dataset.tab));
});
$('#dockT').onclick = () => { const c = $('#dock').classList.toggle('closed'); $('#dockT').textContent = c ? '▴' : '▾'; };

/* ---------- Push pipeline ---------- */
let pushing = false, minio = 0;
const log = m => $('#log').textContent = m;
const flash = n => { n.classList.remove('flash'); void n.offsetWidth; n.classList.add('flash'); };
function fire(c) {
  const t1 = $('#t1'), t2 = $('#t2'), w1 = t1.clientWidth, w2 = t2.clientWidth, ph = el('i', 'ph');
  t1.appendChild(ph);
  ph.animate([{ transform: 'translateX(0)' }, { transform: `translateX(${w1}px)` }], { duration: 700, fill: 'forwards' }).onfinish = () => {
    flash($('#n2'));
    if (c.dup) { // cache hit: bounce off emerald shield
      ph.classList.add('hit');
      const sh = $('#shield'); sh.classList.remove('on'); void sh.offsetWidth; sh.classList.add('on');
      ph.animate([{ transform: `translateX(${w1}px)`, opacity: 1 }, { transform: `translateX(${w1 - 60}px) translateY(-16px)`, opacity: 0 }], { duration: 450 }).onfinish = () => ph.remove();
      log(`CACHE HIT ${c.h.slice(0, 10)}… skipped`);
    } else { // missing chunk: lands in MinIO
      ph.remove();
      const p2 = el('i', 'ph'); t2.appendChild(p2);
      p2.animate([{ transform: 'translateX(0)' }, { transform: `translateX(${w2}px)` }], { duration: 600 }).onfinish = () => {
        p2.remove(); minio++; $('#minioCt').textContent = minio + ' new chunks'; flash($('#n3')); log(`STORED ${c.h.slice(0, 10)}…`);
      };
    }
  };
}
$('#push').onclick = async () => {
  if (pushing) return; pushing = true;
  const list = (S.chunks.length ? S.chunks : Array.from({ length: 12 }, (_, i) => ({ i, h: hex(64), dup: Math.random() < .5 }))).slice(-14);
  if ($('#dock').classList.contains('closed')) $('#dockT').click();
  for (const c of list) { fire(c); await sleep(240); }
  await sleep(1400); log('push complete'); pushing = false;
};

/* ---------- Boot ---------- */
buildTree(); merkle(); gauge();
setTimeout(() => slice(FILES[0]), 500);