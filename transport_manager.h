#pragma once

#include "stop.h"
#include "bus.h"
#include "transport_manager_command.h"
#include "graph.h"
#include "router.h"
#include "map_builder.h"

#include <string_view>
#include <variant>
#include <vector>
#include <map>
#include <unordered_map>
#include <memory>
#include <utility>

class TransportManager {
public:
  using RouteNumber = BusRoute::RouteNumber;

  TransportManager(RoutingSettings routing_settings, RenderSettings render_settings)
    : routing_settings_(std::move(routing_settings))
    , render_settings_(std::move(render_settings))
  {
  }

  void AddStop(const std::string& name, double latitude, double longitude, const std::unordered_map<std::string, unsigned int>& distances);
  void AddBus(const RouteNumber& route_number, const std::vector<std::string>& stop_names, bool cyclic);

  std::pair<unsigned int, double> ComputeBusRouteLength(const RouteNumber& route_number);
  StopInfo GetStopInfo(const std::string& stop_name, int request_id);
  BusInfo GetBusInfo(const RouteNumber& route_number, int request_id);

  void CreateRoutes();
  RouteInfo GetRouteInfo(std::string from, std::string to, int request_id);
  MapDescription GetMap(int request_id) const;

private:
  std::map<std::string, size_t> stop_idx_;
  std::vector<Stop> stops_;
  std::unordered_map<size_t, std::unordered_map<size_t, unsigned int>> distances_;
  std::map<RouteNumber, BusRoute> buses_;
  RoutingSettings routing_settings_;
  std::unique_ptr<Graph::DirectedWeightedGraph<double>> road_graph{nullptr};
  std::unique_ptr<Graph::Router<double>> router{nullptr};
  std::vector<std::variant<WaitActivity, BusActivity>> edge_description;
  RenderSettings render_settings_;

  void InitStop(const std::string& name);
};

