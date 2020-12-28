#include "configuration_loader.hpp"
#include "opencv2/core.hpp"
#include "src/common.hpp"
#include "src/movement_detector.hpp"
#include <fstream>
#include <optional>
#include <toml++/toml.h>

namespace BurglarFucker {
namespace {
// tn = alpha * tn-1 + (1-alpha) * vn
// this gives alpha such that the weights of the k latest elements of the sum
// are >= p
double alpha_from_k_p(int k, double p) { return std::pow(1.0 - p, 1.0 / k); }

Http::ServerConfig load_server(toml::v2::table const &node) {
  Http::ServerConfig conf;
  auto backend = node["Backend"];

  auto connection = backend["Connection"];
  conf.port = connection["port"].value<int>().value();
  conf.ip_binding = connection["ip"].value<std::string>().value();

  auto endpoints = backend["Endpoints"];
  conf.alarm_on_endpoint = endpoints["alarm_on"].value<std::string>().value();
  conf.alarm_off_endpoint = endpoints["alarm_off"].value<std::string>().value();
  conf.alarm_restart_endpoint =
      endpoints["alarm_restart"].value<std::string>().value();
  conf.video_endpoint = endpoints["video"].value<std::string>().value();
  conf.root_endpoint = endpoints["root"].value<std::string>().value();

  auto user_rotation = backend["image_rotation"].value_or<int>(0);
  conf.rotation = [&]() -> decltype(conf.rotation) {
    switch (user_rotation) {
    case 0:
      return std::nullopt;
    case 90:
      return cv::RotateFlags::ROTATE_90_COUNTERCLOCKWISE;
    case -90:
    case 270:
      return cv::RotateFlags::ROTATE_90_CLOCKWISE;
    case 180:
    case -180:
      return cv::RotateFlags::ROTATE_180;
    }

    throw Exception("User provided rotation ({}) is invalid!", user_rotation);
  }();
  return conf;
}

MovementDetectionConfig load_detection(toml::v2::table const &node) {
  MovementDetectionConfig out;
  auto detection = node["Detection"];
  auto moving_average = detection["MovingAverage"];
  auto activation = detection["Activation"];

  out.alpha = alpha_from_k_p(moving_average["k"].value<unsigned int>().value(),
                             moving_average["p"].value<double>().value());
  out.average_moved_pixel_ratio =
      activation["average_moved_pixel_ratio"].value<double>().value();
  out.brightness_threshold =
      activation["brightness_threshold"].value<unsigned int>().value();
  out.sliding_window_size =
      activation["sliding_window_size"].value<unsigned int>().value();

  return out;
}

MovementHandlerConfig load_handler(toml::v2::table const &node) {
  MovementHandlerConfig out;
  auto handler = node["Alarm"];

  out.buffer_before_alarm = std::chrono::duration<double>(
      handler["buffer_before_alarm"].value<double>().value());
  out.volume_buildup_duration = std::chrono::duration<double>(
      handler["volume_buildup_duration"].value<double>().value());
  out.max_ringing_time = std::chrono::duration<double>(
      handler["max_ringing_time"].value<double>().value());
  out.volume_multiplier = handler["volume_multiplier"].value<double>().value();

  return out;
}

} // namespace

Configuration Configuration::load(std::string_view f) {
  auto config = toml::parse_file(f);
  Configuration out;
  out.detection = load_detection(config);
  out.backend = load_server(config);
  out.handler = load_handler(config);
  return out;
}

} // namespace BurglarFucker
