(function(global) {
  function updateResultHeader() {
    const state = global.RiegoState;
    const algoName = state.algoNames[state.selectedAlgo] || "Resultado pendiente";

    document.getElementById("result-algo-name").textContent = algoName;
    document.getElementById("result-algo-badge").textContent = state.selectedAlgo || "-";
  }

  function setDownloadAvailability(enabled) {
    document.getElementById("download-result-btn").disabled = !enabled;
  }

  function renderEmptyState(message) {
    document.getElementById("result-order").innerHTML = `<div class="empty-state">${message}</div>`;
    document.getElementById("detail-tbody").innerHTML = `
      <tr class="empty-row">
        <td colspan="4">Conecta el backend para poblar el detalle por tablon.</td>
      </tr>
    `;
  }

  function renderPendingBackendResult() {
    const state = global.RiegoState;

    document.getElementById("loading").style.display = "none";
    document.getElementById("result-content").style.display = "block";
    document.getElementById("result-cost").textContent = "-";
    document.getElementById("result-state-text").textContent =
      "Backend pendiente: la interfaz ya esta lista para mostrar costo total, permutacion y detalle apenas se conecte la API.";

    state.lastRenderedResult = {
      costo: null,
      permutacion: [],
      detalles: []
    };

    renderEmptyState("Conecta el backend para ver la secuencia calculada y compararla entre algoritmos.");
    setDownloadAvailability(true);
  }

  function renderResult(data) {
    const state = global.RiegoState;
    const normalizedResult = {
      costo: data.costo,
      permutacion: Array.isArray(data.permutacion) ? data.permutacion : [],
      detalles: Array.isArray(data.detalles) ? data.detalles : []
    };

    document.getElementById("loading").style.display = "none";
    document.getElementById("result-content").style.display = "block";
    document.getElementById("result-cost").textContent = normalizedResult.costo;
    document.getElementById("result-state-text").textContent =
      "Resultado calculado correctamente. Revisa el orden de riego y el costo individual de cada tablon.";

    state.lastRenderedResult = normalizedResult;

    updateResultHeader();

    const orderDiv = document.getElementById("result-order");
    orderDiv.innerHTML = normalizedResult.permutacion.length
      ? normalizedResult.permutacion
          .map((plot, index) => `<div class="result-chip ${index === 0 ? "active" : ""}">T${plot}</div>`)
          .join('<span class="arrow">→</span>')
      : '<div class="empty-state">El backend respondio sin una permutacion valida.</div>';

    const tbody = document.getElementById("detail-tbody");
    tbody.innerHTML = normalizedResult.detalles.length
      ? normalizedResult.detalles
          .map(detail => `<tr>
            <td>T${detail.tablon}</td>
            <td>dia ${detail.inicio}</td>
            <td class="case-${detail.caso}">${global.RiegoExportService.formatCaseName(detail.caso)}</td>
            <td>${detail.costo}</td>
          </tr>`)
          .join("")
      : `
          <tr class="empty-row">
            <td colspan="4">El backend respondio sin detalle calculado.</td>
          </tr>
        `;

    setDownloadAvailability(true);
  }

  function downloadResultTxt() {
    const resultStateText = document.getElementById("result-state-text").textContent;

    try {
      global.RiegoExportService.downloadResultTxt(resultStateText);
    } catch (error) {
      alert(error.message);
    }
  }

  global.RiegoResultScreen = {
    updateResultHeader,
    setDownloadAvailability,
    renderEmptyState,
    renderPendingBackendResult,
    renderResult
  };

  global.renderResult = renderResult;
  global.downloadResultTxt = downloadResultTxt;
})(window);
