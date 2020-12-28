#pragma once

#include "common.hpp"
#include "opencv2/imgproc.hpp"
#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <spdlog/spdlog.h>

namespace BurglarFucker {
class Device {
public:
  Device() {
    spdlog::debug("Device ctor");
    _vc.open(0);
  }

  Frame next() {
    auto ts = Clock::now();
    _vc.read(_buf_device);
    cv::cvtColor(_buf_device, _buf_gray, cv::COLOR_BGR2GRAY);
    return {_buf_gray, ts};
  }

private:
  cv::VideoCapture _vc;
  Time _ts;
  cv::Mat _buf_device;
  cv::Mat _buf_gray;
};

} // namespace BurglarFucker
