import argparse
import json
import random
import sys
import urllib.request


PROFILE_RULES = {
    "critico": {
        "ts": (5, 15),
        "slack": lambda ts: (0, 0),
        "p": (1, 4),
    },
    "urgente": {
        "ts": (5, 10),
        "slack": lambda ts: (1, min(3, ts - 1)),
        "p": (3, 4),
    },
    "balanceado": {
        "ts": (7, 13),
        "slack": lambda ts: (2, min(6, ts - 1)),
        "p": (1, 4),
    },
    "relajado": {
        "ts": (10, 15),
        "slack": lambda ts: (7, min(13, ts - 1)),
        "p": (1, 4),
    },
    "largo_plazo": {
        "ts": (20, 100),
        "slack": lambda ts: (10, min(50, ts - 1)),
        "p": (1, 4),
    },
}

DEFAULT_PROFILES = list(PROFILE_RULES.keys())


def parse_args():
    parser = argparse.ArgumentParser(
        description="Ejecuta el benchmark del voraz actual contra PD para perfiles controlados."
    )
    parser.add_argument("--n", type=int, default=20, help="Cantidad de tablones por finca.")
    parser.add_argument(
        "--cases-per-profile",
        type=int,
        default=5000,
        help="Cantidad de fincas a generar por perfil.",
    )
    parser.add_argument(
        "--chunk-size",
        type=int,
        default=500,
        help="Cantidad de fincas enviadas al backend por lote.",
    )
    parser.add_argument("--seed", type=int, default=20260513, help="Semilla del generador.")
    parser.add_argument(
        "--base-url",
        default="http://127.0.0.1:8080",
        help="URL base del backend.",
    )
    parser.add_argument(
        "--profiles",
        nargs="+",
        default=DEFAULT_PROFILES,
        choices=DEFAULT_PROFILES,
        help="Perfiles a evaluar.",
    )
    return parser.parse_args()


def bounded_randint(rng, low, high):
    if low > high:
        low, high = high, low
    return rng.randint(low, high)


def generar_tablon_perfil(rng, perfil):
    rules = PROFILE_RULES[perfil]
    ts = bounded_randint(rng, *rules["ts"])
    slack = bounded_randint(rng, *rules["slack"](ts))
    prioridad = bounded_randint(rng, *rules["p"])
    tr = ts - slack
    rp = bounded_randint(rng, 0, slack)
    return ts, tr, prioridad, rp


def generar_finca(rng, n, perfil):
    tablones = [generar_tablon_perfil(rng, perfil) for _ in range(n)]
    lineas = [str(n)]
    for ts, tr, prioridad, rp in tablones:
        lineas.append(f"{ts},{tr},{prioridad},{rp}")
    return "\n".join(lineas) + "\n"


def ejecutar_chunk(base_url, fincas):
    payload = json.dumps({"fincas": fincas}).encode("utf-8")
    request = urllib.request.Request(
        f"{base_url.rstrip('/')}/benchmark",
        data=payload,
        headers={"Content-Type": "application/json"},
        method="POST",
    )

    with urllib.request.urlopen(request, timeout=3600) as response:
        return json.loads(response.read().decode("utf-8"))


def resumir_perfil(base_url, rng, n, perfil, cases_per_profile, chunk_size):
    exactas = 0
    suma_diff = 0.0
    suma_pct_diff = 0.0

    for inicio in range(0, cases_per_profile, chunk_size):
        chunk_actual = min(chunk_size, cases_per_profile - inicio)
        fincas = [generar_finca(rng, n, perfil) for _ in range(chunk_actual)]
        body = ejecutar_chunk(base_url, fincas)

        resumen = body["resumen"]
        filas = body["filas"]

        exactas += sum(1 for fila in filas if fila["diff"] == 0)
        suma_diff += resumen["promedio_diff"] * chunk_actual
        suma_pct_diff += resumen["promedio_pct_diff"] * chunk_actual

    return {
        "perfil": perfil,
        "promedio_diff": suma_diff / cases_per_profile,
        "promedio_pct_diff": suma_pct_diff / cases_per_profile,
        "exactas": exactas,
        "total": cases_per_profile,
    }


def validar_argumentos(args):
    if args.n <= 0:
        raise ValueError("--n debe ser positivo.")
    if args.cases_per_profile <= 0:
        raise ValueError("--cases-per-profile debe ser positivo.")
    if args.chunk_size <= 0:
        raise ValueError("--chunk-size debe ser positivo.")


def main():
    args = parse_args()

    try:
        validar_argumentos(args)
    except ValueError as error:
        print(f"Error: {error}", file=sys.stderr)
        return 1

    rng = random.Random(args.seed)

    print(f"{'Perfil':<15} | {'Diff Prom.':<10} | {'% Diff':<8} | {'Exactas'}")
    sys.stdout.flush()

    for perfil in args.profiles:
        try:
            resultado = resumir_perfil(
                args.base_url,
                rng,
                args.n,
                perfil,
                args.cases_per_profile,
                args.chunk_size,
            )
        except Exception as error:
            print(f"Error en el perfil {perfil}: {error}", file=sys.stderr)
            return 1

        print(
            f"{resultado['perfil']:<15} | "
            f"{resultado['promedio_diff']:<10.2f} | "
            f"{resultado['promedio_pct_diff']:>6.2f}% | "
            f"{resultado['exactas']}/{resultado['total']}"
        )
        sys.stdout.flush()

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
