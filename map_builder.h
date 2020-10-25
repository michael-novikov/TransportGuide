#pragma once

#include "bus.h"
#include "transport_manager_command.h"
#include "stop.h"

#include <string>
#include <vector>
#include <map>

class MapBuilder {
public:
  MapBuilder(RenderSettings render_settings);
  std::string BuildMap(const std::vector<Stop>& stops,
                       const std::map<std::string, size_t>& stop_idx,
                       const std::map<BusRoute::RouteNumber, BusRoute>& buses) const;

private:
  RenderSettings render_settings_;

  double ComputeZoomCoef(const std::vector<Stop>& stops) const;
};
