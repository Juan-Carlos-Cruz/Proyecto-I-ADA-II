# Proyecto I - ADA II

## Descripción

Este proyecto resuelve el problema de planificar el orden de riego de una finca compuesta por varios tablones. Cada tablón se modela con cuatro parámetros:

- `ts`: tiempo máximo de supervivencia.
- `tr`: tiempo de riego.
- `p`: prioridad o penalización por retraso.
- `rp`: día perfecto de inicio.

El sistema incluye:

- un `backend` en C++ con los algoritmos del proyecto;
- una interfaz `frontend` en HTML/CSS/JS;
- análisis individuales por finca;
- análisis comparativos sobre varias fincas;
- un laboratorio para comparar variantes del criterio voraz.

Los algoritmos disponibles en la interfaz son:

- `roFB`: fuerza bruta;
- `roV`: voraz;
- `roPD`: programación dinámica.

## Estructura principal

```text
.
├── backend/                          # Servidor HTTP y algoritmos en C++
├── assets/                           # JS, CSS y recursos del frontend
├── bateria_pruebas/                  # Casos de prueba del proyecto
│   ├── Tests/tests/                  # Entradas principales
│   ├── Tests/output/                 # Salidas de referencia
│   └── Tests-examples/               # Ejemplos del informe
├── docs/
│   ├── informe/                     # Informe en LaTeX y PDF compilado
│   └── presentacion/                # Presentacion resumida en LaTeX
├── riego_interfaz_frontend.html      # Interfaz principal
├── iniciar_proyecto                  # Lanzador recomendado
└── detener_proyecto                  # Detiene el backend lanzado
```

## Requisitos

Para usar el proyecto tal como está en el repositorio:

- `g++` con soporte de C++20;
- `make`;
- `python3`;
- un navegador web moderno.

En macOS y Linux, los scripts `iniciar_proyecto` y `detener_proyecto` funcionan directamente con `bash`.
En Windows nativo, el arranque recomendado usa `CMake` y un compilador C++20 compatible como `MSVC` o `MinGW-w64`.

## Cómo levantar y detener el backend

### macOS

Desde la raíz del proyecto:

```bash
./iniciar_proyecto
```

Eso hace lo siguiente:

- limpia y recompila el backend;
- levanta el servidor en `http://127.0.0.1:8080`;
- abre automáticamente [riego_interfaz_frontend.html](./riego_interfaz_frontend.html).

Si no quieres que abra el navegador:

```bash
./iniciar_proyecto --no-open
```

Para detenerlo:

```bash
./detener_proyecto
```

Si lo ejecutaste manualmente en primer plano, también puedes detenerlo con `Ctrl + C`.

### Linux

Desde la raíz del proyecto:

```bash
./iniciar_proyecto
```

O, si prefieres compilar y ejecutar el backend manualmente:

```bash
make backend
./backend/bin/riego_backend --port 8080
```

Para detener el backend lanzado con el script:

```bash
./detener_proyecto
```

Si lo levantaste manualmente en la terminal, detenlo con `Ctrl + C`.

### Windows

El repositorio ahora tiene dos rutas para Windows: `WSL` o ejecución nativa.

#### Windows con WSL

Desde la raíz del proyecto dentro de WSL:

```bash
./iniciar_proyecto
```

Para detenerlo:

```bash
./detener_proyecto
```

#### Windows nativo

Requisitos:

- `CMake`
- Visual Studio Build Tools o `MSYS2/MinGW-w64`
- PowerShell o `cmd`

Desde la raíz del proyecto:

En `cmd`:

```bat
iniciar_proyecto.bat
```

En PowerShell:

```powershell
.\iniciar_proyecto.ps1
```

Si no quieres que abra el navegador:

```powershell
.\iniciar_proyecto.ps1 --no-open
```

Para detenerlo:

En `cmd`:

```bat
detener_proyecto.bat
```

En PowerShell:

```powershell
.\detener_proyecto.ps1
```

Si prefieres compilar manualmente en Windows nativo:

```powershell
cmake -S . -B build
cmake --build build --config Release
.\backend\bin\riego_backend.exe --port 8080
```

Si trabajas en `Git Bash` o `MSYS2`, tambien puedes seguir usando `make backend`:

