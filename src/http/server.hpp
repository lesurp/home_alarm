#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/http.hpp>
#include <memory>
#include <mutex>
#include <opencv2/core.hpp>
#include <optional>
#include <thread>
#include <unordered_set>
#include <vector>

namespace BurglarFucker::Http {

class ServerCallback {
public:
  virtual ~ServerCallback() {}
  virtual void on_alarm_off() = 0;
  virtual void on_alarm_on() = 0;
};

struct ServerConfig {
  int port;
  std::string ip_binding;
  std::string alarm_off_endpoint;
  std::string alarm_on_endpoint;
  std::string alarm_restart_endpoint;
  std::string video_endpoint;
  std::string root_endpoint;
  std::optional<cv::RotateFlags> rotation;
};

class Server {
public:
  Server(ServerConfig &&config, ServerCallback &callback);
  void send(cv::Mat &f);

private:
  struct Connection {
    boost::asio::ip::tcp::socket socket;
    boost::beast::http::request<boost::beast::http::string_body> request;
  };

  void accept_connections();
  void handle_set_alarm_off_request(
      boost::asio::ip::tcp::socket &socket,
      boost::beast::http::request<boost::beast::http::string_body> const
          &request);
  void handle_set_alarm_on_request(
      boost::asio::ip::tcp::socket &socket,
      boost::beast::http::request<boost::beast::http::string_body> const
          &request);
  void handle_restart_alarm_request(
      boost::asio::ip::tcp::socket &socket,
      boost::beast::http::request<boost::beast::http::string_body> const
          &request);
  void handle_video_request(
      boost::asio::ip::tcp::socket &socket,
      boost::beast::http::request<boost::beast::http::string_body> &request);

  boost::asio::io_context _io_c;
  boost::asio::ip::tcp::acceptor _acceptor;
  cv::Mat _rotation_buf;
  std::vector<unsigned char> _buf;
  std::vector<Connection> _connections;
  std::mutex _requests_mutex;
  std::thread _accept_request_thread;
  ServerConfig _conf;
  ServerCallback &_callback;
}; // namespace BurglarFucker

} // namespace BurglarFucker::Http
