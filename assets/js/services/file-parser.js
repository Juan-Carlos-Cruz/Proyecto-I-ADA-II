(function(global) {
  function readTextFile(file) {
    return new Promise((resolve, reject) => {
      const reader = new FileReader();

      reader.onload = event => resolve(event.target.result);
      reader.onerror = () => reject(new Error("No se pudo leer el archivo seleccionado."));

      reader.readAsText(file);
    });
  }

  function parseFarmFileContent(rawFileContent) {
    const normalized = rawFileContent.trim();

    if (!normalized) {
      throw new Error("El archivo esta vacio.");
    }

    const lines = normalized
      .split(/\r?\n/)
      .map(line => line.trim())
      .filter(Boolean);

    const totalPlots = parseInt(lines[0], 10);

    if (Number.isNaN(totalPlots) || totalPlots <= 0) {
      throw new Error("La primera linea debe indicar un numero valido de tablones.");
    }

    if (lines.length - 1 < totalPlots) {
      throw new Error("El archivo no contiene suficientes filas para todos los tablones.");
    }

    const finca = [];

    for (let index = 1; index <= totalPlots; index += 1) {
      const parts = lines[index].split(",").map(value => value.trim());

      if (parts.length !== 4) {
        throw new Error("La fila " + (index + 1) + " debe tener el formato ts,tr,p,rp.");
      }

      const [ts, tr, p, rp] = parts.map(value => parseInt(value, 10));

      if ([ts, tr, p, rp].some(Number.isNaN)) {
        throw new Error("La fila " + (index + 1) + " contiene valores no numericos.");
      }

      finca.push({ ts, tr, p, rp });
    }

    return {
      finca,
      rawFileContent: normalized
    };
  }

  global.RiegoFileService = {
    parseFarmFileContent,
    readTextFile
  };
})(window);
