#pragma once

#include "stop.pb.h"

namespace TransportGuide {

constexpr const double PI = 3.1415926535;
constexpr const double ONE_DEG = PI / 180;
constexpr const double EARTH_RADIUS = 6'371'000;

double Distance(const Point& lhs, const Point& rhs);
bool operator<(const Point& lhs, const Point& rhs);

}
