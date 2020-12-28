#include "movement_detector.hpp"
#include "common.hpp"
#include <deque>
#include <spdlog/spdlog.h>

namespace BurglarFucker {
MovementDetector::MovementDetector(MovementDetectionConfig &&config)
    : _config(std::move(config)) {
  _events.resize(_config.sliding_window_size, 0.0);
}

void MovementDetector::reset(Frame const &f) {
  auto l = std::lock_guard{_reset_lock};
  _events.resize(_config.sliding_window_size, 0.0);
  _moving_f = f.frame.clone();
  _cost = 0;
}

MovementDetection MovementDetector::next(Frame const &f) {
  {
    auto l = std::lock_guard{_reset_lock};
    _moving_f = _config.alpha * _moving_f + (1.0 - _config.alpha) * f.frame;
    _cost -= _events.front();
    _events.pop_front();
    _events.push_back(displacement_cost(f.frame, _moving_f));
    _cost += _events.back();
  }

  spdlog::debug("Average cost: {} (threshold: {})",
                _cost / _config.sliding_window_size,
                _config.average_moved_pixel_ratio);

  if (static_cast<double>(_cost) / _events.size() >
      _config.average_moved_pixel_ratio) {
    spdlog::info("Movement detected!");
    return MovementDetection::Positive;
  }

  return MovementDetection::Negative;
}

double MovementDetector::displacement_cost(cv::Mat const &f1,
                                           cv::Mat const &f2) {
  cv::Mat delta = cv::abs(f1 - f2);

  return static_cast<double>(
             cv::countNonZero(delta > _config.brightness_threshold)) /
         (delta.rows * delta.cols);
}

} // namespace BurglarFucker
