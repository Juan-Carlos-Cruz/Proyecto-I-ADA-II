(function(global) {
  global.RiegoConfig = {
    api: {
      enabled: true,
      baseUrl: "http://127.0.0.1:8080",
      calculatePath: "/calcular",
      benchmarkPath: "/benchmark",
      benchmarkGreedyPath: "/benchmark-voraz",
      timeoutMs: 10000,
      benchmarkTimeoutMs: 1800000
    },
    export: {
      filePrefix: "salida_riego_"
    }
  };
})(window);
