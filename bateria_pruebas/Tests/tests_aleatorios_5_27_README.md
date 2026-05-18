# Tests aleatorios 5..27 tablones

Este conjunto fue generado para comparar criterios voraces con fincas sinteticas
pero coherentes con la naturaleza del problema.

Reglas usadas:

- 1000 archivos `.txt`.
- Numero de tablones entre 5 y 27.
- Cada fila sigue `ts,tr,p,rp`.
- La prioridad `p` siempre queda entre 1 y 4.
- Siempre se cumple `ts >= tr` y `0 <= rp <= ts - tr`.
- Se mezclan perfiles urgentes, balanceados, relajados y de baja prioridad.
- La semilla es fija (`20260513`) para reproducibilidad.

La carpeta generada es `Tests/tests_aleatorios_5_27`.
