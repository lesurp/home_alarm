#pragma once

#include "http/server.hpp"
#include "movement_detector.hpp"
#include "movement_handler.hpp"
#include <string_view>

namespace BurglarFucker {
struct Configuration {
  Http::ServerConfig backend;
  MovementDetectionConfig detection;
  MovementHandlerConfig handler;

  static Configuration load(std::string_view f);
};
} // namespace BurglarFucker
