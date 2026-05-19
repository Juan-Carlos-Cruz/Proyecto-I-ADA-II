# Tests aleatorios 20..20 tablones

Generados para comparar criterios voraces combinando:

- restricciones del enunciado
- patron de datos parecido a `Tests/tests`

## Reglas que siempre se cumplen
- ts: 5-15  (como la bateria manual)
- tr: 1-ts  (se permiten casos con `ts = tr`)
- p:  1-4   (prioridad)
- rp: 0-ts-tr (día perfecto)

## Perfiles de tablones
- critico (10%):    `ts = tr`        -> replica casos borde presentes en `Tests/tests`
- urgente (30%):    holgura 1-3      -> poca ventana antes del limite
- balanceado (25%): holgura 2-6      -> mezcla intermedia de casos
- relajado (20%):   holgura 7-13     -> tablones con margen amplio y cualquier prioridad (1-4)
- largo\_plazo (15%): ts entre 20-100  -> casos donde hay tiempo de sobra para regar a muchos a tiempo

## Notas
- Se usa `holgura = ts - tr` para que `rp` siempre quede en un rango valido.
- El generador no copia literalmente `Tests/tests`, pero si replica su estilo general.

## Estructura
- 1 carpetas: n20/ hasta n20/
- 5000 archivos por carpeta
- Semilla fija: 20260513
- Total: 5,000 archivos
