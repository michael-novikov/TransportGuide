#pragma once

#include "projector_interface.h"

#include "stop.h"
#include "svg.h"

#include <algorithm>
#include <cmath>
#include <optional>
#include <map>

struct LonComparator {
  bool operator()(const Coordinates& lhs, const Coordinates& rhs) const { return lhs.longitude < rhs.longitude; };
};

struct LatComparator {
  bool operator()(const Coordinates& lhs, const Coordinates& rhs) const { return lhs.latitude < rhs.latitude; };
};

class  ScanlineProjector : public Projector {
public:
  ScanlineProjector(std::vector<Coordinates> points, double max_width, double max_height, double padding);

  virtual Svg::Point project(Coordinates point) const override;

private:
  double x_step_{0};
  double y_step_{0};

  const double height_{0};
  const double padding_;

  std::map<Coordinates, size_t, LonComparator> sorted_by_lon_;
  std::map<Coordinates, size_t, LatComparator> sorted_by_lat_;
};

