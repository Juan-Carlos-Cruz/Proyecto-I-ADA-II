#include "json_utils.hpp"

#include <cctype>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace {

std::string trim(const std::string& value) {
  std::size_t start = 0;
  std::size_t end = value.size();

  while (start < end && std::isspace(static_cast<unsigned char>(value[start]))) {
    ++start;
  }

  while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
    --end;
  }

  return value.substr(start, end - start);
}

std::string unescape_json_string(const std::string& json, std::size_t& position) {
  std::string result;

  while (position < json.size()) {
    const char current = json[position++];

    if (current == '"') {
      return result;
    }

    if (current != '\\') {
      result.push_back(current);
      continue;
    }

    if (position >= json.size()) {
      throw std::runtime_error("Secuencia JSON invalida.");
    }

    const char escaped = json[position++];

    switch (escaped) {
      case '"':
      case '\\':
      case '/':
        result.push_back(escaped);
        break;
      case 'b':
        result.push_back('\b');
        break;
      case 'f':
        result.push_back('\f');
        break;
      case 'n':
        result.push_back('\n');
        break;
      case 'r':
        result.push_back('\r');
        break;
      case 't':
        result.push_back('\t');
        break;
      case 'u': {
        if (position + 4 > json.size()) {
          throw std::runtime_error("Secuencia unicode JSON incompleta.");
        }

        const std::string hex_value = json.substr(position, 4);
        position += 4;

        unsigned int code_point = 0;
        std::istringstream(hex_value) >> std::hex >> code_point;

        if (code_point <= 0x7F) {
          result.push_back(static_cast<char>(code_point));
        } else {
          result.push_back('?');
        }
        break;
      }
      default:
        throw std::runtime_error("Escape JSON no soportado.");
    }
  }

  throw std::runtime_error("Cadena JSON sin cierre.");
}

std::string escape_json_string(const std::string& value) {
  std::ostringstream stream;

  for (const char current : value) {
    switch (current) {
      case '"':
        stream << "\\\"";
        break;
      case '\\':
        stream << "\\\\";
        break;
      case '\b':
        stream << "\\b";
        break;
      case '\f':
        stream << "\\f";
        break;
      case '\n':
        stream << "\\n";
        break;
      case '\r':
        stream << "\\r";
        break;
      case '\t':
        stream << "\\t";
        break;
      default:
        if (static_cast<unsigned char>(current) < 0x20) {
          stream << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                 << static_cast<int>(static_cast<unsigned char>(current))
                 << std::dec << std::setfill(' ');
        } else {
          stream << current;
        }
        break;
    }
  }

  return stream.str();
}

}  // namespace

