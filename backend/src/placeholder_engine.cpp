#include "placeholder_engine.hpp"

#include <cctype>
#include <sstream>
#include <stdexcept>
#include <string>

#include "algoritmos.hpp"
#include "json_utils.hpp"

namespace {

std::string quitar_espacios(const std::string& texto) {
  std::size_t start = 0;
  std::size_t end = texto.size();

  while (start < end && std::isspace(static_cast<unsigned char>(texto[start]))) {
    ++start;
  }

  while (end > start && std::isspace(static_cast<unsigned char>(texto[end - 1]))) {
    --end;
  }

  return texto.substr(start, end - start);
}

std::vector<std::string> separar_lineas(const std::string& texto) {
  std::vector<std::string> lineas;
  std::istringstream flujo(texto);
  std::string linea;

  while (std::getline(flujo, linea)) {
    const std::string linea_normalizada = quitar_espacios(linea);
    if (!linea_normalizada.empty()) {
      lineas.push_back(linea_normalizada);
    }
  }

  return lineas;
}

}  // namespace

namespace riego {

EntradaCalculo leer_entrada_calculo(const std::string& cuerpo_peticion) {
  const auto algoritmo = extraer_campo_json_texto(cuerpo_peticion, "algoritmo");
  const auto texto_finca = extraer_campo_json_texto(cuerpo_peticion, "finca");

  if (!algoritmo || !texto_finca) {
    throw std::runtime_error("La peticion JSON debe incluir 'algoritmo' y 'finca'.");
  }

  return EntradaCalculo{
      .algoritmo = *algoritmo,
      .texto_finca = *texto_finca,
  };
}

std::vector<Tablon> leer_tablones_desde_texto(const std::string& texto_finca) {
  const std::vector<std::string> lineas = separar_lineas(texto_finca);

  if (lineas.empty()) {
    throw std::runtime_error("La finca recibida esta vacia.");
  }

  const int cantidad_tablones = std::stoi(lineas[0]);

  if (cantidad_tablones <= 0) {
    throw std::runtime_error("La finca debe tener al menos un tablon.");
  }

  if (static_cast<int>(lineas.size()) - 1 < cantidad_tablones) {
    throw std::runtime_error("La finca no contiene suficientes filas para todos los tablones.");
  }

  std::vector<Tablon> tablones;
  tablones.reserve(cantidad_tablones);

  for (int indice = 1; indice <= cantidad_tablones; ++indice) {
    std::istringstream flujo_linea(lineas[indice]);
    std::string token;
    std::vector<int> valores;

    while (std::getline(flujo_linea, token, ',')) {
      valores.push_back(std::stoi(quitar_espacios(token)));
    }

    if (valores.size() != 4) {
      throw std::runtime_error("Cada tablon debe venir con ts,tr,p,rp.");
    }

    tablones.push_back(Tablon{
        .tiempo_supervivencia = valores[0],
        .tiempo_riego = valores[1],
        .prioridad = valores[2],
        .dia_perfecto = valores[3],
    });
  }

  return tablones;
}

ResultadoCalculo calcular_resultado(const EntradaCalculo& entrada) {
  const std::vector<Tablon> tablones = leer_tablones_desde_texto(entrada.texto_finca);

  if (entrada.algoritmo == "roFB") {
    return roFB(tablones);
  }

  if (entrada.algoritmo == "roV") {
    return roV(tablones);
  }

  if (entrada.algoritmo == "roPD") {
    return roPD(tablones);
  }

  throw std::runtime_error("Algoritmo no soportado.");
}

}  // namespace riego
