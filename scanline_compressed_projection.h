#pragma once

#include "projector_interface.h"
#include "scanline_projection.h"

#include "stop.h"
#include "bus.h"
#include "svg.h"

#include <algorithm>
#include <cmath>
#include <optional>
#include <map>

class  ScanlineCompressedProjector : public Projector {
public:
  ScanlineCompressedProjector(
      std::vector<Coordinates> points,
      const std::map<std::string, size_t>& stop_idx,
      const std::map<BusRoute::RouteNumber, BusRoute>& buses,
      double max_width, double max_height, double padding);

  virtual Svg::Point project(Coordinates point) const override;

private:
  double x_step_{0};
  double y_step_{0};

  const double height_{0};
  const double padding_;

  std::map<Coordinates, size_t, LonComparator> sorted_by_lon_;
  std::map<Coordinates, size_t, LatComparator> sorted_by_lat_;
};

