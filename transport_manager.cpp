#include "transport_manager.h"

#include "bus.h"
#include "graph.h"
#include "map_builder.h"
#include "router.pb.h"
#include "stop.h"
#include "transport_manager_command.h"

#include "transport_catalog.pb.h"

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

void TransportManager::InitStop(const string& name) {
  if (!stop_idx_.count(name) || (stop_idx_.count(name) && stops_[stop_idx_[name]].Name() != name)) {
    stops_.emplace_back(name);
    stop_idx_[name] = stops_.size() - 1;
  }
}

void TransportManager::AddStop(const string& name, double latitude, double longitude, const unordered_map<string, unsigned int>& distances) {
  InitStop(name);

  size_t id = stop_idx_[name];
  stops_[id].SetCoordinates(Coordinates{latitude, longitude});

  for (const auto& [stop_name, dist] : distances) {
    InitStop(stop_name);
    distances_[id][stop_idx_[stop_name]] = dist;
    if (!distances_[stop_idx_[stop_name]].count(id) || (distances_[stop_idx_[stop_name]].count(id) && distances_[stop_idx_[stop_name]][id] == 0)) {
      distances_[stop_idx_[stop_name]][id] = dist;
    }
  }
}

void TransportManager::AddBus(const RouteNumber& bus_no, const std::vector<std::string>& stop_names, bool cyclic) {
  buses_[string{bus_no}] = cyclic ? BusRoute::CreateCyclicBusRoute(bus_no, stop_names)
    : BusRoute::CreateRawBusRoute(bus_no, stop_names);
}

std::pair<unsigned int, double> TransportManager::ComputeBusRouteLength(const RouteNumber& route_number) {
  if (!buses_.count(route_number)) {
    return {0, 0};
  }

  if (auto route_length = buses_[route_number].RouteLength(); route_length.has_value()) {
    return route_length.value();
  }

  unsigned int distance_road{0};
  double distance_direct{0.0};
  vector<const Stop*> bus_stops;
  for (const auto& stop_name : buses_[route_number].Stops()) {
    InitStop(stop_name);
    bus_stops.push_back(&stops_[stop_idx_[stop_name]]);
  }

  for (size_t i = 0; i < bus_stops.size() - 1; ++i) {
    distance_direct += Coordinates::Distance(bus_stops[i]->StopCoordinates(),
                                             bus_stops[i + 1]->StopCoordinates());
    distance_road += distances_[stop_idx_[bus_stops[i]->Name()]][stop_idx_[bus_stops[i + 1]->Name()]];
  }

  buses_[route_number].SetRouteLength(distance_road, distance_direct);
  return {distance_road, distance_direct};
}

StopInfo TransportManager::GetStopInfo(const string& stop_name, int request_id) {
  if (stop_info.count(stop_name)) {
    const auto& res = stop_info.at(stop_name)->buses();
    return StopInfo{
      .buses = vector<string>{begin(res), end(res)},
      .request_id = request_id,
    };
  }

  if (!stop_idx_.count(stop_name)) {
    return StopInfo{
      .request_id = request_id,
      .error_message = "not found",
    };
  }

  set<RouteNumber> buses_with_stop;
  for (const auto& bus : buses_) {
    if (bus.second.ContainsStop(stop_name)) {
        buses_with_stop.insert(bus.first);
    }
  }

  return StopInfo{
    .buses = vector<string>{begin(buses_with_stop), end(buses_with_stop)},
    .request_id = request_id,
  };
}

BusInfo TransportManager::GetBusInfo(const RouteNumber& bus_no, int request_id) {
  if (bus_info.count(bus_no)) {
    const auto& bus = *bus_info.at(bus_no);
    return BusInfo {
      .route_length = static_cast<size_t>(bus.route_length()),
      .request_id = request_id,
      .curvature = bus.curvature(),
      .stop_count = static_cast<size_t>(bus.stop_count()),
      .unique_stop_count = static_cast<size_t>(bus.unique_stop_count()),
    };
  }

  if (!buses_.count(bus_no)) {
    return BusInfo{
      .request_id = request_id,
      .error_message = "not found",
    };
  }

  const auto& bus = buses_.at(bus_no);
  const auto [road_length, direct_length] = ComputeBusRouteLength(bus_no);

  return BusInfo {
    .route_length = road_length,
    .request_id = request_id,
    .curvature = road_length / direct_length,
    .stop_count = bus.Stops().size(),
    .unique_stop_count = bus.UniqueStopNumber(),
  };
}

