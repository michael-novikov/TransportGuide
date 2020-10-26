#pragma once

#include "bus.h"
#include "svg.h"
#include "transport_manager_command.h"
#include "stop.h"

#include <string>
#include <vector>
#include <map>

class MapBuilder {
public:
  MapBuilder(RenderSettings render_settings,
             const std::vector<Stop>& stops,
             const std::map<std::string, size_t>& stop_idx,
             const std::map<BusRoute::RouteNumber, BusRoute>& buses);

  std::string GetMap() const { return doc_.ToString(); }

private:
  RenderSettings render_settings_;
  double min_lat_;
  double max_lat_;
  double min_lon_;
  double max_lon_;
  double zoom_coef_;

  Svg::Document doc_;

  const std::vector<Stop>& stops_;
  const std::map<std::string, size_t>& stop_idx_;
  const std::map<BusRoute::RouteNumber, BusRoute>& buses_;

  Svg::Point MapStop(const std::string& stop_name) const;

  void BuildMap();

  void BuildBusLines();
  void BuildBusLabels();
  void BuildStopPoints();
  void BuildStopLabels();
};
