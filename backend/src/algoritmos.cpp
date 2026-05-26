#include "algoritmos.hpp"
#include <algorithm>
#include <climits>
#include <numeric>
#include <vector>

using namespace std;

namespace riego {

// ─────────────────────────────────────────────────────────────────────────────
// Constantes para los casos de costo (Evitamos "números mágicos")
// ─────────────────────────────────────────────────────────────────────────────
const int CASO_DIA_PERFECTO = 1; // Se riega en su día ideal -> Costo mínimo.
const int CASO_A_TIEMPO     = 2; // Se riega antes de ser crítico -> Penalización leve.
const int CASO_TARDE        = 3; // Se riega cuando ya está muriendo -> Penalización grave.

// ── Helpers (Funciones Auxiliares) ───────────────────────────────────────────

static int detectar_caso(const Tablon& tablon, int dia_actual) {
    if (dia_actual == tablon.dia_perfecto) {
        return CASO_DIA_PERFECTO;
    }
    // Si el margen de vida es mayor o igual al día actual, estamos a tiempo.
    if (tablon.tiempo_supervivencia - tablon.tiempo_riego >= dia_actual) {
        return CASO_A_TIEMPO;
    }
    
    // Si no es perfecto ni a tiempo, ya llegamos tarde.
    return CASO_TARDE;
}

static long long calcular_costo(const Tablon& tablon, int dia_actual) {
    int caso = detectar_caso(tablon, dia_actual);
    
    // Usamos 2LL para obligar al compilador a usar matemáticas de 64 bits (long long)
    // Esto previene que los números colapsen (overflow) si los cálculos dan resultados enormes.
    switch (caso) {
        case CASO_DIA_PERFECTO: 
            return tablon.tiempo_supervivencia - dia_actual - tablon.tiempo_riego;
            
        case CASO_A_TIEMPO: 
            return 2LL * (tablon.tiempo_supervivencia - tablon.tiempo_riego - dia_actual);
            
        case CASO_TARDE: 
            return 2LL * tablon.prioridad * (dia_actual + tablon.tiempo_riego - tablon.tiempo_supervivencia);
    }
    return 0;
}

// Simula el riego en el orden dado y devuelve el resultado completo con detalles.
static ResultadoCalculo evaluar_orden(const vector<Tablon>& tablones, const vector<int>& orden_elegido) {
    ResultadoCalculo resultado;
    resultado.orden_riego = orden_elegido;
    resultado.costo_total = 0;

    int dia_actual = 0;
    for (int indice : orden_elegido) {
        const Tablon& tablon = tablones[indice];
        long long costo = calcular_costo(tablon, dia_actual);
        int caso = detectar_caso(tablon, dia_actual);

        resultado.detalles.push_back({indice, dia_actual, caso, costo});
        resultado.costo_total += costo;
        
        dia_actual += tablon.tiempo_riego; // El tiempo avanza lo que tardamos en regar
    }

    return resultado;
}

// Esta función es una versión "ligera" de evaluar_orden.
// Sólo calcula el costo total sin guardar el historial de detalles.
// Es crucial para Fuerza Bruta, porque crear arreglos y guardarlos millones de veces destruiría la memoria y el rendimiento.
static long long costo_total_de_orden(const vector<Tablon>& tablones, const vector<int>& orden_elegido) {
    int dia_actual = 0;
    long long costo_total = 0;
    
    for (int indice : orden_elegido) {
        costo_total += calcular_costo(tablones[indice], dia_actual);
        dia_actual += tablones[indice].tiempo_riego;
    }
    return costo_total;
}


// ── 1. roFB: Fuerza Bruta (Brute Force) ──────────────────────────────────────
// Prueba TODAS las combinaciones posibles y se queda con la mejor.
// Complejidad: O(n! × n) — ¡Peligro! Solo usable si hay muy pocos tablones (n <= 10 o 11).
ResultadoCalculo roFB(const vector<Tablon>& tablones) {
    int cantidad_tablones = (int)tablones.size();

    vector<int> orden_actual(cantidad_tablones);
    iota(orden_actual.begin(), orden_actual.end(), 0);  // Inicializa: [0, 1, 2, ..., n-1]

    long long mejor_costo = LLONG_MAX; // Empezamos con el costo infinito
    vector<int> mejor_orden;

    do {
        // Evaluamos el costo de esta combinación específica
        long long costo = costo_total_de_orden(tablones, orden_actual);
        
        // Si encontramos una combinación más barata, la guardamos
        if (costo < mejor_costo) {
            mejor_costo = costo;
            mejor_orden = orden_actual;
        }
        
    // std::next_permutation reordena el arreglo a la siguiente combinación matemática posible.
    // Devuelve 'false' cuando ya probó absolutamente todas las combinaciones.
    } while (next_permutation(orden_actual.begin(), orden_actual.end()));

    // Finalmente, evaluamos la mejor combinación completa para generar los detalles del reporte
    return evaluar_orden(tablones, mejor_orden);
}


// ── 2. roV: Voraz (Greedy) ───────────────────────────────────────────────────
// Toma decisiones rápidas buscando el beneficio inmediato más lógico.
// Complejidad: O(n log n) — Muy rápido, pero no garantiza el resultado absolutamente perfecto.

ResultadoCalculo roV(const vector<Tablon>& tablones) {
    int cantidad_tablones = (int)tablones.size();

    vector<int> orden(cantidad_tablones);
    iota(orden.begin(), orden.end(), 0);

    sort(orden.begin(), orden.end(), [&](int indice_a, int indice_b) {
        const Tablon& a = tablones[indice_a];
        const Tablon& b = tablones[indice_b];

        // MEJORA: Evitamos dividir usando doubles (float) porque puede generar errores de precisión.
        // Si queremos saber si (a.riego / a.prioridad) < (b.riego / b.prioridad)
        // Multiplicamos cruzado: a.riego * b.prioridad < b.riego * a.prioridad
        long long ratio_a = 1LL * a.tiempo_riego * b.prioridad;
        long long ratio_b = 1LL * b.tiempo_riego * a.prioridad;
        
        if (ratio_a != ratio_b) {
            return ratio_a < ratio_b;
        }
        
        // Desempate: El que tenga menor margen de maniobra (supervivencia - riego), va primero.
        // Esto evita cuellos de botella y minimiza penalizaciones en grupos grandes.
        const int margen_a = a.tiempo_supervivencia - a.tiempo_riego;
        const int margen_b = b.tiempo_supervivencia - b.tiempo_riego;
        if (margen_a != margen_b) {
            return margen_a < margen_b;
        }

        // Si empatan tambien aqui, conservamos el orden original para que el
        // laboratorio Voraz y roV produzcan exactamente la misma salida.
        return indice_a < indice_b;
    });

    return evaluar_orden(tablones, orden);
}


// ── 3. roPD: Programación Dinámica con Máscara de Bits (Bitmask) ─────────────
// Explora caminos de forma inteligente, recordando lo que ya calculó para no repetirlo.
// Complejidad: O(n² × 2ⁿ) tiempo — O(2ⁿ) espacio. Mucho mejor que Fuerza Bruta.
ResultadoCalculo roPD(const vector<Tablon>& tablones) {
    int n = tablones.size();
    int limite_estados = 1 << n; // 2^n estados posibles
    
    // Tabla DP inicializada en -1 (indica que el estado no ha sido calculado)
    vector<long long> dp(limite_estados, -1);
    
    // Tabla para reconstruir el camino óptimo (guarda qué tablón elegimos en cada estado)
    vector<int> elecciones(limite_estados, -1);

    // Función lambda recursiva con memoización
    // 'dia_actual' se pasa como parámetro para evitar recalculaciones costosas
    auto resolver_dp = [&](auto& self, int mascara, int dia_actual) -> long long {
        // Caso base: Si todos los bits están en 1, ya regamos todos los tablones.
        if (mascara == limite_estados - 1) {
            return 0;
        }

        // Si ya calculamos este subproblema, devolvemos el resultado guardado
        if (dp[mascara] != -1) {
            return dp[mascara];
        }

        long long mejor_costo = -1;
        int mejor_siguiente_tablon = -1;

        // Probar todos los tablones que NO han sido regados aún
        for (int i = 0; i < n; ++i) {
            if ((mascara & (1 << i)) == 0) { // Si el bit i está apagado (no regado)
                
                // Calculamos el costo de regar el tablón 'i' en el día actual
                long long costo_de_este = calcular_costo(tablones[i], dia_actual); 
                
                // Llamada recursiva sumando el tiempo de riego del tablón actual al día
                long long costo_del_resto = self(self, mascara | (1 << i), dia_actual + tablones[i].tiempo_riego); 
                long long costo_total_rama = costo_de_este + costo_del_resto;

                // Buscamos minimizar el costo total
                if (mejor_costo == -1 || costo_total_rama < mejor_costo) {
                    mejor_costo = costo_total_rama;
                    mejor_siguiente_tablon = i;
                }
            }
        }

        // Guardamos la decisión para poder reconstruir el orden al final
        elecciones[mascara] = mejor_siguiente_tablon;
        
        // Guardamos en memoria (memoización) y retornamos
        return dp[mascara] = mejor_costo;
    };

    // Ejecutamos el algoritmo DP comenzando desde el estado 0 (ningún tablón regado) en el día 0
    resolver_dp(resolver_dp, 0, 0);

    // Reconstrucción exclusiva del vector de orden óptimo (los índices elegidos)
    vector<int> orden_optimo;
    int mascara_actual = 0;
    
    while (mascara_actual != limite_estados - 1) {
        int siguiente = elecciones[mascara_actual];
        orden_optimo.push_back(siguiente);
        mascara_actual |= (1 << siguiente); // Avanzamos al siguiente estado
    }

    // Retornamos el struct completo llamando a tu función de simulación
    return evaluar_orden(tablones, orden_optimo);
}

}  // namespace riego
