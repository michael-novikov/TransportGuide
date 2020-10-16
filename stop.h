#pragma once

#include <string>
#include <string_view>

struct Coordinates {
public:
  long double latitude;
  long double longitude;

  static long double Distance(const Coordinates& lhs, const Coordinates& rhs);

private:
  static constexpr const long double PI = 3.1415926535;
  static constexpr const long double ONE_DEG = Coordinates::PI / 180;
  static constexpr const long double EARTH_RADIUS = 6'371'000;
};

struct Stop {
public:
  Stop(std::string name, Coordinates coordinates = {});
  Stop() = default;
  std::string Name() const { return name_; }
  Coordinates StopCoordinates() const { return coordinates_; }
  void SetCoordinates(Coordinates coordinates) { coordinates_ = coordinates; }

private:
  std::string name_;
  Coordinates coordinates_;
};

