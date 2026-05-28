#include "http_server.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <limits>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <cerrno>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace {

constexpr std::size_t kMaxHeaderBytes = 64 * 1024;
constexpr std::size_t kMaxBodyBytes = 1024 * 1024;

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

std::string to_lower(std::string value) {
  for (char& current : value) {
    current = static_cast<char>(std::tolower(static_cast<unsigned char>(current)));
  }
  return value;
}

class SocketRuntimeGuard {
 public:
  SocketRuntimeGuard() {
#ifdef _WIN32
    WSADATA data{};
    const int result = WSAStartup(MAKEWORD(2, 2), &data);
    if (result != 0) {
      throw std::runtime_error("No se pudo inicializar WinSock. Codigo: " + std::to_string(result) + ".");
    }
#endif
  }

  ~SocketRuntimeGuard() {
#ifdef _WIN32
    WSACleanup();
#endif
  }
};

int last_socket_error_code() {
#ifdef _WIN32
  return WSAGetLastError();
#else
  return errno;
#endif
}

std::string socket_error_message(int error_code) {
#ifdef _WIN32
  char* buffer = nullptr;
  const DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
  const DWORD message_size = FormatMessageA(flags,
                                            nullptr,
                                            static_cast<DWORD>(error_code),
                                            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                            reinterpret_cast<LPSTR>(&buffer),
                                            0,
                                            nullptr);

  std::string message;
  if (message_size > 0 && buffer != nullptr) {
    message.assign(buffer, message_size);
    LocalFree(buffer);

    while (!message.empty() &&
           std::isspace(static_cast<unsigned char>(message.back()))) {
      message.pop_back();
    }
  } else {
    message = "Codigo WSA " + std::to_string(error_code);
  }

  return message;
#else
  return std::strerror(error_code);
#endif
}

bool is_interrupted_socket_error(int error_code) {
#ifdef _WIN32
  return error_code == WSAEINTR;
#else
  return error_code == EINTR;
#endif
}

void close_socket(riego::http::SocketHandle socket_fd) {
  if (socket_fd == riego::http::kInvalidSocketHandle) {
    return;
  }

#ifdef _WIN32
  closesocket(static_cast<SOCKET>(socket_fd));
#else
  close(socket_fd);
#endif
}

bool send_all(riego::http::SocketHandle socket_fd, const std::string& payload) {
  std::size_t total_sent = 0;

  while (total_sent < payload.size()) {
    const std::size_t remaining = payload.size() - total_sent;
    const std::size_t chunk_size = std::min(remaining, static_cast<std::size_t>(std::numeric_limits<int>::max()));
    const int sent =
        send(socket_fd, payload.data() + total_sent, static_cast<int>(chunk_size), 0);

    if (sent <= 0) {
      return false;
    }

    total_sent += static_cast<std::size_t>(sent);
  }

  return true;
}

