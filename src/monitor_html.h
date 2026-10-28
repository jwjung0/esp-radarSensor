#pragma once
// Embedded SoftAP monitor page (offline canvas chart — no CDN).
static const char MONITOR_HTML[] PROGMEM = R"MONITORHTML(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>RadarSensor Monitor</title>
  <style>
    :root {
      --bg: #0e1116;
      --panel: #171b22;
      --text: #e8ecf1;
      --muted: #8b95a5;
      --accent: #3db8ff;
      --mov: #ff8a3d;
      --occ: #4cd4a0;
      --border: #2a3140;
    }
    * { box-sizing: border-box; }
    body {
      margin: 0;
      font-family: "Segoe UI", system-ui, sans-serif;
      background: radial-gradient(1200px 600px at 10% -10%, #1a2433 0%, var(--bg) 55%);
      color: var(--text);
      min-height: 100vh;
    }
    main {
      max-width: 42rem;
      margin: 0 auto;
      padding: 1.25rem 1rem 2.5rem;
    }
    h1 {
      font-size: 1.6rem;
      letter-spacing: .02em;
      margin: 0 0 .25rem;
    }
    .sub { color: var(--muted); margin: 0 0 1.25rem; font-size: .95rem; }
    .readout {
      display: flex;
      gap: 1rem;
      align-items: baseline;
      background: var(--panel);
      border: 1px solid var(--border);
      border-radius: .6rem;
      padding: .9rem 1rem;
      margin-bottom: 1rem;
    }
    .mode {
      font-weight: 700;
      font-size: 1.1rem;
      min-width: 3.5rem;
    }
    .mode.mov { color: var(--mov); }
    .mode.occ { color: var(--occ); }
    .mode.none { color: var(--muted); }
    .dis { font-size: 1.75rem; font-variant-numeric: tabular-nums; }
    .unit { color: var(--muted); margin-left: .25rem; }
    canvas {
      width: 100%;
      height: 220px;
      background: var(--panel);
      border: 1px solid var(--border);
      border-radius: .6rem;
      display: block;
    }
    .legend {
      display: flex;
      gap: 1rem;
      color: var(--muted);
      font-size: .85rem;
      margin: .5rem 0 1.25rem;
    }
    .legend span::before {
      content: "";
      display: inline-block;
      width: .7rem;
      height: .7rem;
      border-radius: 50%;
      margin-right: .35rem;
      vertical-align: -1px;
    }
    .legend .mov::before { background: var(--mov); }
    .legend .occ::before { background: var(--occ); }
    section {
      background: var(--panel);
      border: 1px solid var(--border);
      border-radius: .6rem;
      padding: 1rem;
      margin-bottom: 1rem;
    }
    section h2 { font-size: 1.05rem; margin: 0 0 .75rem; }
    .sliders { display: flex; flex-direction: column; gap: .9rem; }
    .slider-row label {
      display: flex;
      justify-content: space-between;
      align-items: baseline;
      font-size: .8rem;
      color: var(--muted);
      margin-bottom: .35rem;
    }
    .slider-row .val {
      color: var(--text);
      font-variant-numeric: tabular-nums;
      font-weight: 600;
      font-size: 1.0rem;
    }
    .slider-row .band { font-weight: 400; color: var(--muted); font-size: .75rem; margin-left: .35rem; }
    input[type=range] {
      width: 100%;
      margin: 0;
      padding: 0;
      accent-color: var(--accent);
      height: 1.1rem;
      display: block;
    }
    .slider-ends {
      display: flex;
      justify-content: space-between;
      font-size: 0.8rem;
      color: rgba(255, 255, 255, 0.6);
      margin-top: 0;
      line-height: 1;
      font-variant-numeric: tabular-nums;
    }
    .switch-row {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: .9rem;
      gap: 1rem;
    }
    .switch-row .label {
      font-size: .9rem;
      color: var(--text);
    }
    .switch-row .label .band {
      display: block;
      color: var(--muted);
      font-size: .75rem;
      margin-top: .15rem;
    }
    .switch {
      position: relative;
      width: 4.2rem;
      height: 1.85rem;
      border: none;
      border-radius: 999px;
      background: #2a3342;
      cursor: pointer;
      padding: 0;
      flex: 0 0 auto;
      min-width: 0;
      transition: background .18s ease;
    }
    .switch.on { background: var(--accent); }
    .switch .knob {
      position: absolute;
      top: .2rem;
      left: .2rem;
      width: 1.45rem;
      height: 1.45rem;
      border-radius: 50%;
      background: #fff;
      transition: transform .18s ease;
      box-shadow: 0 1px 3px rgba(0,0,0,.35);
    }
    .switch.on .knob { transform: translateX(2.35rem); }
    .switch .txt {
      position: absolute;
      top: 50%;
      transform: translateY(-50%);
      font-size: .65rem;
      font-weight: 700;
      letter-spacing: .04em;
      color: #041018;
      pointer-events: none;
    }
    .switch .txt.on-l { left: .45rem; opacity: 0; }
    .switch .txt.off-l { right: .4rem; color: var(--muted); }
    .switch.on .txt.on-l { opacity: 1; }
    .switch.on .txt.off-l { opacity: 0; }
    .actions {
      display: flex;
      flex-wrap: wrap;
      gap: .5rem;
      margin-top: 1rem;
    }
    button {
      flex: 1 1 auto;
      min-width: 7rem;
      padding: .65rem .8rem;
      border: none;
      border-radius: .4rem;
      background: var(--accent);
      color: #041018;
      font-weight: 650;
      font-size: .95rem;
      cursor: pointer;
    }
    button.secondary { background: #2a3342; color: var(--text); }
    .status {
      margin-top: .85rem;
      font-size: .85rem;
      color: var(--muted);
      min-height: 1.2em;
    }
    .hint { margin-top: .75rem; font-size: .8rem; color: var(--muted); line-height: 1.4; }
  </style>
</head>
<body>
  <main>
    <h1>RadarSensor</h1>
    <p class="sub">LD1125H realtime distance · SoftAP monitor</p>

    <div class="readout">
      <div id="mode" class="mode none">NONE</div>
      <div><span id="dis" class="dis">—</span><span class="unit">m</span></div>
    </div>

    <canvas id="chart" width="800" height="220"></canvas>
    <div class="legend">
      <span class="mov">MOV</span>
      <span class="occ">OCC</span>
      <span>~60 s window</span>
    </div>

    <section>
      <h2>Filter</h2>
      <div class="switch-row">
        <div class="label">Distance filter
          <span class="band">1D Kalman on raw</span>
        </div>
        <button type="button" id="filtSwitch" class="switch on" aria-pressed="true" title="Toggle filter">
          <span class="txt on-l">ON</span>
          <span class="txt off-l">OFF</span>
          <span class="knob"></span>
        </button>
      </div>
      <div class="sliders">
        <div class="slider-row">
          <label for="filt_q"><span>Track (Q) <span class="band">higher = faster · def 0.05</span></span><span class="val" id="filt_q_val">—</span></label>
          <input id="filt_q" type="range" min="0.01" max="0.5" step="0.01" value="0.05">
          <div class="slider-ends"><span>0.01</span><span>0.5</span></div>
        </div>
        <div class="slider-row">
          <label for="filt_r"><span>Smooth (R) <span class="band">higher = smoother · def 0.4</span></span><span class="val" id="filt_r_val">—</span></label>
          <input id="filt_r" type="range" min="0.1" max="2" step="0.1" value="0.4">
          <div class="slider-ends"><span>0.1</span><span>2.0</span></div>
        </div>
      </div>
      <div class="actions">
        <button type="button" id="btnFilterApply">Apply</button>
      </div>
      <div class="status" id="filtStatus">Filter ready</div>
      <p class="hint"><strong>Apply </strong> saves ESP NVS only (no radar flash). Chart distance uses filtered value when ON.</p>
    </section>

    <section>
      <h2>Thresholds</h2>
      <div class="sliders">
        <div class="slider-row">
          <label for="rmax"><span>rmax <span class="band">max distance · def 8 m</span></span><span class="val" id="rmax_val">—</span></label>
          <input id="rmax" type="range" min="0.4" max="12" step="0.1" value="8">
          <div class="slider-ends"><span>0.4 m</span><span>12 m</span></div>
        </div>
        <div class="slider-row">
          <label for="clear_s"><span>clearance <span class="band">hold after clear · def 5 s</span></span><span class="val" id="clear_s_val">—</span></label>
          <input id="clear_s" type="range" min="0.5" max="20" step="0.5" value="5">
          <div class="slider-ends"><span>0.5 s</span><span>20 s</span></div>
        </div>
        <div class="slider-row">
          <label for="mov_s"><span>movement time <span class="band">MOV→OCC · def 1 s</span></span><span class="val" id="mov_s_val">—</span></label>
          <input id="mov_s" type="range" min="0.5" max="10" step="0.5" value="1">
          <div class="slider-ends"><span>0.5 s</span><span>10 s</span></div>
        </div>
        <div class="slider-row">
          <label for="mth1"><span>mth1 (near)<span class="band">0–2.8 m · def 60</span></span><span class="val" id="mth1_val">—</span></label>
          <input id="mth1" type="range" min="10" max="600" step="10" value="60">
          <div class="slider-ends"><span>10</span><span>600</span></div>
        </div>
        <div class="slider-row">
          <label for="mth2"><span>mth2 (mid)<span class="band">2.8–8 m · def 30</span></span><span class="val" id="mth2_val">—</span></label>
          <input id="mth2" type="range" min="10" max="300" step="10" value="30">
          <div class="slider-ends"><span>10</span><span>300</span></div>
        </div>
        <div class="slider-row">
          <label for="mth3"><span>mth3 (far)<span class="band">>8 m · def 20</span></span><span class="val" id="mth3_val">—</span></label>
          <input id="mth3" type="range" min="10" max="200" step="10" value="20">
          <div class="slider-ends"><span>10</span><span>200</span></div>
        </div>
      </div>
      <div class="actions">
        <button type="button" id="btnApply">Apply</button>
        <button type="button" class="secondary" id="btnReload">Reload from sensor</button>
      </div>
      <div class="status" id="status">Starting live poll…</div>
      <p class="hint">Chart uses MOV/OCC with clearance + movement time on ESP. <strong>Apply</strong> writes radar flash + ESP timers. <strong>Reload</strong> overrides sliders from sensor <code>get_all</code> (+ ESP timers). Lower mth* = more sensitive.</p>
    </section>
  </main>
  <script>
    const WINDOW_MS = 60000;
    const POLL_MS = 250;
    const points = [];
    let lastCfgSeq = -1;
    let formDirty = false;
    let filtDirty = false;
    let forceReload = false;
    let filterOn = true;

    const elMode = document.getElementById('mode');
    const elDis = document.getElementById('dis');
    const elStatus = document.getElementById('status');
    const elFiltStatus = document.getElementById('filtStatus');
    const elSwitch = document.getElementById('filtSwitch');
    const canvas = document.getElementById('chart');
    const ctx = canvas.getContext('2d');
    const fields = ['rmax', 'clear_s', 'mov_s', 'mth1', 'mth2', 'mth3'];
    const filtFields = ['filt_q', 'filt_r'];
    const radarFields = ['rmax', 'mth1', 'mth2', 'mth3'];
    const limits = {
      rmax: { min: 0.4, max: 12, step: 0.1, decimals: 1, unit: ' m' },
      clear_s: { min: 0.5, max: 20, step: 0.5, decimals: 1, unit: ' s' },
      mov_s: { min: 0.5, max: 10, step: 0.5, decimals: 1, unit: ' s' },
      mth1: { min: 10, max: 600, step: 10, decimals: 0, unit: '' },
      mth2: { min: 10, max: 300, step: 10, decimals: 0, unit: '' },
      mth3: { min: 10, max: 200, step: 10, decimals: 0, unit: '' },
      filt_q: { min: 0.01, max: 0.5, step: 0.01, decimals: 2, unit: '' },
      filt_r: { min: 0.1, max: 2, step: 0.1, decimals: 1, unit: '' }
    };

    function clamp(id, v) {
      const L = limits[id];
      let n = Number(v);
      if (!isFinite(n)) n = L.min;
      n = Math.min(L.max, Math.max(L.min, n));
      if (L.decimals === 0) n = Math.round(n);
      else n = Math.round(n / L.step) * L.step;
      return n;
    }

    function syncLabel(id) {
      const L = limits[id];
      const n = clamp(id, document.getElementById(id).value);
      document.getElementById(id).value = n;
      document.getElementById(id + '_val').textContent =
        (L.decimals ? n.toFixed(L.decimals) : String(n)) + L.unit;
    }

    function setSwitchUi(on) {
      filterOn = !!on;
      elSwitch.classList.toggle('on', filterOn);
      elSwitch.setAttribute('aria-pressed', filterOn ? 'true' : 'false');
    }

    fields.forEach(id => {
      const el = document.getElementById(id);
      el.addEventListener('input', () => {
        formDirty = true;
        syncLabel(id);
      });
      syncLabel(id);
    });

    filtFields.forEach(id => {
      const el = document.getElementById(id);
      el.addEventListener('input', () => {
        filtDirty = true;
        syncLabel(id);
      });
      syncLabel(id);
    });

    elSwitch.onclick = () => {
      filtDirty = true;
      setSwitchUi(!filterOn);
    };

    function fillForm(d) {
      if (!filtDirty) {
        if (d.filter_on != null) setSwitchUi(!!d.filter_on);
        filtFields.forEach(id => {
          if (d[id] != null) {
            document.getElementById(id).value = clamp(id, d[id]);
            syncLabel(id);
          }
        });
      }
      if (formDirty && !forceReload) return;
      ['clear_s', 'mov_s'].forEach(id => {
        if (d[id] != null) {
          document.getElementById(id).value = clamp(id, d[id]);
          syncLabel(id);
        }
      });
      if (!d.cfg_ok) return;
      radarFields.forEach(id => {
        document.getElementById(id).value = clamp(id, d[id]);
        syncLabel(id);
      });
      lastCfgSeq = d.cfg_seq;
      formDirty = false;
      forceReload = false;
    }

    function syncCanvasSize() {
      const dpr = window.devicePixelRatio || 1;
      const cssW = Math.max(1, canvas.clientWidth);
      const cssH = Math.max(1, canvas.clientHeight);
      const bw = Math.round(cssW * dpr);
      const bh = Math.round(cssH * dpr);
      if (canvas.width !== bw || canvas.height !== bh) {
        canvas.width = bw;
        canvas.height = bh;
      }
      ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
      return { w: cssW, h: cssH };
    }

    function drawChart() {
      const { w, h } = syncCanvasSize();
      const padL = 40, padR = 6, padT = 12, padB = 14;
      const plotW = w - padL - padR;
      const plotH = h - padT - padB;
      ctx.clearRect(0, 0, w, h);

      const now = Date.now();
      while (points.length && now - points[0].t > WINDOW_MS) points.shift();
      let maxY = 1;
      for (const p of points) if (p.dis > maxY) maxY = p.dis;
      maxY = Math.max(maxY * 1.15, 1);

      const body = getComputedStyle(document.body);
      ctx.font = body.fontWeight + ' 0.8rem ' + body.fontFamily;
      ctx.textAlign = 'right';
      ctx.textBaseline = 'middle';
      for (let i = 0; i < 5; i++) {
        const frac = i / 4;
        const y = padT + frac * plotH;
        const val = maxY * (1 - frac);
        ctx.strokeStyle = '#2a3140';
        ctx.lineWidth = 1;
        ctx.beginPath();
        ctx.moveTo(padL, y);
        ctx.lineTo(w - padR, y);
        ctx.stroke();
        ctx.fillStyle = 'rgba(255, 255, 255, 0.8)';
        const label = (i === 0) ? (val.toFixed(1) + ' m') : val.toFixed(1);
        ctx.fillText(label, padL - 2, y);
      }
      ctx.textAlign = 'left';
      ctx.textBaseline = 'alphabetic';

      for (const p of points) {
        if (p.mode === 'none') continue;
        const x = padL + ((p.t - (now - WINDOW_MS)) / WINDOW_MS) * plotW;
        const y = padT + plotH - (p.dis / maxY) * plotH;
        ctx.fillStyle = p.mode === 'mov' ? '#ff8a3d' : '#4cd4a0';
        ctx.beginPath();
        ctx.arc(x, y, 3, 0, Math.PI * 2);
        ctx.fill();
      }
    }

    async function poll() {
      try {
        const r = await fetch('/api/live');
        const d = await r.json();
        const mode = (d.mode || 'none').toLowerCase();
        elMode.textContent = mode.toUpperCase();
        elMode.className = 'mode ' + mode;
        if (mode === 'none') {
          elDis.textContent = '—';
        } else {
          elDis.textContent = Number(d.dis).toFixed(2);
        }
        points.push({ t: Date.now(), mode, dis: Number(d.dis) || 0 });
        drawChart();
        fillForm(d);
        const filtTag = d.filter_on ? 'filt ON' : 'filt OFF';
        if (d.cfg_ok) {
          elStatus.textContent =
            'Live OK · rmax=' + d.rmax +
            ' clear=' + d.clear_s + 's mov=' + d.mov_s + 's' +
            ' mth=' + d.mth1 + '/' + d.mth2 + '/' + d.mth3 +
            ' · ' + filtTag + ' (seq ' + d.cfg_seq + ')';
        } else {
          elStatus.textContent =
            'Live OK · clear=' + d.clear_s + 's mov=' + d.mov_s +
            's · ' + filtTag + ' · radar thresholds not loaded — tap Reload';
        }
      } catch (e) {
        elStatus.textContent = 'Live poll failed — reconnect SoftAP?';
      }
    }

    async function postConfig() {
      const body = new URLSearchParams();
      fields.forEach(id => body.set(id, document.getElementById(id).value));
      elStatus.textContent = 'Applying + saving…';
      formDirty = true;
      forceReload = false;
      try {
        const r = await fetch('/api/config', {
          method: 'POST',
          headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
          body
        });
        const d = await r.json();
        if (d.ok) {
          fields.forEach(id => {
            if (d[id] != null) {
              document.getElementById(id).value = clamp(id, d[id]);
              syncLabel(id);
            }
          });
          lastCfgSeq = d.cfg_seq;
          formDirty = true;
          elStatus.textContent =
            'Saved · rmax=' + d.rmax + ' mth=' + d.mth1 + '/' + d.mth2 + '/' + d.mth3 +
            ' · tap Reload later to verify sensor flash';
        } else {
          elStatus.textContent = 'Error: ' + (d.err || 'unknown');
        }
      } catch (e) {
        elStatus.textContent = 'Apply failed — check SoftAP connection';
      }
    }

    async function postFilter() {
      const body = new URLSearchParams();
      body.set('filter_on', filterOn ? '1' : '0');
      filtFields.forEach(id => body.set(id, document.getElementById(id).value));
      elFiltStatus.textContent = 'Applying filter…';
      filtDirty = true;
      try {
        const r = await fetch('/api/filter', {
          method: 'POST',
          headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
          body
        });
        const d = await r.json();
        if (d.ok) {
          setSwitchUi(!!d.filter_on);
          filtFields.forEach(id => {
            if (d[id] != null) {
              document.getElementById(id).value = clamp(id, d[id]);
              syncLabel(id);
            }
          });
          filtDirty = false;
          elFiltStatus.textContent =
            'Filter saved · ' + (d.filter_on ? 'ON' : 'OFF') +
            ' Q=' + d.filt_q + ' R=' + d.filt_r;
        } else {
          elFiltStatus.textContent = 'Error: ' + (d.err || 'unknown');
        }
      } catch (e) {
        elFiltStatus.textContent = 'Apply filter failed — check SoftAP';
      }
    }

    document.getElementById('btnApply').onclick = () => postConfig();
    document.getElementById('btnFilterApply').onclick = () => postFilter();
    document.getElementById('btnReload').onclick = async () => {
      formDirty = false;
      filtDirty = false;
      forceReload = true;
      lastCfgSeq = -1;
      elStatus.textContent = 'Reloading — overriding sliders from sensor…';
      await fetch('/api/refresh', { method: 'POST' });
    };

    setInterval(poll, POLL_MS);
    poll();
  </script>
</body>
</html>
)MONITORHTML";
