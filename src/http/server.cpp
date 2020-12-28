#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/core/ostream.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/version.hpp>
#include <boost/config.hpp>
#include <memory>
#include <mutex>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <spdlog/spdlog.h>
#include <thread>
#include <unordered_set>
#include <vector>

#include "server.hpp"

using boost::asio::ip::tcp;
namespace http = boost::beast::http;

namespace BurglarFucker::Http {
Server::Server(ServerConfig &&config, ServerCallback &callback)
    : _acceptor(_io_c, tcp::endpoint(boost::asio::ip::address_v4::from_string(
                                         config.ip_binding),
                                     config.port)),
      _conf(std::move(config)), _callback(callback) {
  spdlog::info("Binding to http://{}:{}", _conf.ip_binding, _conf.port);

  _accept_request_thread =
      std::thread([this]() { return this->accept_connections(); });
}

void Server::handle_set_alarm_on_request(
    tcp::socket &socket, http::request<http::string_body> const &request) {
  spdlog::debug("handle_set_alarm_on_request");
  _callback.on_alarm_on();
  http::response<http::empty_body> res{http::status::ok, request.version()};
  http::response_serializer<http::empty_body> sr{res};
  http::write(socket, sr);
}

void Server::handle_set_alarm_off_request(
    tcp::socket &socket, http::request<http::string_body> const &request) {
  spdlog::debug("handle_set_alarm_off_request");
  _callback.on_alarm_off();
  http::response<http::empty_body> res{http::status::ok, request.version()};
  http::response_serializer<http::empty_body> sr{res};
  http::write(socket, sr);
}

void Server::handle_restart_alarm_request(
    tcp::socket &socket, http::request<http::string_body> const &request) {
  spdlog::debug("handle_restart_alarm_request");
  _callback.on_alarm_off();
  _callback.on_alarm_on();
  http::response<http::empty_body> res{http::status::ok, request.version()};
  http::response_serializer<http::empty_body> sr{res};
  http::write(socket, sr);
}

void Server::handle_video_request(tcp::socket &socket,
                                  http::request<http::string_body> &request) {
  spdlog::debug("handle_video_request");
  http::response<http::empty_body> res{http::status::ok, request.version()};
  res.set(http::field::content_type,
          "multipart/x-mixed-replace; boundary=frame");
  res.keep_alive();
  http::response_serializer<http::empty_body> sr{res};
  http::write_header(socket, sr);

  {
    auto l = std::lock_guard{_requests_mutex};
    _connections.emplace_back(
        Connection{std::move(socket), std::move(request)});
  }
}

void Server::accept_connections() {
  for (;;) {
    {
      tcp::socket socket(_io_c);
      _acceptor.accept(socket);
      http::request<http::string_body> request;
      boost::beast::flat_buffer request_buffer;
      boost::system::error_code err;
      http::read(socket, request_buffer, request, err);

      if (err) {
        spdlog::warn("Error when reading socket for new connection: {}",
                     err.message());
        http::response<http::empty_body> res{
            http::status::internal_server_error, request.version()};
        http::response_serializer<http::empty_body> sr{res};
        http::write(socket, sr);
        continue;
      }

      if (request.target() == _conf.alarm_on_endpoint) {
        handle_set_alarm_on_request(socket, request);
        continue;
      }

      if (request.target() == _conf.alarm_off_endpoint) {
        handle_set_alarm_off_request(socket, request);
        continue;
      }

      if (request.target() == _conf.alarm_restart_endpoint) {
        handle_restart_alarm_request(socket, request);
        continue;
      }

      if (request.target() == _conf.video_endpoint) {
        handle_video_request(socket, request);
        continue;
      }

      if (request.target() == _conf.root_endpoint) {
        http::response<http::file_body> res{http::status::ok,
                                            request.version()};
        res.set(http::field::content_type, "text/html");
        res.body().open("./resources/index.html", boost::beast::file_mode::read,
                        err);
        res.prepare_payload();
        http::response_serializer<http::file_body> sr{res};
        http::write(socket, sr);
        continue;
      }

      http::response<http::empty_body> res{http::status::not_found,
                                           request.version()};
      http::response_serializer<http::empty_body> sr{res};
      http::write(socket, sr);
      continue;
    }
  }
}

void Server::send(cv::Mat &f) {
  if (_conf.rotation) {
    cv::rotate(f, _rotation_buf, *_conf.rotation);
  } else {
    _rotation_buf = f;
  }
  cv::imencode(".jpg", _rotation_buf, _buf,
               std::vector<int>{cv::IMWRITE_JPEG_QUALITY, 95});

  auto const size = _buf.size();

  {
    auto l = std::lock_guard{_requests_mutex};
    for (auto it = _connections.begin(); it != _connections.end();) {
      auto &req = *it;
      boost::system::error_code err;
      http::response<http::vector_body<unsigned char>> res{
          std::piecewise_construct, std::make_tuple(_buf),
          std::make_tuple(http::status::ok, req.request.version())};
      res.set(http::field::body, "--frame");
      res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
      res.set(http::field::content_type, "image/jpeg");
      res.content_length(size);
      res.keep_alive(req.request.keep_alive());
      http::write(req.socket, res, err);
      if (err) {
        if (err == boost::system::errc::errc_t::broken_pipe) {
          spdlog::info("connection closed");
        } else {
          spdlog::warn("Error: {}", err.message());
        }
        it = _connections.erase(it);
      } else {
        ++it;
      }
    }
  }
}
} // namespace BurglarFucker::Http
