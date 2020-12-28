#pragma once

#include "common.hpp"
#include <deque>

namespace BurglarFucker {

struct MovementDetectionConfig {
  double average_moved_pixel_ratio;
  unsigned int sliding_window_size;
  unsigned int brightness_threshold;
  double alpha;
};

enum class MovementDetection {
  Negative,
  Positive,
};

class MovementDetector {
public:
  MovementDetector(MovementDetectionConfig && config);
  void reset(Frame const &f);
  MovementDetection next(Frame const &f);

private:
  MovementDetectionConfig _config;
  std::deque<double> _events;
  double _cost = 0.0;

  double displacement_cost(cv::Mat const &f1, cv::Mat const &f2);
  cv::Mat _moving_f;
  std::mutex _reset_lock;
};

} // namespace BurglarFucker
