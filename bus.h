#pragma once

#include "stop.h"

#include <cstddef>
#include <optional>
#include <vector>
#include <string>
#include <unordered_set>
#include <memory>
#include <utility>

class BusRoute {
public:
  using RouteNumber = std::string;

  BusRoute(RouteNumber bus_no, std::vector<std::string> stop_names, bool is_roundtrip);
  BusRoute() = default;

  RouteNumber Number() const { return number_; }
  const auto& Stop(size_t i) const { return stops_[i]; }
  const auto& Stops() const { return stops_; }
  size_t UniqueStopNumber() const { return stop_names_.size(); }
  bool IsRoundTrip() const { return is_roundtrip_; }
  std::optional<std::pair<double, double>> RouteLength() const { return route_length_; }
  bool ContainsStop(const std::string& stop_name) const { return stop_names_.count(stop_name); }
  std::pair<std::string, std::optional<std::string>> Endpoints() const;

  void SetRouteLength(size_t road_length, double direct_length) { route_length_ = {road_length, direct_length}; }

  static BusRoute CreateRawBusRoute(RouteNumber bus_no, const std::vector<std::string>& stop_names);
  static BusRoute CreateCyclicBusRoute(RouteNumber bus_no, const std::vector<std::string>& stop_names);
private:
  RouteNumber number_;
  std::vector<std::string> stops_;
  std::unordered_set<std::string> stop_names_;
  std::optional<std::pair<size_t, double>> route_length_;
  bool is_roundtrip_;
};