std::optional<riego::http::Request> parse_request(riego::http::SocketHandle socket_fd,
                                                  std::string& error_message) {
  std::string buffer;
  buffer.reserve(8192);
  std::size_t headers_end = std::string::npos;
  char chunk[4096];

  while (headers_end == std::string::npos) {
    const int bytes_read = recv(socket_fd, chunk, static_cast<int>(sizeof(chunk)), 0);

    if (bytes_read <= 0) {
      error_message = "No se pudo leer la cabecera HTTP.";
      return std::nullopt;
    }

    buffer.append(chunk, static_cast<std::size_t>(bytes_read));

    if (buffer.size() > kMaxHeaderBytes) {
      error_message = "Cabecera HTTP demasiado grande.";
      return std::nullopt;
    }

    headers_end = buffer.find("\r\n\r\n");
  }

  const std::string header_block = buffer.substr(0, headers_end);
  std::string body = buffer.substr(headers_end + 4);

  std::istringstream header_stream(header_block);
  std::string request_line;
  std::getline(header_stream, request_line);

  if (!request_line.empty() && request_line.back() == '\r') {
    request_line.pop_back();
  }

  std::istringstream request_line_stream(request_line);
  riego::http::Request request;
  std::string http_version;
  request_line_stream >> request.method >> request.path >> http_version;

  if (request.method.empty() || request.path.empty()) {
    error_message = "Linea de request invalida.";
    return std::nullopt;
  }

  std::string header_line;
  while (std::getline(header_stream, header_line)) {
    if (!header_line.empty() && header_line.back() == '\r') {
      header_line.pop_back();
    }

    if (header_line.empty()) {
      continue;
    }

    const std::size_t separator = header_line.find(':');
    if (separator == std::string::npos) {
      continue;
    }

    const std::string key = to_lower(trim(header_line.substr(0, separator)));
    const std::string value = trim(header_line.substr(separator + 1));
    request.headers[key] = value;
  }

  std::size_t content_length = 0;
  if (const auto it = request.headers.find("content-length"); it != request.headers.end()) {
    try {
      content_length = static_cast<std::size_t>(std::stoul(it->second));
    } catch (const std::exception&) {
      error_message = "Cabecera Content-Length invalida.";
      return std::nullopt;
    }
  }

  if (content_length > kMaxBodyBytes) {
    error_message = "Body HTTP demasiado grande.";
    return std::nullopt;
  }

  while (body.size() < content_length) {
    const int bytes_read = recv(socket_fd, chunk, static_cast<int>(sizeof(chunk)), 0);

    if (bytes_read <= 0) {
      error_message = "No se pudo leer el body HTTP completo.";
      return std::nullopt;
    }

    body.append(chunk, static_cast<std::size_t>(bytes_read));
  }

  request.body = body.substr(0, content_length);
  return request;
}

void write_response(riego::http::SocketHandle socket_fd, const riego::http::Response& response) {
  std::ostringstream stream;
  stream << "HTTP/1.1 " << response.status_code << " " << response.status_text << "\r\n";
  stream << "Content-Type: " << response.content_type << "\r\n";
  stream << "Content-Length: " << response.body.size() << "\r\n";
  stream << "Connection: close\r\n";
  stream << "Access-Control-Allow-Origin: *\r\n";
  stream << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n";
  stream << "Access-Control-Allow-Headers: Content-Type\r\n";
  stream << "Access-Control-Max-Age: 86400\r\n";

  for (const auto& [key, value] : response.extra_headers) {
    stream << key << ": " << value << "\r\n";
  }

  stream << "\r\n";
  stream << response.body;

  send_all(socket_fd, stream.str());
}

void configure_socket_timeouts(riego::http::SocketHandle socket_fd) {
#ifdef _WIN32
  const DWORD timeout_ms = 5000;
  setsockopt(socket_fd,
             SOL_SOCKET,
             SO_RCVTIMEO,
             reinterpret_cast<const char*>(&timeout_ms),
             static_cast<int>(sizeof(timeout_ms)));
  setsockopt(socket_fd,
             SOL_SOCKET,
             SO_SNDTIMEO,
             reinterpret_cast<const char*>(&timeout_ms),
             static_cast<int>(sizeof(timeout_ms)));
#else
  timeval timeout{};
  timeout.tv_sec = 5;
  timeout.tv_usec = 0;

  setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
  setsockopt(socket_fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
#endif
}

bool assign_loopback_address(sockaddr_in& address) {
#ifdef _WIN32
  return InetPtonA(AF_INET, "127.0.0.1", &address.sin_addr) == 1;
#else
  return inet_pton(AF_INET, "127.0.0.1", &address.sin_addr) == 1;
#endif
}

}  // namespace

namespace riego::http {

HttpServer::HttpServer(int port, std::atomic<bool>& running_flag, RequestHandler handler)
    : port_(port),
      running_flag_(running_flag),
      handler_(std::move(handler)),
      server_fd_(kInvalidSocketHandle) {}

void HttpServer::run() {
  SocketRuntimeGuard socket_runtime;
  open_listening_socket();

  try {
    accept_loop();
  } catch (...) {
    close_listening_socket();
    throw;
  }

  close_listening_socket();
}

void HttpServer::open_listening_socket() {
  server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd_ == kInvalidSocketHandle) {
    throw std::runtime_error("No se pudo crear el socket del servidor: " +
                             socket_error_message(last_socket_error_code()) + ".");
  }

