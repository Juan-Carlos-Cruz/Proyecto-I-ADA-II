#include "voraz_criterios.hpp"
#include <algorithm>
#include <numeric>
#include <vector>

using namespace std;

namespace riego {

// Definimos constantes para evitar "números mágicos" (1, 2, 3) que son difíciles de entender.
const int CASO_DIA_PERFECTO = 1;
const int CASO_A_TIEMPO = 2;
const int CASO_TARDE = 3;

// ── Helpers (Funciones Auxiliares) ───────────────────────────────────────────

static int detectar_caso(const Tablon& tablon, int dia_actual) {
    if (dia_actual == tablon.dia_perfecto) {
        return CASO_DIA_PERFECTO;
    }
    // Si la diferencia entre lo que sobrevive y lo que tarda en regarse es mayor o igual al día actual, 
    // significa que llegamos a tiempo antes de que muera.
    if (tablon.tiempo_supervivencia - tablon.tiempo_riego >= dia_actual) {
        return CASO_A_TIEMPO;
    }
    
    // Si no es perfecto ni a tiempo, llegamos tarde.
    return CASO_TARDE;
}

static long long calcular_costo(const Tablon& tablon, int dia_actual) {
    int caso = detectar_caso(tablon, dia_actual);
    
    // Utilizamos 2LL para forzar que la multiplicación se haga en 64 bits (long long).
    // Esto previene errores de desbordamiento (overflow) si los números son muy grandes.
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

static ResultadoCalculo evaluar_orden(const vector<Tablon>& tablones, const vector<int>& orden_elegido) {
    ResultadoCalculo resultado;
    resultado.orden_riego = orden_elegido;
    resultado.costo_total = 0;
    
    int dia_actual = 0;
    
    // Iteramos sobre los índices en el orden en que decidimos regar los tablones
    for (int indice : orden_elegido) {
        const Tablon& tablon = tablones[indice];
        
        long long costo = calcular_costo(tablon, dia_actual);
        int caso = detectar_caso(tablon, dia_actual);
        
        resultado.detalles.push_back({indice, dia_actual, caso, costo});
        resultado.costo_total += costo;
        
        // Avanzamos el tiempo según lo que tardamos en regar este tablón
        dia_actual += tablon.tiempo_riego;
    }
    return resultado;
}

// ── Comparadores por criterio ────────────────────────────────────────────────

// Nota: Estas funciones devuelven 'true' si el tablon 'a' debe ir ANTES que el 'b'
// (Es decir, si 'a' es "menor" o "mejor" que 'b' según el criterio elegido).

static bool menor_ratio_riego_prioridad(const Tablon& a, const Tablon& b) {
    // Matemáticamente queremos comparar si (a.tiempo_riego / a.prioridad) < (b.tiempo_riego / b.prioridad)
    // Para evitar problemas de división por cero y pérdida de decimales, multiplicamos en cruz.
    // Usamos 1LL para convertir a long long y evitar desbordamiento (overflow) de enteros.
    long long ratio_a = 1LL * a.tiempo_riego * b.prioridad;
    long long ratio_b = 1LL * b.tiempo_riego * a.prioridad;
    
    return ratio_a < ratio_b;
}

static bool menor_limite_inicio(const Tablon& a, const Tablon& b) {
    // El que tiene menos margen de tiempo antes de morir, va primero.
    int margen_a = a.tiempo_supervivencia - a.tiempo_riego;
    int margen_b = b.tiempo_supervivencia - b.tiempo_riego;
    return margen_a < margen_b;
}

static bool menor_dia_perfecto(const Tablon& a, const Tablon& b) {
    return a.dia_perfecto < b.dia_perfecto;
}

static bool menor_supervivencia(const Tablon& a, const Tablon& b) {
    return a.tiempo_supervivencia < b.tiempo_supervivencia;
}

static bool mayor_prioridad(const Tablon& a, const Tablon& b) {
    return a.prioridad > b.prioridad;  // IMPORTANTE: Mayor prioridad va primero, por eso usamos '>'
}

// Ejecuta la función comparadora correspondiente al enumerador seleccionado
static bool aplicar_criterio(const Tablon& a, const Tablon& b, CriterioVoraz criterio) {
    switch (criterio) {
        case CriterioVoraz::RatioRiegoPrioridad: return menor_ratio_riego_prioridad(a, b);
        case CriterioVoraz::LimiteInicio:        return menor_limite_inicio(a, b);
        case CriterioVoraz::DiaPerfecto:         return menor_dia_perfecto(a, b);
        case CriterioVoraz::Supervivencia:       return menor_supervivencia(a, b);
        case CriterioVoraz::PrioridadDesc:       return mayor_prioridad(a, b);
    }
    return false; // Por seguridad
}

// Dos tablones empatan si 'a' no es mejor que 'b', y 'b' tampoco es mejor que 'a'
static bool empatan(const Tablon& a, const Tablon& b, CriterioVoraz criterio) {
    bool a_gana_a_b = aplicar_criterio(a, b, criterio);
    bool b_gana_a_a = aplicar_criterio(b, a, criterio);
    
    return !a_gana_a_b && !b_gana_a_a;
}

// ── API pública ──────────────────────────────────────────────────────────────

const char* nombre_criterio_voraz(CriterioVoraz criterio) {
    switch (criterio) {
        case CriterioVoraz::RatioRiegoPrioridad: return "tr / prioridad";
        case CriterioVoraz::LimiteInicio:        return "ts - tr";
        case CriterioVoraz::DiaPerfecto:         return "dia perfecto";
        case CriterioVoraz::Supervivencia:       return "supervivencia";
        case CriterioVoraz::PrioridadDesc:       return "prioridad alta";
    }
    return "desconocido";
}

ResultadoCalculo roVConCriterios(const vector<Tablon>& tablones,
                                 CriterioVoraz criterio_principal,
                                 CriterioVoraz criterio_desempate) {
    int cantidad_tablones = (int)tablones.size();

    // Creamos un arreglo con los índices: [0, 1, 2, ..., n-1]
    vector<int> orden_indices(cantidad_tablones);
    
    // std::iota rellena el arreglo secuencialmente (0, 1, 2, 3...)
    iota(orden_indices.begin(), orden_indices.end(), 0);

    // Ordenamos los ÍNDICES (no los tablones directamente) usando una función lambda
    sort(orden_indices.begin(), orden_indices.end(), [&](int indice_a, int indice_b) {
        const Tablon& tablon_a = tablones[indice_a];
        const Tablon& tablon_b = tablones[indice_b];

        // PASO 1: Intentamos decidir usando el criterio principal
        if (!empatan(tablon_a, tablon_b, criterio_principal)) {
            return aplicar_criterio(tablon_a, tablon_b, criterio_principal);
        }

        // PASO 2: Si hay empate principal, usamos el criterio de desempate
        if (!empatan(tablon_a, tablon_b, criterio_desempate)) {
            return aplicar_criterio(tablon_a, tablon_b, criterio_desempate);
        }

        // PASO 3: Si empatan en absolutamente todo, mantenemos el orden original
        // El que tenga el índice menor va primero (esto garantiza un ordenamiento estable)
        return indice_a < indice_b;
    });

    return evaluar_orden(tablones, orden_indices);
}

}  // namespace riego