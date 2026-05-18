(function(global) {

  const BENCHMARK_SERIES = {
    dp: {
      legendLabel: "DP (optimo)",
      tooltipLabel: "Programacion dinamica",
      color: "#4e7a28",
      legendFill: "rgba(78,122,40,0.12)"
    },
    v: {
      legendLabel: "Voraz",
      tooltipLabel: "Voraz",
      color: "#a85d11",
      legendFill: "rgba(168,93,17,0.12)"
    },
    fb: {
      legendLabel: "FB (n≤10)",
      tooltipLabel: "Fuerza bruta",
      color: "#3a5fa8",
      legendFill: "rgba(58,95,168,0.12)",
      dash: "4 2"
    }
  };

  let benchmarkChartRows = [];
  let benchmarkActiveSeries = null;
  let greedyBenchmarkRows = [];
  let greedyBenchmarkUnlocked = false;
  let benchmarkBusy = false;
  let greedyBusy = false;
  let benchmarkRequestToken = 0;
  let greedyRequestToken = 0;
  let benchmarkAbortController = null;
  let greedyAbortController = null;

  function hasLoadedBenchmarkFiles() {
    const fincas = global.RiegoState.benchmarkFincas || [];
    return fincas.length > 0;
  }

  function syncActionButtons() {
    const runBtn = document.getElementById("benchmark-run-btn");
    const greedyRunBtn = document.getElementById("greedy-benchmark-run-btn");
    const hasFiles = hasLoadedBenchmarkFiles();
    const anyAnalysisBusy = benchmarkBusy || greedyBusy;

    if (runBtn) runBtn.disabled = !hasFiles || anyAnalysisBusy;
    if (greedyRunBtn) greedyRunBtn.disabled = !hasFiles || !greedyBenchmarkUnlocked || anyAnalysisBusy;
  }

  function cancelBenchmarkRequest() {
    benchmarkRequestToken += 1;
    if (benchmarkAbortController) {
      benchmarkAbortController.abort();
      benchmarkAbortController = null;
    }
    benchmarkBusy = false;
  }

  function cancelGreedyRequest() {
    greedyRequestToken += 1;
    if (greedyAbortController) {
      greedyAbortController.abort();
      greedyAbortController = null;
    }
    greedyBusy = false;
  }

  // ── Lista de fincas cargadas ──────────────────────────────────────────────

  function renderFileList() {
    const state = global.RiegoState;
    const fincas = state.benchmarkFincas || [];
    const list = document.getElementById("benchmark-file-list");
    const count = document.getElementById("benchmark-file-count");

    if (count) count.textContent = String(fincas.length);

    if (!fincas.length) {
      list.innerHTML = '<div class="benchmark-empty">Sin fincas cargadas. Selecciona uno o mas archivos .txt.</div>';
      syncActionButtons();
      return;
    }

    list.innerHTML = fincas.map((f, i) => `
      <div class="benchmark-file-item">
        <div>
          <div class="file-name">${escapeHtml(f.name)}</div>
          <div class="file-meta">${f.n} tablon${f.n !== 1 ? "es" : ""}</div>
        </div>
        <button class="btn-remove" onclick="RiegoBenchmarkScreen.removeFile(${i})" title="Quitar">&#x2715;</button>
      </div>
    `).join("");

    syncActionButtons();
  }

  function removeFile(index) {
    const state = global.RiegoState;
    if (state.benchmarkFincas) {
      state.benchmarkFincas.splice(index, 1);
    }
    renderFileList();
    invalidateAnalysisFlow();
  }

  function clearFiles() {
    global.RiegoState.benchmarkFincas = [];
    renderFileList();
    invalidateAnalysisFlow();
  }

  // ── Ejecutar benchmark ────────────────────────────────────────────────────

  async function executeBenchmark() {
    const state = global.RiegoState;
    const fincas = state.benchmarkFincas || [];

    if (!fincas.length) {
      alert("Carga al menos una finca antes de ejecutar el analisis.");
      return;
    }

    cancelGreedyRequest();
    cancelBenchmarkRequest();
    const requestToken = benchmarkRequestToken;
    benchmarkAbortController = new AbortController();
    greedyBenchmarkUnlocked = false;
    resetBenchmarkResults();
    resetGreedyResults();
    setBenchmarkBusyState(true);

    try {
      const resultado = await global.RiegoApiClient.runBenchmark(fincas.map(f => f.raw), {
        signal: benchmarkAbortController.signal
      });
      if (requestToken !== benchmarkRequestToken) return;
      const filas = Array.isArray(resultado.filas) ? resultado.filas : [];
      const resumen = resultado.resumen || null;
      renderTable(filas, resumen);
      renderChart(filas);
      greedyBenchmarkUnlocked = true;
    } catch (err) {
      if (requestToken !== benchmarkRequestToken || err.message === "REQUEST_CANCELLED") {
        return;
      }
      alert("Error en el benchmark: " + err.message);
    } finally {
      if (requestToken === benchmarkRequestToken) {
        benchmarkAbortController = null;
        setBenchmarkBusyState(false);
      }
    }
  }

  async function executeGreedyBenchmark() {
    const state = global.RiegoState;
    const fincas = state.benchmarkFincas || [];

    if (!fincas.length) {
      alert("Carga al menos una finca antes de ejecutar el laboratorio Voraz.");
      return;
    }
    if (!greedyBenchmarkUnlocked) {
      alert("Ejecuta primero el analisis comparativo para habilitar el laboratorio Voraz.");
      return;
    }

    cancelGreedyRequest();
    const requestToken = greedyRequestToken;
    greedyAbortController = new AbortController();
    resetGreedyResults();
    setGreedyBusyState(true);

    try {
      const filas = await global.RiegoApiClient.runGreedyBenchmark(fincas.map(f => f.raw), {
        signal: greedyAbortController.signal
      });
      if (requestToken !== greedyRequestToken) return;
      renderGreedyBenchmark(filas);
    } catch (err) {
      if (requestToken !== greedyRequestToken || err.message === "REQUEST_CANCELLED") {
        return;
      }
      alert("Error en el laboratorio Voraz: " + err.message);
    } finally {
      if (requestToken === greedyRequestToken) {
        greedyAbortController = null;
        setGreedyBusyState(false);
      }
    }
  }

  function setBenchmarkBusyState(busy) {
    benchmarkBusy = busy;
    document.getElementById("benchmark-loading").style.display = busy ? "grid" : "none";
    document.getElementById("benchmark-results").style.display = busy ? "none" : "block";
    syncActionButtons();
  }

  function setGreedyBusyState(busy) {
    greedyBusy = busy;
    document.getElementById("greedy-benchmark-loading").style.display = busy ? "grid" : "none";
    document.getElementById("greedy-benchmark-results").style.display = busy ? "none" : "block";
    syncActionButtons();
  }

  function resetBenchmarkResults() {
    benchmarkChartRows = [];
    benchmarkActiveSeries = null;
    hideBenchmarkTooltip();
    document.getElementById("benchmark-results").style.display = "none";
    document.getElementById("benchmark-loading").style.display = "none";
    document.getElementById("benchmark-tbody").innerHTML =
      '<tr class="empty-row"><td colspan="8">Ejecuta el analisis para ver los resultados.</td></tr>';
    document.getElementById("benchmark-summary-tbody").innerHTML =
      '<tr class="empty-row"><td colspan="4">Ejecuta el analisis para ver el resumen.</td></tr>';
    const svg = document.getElementById("benchmark-chart");
    if (svg) svg.innerHTML = "";
  }

  function resetGreedyResults() {
    greedyBenchmarkRows = [];
    document.getElementById("greedy-benchmark-results").style.display = "none";
    document.getElementById("greedy-benchmark-loading").style.display = "none";
    document.getElementById("greedy-benchmark-best").innerHTML =
      '<div class="benchmark-empty">Ejecuta el laboratorio para ver la mejor combinacion.</div>';
    document.getElementById("greedy-benchmark-tbody").innerHTML =
      '<tr class="empty-row"><td colspan="8">Ejecuta el laboratorio de Voraz para ver el ranking.</td></tr>';
  }

  function resetResults() {
    resetBenchmarkResults();
    resetGreedyResults();
    syncActionButtons();
  }

  function invalidateAnalysisFlow() {
    cancelBenchmarkRequest();
    cancelGreedyRequest();
    greedyBenchmarkUnlocked = false;
    resetResults();
  }

  // ── Tabla de resultados ───────────────────────────────────────────────────

  function renderTable(filas, resumen = null) {
    const tbody = document.getElementById("benchmark-tbody");

    if (!filas.length) {
      tbody.innerHTML = '<tr class="empty-row"><td colspan="8">No se obtuvieron filas del backend.</td></tr>';
      renderSummary([], null);
      return;
    }

    tbody.innerHTML = filas.map(f => {
      const pctDiff = Number.isFinite(f.pct_diff) ? f.pct_diff : 0;
      const pctClass = pctDiff >= 15 ? "diff-worse" : pctDiff >= 5 ? "diff-mid" : "diff-better";
      const tiempoDp = formatSeconds(f.tiempo_dp_s);
      const tiempoV = formatSeconds(f.tiempo_v_s);
      const fbCell = f.costo_fb >= 0
        ? f.costo_fb
        : '<span style="color:var(--text-muted);letter-spacing:0.1em">&mdash;</span>';
      return `<tr>
        <td>${f.n}</td>
        <td>${fbCell}</td>
        <td>${f.costo_dp}</td>
        <td>${f.costo_v}</td>
        <td>${f.diff}</td>
        <td class="${pctClass}">${formatMetric(pctDiff)}%</td>
        <td>${tiempoDp}</td>
        <td>${tiempoV}</td>
      </tr>`;
    }).join("");

    renderSummary(filas, resumen);
  }

  function renderSummary(filas, resumen = null) {
    const tbody = document.getElementById("benchmark-summary-tbody");

    if (!filas.length) {
      tbody.innerHTML = '<tr class="empty-row"><td colspan="4">No hay datos suficientes para resumir.</td></tr>';
      return;
    }

    const total = resumen && Number.isFinite(resumen.total_fincas) ? resumen.total_fincas : filas.length;
    const avgDiff = resumen && Number.isFinite(resumen.promedio_diff)
      ? resumen.promedio_diff
      : filas.reduce((sum, f) => sum + f.diff, 0) / total;
    const avgPct = resumen && Number.isFinite(resumen.promedio_pct_diff)
      ? resumen.promedio_pct_diff
      : filas.reduce((sum, f) => sum + (Number.isFinite(f.pct_diff) ? f.pct_diff : 0), 0) / total;
    const exactPct = resumen && Number.isFinite(resumen.porcentaje_exactos)
      ? resumen.porcentaje_exactos
      : (filas.filter(f => f.diff === 0).length / total) * 100;

    tbody.innerHTML = `<tr>
      <td>${total}</td>
      <td>${formatMetric(avgDiff)}</td>
      <td>${formatMetric(avgPct)}%</td>
      <td>${formatMetric(exactPct)}%</td>
    </tr>`;
  }

  function renderGreedyBenchmark(filas) {
    const tbody = document.getElementById("greedy-benchmark-tbody");
    greedyBenchmarkRows = filas.slice();

    if (!filas.length) {
      document.getElementById("greedy-benchmark-best").innerHTML =
        '<div class="benchmark-empty">No se obtuvieron combinaciones para comparar.</div>';
      tbody.innerHTML = '<tr class="empty-row"><td colspan="8">No se obtuvieron filas del backend.</td></tr>';
      return;
    }

    renderGreedyBest(filas[0]);

    tbody.innerHTML = filas.map((f, index) => {
      const estado = f.es_actual
        ? '<span class="greedy-status-badge current">Actual</span>'
        : '<span class="greedy-status-badge">Alterno</span>';
      const rowClass = f.es_actual ? ' class="current-row"' : "";
      return `<tr${rowClass}>
        <td class="greedy-rank">${index + 1}</td>
        <td>${escapeHtml(f.criterio_principal)}</td>
        <td>${escapeHtml(f.criterio_desempate)}</td>
        <td>${formatMetric(f.promedio_diff)}</td>
        <td>${formatMetric(f.promedio_pct_diff)}%</td>
        <td>${f.coincidencias_optimo}/${f.total_fincas}</td>
        <td>${formatSeconds(f.promedio_tiempo_v_s)}</td>
        <td>${estado}</td>
      </tr>`;
    }).join("");
  }

  function renderGreedyBest(best) {
    const container = document.getElementById("greedy-benchmark-best");
    if (!best) {
      container.innerHTML = '<div class="benchmark-empty">Ejecuta el laboratorio para ver la mejor combinacion.</div>';
      return;
    }

    const estado = best.es_actual ? "Coincide con tu criterio actual" : "Es una alternativa al criterio actual";

    container.innerHTML = `
      <div class="benchmark-greedy-metric">
        <div class="benchmark-greedy-metric-label">Mejor principal</div>
        <div class="benchmark-greedy-metric-value">${escapeHtml(best.criterio_principal)}</div>
        <div class="benchmark-greedy-metric-copy">Ranking #1 del laboratorio</div>
      </div>
      <div class="benchmark-greedy-metric">
        <div class="benchmark-greedy-metric-label">Mejor desempate</div>
        <div class="benchmark-greedy-metric-value">${escapeHtml(best.criterio_desempate)}</div>
        <div class="benchmark-greedy-metric-copy">${estado}</div>
      </div>
      <div class="benchmark-greedy-metric">
        <div class="benchmark-greedy-metric-label">Promedio %diff</div>
        <div class="benchmark-greedy-metric-value">${formatMetric(best.promedio_pct_diff)}%</div>
        <div class="benchmark-greedy-metric-copy">Respecto al costo optimo de PD</div>
      </div>
      <div class="benchmark-greedy-metric">
        <div class="benchmark-greedy-metric-label">Coincidencias exactas</div>
        <div class="benchmark-greedy-metric-value">${best.coincidencias_optimo}/${best.total_fincas}</div>
        <div class="benchmark-greedy-metric-copy">Mismas fincas que igualan el optimo</div>
      </div>`;
  }

  // ── Grafico SVG ───────────────────────────────────────────────────────────

  function renderChart(filas) {
    const svg = document.getElementById("benchmark-chart");
    if (!svg || !filas.length) return;

    benchmarkChartRows = filas.slice();

    const W = 620, H = 320;
    const PL = 72, PR = 28, PT = 28, PB = 58;
    const CW = W - PL - PR;
    const CH = H - PT - PB;

    const ns       = filas.map(f => f.n);
    const minN     = Math.min(...ns);
    const maxN     = Math.max(...ns);
    const fbFilas  = filas.filter(f => f.costo_fb >= 0);
    const allCosts = filas.flatMap(f => {
      const vals = [f.costo_dp, f.costo_v];
      if (f.costo_fb >= 0) vals.push(f.costo_fb);
      return vals;
    });
    const minC  = Math.min(...allCosts);
    const maxC  = Math.max(...allCosts);
    const pad   = (maxC - minC) * 0.12 || maxC * 0.12 || 10;
    const yMin  = Math.max(0, minC - pad);
    const yMax  = maxC + pad;
    const yRange = yMax - yMin;

    if (benchmarkActiveSeries === "fb" && !fbFilas.length) {
      benchmarkActiveSeries = null;
    }

    function toX(n) {
      return maxN === minN ? PL + CW / 2 : PL + ((n - minN) / (maxN - minN)) * CW;
    }
    function toY(c) {
      return PT + CH - ((c - yMin) / yRange) * CH;
    }

    function isVisible(seriesKey) {
      return !benchmarkActiveSeries || benchmarkActiveSeries === seriesKey;
    }

    function addInteractivePoint(pointId, seriesKey, n, cost) {
      const meta = BENCHMARK_SERIES[seriesKey];
      if (!meta || !isVisible(seriesKey)) return;

      const x = toX(n).toFixed(1);
      const y = toY(cost).toFixed(1);

      h += `<circle cx="${x}" cy="${y}" r="4" fill="${meta.color}" stroke="white" stroke-width="1.5" pointer-events="none" data-dot="${pointId}"/>`;
      h += `<circle cx="${x}" cy="${y}" r="11" fill="transparent" style="cursor:pointer" data-hit="1" data-point-id="${pointId}" data-series-label="${meta.tooltipLabel}" data-color="${meta.color}" data-n="${n}" data-cost="${cost}"/>`;
    }

    function addLegendItem(index, seriesKey) {
      const meta = BENCHMARK_SERIES[seriesKey];
      if (!meta) return;

      const itemY = ly + 8 + index * 20;
      const selected = benchmarkActiveSeries === seriesKey;
      const dimmed = !!benchmarkActiveSeries && !selected;
      const dash = meta.dash ? ` stroke-dasharray="${meta.dash}"` : "";

      h += `<g data-legend-key="${seriesKey}" style="cursor:pointer" opacity="${dimmed ? "0.42" : "1"}">`;
      h += `<rect x="${lx + 6}" y="${itemY}" width="120" height="18" rx="6" fill="${selected ? meta.legendFill : "transparent"}" stroke="${selected ? "rgba(86,68,41,0.14)" : "transparent"}"/>`;
      h += `<line x1="${lx + 16}" y1="${itemY + 9}" x2="${lx + 34}" y2="${itemY + 9}" stroke="${meta.color}" stroke-width="${seriesKey === "fb" ? "2" : "2.5"}"${dash}/>`;
      h += `<circle cx="${lx + 25}" cy="${itemY + 9}" r="3.5" fill="${meta.color}"/>`;
      h += `<text x="${lx + 40}" y="${itemY + 13}" font-size="11" fill="#2f2417" font-weight="700">${meta.legendLabel}</text>`;
      h += `</g>`;
    }

    let h = "";

    // Grid lines
    for (let i = 0; i <= 5; i++) {
      const cv = yMin + (yRange * i) / 5;
      const y  = toY(cv);
      h += `<line x1="${PL}" y1="${y.toFixed(1)}" x2="${W - PR}" y2="${y.toFixed(1)}" stroke="rgba(86,68,41,0.08)" stroke-width="1"/>`;
      h += `<text x="${PL - 8}" y="${(y + 4).toFixed(1)}" text-anchor="end" font-size="11" fill="#8a7a67">${Math.round(cv)}</text>`;
    }

    // X ticks
    filas.forEach(f => {
      const x = toX(f.n);
      h += `<line x1="${x.toFixed(1)}" y1="${PT + CH}" x2="${x.toFixed(1)}" y2="${PT + CH + 6}" stroke="rgba(86,68,41,0.28)" stroke-width="1"/>`;
      h += `<text x="${x.toFixed(1)}" y="${PT + CH + 20}" text-anchor="middle" font-size="11" fill="#8a7a67">${f.n}</text>`;
    });

    // Axes
    h += `<line x1="${PL}" y1="${PT}" x2="${PL}" y2="${PT + CH}" stroke="rgba(86,68,41,0.3)" stroke-width="1.5"/>`;
    h += `<line x1="${PL}" y1="${PT + CH}" x2="${W - PR}" y2="${PT + CH}" stroke="rgba(86,68,41,0.3)" stroke-width="1.5"/>`;

    // Area fill DP
    if (isVisible("dp")) {
      const dpAreaPath = `${PL},${PT + CH} ` +
        filas.map(f => `${toX(f.n).toFixed(1)},${toY(f.costo_dp).toFixed(1)}`).join(" ") +
        ` ${toX(filas[filas.length - 1].n).toFixed(1)},${PT + CH}`;
      h += `<polygon points="${dpAreaPath}" fill="rgba(78,122,40,0.07)"/>`;
    }

    // DP polyline + visible circles (pointer-events:none, hit handled below)
    if (isVisible("dp")) {
      h += `<polyline points="${filas.map(f => `${toX(f.n).toFixed(1)},${toY(f.costo_dp).toFixed(1)}`).join(" ")}" fill="none" stroke="#4e7a28" stroke-width="2.5" stroke-linejoin="round" stroke-linecap="round" pointer-events="none"/>`;
      filas.forEach((f, i) => {
        addInteractivePoint(`dp-${i}`, "dp", f.n, f.costo_dp);
      });
    }

    // Voraz polyline + visible circles
    if (isVisible("v")) {
      h += `<polyline points="${filas.map(f => `${toX(f.n).toFixed(1)},${toY(f.costo_v).toFixed(1)}`).join(" ")}" fill="none" stroke="#a85d11" stroke-width="2.5" stroke-linejoin="round" stroke-linecap="round" pointer-events="none"/>`;
      filas.forEach((f, i) => {
        addInteractivePoint(`v-${i}`, "v", f.n, f.costo_v);
      });
    }

    // FB polyline + visible circles
    if (isVisible("fb") && fbFilas.length >= 2) {
      h += `<polyline points="${fbFilas.map(f => `${toX(f.n).toFixed(1)},${toY(f.costo_fb).toFixed(1)}`).join(" ")}" fill="none" stroke="#3a5fa8" stroke-width="2" stroke-dasharray="6 3" stroke-linejoin="round" stroke-linecap="round" pointer-events="none"/>`;
    }
    if (isVisible("fb")) {
      filas.forEach((f, i) => {
        if (f.costo_fb < 0) return;
        addInteractivePoint(`fb-${i}`, "fb", f.n, f.costo_fb);
      });
    }

    // Axis labels
    h += `<text x="${(PL + CW / 2).toFixed(1)}" y="${H - 8}" text-anchor="middle" font-size="12" font-weight="700" fill="#675540">Tablones (n)</text>`;
    h += `<text x="14" y="${(PT + CH / 2).toFixed(1)}" text-anchor="middle" font-size="12" font-weight="700" fill="#675540" transform="rotate(-90 14 ${(PT + CH / 2).toFixed(1)})">Costo</text>`;

    // Legend (top-right)
    const lx = W - PR - 136, ly = PT + 8;
    const legendRows = fbFilas.length ? 3 : 2;
    const legH = legendRows * 20 + 18;
    h += `<rect x="${lx}" y="${ly}" width="130" height="${legH}" rx="7" fill="rgba(255,255,255,0.92)" stroke="rgba(86,68,41,0.14)"/>`;
    addLegendItem(0, "dp");
    addLegendItem(1, "v");
    if (fbFilas.length) {
      addLegendItem(2, "fb");
    }

    svg.setAttribute("viewBox", `0 0 ${W} ${H}`);
    svg.innerHTML = h;

    attachTooltipListeners(svg);
    attachLegendListeners(svg);
  }

  // ── Tooltip interactivo ───────────────────────────────────────────────────

  function attachTooltipListeners(svg) {
    const tooltip = ensureTooltipLayer();
    if (!tooltip) return;

    svg.querySelectorAll('circle[data-hit="1"]').forEach(circle => {
      const pointId = circle.getAttribute("data-point-id");
      const seriesLabel = circle.getAttribute("data-series-label");
      const color = circle.getAttribute("data-color");
      const n = circle.getAttribute("data-n");
      const cost = circle.getAttribute("data-cost");

      circle.addEventListener("mouseenter", (e) => {
        const dot = svg.querySelector(`[data-dot="${pointId}"]`);
        if (dot) dot.setAttribute("r", "6");

        tooltip.innerHTML =
          `<div class="bm-tt-title">${seriesLabel}</div>` +
          `<div class="bm-tt-row"><span class="bm-tt-dot" style="background:${color}"></span>Punto <span class="bm-tt-val">(${n}, ${cost})</span></div>` +
          `<div class="bm-tt-row">X (tablones) <span class="bm-tt-val">${n}</span></div>` +
          `<div class="bm-tt-row">Y (costo) <span class="bm-tt-val">${cost}</span></div>`;

        tooltip.style.display = "block";
        moveTooltip(circle, tooltip);
      });

      circle.addEventListener("mousemove", () => moveTooltip(circle, tooltip));

      circle.addEventListener("mouseleave", () => {
        const dot = svg.querySelector(`[data-dot="${pointId}"]`);
        if (dot) dot.setAttribute("r", "4");
        tooltip.style.display = "none";
      });
    });
  }

  function ensureTooltipLayer() {
    const tooltip = document.getElementById("benchmark-tooltip");
    if (!tooltip) return null;

    if (tooltip.parentElement !== document.body) {
      document.body.appendChild(tooltip);
    }

    return tooltip;
  }

  function hideBenchmarkTooltip() {
    const tooltip = document.getElementById("benchmark-tooltip");
    if (tooltip) tooltip.style.display = "none";
  }

  function formatSeconds(seconds) {
    if (!Number.isFinite(seconds)) return "&mdash;";
    return seconds < 0.01 ? `${seconds.toFixed(4)}s` : `${seconds.toFixed(2)}s`;
  }

  function formatMetric(value) {
    if (!Number.isFinite(value)) return "&mdash;";
    return value.toFixed(2);
  }

  function attachLegendListeners(svg) {
    svg.querySelectorAll("[data-legend-key]").forEach(item => {
      item.addEventListener("click", () => {
        const seriesKey = item.getAttribute("data-legend-key");
        if (!seriesKey) return;

        benchmarkActiveSeries = benchmarkActiveSeries === seriesKey ? null : seriesKey;
        hideBenchmarkTooltip();
        renderChart(benchmarkChartRows);
      });
    });
  }

  function moveTooltip(circle, tip) {
    const rect = circle.getBoundingClientRect();
    const tw = tip.offsetWidth || 180;
    const th = tip.offsetHeight || 96;
    const vw = window.innerWidth;
    const vh = window.innerHeight;
    const gap = 10;
    const edge = 12;

    let left = rect.left + rect.width / 2 - tw / 2;
    let top = rect.top - th - gap;

    if (top < edge) {
      top = rect.bottom + gap;
    }

    if (left < edge) {
      left = edge;
    } else if (left + tw > vw - edge) {
      left = vw - tw - edge;
    }

    if (top + th > vh - edge) {
      top = Math.max(edge, rect.top - th - gap);
    }

    tip.style.left = `${Math.round(left)}px`;
    tip.style.top = `${Math.round(top)}px`;
  }

  // ── Utilidades ────────────────────────────────────────────────────────────

  function escapeHtml(str) {
    return str.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;");
  }

  // ── API publica ───────────────────────────────────────────────────────────

  global.RiegoBenchmarkScreen = {
    invalidateAnalysisFlow,
    renderFileList,
    removeFile,
    clearFiles,
    executeBenchmark,
    executeGreedyBenchmark,
    renderTable,
    renderChart
  };

  global.executeBenchmark = executeBenchmark;
  global.executeGreedyBenchmark = executeGreedyBenchmark;

})(window);
