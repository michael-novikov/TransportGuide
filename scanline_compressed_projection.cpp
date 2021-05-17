#include "scanline_compressed_projection.h"
#include "stop.h"

#include <map>
#include <set>

using namespace std;
using namespace TransportGuide;

ScanlineCompressedProjector::ScanlineCompressedProjector(const TransportGuide::StopDict& stop_dict,
                                                         const TransportGuide::BusDict& buses,
                                                         double max_width, double max_height, double padding)
  : height_(max_height)
  , padding_(padding)
{
  if (stop_dict.empty()) {
    return;
  }

  map<Point, set<Point>> route_neighbours_;
  for (const auto& [bus_name, bus] : buses) {
    const auto& bus_stops = bus->stops();
    for (int i = 1; i < bus_stops.size(); ++i) {
      route_neighbours_[stop_dict.at(bus_stops[i-1])->coordinates()].insert(stop_dict.at(bus_stops[i])->coordinates());
      route_neighbours_[stop_dict.at(bus_stops[i])->coordinates()].insert(stop_dict.at(bus_stops[i-1])->coordinates());
    }
  }

  {
    for (const auto& [name, stop] : stop_dict) {
      sorted_by_lon_[stop->coordinates()] = 0;
    }

    size_t i{0};
    for (auto it = next(begin(sorted_by_lon_)); it != end(sorted_by_lon_); ++it) {
      std::set<size_t> neigbour_idx = {};
      for (auto it_prev = begin(sorted_by_lon_); it_prev != it; ++it_prev) {
        if (route_neighbours_[it->first].count(it_prev->first)) {
          neigbour_idx.insert(it_prev->second);
        }
      }
      it->second = neigbour_idx.empty() ? 0 : *neigbour_idx.rbegin() + 1;
      i = std::max(i, it->second);
    }

    if (sorted_by_lon_.size() > 1) {
      x_step_ = (max_width - 2 * padding) / i;
    }
  }

  {
    for (const auto& [name, stop] : stop_dict) {
      sorted_by_lat_[stop->coordinates()] = 0;
    }

    size_t i{0};
    for (auto it = next(begin(sorted_by_lat_)); it != end(sorted_by_lat_); ++it) {
      std::set<size_t> neigbour_idx = {};
      for (auto it_prev = begin(sorted_by_lat_); it_prev != it; ++it_prev) {
        if (route_neighbours_[it->first].count(it_prev->first)) {
          neigbour_idx.insert(it_prev->second);
        }
      }
      it->second = neigbour_idx.empty() ? 0 : *neigbour_idx.rbegin() + 1;
      i = std::max(i, it->second);
    }

    if (sorted_by_lat_.size() > 1) {
      y_step_ = (max_height - 2 * padding_) / i;
    }
  }
}

Svg::Point ScanlineCompressedProjector::project(const TransportGuide::Point& point) const {
    return {
      sorted_by_lon_.at(point) * x_step_ + padding_,
      height_ - padding_ - sorted_by_lat_.at(point) * y_step_,
    };
}

