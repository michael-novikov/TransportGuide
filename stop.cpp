#include "stop.h"

#include <cmath>
#include <utility>

using namespace std;

Stop::Stop(string name, Coordinates coordinates)
  : name_(move(name))
  , coordinates_(move(coordinates))
{
}

double Coordinates::Distance(const Coordinates& lhs, const Coordinates& rhs) {
  auto to_radians = [](double degrees) -> double { return Coordinates::ONE_DEG * degrees; };

  auto lhs_lat = to_radians(lhs.latitude);
  auto lhs_long = to_radians(lhs.longitude);
  auto rhs_lat = to_radians(rhs.latitude);
  auto rhs_long = to_radians(rhs.longitude);

  double ans = acos(sin(lhs_lat) * sin(rhs_lat) +
                    cos(lhs_lat) * cos(rhs_lat) * cos(abs(lhs_long - rhs_long)));
  ans *= Coordinates::EARTH_RADIUS;

  return ans;
}

bool operator<(const Coordinates& lhs, const Coordinates& rhs) {
  return (lhs.longitude < rhs.longitude)
      || ((lhs.longitude == rhs.longitude) && (lhs.latitude < rhs.latitude));
}
