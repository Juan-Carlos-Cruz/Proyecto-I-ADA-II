(function(global) {
  function formatCaseName(caso) {
    if (caso === 1) return "Perfecto";
    if (caso === 2) return "A tiempo";
    if (caso === 3) return "Tarde";
    return "Sin clasificar";
  }

  function buildResultTxtContent(resultStateText) {
    const state = global.RiegoState;
    const result = state.lastRenderedResult;
    const costo = result && result.costo !== null ? result.costo : 0;
    const lines = [String(costo)];

    if (result && result.permutacion.length) {
      result.permutacion.forEach(plot => lines.push(String(plot)));
    }

    return lines.join("\n");
  }

  function downloadResultTxt(resultStateText) {
    const state = global.RiegoState;

    if (!state.lastRenderedResult) {
      throw new Error("Primero genera o visualiza un resultado para exportarlo.");
    }

    const content = buildResultTxtContent(resultStateText);
    const blob = new Blob([content], { type: "text/plain;charset=utf-8" });
    const url = URL.createObjectURL(blob);
    const link = document.createElement("a");
    const algoId = state.selectedAlgo || "resultado";

    link.href = url;
    link.download = global.RiegoConfig.export.filePrefix + algoId + ".txt";
    document.body.appendChild(link);
    link.click();
    document.body.removeChild(link);
    URL.revokeObjectURL(url);
  }

  global.RiegoExportService = {
    buildResultTxtContent,
    downloadResultTxt,
    formatCaseName
  };
})(window);
