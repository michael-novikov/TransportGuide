#include "scanline_projection.h"
#include "stop.h"

ScanlineProjector::ScanlineProjector(std::vector<Coordinates> points, double max_width, double max_height, double padding)
  : height_(max_height)
  , padding_(padding)
{
  if (points.empty()) {
    return;
  }

  {
    for (const auto& p : points) {
      sorted_by_lon_[p] = 0;
    }

    size_t i{0};
    for (auto& [_, idx] : sorted_by_lon_) {
      idx = i++;
    }

    if (sorted_by_lon_.size() > 1) {
      x_step_ = (max_width - 2 * padding) / (i - 1);
    }
  }

  {
    for (const auto& p : points) {
      sorted_by_lat_[p] = 0;
    }

    size_t i{0};
    for (auto& [_, idx] : sorted_by_lat_) {
      idx = i++;
    }

    if (sorted_by_lat_.size() > 1) {
      y_step_ = (max_height - 2 * padding) / (i - 1);
    }
  }
}

Svg::Point ScanlineProjector::project(Coordinates point) const {
    return {
      sorted_by_lon_.at(point) * x_step_ + padding_,
      height_ - padding_ - sorted_by_lat_.at(point) * y_step_,
    };
}

