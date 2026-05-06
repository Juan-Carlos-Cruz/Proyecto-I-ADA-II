#pragma once

#include <atomic>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace riego::http {

struct Request {
  std::string method;
  std::string path;
  std::unordered_map<std::string, std::string> headers;
  std::string body;
};

struct Response {
  int status_code;
  std::string status_text;
  std::string content_type = "application/json; charset=utf-8";
  std::string body;
  std::vector<std::pair<std::string, std::string>> extra_headers;
};

class HttpServer {
 public:
  using RequestHandler = std::function<Response(const Request&)>;

  HttpServer(int port, std::atomic<bool>& running_flag, RequestHandler handler);
  void run();

 private:
  int port_;
  std::atomic<bool>& running_flag_;
  RequestHandler handler_;
  int server_fd_;

  void open_listening_socket();
  void close_listening_socket();
  void accept_loop();
  void handle_client(int client_fd);
};

}  // namespace riego::http
