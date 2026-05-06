(function(global) {
  function isEnabled() {
    return Boolean(global.RiegoConfig.api.enabled);
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

  global.RiegoApiClient = {
    isEnabled,
    calculatePlan
  };
})(window);
