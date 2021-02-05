#pragma once

#include "stop.h"
#include "svg.h"

class Projector {
public:
  virtual ~Projector() {};
  virtual Svg::Point project(Coordinates point) const = 0;
  Svg::Point operator()(Coordinates point) const { return project(point); };
};

