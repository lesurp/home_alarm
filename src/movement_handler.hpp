#pragma once

#include <SFML/Audio.hpp>
#include <chrono>
#include <future>
#include <spdlog/spdlog.h>
#include <thread>

namespace BurglarFucker {
struct MovementHandlerConfig {
  std::chrono::duration<double> buffer_before_alarm;
  std::chrono::duration<double> volume_buildup_duration;
  std::chrono::duration<double> max_ringing_time;
  double volume_multiplier;
};

class MovementHandler {
public:
  MovementHandler(MovementHandlerConfig const &config);
  void movement_detected(AtomicAlarmState &state);

private:
  void movement_detected_impl(AtomicAlarmState &state);
  bool is_handler_running() const;

  sf::SoundBuffer _sound_buf;
  sf::Sound _sound;
  std::future<void> _handler_future;
  MovementHandlerConfig _config;
};

} // namespace BurglarFucker
