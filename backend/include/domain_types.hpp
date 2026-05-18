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

// Una fila de comparacion FB/DP/Voraz para un tamano de finca dado.
// costo_fb == -1 indica que la fuerza bruta no se ejecuto (n > FB_MAX_N).
struct FilaBenchmark {
  int n;
  long long costo_fb;
  long long costo_dp;
  long long costo_v;
  long long diff;
  double pct_diff;
  double tiempo_dp_s;
  double tiempo_v_s;
};

struct ResumenBenchmark {
  int total_fincas;
  double promedio_diff;
  double promedio_pct_diff;
  double porcentaje_exactos;
};

// Resultado completo de un analisis comparativo sobre varias fincas.
struct ResultadoBenchmark {
  std::vector<FilaBenchmark> filas;
  ResumenBenchmark resumen;
};

// Una fila de comparacion entre variantes del criterio voraz.
struct FilaBenchmarkVoraz {
  std::string criterio_principal;
  std::string criterio_desempate;
  double promedio_diff;
  double promedio_pct_diff;
  double promedio_tiempo_v_s;
  int coincidencias_optimo;
  int total_fincas;
  bool es_actual;
};

// Resultado agregado para elegir criterio principal y desempate en Voraz.
struct ResultadoBenchmarkVoraz {
  std::vector<FilaBenchmarkVoraz> filas;
};

}  // namespace riego
