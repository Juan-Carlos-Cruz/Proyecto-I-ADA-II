#include <atomic>
#include <csignal>
#include <exception>
#include <iostream>
#include <string>

#include "benchmark_engine.hpp"
#include "http_server.hpp"
#include "json_utils.hpp"
#include "placeholder_engine.hpp"

namespace {

std::atomic<bool>* g_running_flag = nullptr;

void handle_signal(int) {
  if (g_running_flag != nullptr) {
    g_running_flag->store(false);
  }
}

int parse_port(int argc, char** argv) {
  int port = 8080;

  for (int index = 1; index < argc; ++index) {
    const std::string argument = argv[index];

    if (argument == "--port" && index + 1 < argc) {
      port = std::stoi(argv[++index]);
    }
  }

  return port;
}

riego::http::Response handle_request(const riego::http::Request& request) {
  if (request.method == "OPTIONS") {
    return {
        .status_code = 204,
        .status_text = "No Content",
        .body = "",
        .extra_headers = {},
    };
  }

  if (request.method == "GET" && request.path == "/health") {
    return {
        .status_code = 200,
        .status_text = "OK",
        .body = "{\"status\":\"ok\",\"message\":\"Backend provisional activo\"}",
        .extra_headers = {},
    };
  }

  if (request.method == "POST" && request.path == "/calcular") {
    try {
      const riego::EntradaCalculo entrada = riego::leer_entrada_calculo(request.body);
      const riego::ResultadoCalculo resultado = riego::calcular_resultado(entrada);

      return {
          .status_code = 200,
          .status_text = "OK",
          .body = riego::construir_json_resultado(resultado),
          .extra_headers = {},
      };
    } catch (const std::exception& error) {
      return {
          .status_code = 400,
          .status_text = "Bad Request",
          .body = riego::construir_json_error(error.what()),
          .extra_headers = {},
      };
    }
  }

  if (request.method == "POST" && request.path == "/benchmark") {
    try {
      const riego::ResultadoBenchmark resultado = riego::ejecutar_benchmark(request.body);
      return {
          .status_code = 200,
          .status_text = "OK",
          .body = riego::construir_json_benchmark(resultado),
          .extra_headers = {},
      };
    } catch (const std::exception& error) {
      return {
          .status_code = 400,
          .status_text = "Bad Request",
          .body = riego::construir_json_error(error.what()),
          .extra_headers = {},
      };
    }
  }

  if (request.method == "POST" && request.path == "/benchmark-voraz") {
    try {
      const riego::ResultadoBenchmarkVoraz resultado =
          riego::ejecutar_benchmark_voraz(request.body);
      return {
          .status_code = 200,
          .status_text = "OK",
          .body = riego::construir_json_benchmark_voraz(resultado),
          .extra_headers = {},
      };
    } catch (const std::exception& error) {
      return {
          .status_code = 400,
          .status_text = "Bad Request",
          .body = riego::construir_json_error(error.what()),
          .extra_headers = {},
      };
    }
  }

  return {
      .status_code = 404,
      .status_text = "Not Found",
      .body = riego::construir_json_error("Ruta no soportada."),
      .extra_headers = {},
  };
}

}  // namespace

int main(int argc, char** argv) {
  const int port = parse_port(argc, argv);
  std::atomic<bool> running{true};
  g_running_flag = &running;

  std::signal(SIGINT, handle_signal);
#ifdef SIGTERM
  std::signal(SIGTERM, handle_signal);
#endif
#ifdef SIGPIPE
  std::signal(SIGPIPE, SIG_IGN);
#endif

  try {
    riego::http::HttpServer server(port, running, handle_request);

    std::cout << "Backend provisional escuchando en http://127.0.0.1:" << port << '\n';
    server.run();
    std::cout << "Backend provisional detenido.\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "Error fatal del backend: " << error.what() << '\n';
    return 1;
  }
}
