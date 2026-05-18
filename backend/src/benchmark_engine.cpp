#include "benchmark_engine.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "algoritmos.hpp"
#include "json_utils.hpp"
#include "placeholder_engine.hpp"
#include "voraz_criterios.hpp"

namespace {

constexpr std::size_t kBytesPorEstadoPD = sizeof(long long) + 3ULL * sizeof(int);
constexpr std::size_t kBudgetMemoriaBenchmarkBytes = 1024ULL * 1024ULL * 1024ULL;
constexpr unsigned int kHardwareConcurrencyFallback = 2;
constexpr int kFBMaxN = 10;

struct VarianteVorazConfig {
  riego::CriterioVoraz principal;
  riego::CriterioVoraz desempate;
  bool es_actual;
};

int leer_cantidad_tablones_desde_texto(const std::string& texto_finca) {
  const std::vector<riego::Tablon> tablones = riego::leer_tablones_desde_texto(texto_finca);
  return static_cast<int>(tablones.size());
}

std::size_t estimar_memoria_ro_pd_bytes(int n) {
  if (n < 0 || n >= 63) {
    return std::numeric_limits<std::size_t>::max();
  }

  const unsigned long long estados = 1ULL << n;
  const unsigned long long bytes = estados * kBytesPorEstadoPD + 4096ULL;

  if (bytes > std::numeric_limits<std::size_t>::max()) {
    return std::numeric_limits<std::size_t>::max();
  }

  return static_cast<std::size_t>(bytes);
}

std::size_t calcular_workers_benchmark(const std::vector<std::string>& fincas) {
  if (fincas.empty()) {
    return 0;
  }

  const std::size_t total_fincas = fincas.size();
  const unsigned int hardware = std::thread::hardware_concurrency();
  std::size_t workers = std::min<std::size_t>(
      total_fincas, hardware == 0 ? kHardwareConcurrencyFallback : hardware);

  std::size_t max_memoria_por_finca = 0;
  for (const auto& texto : fincas) {
    const int n = leer_cantidad_tablones_desde_texto(texto);
    max_memoria_por_finca =
        std::max(max_memoria_por_finca, estimar_memoria_ro_pd_bytes(n));
  }

  if (max_memoria_por_finca == 0 ||
      max_memoria_por_finca == std::numeric_limits<std::size_t>::max()) {
    return 1;
  }

  const std::size_t workers_por_memoria =
      std::max<std::size_t>(1, kBudgetMemoriaBenchmarkBytes / max_memoria_por_finca);

  return std::max<std::size_t>(1, std::min(workers, workers_por_memoria));
}

template <typename WorkerFn>
void ejecutar_trabajos_en_paralelo(std::size_t total_tareas,
                                   std::size_t workers,
                                   WorkerFn worker_fn) {
  if (total_tareas == 0) {
    return;
  }

  workers = std::max<std::size_t>(1, std::min(workers, total_tareas));

  if (workers == 1) {
    for (std::size_t indice = 0; indice < total_tareas; ++indice) {
      worker_fn(indice, 0);
    }
    return;
  }

  std::atomic<std::size_t> siguiente{0};
  std::atomic<bool> cancelar{false};
  std::exception_ptr primer_error;
  std::mutex error_mutex;
  std::vector<std::thread> hilos;
  hilos.reserve(workers);

  for (std::size_t worker_id = 0; worker_id < workers; ++worker_id) {
    hilos.emplace_back([&, worker_id]() {
      while (!cancelar.load()) {
        const std::size_t indice = siguiente.fetch_add(1);
        if (indice >= total_tareas) {
          break;
        }

        try {
          worker_fn(indice, worker_id);
        } catch (...) {
          cancelar.store(true);
          std::lock_guard<std::mutex> lock(error_mutex);
          if (!primer_error) {
            primer_error = std::current_exception();
          }
          break;
        }
      }
    });
  }

  for (auto& hilo : hilos) {
    hilo.join();
  }

  if (primer_error) {
    std::rethrow_exception(primer_error);
  }
}

const std::vector<VarianteVorazConfig>& obtener_variantes_voraces() {
  static const std::vector<VarianteVorazConfig> variantes = [] {
    const std::vector<riego::CriterioVoraz> criterios = {
        riego::CriterioVoraz::RatioRiegoPrioridad,
        riego::CriterioVoraz::LimiteInicio,
        riego::CriterioVoraz::DiaPerfecto,
        riego::CriterioVoraz::Supervivencia,
        riego::CriterioVoraz::PrioridadDesc,
    };

    std::vector<VarianteVorazConfig> combinaciones;
    for (const auto principal : criterios) {
      for (const auto desempate : criterios) {
        if (principal == desempate) continue;
        combinaciones.push_back(VarianteVorazConfig{
            .principal = principal,
            .desempate = desempate,
            .es_actual = principal == riego::CriterioVoraz::RatioRiegoPrioridad &&
                         desempate == riego::CriterioVoraz::LimiteInicio,
        });
      }
    }
    return combinaciones;
  }();

  return variantes;
}

}  // namespace

