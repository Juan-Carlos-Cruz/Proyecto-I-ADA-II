(function(global) {
  async function loadFile(event) {
    const file = event.target.files[0];

    if (!file) {
      return;
    }

    try {
      const rawFileContent = await global.RiegoFileService.readTextFile(file);
      const parsed = global.RiegoFileService.parseFarmFileContent(rawFileContent);

      global.RiegoState.finca = parsed.finca;
      global.RiegoState.rawFileContent = parsed.rawFileContent;
      global.RiegoState.lastRenderedResult = null;

      global.RiegoConfigScreen.renderConfig();
    } catch (error) {
      alert("No se pudo cargar la finca: " + error.message);
      event.target.value = "";
    }
  }

  async function runAlgorithm() {
    const state = global.RiegoState;

    if (!state.selectedAlgo) {
      alert("Selecciona un algoritmo primero");
      return;
    }

    global.RiegoNavigation.goTo("result");
    global.RiegoResultScreen.updateResultHeader();
    global.RiegoResultScreen.setDownloadAvailability(false);
    document.getElementById("loading").style.display = "grid";
    document.getElementById("result-content").style.display = "none";
    document.getElementById("result-state-text").textContent =
      "Esperando la respuesta del backend para reemplazar este placeholder con la salida real del algoritmo.";

    try {
      const data = await global.RiegoApiClient.calculatePlan({
        algoritmo: state.selectedAlgo,
        finca: state.rawFileContent
      });

      global.RiegoResultScreen.renderResult(data);
    } catch (error) {
      if (error.message === "BACKEND_DISABLED") {
        setTimeout(() => {
          global.RiegoResultScreen.renderPendingBackendResult();
        }, 800);
        return;
      }

      document.getElementById("loading").style.display = "none";
      alert("Error conectando con el backend: " + error.message);
    }
  }

  async function loadBenchmarkFiles(event) {
    const files = Array.from(event.target.files);
    if (!files.length) return;

    const state = global.RiegoState;
    if (!state.benchmarkFincas) state.benchmarkFincas = [];

    for (const file of files) {
      try {
        const raw = await global.RiegoFileService.readTextFile(file);
        const parsed = global.RiegoFileService.parseFarmFileContent(raw);
        state.benchmarkFincas.push({ name: file.name, raw, n: parsed.finca.length });
      } catch (e) {
        alert("No se pudo leer " + file.name + ": " + e.message);
      }
    }

    event.target.value = "";
    global.RiegoBenchmarkScreen.renderFileList();
    global.RiegoBenchmarkScreen.invalidateAnalysisFlow();
  }

  global.loadFile = loadFile;
  global.runAlgorithm = runAlgorithm;
  global.loadBenchmarkFiles = loadBenchmarkFiles;
})(window);
