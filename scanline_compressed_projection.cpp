#include "scanline_compressed_projection.h"
#include "stop.h"

#include <map>
#include <set>

ScanlineCompressedProjector::ScanlineCompressedProjector(
      std::vector<Coordinates> points,
      const std::map<std::string, size_t>& stop_idx,
      const std::map<BusRoute::RouteNumber, BusRoute>& buses,
      double max_width, double max_height, double padding)
  : height_(max_height)
  , padding_(padding)
{
  if (points.empty()) {
    return;
  }

  std::map<Coordinates, std::set<Coordinates>> route_neighbours_;
  for (const auto& [bus_name, bus] : buses) {
    const auto& bus_stops = bus.Stops();
    for (int i = 1; i < bus_stops.size(); ++i) {
      const auto& cur = points[stop_idx.at(bus_stops[i])];
      const auto& prev = points[stop_idx.at(bus_stops[i-1])];
      route_neighbours_[prev].insert(cur);
      route_neighbours_[cur].insert(prev);
    }
  }

  {
    for (const auto& p : points) {
      sorted_by_lon_[p] = 0;
    }

    size_t i{0};
    std::set<Coordinates, LonComparator> group;
    for (auto& [point, idx] : sorted_by_lon_) {
      bool neigbour{false};
      for (const auto& p : group) {
        if (route_neighbours_[point].count(p)) {
          neigbour = true;
          break;
        }
      }
      if (neigbour) {
        i++;
        group.clear();
      }
      idx = i;
      group.insert(point);
    }

    if (sorted_by_lon_.size() > 1) {
      x_step_ = (max_width - 2 * padding) / i;
    }
  }

  {
    for (const auto& p : points) {
      sorted_by_lat_[p] = 0;
    }

    size_t i{0};
    std::set<Coordinates, LonComparator> group;
    for (auto& [point, idx] : sorted_by_lat_) {
      bool neigbour{false};
      for (const auto& p : group) {
        if (route_neighbours_[point].count(p)) {
          neigbour = true;
          break;
        }
      }
      if (neigbour) {
        i++;
        group.clear();
      }
      idx = i;
      group.insert(point);
    }

    if (sorted_by_lat_.size() > 1) {
      y_step_ = (max_height - 2 * padding_) / i;
    }
  }
}

Svg::Point ScanlineCompressedProjector::project(Coordinates point) const {
    return {
      sorted_by_lon_.at(point) * x_step_ + padding_,
      height_ - padding_ - sorted_by_lat_.at(point) * y_step_,
    };
}

