#pragma once

#include "bus.h"
#include "svg.h"
#include "transport_manager_command.h"
#include "stop.h"

#include <string>
#include <vector>
#include <map>
#include <unordered_map>

class MapBuilder {
public:
  MapBuilder(RenderSettings render_settings,
             const std::vector<Stop>& stops,
             const std::map<std::string, size_t>& stop_idx,
             const std::map<BusRoute::RouteNumber, BusRoute>& buses);

  std::string GetMap() const { return doc_.ToString(); }

private:
  RenderSettings render_settings_;

  double x_step{0};
  double y_step{0};
  std::map<size_t, size_t> longitude_sorted;
  std::map<size_t, size_t> latitude_sorted;

  Svg::Document doc_;

  const std::map<std::string, size_t>& stop_idx_;
  const std::map<BusRoute::RouteNumber, BusRoute>& buses_;

  std::vector<Stop> sorted_lon_;
  std::map<std::string, size_t> sorted_lon_idx_;
  std::vector<Stop> sorted_lat_;
  std::map<std::string, size_t> sorted_lat_idx_;

  Svg::Point MapStop(const std::string& stop_name) const;

  Svg::Document BuildMap();

  void BuildBusLines(Svg::Document& doc) const;
  void BuildBusLabels(Svg::Document& doc) const;
  void BuildStopPoints(Svg::Document& doc) const;
  void BuildStopLabels(Svg::Document& doc) const;

  static const std::unordered_map<MapLayer, void (MapBuilder::*)(Svg::Document&) const> build;
};
