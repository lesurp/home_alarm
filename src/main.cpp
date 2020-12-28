#include "configuration_loader.hpp"
#include "http/server.hpp"
#include "movement_detector.hpp"
#include "movement_handler.hpp"
#include "src/common.hpp"
#include "src/device.hpp"
#include <spdlog/spdlog.h>

class CallbackDispatcher : public BurglarFucker::Http::ServerCallback {
  BurglarFucker::AtomicAlarmState _state;

public:
  CallbackDispatcher(BurglarFucker::AlarmState init_state)
      : _state(init_state) {}
  void on_alarm_off() override { _state(BurglarFucker::AlarmState::Off); }
  void on_alarm_on() override { _state(BurglarFucker::AlarmState::Starting); }
  void ack_alarm_on() { _state(BurglarFucker::AlarmState::On); }
  BurglarFucker::AtomicAlarmState const &alarm_state() const { return _state; }
  BurglarFucker::AtomicAlarmState &alarm_state() { return _state; }
};

int real_main() {
  spdlog::set_level(spdlog::level::debug);
  auto conf = BurglarFucker::Configuration::load("burglar_fucker_conf.toml");

  auto device = BurglarFucker::Device{};
  auto detector = BurglarFucker::MovementDetector{std::move(conf.detection)};
  auto handler = BurglarFucker::MovementHandler{std::move(conf.handler)};
  auto callbacks = CallbackDispatcher{BurglarFucker::AlarmState::Off};
  auto streamer =
      BurglarFucker::Http::Server{std::move(conf.backend), callbacks};

  // leave some time to the camera to auto focus, adjust to brightness etc.
  device.next();
  std::this_thread::sleep_for(std::chrono::seconds(5));

  for (;;) {
    auto f = device.next();
    auto state = callbacks.alarm_state()();

    auto color = [&] {
      switch (state) {
      case BurglarFucker::AlarmState::On:
        switch (detector.next(f)) {
        case BurglarFucker::MovementDetection::Negative:
          break;
        case BurglarFucker::MovementDetection::Positive:
          handler.movement_detected(callbacks.alarm_state());
          break;
        }
        return cv::Scalar(0, 255, 0);
        break;

      case BurglarFucker::AlarmState::Starting:
        detector.reset(f);
        callbacks.ack_alarm_on();
        return cv::Scalar(50, 50, 50);
        break;
      case BurglarFucker::AlarmState::Off:
        return cv::Scalar(50, 50, 50);
        break;
      case BurglarFucker::AlarmState::Ringing:
        return cv::Scalar(0, 0, 255);
        break;
      case BurglarFucker::AlarmState::WaitingForPin:
        return cv::Scalar(0, 140, 255);
        break;
      }
      throw "Unreachable";
    }();

    // all of this is temporary
    cv::Mat f2;
    cv::cvtColor(f.frame, f2, cv::COLOR_GRAY2BGR);
    cv::rectangle(f2, cv::Point(0, 0),
                  cv::Point(f2.cols - 1, f2.rows - 1), color,
                  std::min(f2.cols, f2.rows) / 25);

    streamer.send(f2);
  }
}

int main() {
  try {
    return real_main();
  } catch (BurglarFucker::Exception const &err) {
    throw;
  } catch (std::runtime_error const &err) {
    throw BurglarFucker::Exception(err);
  }
}
