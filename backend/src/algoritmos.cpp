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
    int n = (int)tablones.size();
    
    // Un "Bitmask" (Máscara de bits) usa un número entero como un array de booleanos.
    // Ejemplo si n=3: El estado 5 en binario es '101'.
    // Eso significa: "Ya regamos el tablón 0 y el tablón 2, pero no el tablón 1".
    // 1 << n es lo mismo que decir "2 elevado a la n".
    int total_estados = 1 << n;  

    const long long INF = LLONG_MAX / 2; // Infinito (dividido por 2 para evitar overflows al sumar)

    // Tablas de memoria para guardar lo que vamos descubriendo:
    vector<long long> costo_minimo(total_estados, INF);  // El camino más barato para llegar a un estado
    vector<int>  estado_anterior(total_estados, -1);     // Como "migas de pan" para saber por dónde llegamos
    vector<int>  ultimo_tablon_regado(total_estados, -1);// Qué tablón regamos justo antes de llegar a este estado
    vector<int>  tiempo_acumulado(total_estados, 0);     // Día actual en ese estado específico

    // Estado inicial: 0 en binario (000...). Ningún tablón regado = costo 0.
    costo_minimo[0] = 0;  

    // Iteramos por todos los estados posibles (desde 000... hasta 111...)
    for (int estado_actual = 0; estado_actual < total_estados; estado_actual++) {
        
        // Si el costo es infinito, significa que es una combinación imposible de alcanzar aún, la saltamos.
        if (costo_minimo[estado_actual] == INF) continue;

        int dia_inicio = tiempo_acumulado[estado_actual];

        // Desde nuestro estado actual, probamos regar CADA tablón disponible
        for (int i = 0; i < n; i++) {
            
            // Operador AND a nivel de bits (&):
            // (1 << i) crea un número con solo el bit 'i' encendido.
            // Si (estado_actual & (1 << i)) da verdadero, significa que el tablón 'i' YA fue regado.
            if (estado_actual & (1 << i)) {
                continue; // Ya está regado, saltamos al siguiente
            }

            // Operador OR a nivel de bits (|):
            // Encendemos el bit 'i' para crear el "futuro" estado tras regar este tablón.
            int siguiente_estado = estado_actual | (1 << i);
            
            long long costo_del_riego = calcular_costo(tablones[i], dia_inicio);
            long long costo_total_simulado = costo_minimo[estado_actual] + costo_del_riego;

            // Si descubrimos un camino más barato hacia ese 'siguiente_estado', lo actualizamos
            if (costo_total_simulado < costo_minimo[siguiente_estado]) {
                costo_minimo[siguiente_estado]       = costo_total_simulado;
                estado_anterior[siguiente_estado]    = estado_actual;
                ultimo_tablon_regado[siguiente_estado] = i;
                tiempo_acumulado[siguiente_estado]   = dia_inicio + tablones[i].tiempo_riego;
            }
        }
    }

    // Ya calculamos todo. Ahora toca reconstruir el mejor camino leyendo las "migas de pan".
    vector<int> orden_optimo;
    
    // Empezamos desde el final: el estado donde todos los bits están encendidos (todos regados)
    int estado_reconstruccion = total_estados - 1; 
    
    while (estado_reconstruccion != 0) { // Mientras no lleguemos al inicio
        orden_optimo.push_back(ultimo_tablon_regado[estado_reconstruccion]);
        estado_reconstruccion = estado_anterior[estado_reconstruccion]; // Retrocedemos un paso
    }
    
    // Como reconstruimos de fin a inicio, el arreglo está al revés. Lo volteamos.
    reverse(orden_optimo.begin(), orden_optimo.end());

    // Devolvemos el resultado empaquetado y detallado
    return evaluar_orden(tablones, orden_optimo);
}

}  // namespace riego
