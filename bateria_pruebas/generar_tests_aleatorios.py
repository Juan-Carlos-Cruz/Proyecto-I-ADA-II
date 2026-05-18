from __future__ import annotations
import random
from pathlib import Path

MIN_TABLONES = 5
MAX_TABLONES = 20
CASOS_POR_N  = 5000
SEED         = 20260513

BASE_DIR   = Path(__file__).resolve().parent
OUTPUT_DIR = BASE_DIR / "Tests" / "tests_aleatorios_por_n_5_27"
README_PATH= BASE_DIR / "Tests" / "tests_aleatorios_por_n_5_27_README.md"

def bounded_randint(rng: random.Random, low: int, high: int) -> int:
    if low > high: low, high = high, low
    return rng.randint(low, high)

def generar_tablon(rng: random.Random) -> tuple[int, int, int, int]:
    """
    Genera tablones compatibles con dos referencias al mismo tiempo:

    1. El enunciado:
       - formato ts,tr,p,rp
       - prioridad 1..4
       - 0 <= rp <= ts - tr

    2. La bateria manual en Tests/tests:
       - ts observado principalmente entre 5 y 15
       - existen casos borde con ts == tr
       - predominan holguras pequenas o medias, pero tambien hay casos relajados

    Aqui modelamos la holgura slack = ts - tr. Eso permite controlar mejor:
       - casos exactos ts == tr  -> slack = 0
       - urgencia real del tablon
       - validez de rp, que siempre se toma en 0..slack
    """
    perfil = rng.choices(
        population=["critico", "urgente", "balanceado", "relajado"],
        weights=[13, 35, 30, 22],
        k=1
    )[0]

    if perfil == "critico":
        ts = bounded_randint(rng, 5, 15)
        slack = 0                            # replica casos ts == tr de Tests/tests
        p = bounded_randint(rng, 1, 4)

    elif perfil == "urgente":
        ts = bounded_randint(rng, 5, 10)
        slack = bounded_randint(rng, 1, min(3, ts - 1))
        p = bounded_randint(rng, 3, 4)

    elif perfil == "balanceado":
        ts = bounded_randint(rng, 7, 13)
        slack = bounded_randint(rng, 2, min(6, ts - 1))
        p = bounded_randint(rng, 1, 4)

    else:  # relajado
        ts = bounded_randint(rng, 10, 15)
        slack = bounded_randint(rng, 7, min(13, ts - 1))
        p = bounded_randint(rng, 1, 2)

    tr = ts - slack
    rp = bounded_randint(rng, 0, slack)

    return ts, tr, p, rp

def generar_finca(rng: random.Random, n: int) -> str:
    tablones = [generar_tablon(rng) for _ in range(n)]
    lineas = [str(n)]
    for ts, tr, p, rp in tablones:
        lineas.append(f"{ts},{tr},{p},{rp}")
    return "\n".join(lineas) + "\n"

def escribir_readme() -> None:
    contenido = f"""# Tests aleatorios {MIN_TABLONES}..{MAX_TABLONES} tablones

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
- {MAX_TABLONES-MIN_TABLONES+1} carpetas: n{MIN_TABLONES:02d}/ hasta n{MAX_TABLONES:02d}/
- {CASOS_POR_N} archivos por carpeta
- Semilla fija: {SEED}
- Total: {(MAX_TABLONES-MIN_TABLONES+1)*CASOS_POR_N:,} archivos
"""
    README_PATH.parent.mkdir(parents=True, exist_ok=True)
    README_PATH.write_text(contenido, encoding="utf-8")

def main() -> None:
    rng = random.Random(SEED)
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

    total = 0
    for n in range(MIN_TABLONES, MAX_TABLONES + 1):
        carpeta_n = OUTPUT_DIR / f"n{n:02d}"
        carpeta_n.mkdir(parents=True, exist_ok=True)

        for indice in range(CASOS_POR_N):
            contenido = generar_finca(rng, n)
            nombre = f"rand_n{n:02d}_{indice+1:03d}.txt"
            (carpeta_n / nombre).write_text(contenido, encoding="utf-8")
            total += 1

        print(f"n={n:2d} → {CASOS_POR_N} archivos generados")

    escribir_readme()
    print(f"\nTotal: {total:,} archivos en {OUTPUT_DIR}")

if __name__ == "__main__":
    main()
