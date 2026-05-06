#include "algoritmos.hpp"
#include <algorithm>
#include <climits>
#include <vector>
using namespace std;

namespace riego {


static ResultadoCalculo armar_resultado(vector<Tablon>& T, vector<int>& perm){
    
    int n= (int)T.size();


    ResultadoCalculo res_final;
    res_final.orden_riego = perm;



    int tiempo_inicio_riego = 0;
    long long costo_acumulado = 0;
    
    
    
    return armar_resultado{};
}


// ── roFB: Fuerza bruta ───────────────────────────────────────────────────────
ResultadoCalculo roFB(const vector<Tablon>& T) {
    [[maybe_unused]] int n = (int)T.size();

    // TODO

    return ResultadoCalculo{};
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
