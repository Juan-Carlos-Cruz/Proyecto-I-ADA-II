#include "algoritmos.hpp"
#include <algorithm>
#include <climits>
#include <vector>
using namespace std;

namespace riego {


static ResultadoCalculo armar_resultado( const vector<Tablon>& T,const vector<int>& perm){

// ──────────────────────────────────────────────────────────────
// Función auxiliar: calcular costo de una permutación
// ──────────────────────────────────────────────────────────────
   
    int tiempo_inicio_riego = 0;
    long long costo_acumulado = 0;
    vector<DetalleRiego> detalles;
    
      for (int idx : perm) {
        const Tablon& t = T[idx];
        int inicio = (int)tiempo_inicio_riego;
        int fin = inicio + t.tiempo_riego;
        
        int caso;
        long long costo_tablon;
        
        if (inicio == t.dia_perfecto) {
            caso = 1;
            costo_tablon = t.tiempo_supervivencia - fin;
        } else if (t.tiempo_supervivencia - t.tiempo_riego >= inicio) {
            caso = 2;
            costo_tablon = 2LL * (t.tiempo_supervivencia - fin);
        } else {
            caso = 3;
            costo_tablon = 2LL * t.prioridad * (fin - t.tiempo_supervivencia);
        }
        
        costo_acumulado  += costo_tablon;
        detalles.push_back({idx, inicio, caso, costo_tablon});
        tiempo_actual = fin;
    }
    
    return {costo_acumulado, perm, detalles};
}
 
    // int n= (int)T.size();
    //ResultadoCalculo res_final;
    //res_final.orden_riego = perm;   
    //return armar_resultado{};
}


// ── roFB: Fuerza bruta ───────────────────────────────────────────────────────
ResultadoCalculo roFB(const vector<Tablon>& T) {
    [[maybe_unused]]int n = (int)T.size();
    vector<int> perm(n);
    for (int i = 0; i < n; i++) perm[i] = i;
    
    ResultadoCalculo mejor;
    mejor.costo_acumulado = LLONG_MAX;
    
    do {
        ResultadoCalculo actual = armar_resultado(T, perm);
        if (actual.costo_acumulado < mejor.costo_acumulado) {
            mejor = actual;
        }
    } while (next_permutation(perm.begin(), perm.end()));
    
    return mejor;
}


// ── roV: Voraz ───────────────────────────────────────────────────────────────
ResultadoCalculo roV(const vector<Tablon>& T) {
    [[maybe_unused]] int n = (int)T.size();

    // TODO

    return ResultadoCalculo{};
}


// ── roPD: Programacion dinamica ──────────────────────────────────────────────
ResultadoCalculo roPD(const vector<Tablon>& T) {
    [[maybe_unused]] int n = (int)T.size();

    // TODO

    return ResultadoCalculo{};
}


}  // namespace riego
