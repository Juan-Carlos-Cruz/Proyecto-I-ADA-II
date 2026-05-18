#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "algoritmos.hpp"
#include "placeholder_engine.hpp"
#include "voraz_criterios.hpp"

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "Uso: compare_voraz_modes <archivo1> [archivo2 ...]\n";
    return 1;
  }

  int mismatches = 0;
  long long total_ro_v = 0;
  long long total_ro_v_criterios = 0;

  for (int i = 1; i < argc; ++i) {
    const std::string path = argv[i];
    try {
      const std::string raw = [] (const std::string& p) {
        std::ifstream in(p);
        return std::string((std::istreambuf_iterator<char>(in)),
                           std::istreambuf_iterator<char>());
      }(path);

      const std::vector<riego::Tablon> tablones = riego::leer_tablones_desde_texto(raw);
      const riego::ResultadoCalculo actual = riego::roV(tablones);
      const riego::ResultadoCalculo criterios = riego::roVConCriterios(
          tablones,
          riego::CriterioVoraz::RatioRiegoPrioridad,
          riego::CriterioVoraz::Supervivencia);

      total_ro_v += actual.costo_total;
      total_ro_v_criterios += criterios.costo_total;

      if (actual.costo_total != criterios.costo_total ||
          actual.orden_riego != criterios.orden_riego) {
        ++mismatches;
        std::cout << path << "\n";
        std::cout << "  roV costo: " << actual.costo_total << "\n";
        std::cout << "  criterios costo: " << criterios.costo_total << "\n";
      }
    } catch (const std::exception& error) {
      std::cerr << "Error en " << path << ": " << error.what() << "\n";
      return 2;
    }
  }

  std::cout << "mismatches=" << mismatches << "\n";
  std::cout << "total_roV=" << total_ro_v << "\n";
  std::cout << "total_roVConCriterios=" << total_ro_v_criterios << "\n";
  return 0;
}
