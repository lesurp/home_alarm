#include <SFML/Audio.hpp>
#include <chrono>
#include <future>
#include <spdlog/spdlog.h>
#include <thread>

#include "device.hpp"
#include "movement_handler.hpp"
#include "src/common.hpp"

namespace BurglarFucker {

MovementHandler::MovementHandler(MovementHandlerConfig const &config)
    : _config(config) {
  _sound_buf.loadFromFile("resources/quack.ogg");
  _sound.setBuffer(_sound_buf);
}

void MovementHandler::movement_detected(AtomicAlarmState &state) {
  if (is_handler_running()) {
    spdlog::warn("Trying to handle another movement event while the previous "
                 "one is still being processed");
    return;
  }

  _handler_future = std::async(std::launch::async, [this, &state]() {
    this->movement_detected_impl(state);
  });
}

void MovementHandler::movement_detected_impl(AtomicAlarmState &state) {
  spdlog::debug("entering movement_detected_impl");

  state(AlarmState::WaitingForPin);
  std::this_thread::sleep_for(_config.buffer_before_alarm);
  if (state() != AlarmState::WaitingForPin) {
    spdlog::info("Alarm was stopped during the waiting period");
    return;
  }
  state(AlarmState::Ringing);

  spdlog::info("Alarm was NOT stopped in the buffer period: starting to ring!");

  _sound.play();
  _sound.setLoop(true);

  auto start = Clock::now();
  for (;;) {
    if (state() != AlarmState::Ringing) {
      spdlog::info("early_stop evaluated to true; stopping alarm");
      _sound.stop();
      return;
    }
    auto time = Clock::now() - start;
    if (time > _config.max_ringing_time) {
      spdlog::info("Alarm timed out after ringing for {} seconds, returning to "
                   "'On' mode",
                   _config.max_ringing_time.count());
      state(AlarmState::On);
      _sound.stop();
      return;
    }

    auto time_f =
        std::chrono::duration_cast<std::chrono::duration<double>>(time).count();
    auto volume =
        _config.volume_multiplier *
        std::min(100.0, 100 * time_f / _config.volume_buildup_duration.count());
    spdlog::debug("Setting volume to {}", volume);
    _sound.setVolume(volume);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}

bool MovementHandler::is_handler_running() const {
  return _handler_future.valid() &&
         _handler_future.wait_for(std::chrono::nanoseconds{0}) ==
             std::future_status::timeout;
}

} // namespace BurglarFucker
