(function(global) {
  global.RiegoConfig = {
    api: {
      enabled: true,
      baseUrl: "http://127.0.0.1:8080",
      calculatePath: "/calcular",
      timeoutMs: 10000
    },
    export: {
      filePrefix: "salida_riego_"
    }
  };
})(window);