void TransportManager::CreateGraph() {
  road_graph = make_unique<Graph::DirectedWeightedGraph<double>>(2 * stops_.size());

  for (size_t i = 0; i < stops_.size(); ++i) {
    road_graph->AddEdge(Graph::Edge<double>{
        .from = 2 * i,
        .to = 2 * i + 1,
        .weight = static_cast<double>(routing_settings_.bus_wait_time),
    });
    edge_description.push_back(WaitActivity{
      .type = "Wait",
      .time = routing_settings_.bus_wait_time,
      .stop_name = stops_[i].Name(),
    });
  }

  for (const auto& [bus_no, bus] : buses_) {
    const auto& bus_stops = bus.Stops();
    for (size_t i = 0; i < bus_stops.size(); ++i) {
      double time_sum{0.0};
      unsigned int span_count{0};
      for (size_t j = i + 1; j < bus_stops.size(); ++j) {
        time_sum += distances_[stop_idx_[bus_stops[j - 1]]][stop_idx_[bus_stops[j]]]
          / (routing_settings_.bus_velocity * 1000 / 60);
        road_graph->AddEdge(Graph::Edge<double>{
            .from = 2 * stop_idx_[bus_stops[i]] + 1,
            .to = 2 * stop_idx_[bus_stops[j]],
            .weight = time_sum
        });
        edge_description.push_back(BusActivity{
          .type = "Bus",
          .time = time_sum,
          .bus = bus_no,
          .span_count = ++span_count,
          .start_stop_idx = i,
        });
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

RouteInfo TransportManager::GetRouteInfo(std::string from, std::string to, int request_id) {
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
    //.svg_map = MapBuilder{render_settings_, stops_, stop_idx_, buses_}.GetRouteMap(items),
  };
}

MapDescription TransportManager::GetMap(int request_id) const {
  return {
    .request_id = request_id,
    //.svg_map = MapBuilder{render_settings_, stops_, stop_idx_, buses_}.GetMap(),
  };
}

void TransportManager::FillBase() {
  for (const auto& stop : stops_) {
    auto stop_ptr = base_.add_stops();
    stop_ptr->set_name(stop.Name());
    for (const auto& bus : GetStopInfo(stop.Name(), -1).buses) {
      stop_ptr->add_buses(bus);
    }
  }

  for (const auto& p : buses_) {
    const auto bus_info = GetBusInfo(p.first, -1);
    auto bus = base_.add_buses();
    bus->set_name(p.first);
    bus->set_route_length(bus_info.route_length);
    bus->set_curvature(bus_info.curvature);
    bus->set_stop_count(bus_info.stop_count);
    bus->set_unique_stop_count(bus_info.unique_stop_count);
  }

  auto router_serialized = base_.mutable_router();

  auto settings = router_serialized->mutable_settings();
  settings->set_bus_wait_time(routing_settings_.bus_wait_time);
  settings->set_bus_velocity(routing_settings_.bus_velocity);

  for (const auto& activity : edge_description) {
    if (holds_alternative<WaitActivity>(activity)) {
      auto wait_activity = get<WaitActivity>(activity);
      auto wait_serialized = router_serialized->add_wait_activity();
      wait_serialized->set_stop_name(wait_activity.stop_name);
    } else if (holds_alternative<BusActivity>(activity)) {
      auto bus_activity = get<BusActivity>(activity);
      auto bus_serialized = router_serialized->add_bus_activity();
      bus_serialized->set_time(bus_activity.time);
      bus_serialized->set_bus(bus_activity.bus);
      bus_serialized->set_span_count(bus_activity.span_count);
      bus_serialized->set_start_stop_idx(bus_activity.start_stop_idx);
    } else {
      throw std::runtime_error("Activity is not supported");
    }
  }

  for (const auto& from : stops_) {
    for (const auto& to : stops_) {
      size_t from_id = 2 * stop_idx_[from.Name()];
      size_t to_id = 2 * stop_idx_[to.Name()];

      if (auto route_info_opt = router->BuildRoute(from_id, to_id); route_info_opt) {
        auto route_info = route_info_opt.value();
        auto route_info_serialized = router_serialized->add_route_info();
        route_info_serialized->set_id(route_info.id);
        route_info_serialized->set_weight(route_info.weight);
        route_info_serialized->set_edge_count(route_info.edge_count);
        route_info_serialized->set_vertex_from(from.Name());
        route_info_serialized->set_vertex_to(to.Name());

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
    stop_info[base_.stops(i).name()] = &base_.stops(i);
  }

  for (const auto& bus : base_.buses()) {
    bus_info[bus.name()] = &bus;
  }

  routing_settings_ = RoutingSettings{
    base_.router().settings().bus_wait_time(),
    base_.router().settings().bus_velocity(),
  };

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

  edge_description.reserve(base_.router().wait_activity_size() + base_.router().bus_activity_size());
  for (size_t i = 0; i < base_.router().wait_activity_size(); ++i) {
    edge_description.push_back(WaitActivity{
      .type = "Wait",
      .time = routing_settings_.bus_wait_time,
      .stop_name = base_.router().wait_activity(i).stop_name(),
    });
  }

  for (size_t i = 0; i < base_.router().bus_activity_size(); ++i) {
    const auto& bus = base_.router().bus_activity(i);
    edge_description.push_back(BusActivity{
      .type = "Bus",
      .time = bus.time(),
      .bus = bus.bus(),
      .span_count = bus.span_count(),
      .start_stop_idx = bus.start_stop_idx(),
    });
  }
}
