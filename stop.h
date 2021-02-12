#pragma once

#include <string>
#include <string_view>

struct Coordinates {
public:
  double latitude;
  double longitude;

  static double Distance(const Coordinates& lhs, const Coordinates& rhs);

private:
  static constexpr const double PI = 3.1415926535;
  static constexpr const double ONE_DEG = Coordinates::PI / 180;
  static constexpr const double EARTH_RADIUS = 6'371'000;
};

bool operator<(const Coordinates& lhs, const Coordinates& rhs);

struct Stop {
public:
  Stop(std::string name, Coordinates coordinates = {});
  Stop() = default;
  Stop(const Stop& other) = default;
  std::string Name() const { return name_; }
  const Coordinates& StopCoordinates() const { return coordinates_; }
  Coordinates& StopCoordinates() { return coordinates_; }
  void SetCoordinates(Coordinates coordinates) { coordinates_ = coordinates; }

private:
  std::string name_;
  Coordinates coordinates_;
};

