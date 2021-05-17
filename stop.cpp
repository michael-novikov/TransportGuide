#include "stop.h"

#include <cmath>
#include <utility>

using namespace std;

namespace TransportGuide {

double Distance(const Point& lhs, const Point& rhs) {
  auto to_radians = [](double degrees) -> double { return ONE_DEG * degrees; };

  auto lhs_lat = to_radians(lhs.latitude());
  auto lhs_long = to_radians(lhs.longitude());
  auto rhs_lat = to_radians(rhs.latitude());
  auto rhs_long = to_radians(rhs.longitude());

  double ans = acos(sin(lhs_lat) * sin(rhs_lat) +
                    cos(lhs_lat) * cos(rhs_lat) * cos(abs(lhs_long - rhs_long)));
  ans *= EARTH_RADIUS;

  return ans;
}

bool operator<(const Point& lhs, const Point& rhs) {
  return (lhs.longitude() < rhs.longitude())
      || ((lhs.longitude() == rhs.longitude()) && (lhs.latitude() < rhs.latitude()));
}

}