  const int reuse_address = 1;
#ifdef _WIN32
  const int setopt_result =
      setsockopt(server_fd_,
                 SOL_SOCKET,
                 SO_REUSEADDR,
                 reinterpret_cast<const char*>(&reuse_address),
                 static_cast<int>(sizeof(reuse_address)));
#else
  const int setopt_result = setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &reuse_address, sizeof(reuse_address));
#endif
  if (setopt_result < 0) {
    close_listening_socket();
    throw std::runtime_error("No se pudo configurar SO_REUSEADDR.");
  }

  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_port = htons(static_cast<std::uint16_t>(port_));

  if (!assign_loopback_address(address)) {
    close_listening_socket();
    throw std::runtime_error("No se pudo configurar la direccion del backend.");
  }

  if (bind(server_fd_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
    const std::string error = socket_error_message(last_socket_error_code());
    close_listening_socket();
    throw std::runtime_error("No se pudo enlazar el backend al puerto " + std::to_string(port_) + ": " + error + ".");
  }

  if (listen(server_fd_, 16) < 0) {
    close_listening_socket();
    throw std::runtime_error("No se pudo poner el backend en modo escucha.");
  }
}

void HttpServer::close_listening_socket() {
  if (server_fd_ != kInvalidSocketHandle) {
    close_socket(server_fd_);
    server_fd_ = kInvalidSocketHandle;
  }
}

void HttpServer::accept_loop() {
  while (running_flag_.load()) {
#ifndef _WIN32
    while (waitpid(-1, nullptr, WNOHANG) > 0) {
    }
#endif

    fd_set read_set;
    FD_ZERO(&read_set);
    FD_SET(server_fd_, &read_set);

    timeval timeout{};
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    const int ready =
#ifdef _WIN32
        select(0, &read_set, nullptr, nullptr, &timeout);
#else
        select(server_fd_ + 1, &read_set, nullptr, nullptr, &timeout);
#endif

    if (ready < 0) {
      if (is_interrupted_socket_error(last_socket_error_code())) {
        continue;
      }

      throw std::runtime_error("Error esperando conexiones entrantes.");
    }

    if (ready == 0) {
      continue;
    }

    sockaddr_in client_address{};
#ifdef _WIN32
    int client_length = static_cast<int>(sizeof(client_address));
#else
    socklen_t client_length = sizeof(client_address);
#endif
    const SocketHandle client_fd =
        accept(server_fd_, reinterpret_cast<sockaddr*>(&client_address), &client_length);

    if (client_fd == kInvalidSocketHandle) {
      if (is_interrupted_socket_error(last_socket_error_code())) {
        continue;
      }
      continue;
    }

#ifdef _WIN32
    handle_client(client_fd);
#else
    const pid_t child_pid = fork();

    if (child_pid < 0) {
      handle_client(client_fd);
      continue;
    }

    if (child_pid == 0) {
      close(server_fd_);
      handle_client(client_fd);
      _exit(0);
    }

    close(client_fd);
#endif
  }
}

void HttpServer::handle_client(SocketHandle client_fd) {
  configure_socket_timeouts(client_fd);

  try {
    std::string error_message;
    const auto request = parse_request(client_fd, error_message);

    if (!request) {
      write_response(client_fd, Response{
                                     .status_code = 400,
                                     .status_text = "Bad Request",
                                     .body = "{\"error\":\"" + error_message + "\"}",
                                     .extra_headers = {},
                                 });
      close_socket(client_fd);
      return;
    }

    const Response response = handler_(*request);
    write_response(client_fd, response);
  } catch (const std::exception& error) {
    write_response(client_fd, Response{
                                   .status_code = 500,
                                   .status_text = "Internal Server Error",
                                   .body = "{\"error\":\"" + std::string(error.what()) + "\"}",
                                   .extra_headers = {},
                               });
  }

  close_socket(client_fd);
}

}  // namespace riego::http
