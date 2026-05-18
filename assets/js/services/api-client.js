(function(global) {
  function toNumber(value, fallback = 0) {
    return Number.isFinite(value) ? value : fallback;
  }

  function toOptionalNumber(value) {
    return Number.isFinite(value) ? value : null;
  }

  function toText(value, fallback = "") {
    return typeof value === "string" ? value : fallback;
  }

  function buildTimeoutMessage(context, timeoutMs) {
    const minutes = timeoutMs >= 60000
      ? `${(timeoutMs / 60000).toFixed(timeoutMs % 60000 === 0 ? 0 : 1)} min`
      : `${Math.round(timeoutMs / 1000)} s`;
    return `${context} excedio el tiempo limite (${minutes}). Reduce la cantidad de fincas o usa un conjunto mas pequeno.`;
  }

  function isEnabled() {
    return Boolean(global.RiegoConfig.api.enabled);
  }

  function createTimedController(timeoutMs, externalSignal) {
    const controller = new AbortController();
    let abortReason = null;
    let detachExternalAbort = () => {};

    const markAndAbort = (reason) => {
      if (abortReason) return;
      abortReason = reason;
      controller.abort();
    };

    const timeoutId = setTimeout(() => markAndAbort("timeout"), timeoutMs);

    if (externalSignal) {
      if (externalSignal.aborted) {
        markAndAbort("cancelled");
      } else {
        const handleExternalAbort = () => markAndAbort("cancelled");
        externalSignal.addEventListener("abort", handleExternalAbort, { once: true });
        detachExternalAbort = () => {
          externalSignal.removeEventListener("abort", handleExternalAbort);
        };
      }
    }

    return {
      controller,
      getAbortReason() {
        return abortReason;
      },
      cleanup() {
        clearTimeout(timeoutId);
        detachExternalAbort();
      }
    };
  }

  async function calculatePlan(payload) {
    if (!isEnabled()) {
      throw new Error("BACKEND_DISABLED");
    }

    const { baseUrl, calculatePath, timeoutMs } = global.RiegoConfig.api;
    const controller = new AbortController();
    const timeoutId = setTimeout(() => controller.abort(), timeoutMs);

    try {
      const response = await fetch(new URL(calculatePath, baseUrl).toString(), {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(payload),
        signal: controller.signal
      });

      if (!response.ok) {
        throw new Error("El backend respondio con estado " + response.status + ".");
      }

      const data = await response.json();

      return {
        costo: data.costo ?? "-",
        permutacion: Array.isArray(data.permutacion) ? data.permutacion : [],
        detalles: Array.isArray(data.detalles) ? data.detalles : []
      };
    } finally {
      clearTimeout(timeoutId);
    }
  }

  async function runBenchmark(fincas, options = {}) {
    if (!isEnabled()) {
      throw new Error("BACKEND_DISABLED");
    }

    const { baseUrl, benchmarkPath, benchmarkTimeoutMs } = global.RiegoConfig.api;
    const request = createTimedController(benchmarkTimeoutMs, options.signal);

    try {
      const response = await fetch(new URL(benchmarkPath, baseUrl).toString(), {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ fincas }),
        signal: request.controller.signal
      });

      if (!response.ok) {
        const err = await response.json().catch(() => ({}));
        throw new Error(err.error || "El backend respondio con estado " + response.status);
      }

      const data = await response.json();
      const filas = Array.isArray(data.filas) ? data.filas.map(row => ({
        n: toNumber(row.n),
        costo_fb: toNumber(row.costo_fb, -1),
        costo_dp: toNumber(row.costo_dp),
        costo_v: toNumber(row.costo_v),
        diff: toNumber(row.diff),
        pct_diff: toNumber(row.pct_diff),
        tiempo_dp_s: toOptionalNumber(row.tiempo_dp_s),
        tiempo_v_s: toOptionalNumber(row.tiempo_v_s)
      })) : [];

      const resumen = data && typeof data.resumen === "object" && data.resumen !== null
        ? {
            total_fincas: toNumber(data.resumen.total_fincas),
            promedio_diff: toNumber(data.resumen.promedio_diff),
            promedio_pct_diff: toNumber(data.resumen.promedio_pct_diff),
            porcentaje_exactos: toNumber(data.resumen.porcentaje_exactos)
          }
        : null;

      return { filas, resumen };
    } catch (error) {
      if (error && error.name === "AbortError") {
        if (request.getAbortReason() === "cancelled") {
          throw new Error("REQUEST_CANCELLED");
        }
        throw new Error(buildTimeoutMessage("El benchmark comparativo", benchmarkTimeoutMs));
      }
      throw error;
    } finally {
      request.cleanup();
    }
  }

  async function runGreedyBenchmark(fincas, options = {}) {
    if (!isEnabled()) {
      throw new Error("BACKEND_DISABLED");
    }

    const { baseUrl, benchmarkGreedyPath, benchmarkTimeoutMs } = global.RiegoConfig.api;
    const request = createTimedController(benchmarkTimeoutMs, options.signal);

    try {
      const response = await fetch(new URL(benchmarkGreedyPath, baseUrl).toString(), {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ fincas }),
        signal: request.controller.signal
      });

      if (!response.ok) {
        const err = await response.json().catch(() => ({}));
        throw new Error(err.error || "El backend respondio con estado " + response.status);
      }

      const data = await response.json();
      if (!Array.isArray(data.filas)) return [];

      return data.filas.map(row => ({
        criterio_principal: toText(row.criterio_principal, "-"),
        criterio_desempate: toText(row.criterio_desempate, "-"),
        promedio_diff: toNumber(row.promedio_diff),
        promedio_pct_diff: toNumber(row.promedio_pct_diff),
        promedio_tiempo_v_s: toOptionalNumber(row.promedio_tiempo_v_s),
        coincidencias_optimo: toNumber(row.coincidencias_optimo),
        total_fincas: toNumber(row.total_fincas),
        es_actual: row.es_actual === true
      }));
    } catch (error) {
      if (error && error.name === "AbortError") {
        if (request.getAbortReason() === "cancelled") {
          throw new Error("REQUEST_CANCELLED");
        }
        throw new Error(buildTimeoutMessage("El laboratorio Voraz", benchmarkTimeoutMs));
      }
      throw error;
    } finally {
      request.cleanup();
    }
  }

  global.RiegoApiClient = {
    isEnabled,
    calculatePlan,
    runBenchmark,
    runGreedyBenchmark
  };
})(window);