namespace riego {

std::optional<std::string> extraer_campo_json_texto(const std::string& json,
                                                    const std::string& key) {
  const std::string needle = "\"" + key + "\"";
  std::size_t key_position = json.find(needle);

  if (key_position == std::string::npos) {
    return std::nullopt;
  }

  std::size_t colon_position = json.find(':', key_position + needle.size());
  if (colon_position == std::string::npos) {
    return std::nullopt;
  }

  std::size_t value_position = colon_position + 1;
  while (value_position < json.size() &&
         std::isspace(static_cast<unsigned char>(json[value_position]))) {
    ++value_position;
  }

  if (value_position >= json.size() || json[value_position] != '"') {
    return std::nullopt;
  }

  ++value_position;

  try {
    return unescape_json_string(json, value_position);
  } catch (const std::exception&) {
    return std::nullopt;
  }
}

std::string construir_json_resultado(const ResultadoCalculo& resultado) {
  std::ostringstream stream;
  stream << "{";
  stream << "\"costo\":" << resultado.costo_total << ",";
  stream << "\"permutacion\":[";

  for (std::size_t index = 0; index < resultado.orden_riego.size(); ++index) {
    if (index > 0) {
      stream << ",";
    }
    stream << resultado.orden_riego[index];
  }

  stream << "],";
  stream << "\"detalles\":[";

  for (std::size_t index = 0; index < resultado.detalles.size(); ++index) {
    if (index > 0) {
      stream << ",";
    }

    const DetalleRiego& detail = resultado.detalles[index];
    stream << "{"
           << "\"tablon\":" << detail.tablon << ","
           << "\"inicio\":" << detail.dia_inicio << ","
           << "\"caso\":" << detail.caso_aplicado << ","
           << "\"costo\":" << detail.costo_tablon
           << "}";
  }

  stream << "]";
  stream << "}";

  return stream.str();
}

std::vector<std::string> extraer_campo_json_array_textos(const std::string& json,
                                                         const std::string& key) {
  const std::string needle = "\"" + key + "\"";
  std::size_t key_pos = json.find(needle);
  if (key_pos == std::string::npos) return {};

  std::size_t colon_pos = json.find(':', key_pos + needle.size());
  if (colon_pos == std::string::npos) return {};

  std::size_t pos = colon_pos + 1;
  while (pos < json.size() && std::isspace(static_cast<unsigned char>(json[pos]))) ++pos;
  if (pos >= json.size() || json[pos] != '[') return {};
  ++pos;

  std::vector<std::string> result;
  while (pos < json.size()) {
    while (pos < json.size() && std::isspace(static_cast<unsigned char>(json[pos]))) ++pos;
    if (pos >= json.size()) break;
    if (json[pos] == ']') break;
    if (json[pos] == ',') { ++pos; continue; }
    if (json[pos] == '"') {
      ++pos;
      result.push_back(unescape_json_string(json, pos));
    } else {
      break;
    }
  }
  return result;
}

std::string construir_json_benchmark(const ResultadoBenchmark& resultado) {
  std::ostringstream stream;
  stream << "{\"filas\":[";
  for (std::size_t i = 0; i < resultado.filas.size(); ++i) {
    if (i > 0) stream << ",";
    const FilaBenchmark& f = resultado.filas[i];
    stream << "{"
           << "\"n\":" << f.n << ","
           << "\"costo_fb\":" << f.costo_fb << ","
           << "\"costo_dp\":" << f.costo_dp << ","
           << "\"costo_v\":" << f.costo_v << ","
           << "\"diff\":" << f.diff << ","
           << "\"pct_diff\":" << std::fixed << std::setprecision(4) << f.pct_diff << ","
           << "\"tiempo_dp_s\":" << std::fixed << std::setprecision(4) << f.tiempo_dp_s << ","
           << "\"tiempo_v_s\":" << std::fixed << std::setprecision(4) << f.tiempo_v_s
           << "}";
  }
  const ResumenBenchmark& r = resultado.resumen;
  stream << "],\"resumen\":{"
         << "\"total_fincas\":" << r.total_fincas << ","
         << "\"promedio_diff\":" << std::fixed << std::setprecision(4) << r.promedio_diff << ","
         << "\"promedio_pct_diff\":" << std::fixed << std::setprecision(4) << r.promedio_pct_diff
         << ","
         << "\"porcentaje_exactos\":" << std::fixed << std::setprecision(4)
         << r.porcentaje_exactos << "}}";
  return stream.str();
}

std::string construir_json_benchmark_voraz(const ResultadoBenchmarkVoraz& resultado) {
  std::ostringstream stream;
  stream << "{\"filas\":[";
  for (std::size_t i = 0; i < resultado.filas.size(); ++i) {
    if (i > 0) stream << ",";
    const FilaBenchmarkVoraz& f = resultado.filas[i];
    stream << "{"
           << "\"criterio_principal\":\"" << escape_json_string(f.criterio_principal) << "\","
           << "\"criterio_desempate\":\"" << escape_json_string(f.criterio_desempate) << "\","
           << "\"promedio_diff\":" << std::fixed << std::setprecision(4) << f.promedio_diff << ","
           << "\"promedio_pct_diff\":" << std::fixed << std::setprecision(4) << f.promedio_pct_diff << ","
           << "\"promedio_tiempo_v_s\":" << std::fixed << std::setprecision(4) << f.promedio_tiempo_v_s << ","
           << "\"coincidencias_optimo\":" << f.coincidencias_optimo << ","
           << "\"total_fincas\":" << f.total_fincas << ","
           << "\"es_actual\":" << (f.es_actual ? "true" : "false")
           << "}";
  }
  stream << "]}";
  return stream.str();
}

std::string construir_json_error(const std::string& message) {
  return "{\"error\":\"" + escape_json_string(trim(message)) + "\"}";
}

}  // namespace riego
