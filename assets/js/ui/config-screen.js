(function(global) {
  function updateFarmMetrics() {
    const state = global.RiegoState;
    const totalTime = state.finca.reduce((sum, plot) => sum + plot.tr, 0);
    const survivalValues = state.finca.map(plot => plot.ts);
    const minSurvival = Math.min(...survivalValues);
    const maxSurvival = Math.max(...survivalValues);

    document.getElementById("farm-count").textContent = state.finca.length;
    document.getElementById("farm-total-time").textContent = totalTime + (totalTime === 1 ? " dia" : " dias");
    document.getElementById("farm-survival-range").textContent =
      minSurvival === maxSurvival ? "dia " + minSurvival : minSurvival + " a " + maxSurvival + " dias";
  }

  function buildTablonCard(plot, index) {
    return `<div class="tablon-card">
      <div class="tablon-card-top">
        <div class="tablon-header">Tablon T${index}</div>
        <div class="tablon-priority">P ${plot.p}</div>
      </div>
      <div class="tablon-stat"><span>Supervivencia</span><span>${plot.ts} dias</span></div>
      <div class="tablon-stat"><span>Tiempo de riego</span><span>${plot.tr} dias</span></div>
      <div class="tablon-stat"><span>Dia perfecto</span><span>${plot.rp}</span></div>
      <div class="tablon-stat"><span>Prioridad</span><span>${plot.p}</span></div>
    </div>`;
  }

  function renderConfig() {
    const state = global.RiegoState;
    document.getElementById("finca-info").textContent = state.finca.length + " tablones listos para evaluacion";

    updateFarmMetrics();

    const grid = document.getElementById("tablon-grid");
    grid.innerHTML = state.finca.map(buildTablonCard).join("");

    if (state.selectedAlgo) {
      selectAlgo(state.selectedAlgo);
    } else {
      document.getElementById("fb-warning").style.display = "none";
    }

    global.RiegoNavigation.goTo("config");
  }

  function selectAlgo(algo) {
    const state = global.RiegoState;
    state.selectedAlgo = algo;

    document.querySelectorAll(".algo-card").forEach(card => card.classList.remove("selected"));
    document.getElementById("algo-" + algo).classList.add("selected");
    document.getElementById("fb-warning").style.display =
      algo === "roFB" && state.finca.length > 12 ? "block" : "none";
  }

  global.RiegoConfigScreen = {
    updateFarmMetrics,
    renderConfig,
    selectAlgo
  };

  global.selectAlgo = selectAlgo;
})(window);
