#pragma once

#include "stop.h"
#include "bus.h"
#include "transport_manager_command.h"
#include "graph.h"
#include "router.h"
#include "map_builder.h"

#include "transport_catalog.pb.h"

#include <string_view>
#include <variant>
#include <vector>
#include <map>
#include <unordered_map>
#include <memory>
#include <utility>

namespace TransportGuide {

class TransportManager {
public:
  TransportManager(RoutingSettings routing_settings,
                   RenderSettings render_settings,
                   SerializationSettings serialization_settings);

  void AddStop(const std::string& name,
               double latitude,
               double longitude,
               const std::unordered_map<std::string, unsigned int>& distances);

  void AddBus(const std::string& name,
              const std::vector<std::string>& stops, bool is_roundtrip);

  StopInfo GetStopInfo(const std::string& stop_name, int request_id);
  BusInfo GetBusInfo(const std::string& route_number, int request_id);

  void CreateGraph();
  void CreateRouter();
  RouteDescription GetRouteInfo(std::string from, std::string to, int request_id);
  MapDescription GetMap(int request_id) const;

  void FillBase();
  void Serialize() const;
  void Deserialize();

  TransportCatalog base_;
  RenderSettings render_settings_;
  SerializationSettings serialization_settings_;

private:
  std::unordered_map<std::string, std::unordered_map<std::string, unsigned int>> distances_;

  StopDict stop_dict_;
  BusDict bus_dict_;

  std::unordered_map<std::string, std::unordered_map<std::string, Graph::Router<double>::RouteInfo>> route_infos;
  std::unordered_map<Graph::Router<double>::RouteId, Graph::Router<double>::ExpandedRoute> expanded_routes_cache;

  std::unique_ptr<Graph::DirectedWeightedGraph<double>> road_graph{nullptr};
  std::unique_ptr<Graph::Router<double>> router{nullptr};
  std::vector<std::variant<WaitActivity, BusActivity>> edge_description;

  struct StopVertexIds {
    Graph::VertexId in;
    Graph::VertexId out;
  };
  std::unordered_map<std::string, StopVertexIds> stop_vertex_ids_;

  void InitStop(const std::string& name);

  void ComputeBusRouteLength(const std::string& bus_name);
};

}
