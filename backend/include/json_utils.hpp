#pragma once

#include <optional>
#include <string>

#include "domain_types.hpp"

namespace riego {

std::optional<std::string> extraer_campo_json_texto(const std::string& json,
                                                    const std::string& key);
std::vector<std::string> extraer_campo_json_array_textos(const std::string& json,
                                                         const std::string& key);
std::string construir_json_resultado(const ResultadoCalculo& resultado);
std::string construir_json_benchmark(const ResultadoBenchmark& resultado);
std::string construir_json_benchmark_voraz(const ResultadoBenchmarkVoraz& resultado);
std::string construir_json_error(const std::string& message);

}  // namespace riego