namespace riego {

ResultadoBenchmark ejecutar_benchmark(const std::string& json_body) {
  const std::vector<std::string> fincas =
      extraer_campo_json_array_textos(json_body, "fincas");

  if (fincas.empty()) {
    throw std::runtime_error("El campo 'fincas' debe ser un arreglo no vacio de textos.");
  }

  std::vector<FilaBenchmark> filas(fincas.size());
  const std::size_t workers = calcular_workers_benchmark(fincas);

  ejecutar_trabajos_en_paralelo(fincas.size(), workers, [&](std::size_t indice, std::size_t) {
    const auto& texto = fincas[indice];
    const std::vector<Tablon> tablones = leer_tablones_desde_texto(texto);
    const int n = static_cast<int>(tablones.size());

    const auto t0 = std::chrono::high_resolution_clock::now();
    const ResultadoCalculo res_dp = roPD(tablones);
    const auto t1 = std::chrono::high_resolution_clock::now();
    const double tiempo_dp_s = std::chrono::duration<double>(t1 - t0).count();

    const auto t2 = std::chrono::high_resolution_clock::now();
    const ResultadoCalculo res_v = roV(tablones);
    const auto t3 = std::chrono::high_resolution_clock::now();
    const double tiempo_v_s = std::chrono::duration<double>(t3 - t2).count();

    const long long costo_fb = (n <= kFBMaxN) ? roFB(tablones).costo_total : -1LL;
    const long long costo_dp = res_dp.costo_total;
    const long long costo_v = res_v.costo_total;
    const long long diff = (costo_v >= costo_dp) ? (costo_v - costo_dp)
                                                 : (costo_dp - costo_v);
    const double pct_diff = (costo_dp > 0)
        ? (100.0 * static_cast<double>(diff) / static_cast<double>(costo_dp))
        : 0.0;

    filas[indice] = FilaBenchmark{
        .n = n,
        .costo_fb = costo_fb,
        .costo_dp = costo_dp,
        .costo_v = costo_v,
        .diff = diff,
        .pct_diff = pct_diff,
        .tiempo_dp_s = tiempo_dp_s,
        .tiempo_v_s = tiempo_v_s,
    };
  });

  std::sort(filas.begin(), filas.end(),
            [](const FilaBenchmark& a, const FilaBenchmark& b) { return a.n < b.n; });

  double suma_diff = 0.0;
  double suma_pct_diff = 0.0;
  int coincidencias_exactas = 0;
  for (const auto& fila : filas) {
    suma_diff += static_cast<double>(fila.diff);
    suma_pct_diff += fila.pct_diff;
    coincidencias_exactas += (fila.diff == 0) ? 1 : 0;
  }

  const double total = static_cast<double>(filas.size());
  return ResultadoBenchmark{
      .filas = filas,
      .resumen =
          {
              .total_fincas = static_cast<int>(filas.size()),
              .promedio_diff = (total > 0.0) ? (suma_diff / total) : 0.0,
              .promedio_pct_diff = (total > 0.0) ? (suma_pct_diff / total) : 0.0,
              .porcentaje_exactos =
                  (total > 0.0) ? (100.0 * static_cast<double>(coincidencias_exactas) / total)
                                : 0.0,
          },
  };
}

ResultadoBenchmarkVoraz ejecutar_benchmark_voraz(const std::string& json_body) {
  const std::vector<std::string> fincas =
      extraer_campo_json_array_textos(json_body, "fincas");

  if (fincas.empty()) {
    throw std::runtime_error("El campo 'fincas' debe ser un arreglo no vacio de textos.");
  }

  struct AcumuladoVariante {
    VarianteVorazConfig config;
    double suma_diff = 0.0;
    double suma_pct_diff = 0.0;
    double suma_tiempo_v_s = 0.0;
    int coincidencias_optimo = 0;
    int total_fincas = 0;
  };

  const auto& variantes = obtener_variantes_voraces();
  const std::size_t workers = calcular_workers_benchmark(fincas);
  std::vector<std::vector<AcumuladoVariante>> acumulados_por_worker(
      workers, std::vector<AcumuladoVariante>(variantes.size()));

  for (auto& acumulados : acumulados_por_worker) {
    for (std::size_t indice = 0; indice < variantes.size(); ++indice) {
      acumulados[indice].config = variantes[indice];
    }
  }

  ejecutar_trabajos_en_paralelo(fincas.size(), workers, [&](std::size_t indice_finca,
                                                             std::size_t worker_id) {
    const auto& texto = fincas[indice_finca];
    auto& acumulados = acumulados_por_worker[worker_id];
    const std::vector<Tablon> tablones = leer_tablones_desde_texto(texto);
    const ResultadoCalculo res_dp = roPD(tablones);
    const long long costo_dp = res_dp.costo_total;

    for (auto& acumulado : acumulados) {
      const auto t0 = std::chrono::high_resolution_clock::now();
      const ResultadoCalculo res_v = roVConCriterios(
          tablones, acumulado.config.principal, acumulado.config.desempate);
      const auto t1 = std::chrono::high_resolution_clock::now();

      const double tiempo_v_s = std::chrono::duration<double>(t1 - t0).count();
      const long long costo_v = res_v.costo_total;
      const long long diff = (costo_v >= costo_dp) ? (costo_v - costo_dp)
                                                   : (costo_dp - costo_v);
      const double pct_diff = (costo_dp > 0)
          ? (100.0 * static_cast<double>(diff) / static_cast<double>(costo_dp))
          : 0.0;

      acumulado.suma_diff += static_cast<double>(diff);
      acumulado.suma_pct_diff += pct_diff;
      acumulado.suma_tiempo_v_s += tiempo_v_s;
      acumulado.coincidencias_optimo += (costo_v == costo_dp) ? 1 : 0;
      acumulado.total_fincas += 1;
    }
  });

  std::vector<AcumuladoVariante> acumulados(variantes.size());
  for (std::size_t indice = 0; indice < variantes.size(); ++indice) {
    acumulados[indice].config = variantes[indice];
  }

  for (const auto& acumulados_worker : acumulados_por_worker) {
    for (std::size_t indice = 0; indice < acumulados.size(); ++indice) {
      acumulados[indice].suma_diff += acumulados_worker[indice].suma_diff;
      acumulados[indice].suma_pct_diff += acumulados_worker[indice].suma_pct_diff;
      acumulados[indice].suma_tiempo_v_s += acumulados_worker[indice].suma_tiempo_v_s;
      acumulados[indice].coincidencias_optimo +=
          acumulados_worker[indice].coincidencias_optimo;
      acumulados[indice].total_fincas += acumulados_worker[indice].total_fincas;
    }
  }

  std::vector<FilaBenchmarkVoraz> filas;
  filas.reserve(acumulados.size());

  for (const auto& acumulado : acumulados) {
    const double total = static_cast<double>(acumulado.total_fincas);
    filas.push_back(FilaBenchmarkVoraz{
        .criterio_principal = nombre_criterio_voraz(acumulado.config.principal),
        .criterio_desempate = nombre_criterio_voraz(acumulado.config.desempate),
        .promedio_diff = (total > 0.0) ? (acumulado.suma_diff / total) : 0.0,
        .promedio_pct_diff = (total > 0.0) ? (acumulado.suma_pct_diff / total) : 0.0,
        .promedio_tiempo_v_s = (total > 0.0) ? (acumulado.suma_tiempo_v_s / total) : 0.0,
        .coincidencias_optimo = acumulado.coincidencias_optimo,
        .total_fincas = acumulado.total_fincas,
        .es_actual = acumulado.config.es_actual,
    });
  }

  std::sort(filas.begin(), filas.end(), [](const FilaBenchmarkVoraz& a, const FilaBenchmarkVoraz& b) {
    if (a.promedio_pct_diff != b.promedio_pct_diff) {
      return a.promedio_pct_diff < b.promedio_pct_diff;
    }
    if (a.coincidencias_optimo != b.coincidencias_optimo) {
      return a.coincidencias_optimo > b.coincidencias_optimo;
    }
    if (a.promedio_diff != b.promedio_diff) {
      return a.promedio_diff < b.promedio_diff;
    }
    return a.promedio_tiempo_v_s < b.promedio_tiempo_v_s;
  });

  return ResultadoBenchmarkVoraz{.filas = filas};
}

}  // namespace riego
