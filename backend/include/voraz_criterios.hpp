#pragma once

#include <vector>

#include "domain_types.hpp"

namespace riego {

enum class CriterioVoraz {
  RatioRiegoPrioridad,
  LimiteInicio,
  DiaPerfecto,
  Supervivencia,
  PrioridadDesc,
};

const char* nombre_criterio_voraz(CriterioVoraz criterio);
ResultadoCalculo roVConCriterios(const std::vector<Tablon>& f,
                                 CriterioVoraz principal,
                                 CriterioVoraz desempate);

}  // namespace riego