```bash
make backend
./backend/bin/riego_backend.exe --port 8080
```

Para detenerlo, usa `Ctrl + C` en la misma terminal.

## Frontend

El archivo principal de la interfaz es [riego_interfaz_frontend.html](./riego_interfaz_frontend.html).

Si usaste `./iniciar_proyecto`, normalmente se abre solo. Si no, puedes abrirlo manualmente en tu navegador.

La interfaz espera que el backend esté activo en:

```text
http://127.0.0.1:8080
```

Esa configuración está en [assets/js/config/app-config.js](./assets/js/config/app-config.js).

## Cómo usar el frontend para hacer análisis

### 1. Análisis de una finca

1. Abre la interfaz.
2. En `Paso 1`, carga un archivo `.txt`.
3. El formato esperado es:

```text
n
ts,tr,p,rp
ts,tr,p,rp
...
```

4. En `Paso 2`, revisa los tablones cargados.
5. Escoge uno de los algoritmos:
   - `Fuerza Bruta`
   - `Voraz`
   - `Programación Dinámica`
6. Pulsa `Calcular plan de riego`.
7. En `Paso 3`, revisa:
   - costo total;
   - permutación calculada;
   - detalle por tablón.

Este flujo sirve para analizar una finca puntual y comparar visualmente qué produce cada algoritmo.

### 2. Análisis comparativo con varias fincas

La pantalla `Análisis comparativo (varias fincas)` sirve para evaluar el comportamiento del proyecto sobre un conjunto de entradas.

Flujo:

1. En la pantalla inicial, entra a `Análisis comparativo (varias fincas)`.
2. Carga uno o más archivos `.txt`.
3. Pulsa `Ejecutar análisis comparativo`.
4. La interfaz consulta el backend y construye:
   - una tabla comparativa;
   - un gráfico;
   - un resumen promedio de diferencias.

En este análisis se comparan principalmente:

- costo óptimo con `Programación Dinámica`;
- costo del `Voraz`;
- costo de `Fuerza Bruta` cuando aplica.

Este bloque sirve para estudiar:

- cuánto se aleja el voraz del óptimo;
- en qué tamaños de finca aparecen más diferencias;
- qué tan frecuente es que el voraz coincida con el costo óptimo.

### 3. Laboratorio de variantes del criterio voraz

Después de ejecutar el análisis comparativo, se habilita un bloque adicional en la misma pantalla para comparar varias combinaciones del criterio voraz.

Flujo:

1. Carga varias fincas.
2. Ejecuta primero el análisis comparativo.
3. Luego pulsa `Ejecutar laboratorio Voraz`.
4. Revisa el ranking de combinaciones evaluadas.

Ese bloque permite analizar:

- cuál criterio principal produce menor diferencia promedio;
- qué desempates funcionan mejor;
- cuántas veces una variante coincide exactamente con el costo óptimo.

## Archivos útiles para probar la interfaz

Puedes cargar archivos de estas carpetas:

- [bateria_pruebas/Tests/tests](./bateria_pruebas/Tests/tests)
- [bateria_pruebas/Tests-examples/INPUT](./bateria_pruebas/Tests-examples/INPUT)

Si necesitas más entradas aleatorias:

```bash
python3 bateria_pruebas/generar_tests_aleatorios.py
```

## Logs y verificación rápida

Cuando el backend se levanta con `./iniciar_proyecto`, los logs quedan en:

- [backend/logs/backend.out.log](./backend/logs/backend.out.log)
- [backend/logs/backend.err.log](./backend/logs/backend.err.log)

El estado del backend se puede verificar en:

```text
http://127.0.0.1:8080/health
```

## Resumen rápido

- `./iniciar_proyecto`: compila, levanta backend y abre la interfaz.
- `./detener_proyecto`: detiene el backend lanzado por el proyecto.
- `iniciar_proyecto.bat` o `.\iniciar_proyecto.ps1`: arranque nativo en Windows.
- `detener_proyecto.bat` o `.\detener_proyecto.ps1`: detención nativa en Windows.
- `riego_interfaz_frontend.html`: interfaz para análisis individuales y comparativos.
- `bateria_pruebas/`: conjunto de entradas para pruebas y validación.
