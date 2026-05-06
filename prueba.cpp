// Compilar con:
// g++ -std=c++20 -Ibackend/include backend/src/algoritmos.cpp prueba.cpp -o prueba
// Ejecutar con:
// ./prueba

#include <iostream>
#include <vector>
#include "domain_types.hpp"
#include "algoritmos.hpp"
using namespace std;
using namespace riego;

void imprimir(const string& nombre, const ResultadoCalculo& R) {
    cout << "=== " << nombre << " ===" << endl;
    cout << "Costo total: " << R.costo_total << endl;
    cout << "Orden de riego: ";
    for (int i : R.orden_riego) cout << i << " ";
    cout << endl << endl;
}

int main() {
    // Finca de ejemplo (F1.txt)
    // Cada tablon: { ts, tr, p, rp }
    vector<Tablon> finca = {
        { 10, 3, 4, 0 },
        {  6, 3, 3, 1 },
        {  2, 2, 1, 0 },
        {  8, 1, 1, 6 },
        { 10, 4, 2, 6 },
    };

    imprimir("roFB", roFB(finca));
    imprimir("roV",  roV(finca));
    imprimir("roPD", roPD(finca));

    return 0;
}
