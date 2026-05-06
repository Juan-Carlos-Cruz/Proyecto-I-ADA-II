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

std::string construir_json_error(const std::string& message) {
  return "{\"error\":\"" + escape_json_string(trim(message)) + "\"}";
}

}  // namespace riego
