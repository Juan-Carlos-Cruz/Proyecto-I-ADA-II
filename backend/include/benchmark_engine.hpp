#pragma once

#include <string>

#include "domain_types.hpp"

namespace riego {

ResultadoBenchmark ejecutar_benchmark(const std::string& json_body);
ResultadoBenchmarkVoraz ejecutar_benchmark_voraz(const std::string& json_body);

}  // namespace riego
