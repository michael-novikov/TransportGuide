#pragma once

#include "transport_manager.h"

#include "stop.h"
#include "bus.h"
#include "svg.h"

#include <algorithm>
#include <cmath>
#include <optional>
#include <map>

struct LonComparator {
  bool operator()(const TransportGuide::Point& lhs, const TransportGuide::Point& rhs) const {
    return lhs.longitude() < rhs.longitude();
  };
};

struct LatComparator {
  bool operator()(const TransportGuide::Point& lhs, const TransportGuide::Point& rhs) const {
    return lhs.latitude() < rhs.latitude();
  };
};

class ScanlineCompressedProjector {
public:
  ScanlineCompressedProjector(const TransportGuide::StopDict& stop_dict,
                              const TransportGuide::BusDict& buses,
                              double max_width, double max_height, double padding);

  Svg::Point project(const TransportGuide::Point& point) const;
  Svg::Point operator()(const TransportGuide::Point& point) const { return project(point); };

private:
  double x_step_{0};
  double y_step_{0};

  const double height_{0};
  const double padding_;

  std::map<TransportGuide::Point, size_t, LonComparator> sorted_by_lon_;
  std::map<TransportGuide::Point, size_t, LatComparator> sorted_by_lat_;
};

