#pragma once

#include <string>
#include <vector>

namespace riego {

// Datos de un tablon tal como llegan desde el archivo de entrada.
struct Tablon {
  int tiempo_supervivencia;
  int tiempo_riego;
  int prioridad;
  int dia_perfecto;
};

// Peticion que el frontend envia al backend.
struct EntradaCalculo {
  std::string algoritmo;
  std::string texto_finca;
};

// Informacion puntual de como se riega un tablon en la solucion.
struct DetalleRiego {
  int tablon;
  int dia_inicio;
  int caso_aplicado;
  long long costo_tablon;
};

// Resultado final que un algoritmo devuelve al frontend.
struct ResultadoCalculo {
  long long costo_total;
  std::vector<int> orden_riego;
  std::vector<DetalleRiego> detalles;
};

}  // namespace riego
