#include "transport_manager.h"

#include "bus.h"
#include "graph.h"
#include "map_builder.h"
#include "router.pb.h"
#include "stop.h"
#include "transport_manager_command.h"

#include "transport_catalog.pb.h"

#include <cstdint>
#include <iterator>
#include <sstream>
#include <set>
#include <stdexcept>
#include <type_traits>
#include <unordered_set>
#include <memory>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <variant>

using namespace std;

namespace TransportGuide {

TransportManager::TransportManager(RoutingSettings routing_settings,
                                   RenderSettings render_settings,
                                   SerializationSettings serialization_settings)
  : render_settings_(std::move(render_settings))
  , serialization_settings_(std::move(serialization_settings))
{
  base_.mutable_router()->mutable_settings()->CopyFrom(routing_settings);
}

void TransportManager::InitStop(const string& name) {
  if (!stop_dict_.count(name)) {
    auto new_stop = stop_dict_[name] = base_.add_stops();
    new_stop->set_name(name);
  }
}

void TransportManager::AddStop(const string& name, double latitude, double longitude, const unordered_map<string, unsigned int>& distances) {
  InitStop(name);

  Stop *stop = stop_dict_[name];
  auto point = stop->mutable_coordinates();
  point->set_longitude(longitude);
  point->set_latitude(latitude);

  for (const auto& [stop_name, dist] : distances) {
    InitStop(stop_name);
    distances_[name][stop_name] = dist;
    if (!distances_[stop_name].count(name) || (distances_[stop_name].count(name) && distances_[stop_name][name] == 0)) {
      distances_[stop_name][name] = dist;
    }
  }
}

void TransportManager::AddBus(const std::string& name,
                              const std::vector<std::string>& stops, bool is_roundtrip) {
  //for (const auto& stop: stops) {
    //const auto& buses = stop_dict_[stop]->buses();
    //if (find(begin(buses), end(buses), bus_no) == end(buses)) {
    //  stop_dict_[stop]->add_buses(bus_no);
    //}
  //}

  auto bus = bus_dict_[name] = base_.add_buses();

  bus->set_name(name);
  bus->set_is_roundtrip(is_roundtrip);

  vector<string> route_stops(begin(stops), end(stops));
  if (!is_roundtrip) {
    route_stops.insert(end(route_stops), next(rbegin(stops)), rend(stops));
  }

  for (const auto& stop : route_stops) {
    InitStop(stop);
    bus->add_stops(stop);
  }
}

void TransportManager::ComputeBusRouteLength(const std::string& bus_name) {
  auto bus = bus_dict_.at(bus_name);

  uint32_t route_length{0};
  double direct_length{0.0};

  const auto& stops = bus->stops();
  for (size_t i = 0; i < stops.size() - 1; ++i) {
    direct_length += Distance(stop_dict_[stops[i]]->coordinates(),
                              stop_dict_[stops[i + 1]]->coordinates());
    route_length += distances_[stops[i]][stops[i + 1]];
  }

  bus->set_route_length(route_length);
  bus->set_curvature(static_cast<double>(route_length) / direct_length);

  bus->set_unique_stop_count(set(begin(bus->stops()), end(bus->stops())).size());
}

StopInfo TransportManager::GetStopInfo(const string& name, int request_id) {
  StopInfo stop_info = {
    .request_id = request_id,
  };

  if (stop_dict_.count(name)) {
    const auto& buses = stop_dict_.at(name)->buses();
    stop_info.buses = vector<string>(begin(buses), end(buses));
  }
  else {
    stop_info.error_message = "not found";
  }

  return stop_info;
}

BusInfo TransportManager::GetBusInfo(const std::string& name, int request_id) {
  if (!bus_dict_.count(name)) {
    return BusInfo{
      .request_id = request_id,
      .error_message = "not found",
    };
  }

  const auto& bus = *bus_dict_.at(name);
  return BusInfo {
    .route_length = static_cast<size_t>(bus.route_length()),
    .request_id = request_id,
    .curvature = bus.curvature(),
    .stop_count = static_cast<size_t>(bus.stops_size()),
    .unique_stop_count = static_cast<size_t>(bus.unique_stop_count()),
  };
}

void TransportManager::CreateGraph() {
  road_graph = make_unique<Graph::DirectedWeightedGraph<double>>(2 * base_.stops_size());

  Graph::VertexId vertex_id = 0;
  for (size_t i = 0; i < base_.stops_size(); ++i) {
    auto& vertex_ids = stop_vertex_ids_[base_.stops(i).name()];
    vertex_ids.in = vertex_id++;
    vertex_ids.out = vertex_id++;

    road_graph->AddEdge(Graph::Edge<double>{
        .from = vertex_ids.in,
        .to = vertex_ids.out,
        .weight = static_cast<double>(base_.router().settings().bus_wait_time()),
    });

    WaitActivity w;
    w.set_stop_name(base_.stops(i).name());
    edge_description.push_back(move(w));
  }

  for (const auto& [bus_no, bus] : bus_dict_) {
    const auto& bus_stops = bus->stops();
    for (size_t i = 0; i < bus_stops.size(); ++i) {
      double time_sum{0.0};
      unsigned int span_count{0};
      for (size_t j = i + 1; j < bus_stops.size(); ++j) {
        time_sum += distances_[bus_stops[j - 1]][bus_stops[j]]
          / (base_.router().settings().bus_velocity() * 1000 / 60);
        road_graph->AddEdge(Graph::Edge<double>{
            .from = stop_vertex_ids_[bus_stops[i]].out,
            .to = stop_vertex_ids_[bus_stops[j]].in,
            .weight = time_sum
        });

        BusActivity b;
        b.set_bus(bus_no);
        b.set_time(time_sum);
        b.set_span_count(++span_count);
        b.set_start_stop_idx(i);
        edge_description.push_back(move(b));
      }
    }
  }
}

void TransportManager::CreateRouter() {
  if (!road_graph) {
    CreateGraph();
  }
  router = make_unique<Graph::Router<double>>(*road_graph);
}

RouteDescription TransportManager::GetRouteInfo(std::string from, std::string to, int request_id) {
  if (!(route_infos.count(from) && route_infos.at(from).count(to))) {
    return {
      .request_id = request_id,
      .error_message = "not found",
    };
  }

  const auto& route_info = route_infos[from][to];

  std::vector<std::variant<WaitActivity, BusActivity>> items;
  items.reserve(route_info.edge_count);

  for (size_t i = 0; i < route_info.edge_count; ++i) {
    auto edge_id = expanded_routes_cache.at(route_info.id)[i];
    items.push_back(edge_description[edge_id]);
  }

  return {
    .request_id = request_id,
    .total_time = route_info.weight,
    .items = items,
    // .svg_map = MapBuilder{render_settings_, stop_dict_, bus_dict_}.GetRouteMap(items),
  };
}

MapDescription TransportManager::GetMap(int request_id) const {
  return {
    .request_id = request_id,
    // .svg_map = MapBuilder{render_settings_, stop_dict_, bus_dict_}.GetMap(),
  };
}

void TransportManager::FillBase() {
  for (size_t i = 0; i < base_.stops_size(); ++i) {
    set<string> buses_with_stop;
    for (const auto& bus : base_.buses()) {
      if (find(begin(bus.stops()), end(bus.stops()), base_.stops(i).name()) != end(bus.stops())) {
        buses_with_stop.insert(bus.name());
      }
    }
    auto stop_ptr = base_.mutable_stops(i);
    for (const auto& bus_name : buses_with_stop) {
      stop_ptr->add_buses(bus_name);
    }
  }

  for (const auto& bus : base_.buses()) {
    ComputeBusRouteLength(bus.name());
  }

  auto router_serialized = base_.mutable_router();
  for (const auto& activity : edge_description) {
    if (holds_alternative<WaitActivity>(activity)) {
      router_serialized->add_wait_activity()->CopyFrom(get<WaitActivity>(activity));
    } else if (holds_alternative<BusActivity>(activity)) {
      router_serialized->add_bus_activity()->CopyFrom(get<BusActivity>(activity));
    } else {
      throw std::runtime_error("Activity is not supported");
    }
  }

  for (const auto& from : base_.stops()) {
    for (const auto& to : base_.stops()) {
      size_t from_id = stop_vertex_ids_[from.name()].in;
      size_t to_id = stop_vertex_ids_[to.name()].in;

      if (auto route_info_opt = router->BuildRoute(from_id, to_id); route_info_opt) {
        auto route_info = route_info_opt.value();
        auto route_info_serialized = router_serialized->add_route_info();
        route_info_serialized->set_id(route_info.id);
        route_info_serialized->set_weight(route_info.weight);
        route_info_serialized->set_edge_count(route_info.edge_count);
        route_info_serialized->set_vertex_from(from.name());
        route_info_serialized->set_vertex_to(to.name());

        for (size_t i = 0; i < route_info.edge_count; ++i) {
          route_info_serialized->add_expanded_route(router->GetRouteEdge(route_info.id, i));
        }
      }

    }
  }
}

void TransportManager::Serialize() const {
  ofstream out_file(serialization_settings_.file);
  base_.SerializeToOstream(&out_file);
}

void TransportManager::Deserialize() {
  ifstream in_file(serialization_settings_.file);

  base_.Clear();
  base_.ParseFromIstream(&in_file);

  for (size_t i = 0; i < base_.stops_size(); ++i) {
    stop_dict_[base_.stops(i).name()] = base_.mutable_stops(i);
  }

  for (size_t i = 0; i < base_.buses_size(); ++i) {
    bus_dict_[base_.buses(i).name()] = base_.mutable_buses(i);
  }

  for (const auto& route_info_serialized : base_.router().route_info()) {
    route_infos[route_info_serialized.vertex_from()][route_info_serialized.vertex_to()] = {
      route_info_serialized.id(),
      route_info_serialized.weight(),
      route_info_serialized.edge_count(),
    };

  expanded_routes_cache[route_info_serialized.id()].reserve(route_info_serialized.expanded_route_size());
    for (const auto& edge_id : route_info_serialized.expanded_route()) {
      expanded_routes_cache[route_info_serialized.id()].push_back(edge_id);
    }
  }

  edge_description.reserve(
    base_.router().wait_activity_size() + base_.router().bus_activity_size()
  );

  for (const auto& w : base_.router().wait_activity()) {
    edge_description.push_back(w);
  }

  for (const auto& b : base_.router().bus_activity()) {
    edge_description.push_back(b);
  }
}

}
