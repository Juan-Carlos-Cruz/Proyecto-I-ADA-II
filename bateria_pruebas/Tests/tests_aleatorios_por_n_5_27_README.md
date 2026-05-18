# Tests aleatorios 5..20 tablones

Generados para comparar criterios voraces combinando:

- restricciones del enunciado
- patron de datos parecido a `Tests/tests`

## Reglas que siempre se cumplen
- ts: 5-15  (como la bateria manual)
- tr: 1-ts  (se permiten casos con `ts = tr`)
- p:  1-4   (prioridad)
- rp: 0-ts-tr (día perfecto)

## Perfiles de tablones
- critico (13%):    `ts = tr`        -> replica casos borde presentes en `Tests/tests`
- urgente (35%):    holgura 1-3      -> poca ventana antes del limite
- balanceado (30%): holgura 2-6      -> mezcla intermedia de casos
- relajado (22%):   holgura 7-13     -> tablones con margen amplio

## Notas
- Se usa `holgura = ts - tr` para que `rp` siempre quede en un rango valido.
- El generador no copia literalmente `Tests/tests`, pero si replica su estilo general.

## Estructura
- 16 carpetas: n05/ hasta n20/
- 5000 archivos por carpeta
- Semilla fija: 20260513
- Total: 80,000 archivos
