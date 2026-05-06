#pragma once

#include <vector>

#include "domain_types.hpp"

namespace riego {

ResultadoCalculo roFB(const std::vector<Tablon>& f);
ResultadoCalculo roV(const std::vector<Tablon>& f);
ResultadoCalculo roPD(const std::vector<Tablon>& f);

}  // namespace riego
