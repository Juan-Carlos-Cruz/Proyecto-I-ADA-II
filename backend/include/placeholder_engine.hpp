#pragma once

#include <vector>

#include "domain_types.hpp"

namespace riego {

EntradaCalculo leer_entrada_calculo(const std::string& cuerpo_peticion);
std::vector<Tablon> leer_tablones_desde_texto(const std::string& texto_finca);
ResultadoCalculo calcular_resultado(const EntradaCalculo& entrada);

}  // namespace riego
