#pragma once

#include <atomic>
#include <chrono>
#include <opencv2/core.hpp>
#include <spdlog/fmt/bundled/core.h>
#include <spdlog/spdlog.h>

namespace BurglarFucker {
using Clock = std::chrono::steady_clock;
using Time = Clock::time_point;
using Duration = Clock::duration;

class Frame {
public:
  cv::Mat frame;
  Time timestamp;

  Frame clone() const { return {frame.clone(), timestamp}; }
};

enum class AlarmState {
  Off,
  Starting,
  On,
  WaitingForPin,
  Ringing,
};

class AtomicAlarmState {
  std::atomic<AlarmState> _state;

public:
  AtomicAlarmState(AlarmState init_state) : _state(init_state) {}
  AlarmState operator()() { return _state; }
  void operator()(AlarmState state) { _state = state; }
};

struct Exception : std::runtime_error {
  Exception(std::runtime_error const &parent) : Exception(parent.what()) {}

  Exception(std::string const &err) : std::runtime_error(err) {
    spdlog::error("😭 exception caught: {}", what());
  }

  template <typename... Args>
  Exception(char const *format, Args &&... args)
      : Exception(fmt::format(format, std::forward<Args>(args)...)) {}
};
} // namespace BurglarFucker
