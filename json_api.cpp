#include "json_api.h"

#include "json.h"
#include "router.pb.h"
#include "svg.h"
#include "transport_manager.h"
#include "transport_manager_command.h"

#include <cstdint>
#include <algorithm>
#include <iterator>
#include <optional>
#include <variant>
#include <stdexcept>

using namespace std;
using namespace Json;

namespace TransportGuide {
namespace JsonArgs {

InCommand ReadInputCommand(const Node& node) {
  auto command = node.AsMap();
  auto type = command["type"].AsString();
  if (type == "Stop") {
    auto stop_name = command["name"].AsString();
    double latitude = command["latitude"].AsDouble();
    double longitude = command["longitude"].AsDouble();

    std::unordered_map<std::string, unsigned int> distances;
    auto distances_nodes = command["road_distances"].AsMap();
    for (const auto& [to, road_length] : distances_nodes) {
      distances[to] = static_cast<unsigned int>(road_length.AsInt());
    }

    return NewStopCommand{string{stop_name}, latitude, longitude, distances};
  } else if (type == "Bus") {
    auto route_number = command["name"].AsString();

    vector<string> stops;
    auto stop_nodes = command["stops"].AsArray();
    transform(begin(stop_nodes), end(stop_nodes),
              back_inserter(stops),
              [](const Node& n) { return n.AsString(); });
    auto is_roundtrip = command["is_roundtrip"].AsBool();
    return NewBusCommand{route_number, stops, is_roundtrip};
  } else {
    throw invalid_argument("Unsupported command");
  }
}

OutCommand ReadOutputCommand(const Node& node) {
  auto command = node.AsMap();
  auto type = command["type"].AsString();
  auto request_id = command["id"].AsInt();
  if (type == "Stop") {
    auto stop_name = command["name"].AsString();
    return StopDescriptionCommand{stop_name, request_id};
  } else if (type == "Bus") {
    auto route_number = command["name"].AsString();
    return BusDescriptionCommand{route_number, request_id};
  } else if (type == "Route") {
    auto from = command["from"].AsString();
    auto to = command["to"].AsString();
    return RouteCommand{from, to, request_id};
  } else if (type == "Map") {
    return MapCommand{request_id};
  } else {
    throw invalid_argument("Unsupported command");
  }
}

static vector<InCommand> ParseBaseRequests(const vector<Node>& base_request_nodes) {
  vector<InCommand> base_requests;
  for (const auto& node : base_request_nodes) {
    base_requests.push_back(ReadInputCommand(node));
  }
  return base_requests;
}

static vector<OutCommand> ParseStatRequests(const vector<Node>& stat_request_nodes) {
  vector<OutCommand> stat_requests;
  for (const auto& node : stat_request_nodes) {
    stat_requests.push_back(ReadOutputCommand(node));
  }
  return stat_requests;
}

static TransportGuide::RoutingSettings ParseRoutingSettings(const map<string, Node> &routing_settings_node) {
  TransportGuide::RoutingSettings settings;
  settings.set_bus_velocity(
    routing_settings_node.at("bus_velocity").AsDouble()
  );
  settings.set_bus_wait_time(
    static_cast<uint32_t>(routing_settings_node.at("bus_wait_time").AsInt())
  );
  return settings;
}

static Color ParseColor(const Node& node) {
  Color color;
  if (node.IsArray()) {
    const auto color_array = node.AsArray();
    if (color_array.size() == 4) {
      auto rgba = color.mutable_rgba();
      rgba->set_red(static_cast<uint8_t>(color_array[0].AsInt()));
      rgba->set_green(static_cast<uint8_t>(color_array[1].AsInt()));
      rgba->set_blue(static_cast<uint8_t>(color_array[2].AsInt()));
      rgba->set_alpha(color_array[3].AsDouble());
    }
    else {
      auto rgb = color.mutable_rgb();
      rgb->set_red(static_cast<uint8_t>(color_array[0].AsInt()));
      rgb->set_green(static_cast<uint8_t>(color_array[1].AsInt()));
      rgb->set_blue(static_cast<uint8_t>(color_array[2].AsInt()));
    }
  }
  else {
    color.set_name(node.AsString());
  }
  return color;
}

static RenderSettings ParseRenderSettings(const map<string, Node>& render_settings_node) {
  RenderSettings render_settings;

  render_settings.set_width(render_settings_node.at("width").AsDouble());
  render_settings.set_height(render_settings_node.at("height").AsDouble());
  render_settings.set_padding(render_settings_node.at("padding").AsDouble());
  render_settings.set_stop_radius(render_settings_node.at("stop_radius").AsDouble());
  render_settings.set_line_width(render_settings_node.at("line_width").AsDouble());
  render_settings.set_stop_label_font_size(render_settings_node.at("stop_label_font_size").AsInt());

  const auto stop_label_offset_node = render_settings_node.at("stop_label_offset").AsArray();
  render_settings.mutable_stop_label_offset()->set_x(stop_label_offset_node[0].AsDouble());
  render_settings.mutable_stop_label_offset()->set_y(stop_label_offset_node[1].AsDouble());

  render_settings.mutable_underlayer_color()->CopyFrom(ParseColor(render_settings_node.at("underlayer_color")));
  render_settings.set_underlayer_width(render_settings_node.at("underlayer_width").AsDouble());
  render_settings.set_outer_margin(render_settings_node.at("outer_margin").AsDouble());

  const auto color_palette_node = render_settings_node.at("color_palette").AsArray();
  for (const auto& color_node : color_palette_node) {
    render_settings.add_color_palette()->CopyFrom(ParseColor(color_node));
  }

  render_settings.set_bus_label_font_size(render_settings_node.at("bus_label_font_size").AsInt());

  const auto bus_label_offset_node = render_settings_node.at("bus_label_offset").AsArray();
  render_settings.mutable_bus_label_offset()->set_x(bus_label_offset_node[0].AsDouble());
  render_settings.mutable_bus_label_offset()->set_y(bus_label_offset_node[1].AsDouble());

  const auto layers_node = render_settings_node.at("layers").AsArray();
  for (const auto& layer_item : layers_node) {
    render_settings.add_layers(layer_item.AsString());
  }

  return render_settings;
}

static SerializationSettings ParseSerializationSettings(const map<string, Node>& serialization_settings_node) {
  return {
    serialization_settings_node.at("file").AsString(),
  };
}

TransportManagerCommands ReadCommands(istream& s) {
  auto root = Load(s).GetRoot().AsMap();

  auto base_requests = ParseBaseRequests(
      root.count("base_requests") ? root.at("base_requests").AsArray() : vector<Node>{}
  );

  auto stat_requests = ParseStatRequests(
      root.count("stat_requests") ? root.at("stat_requests").AsArray() : vector<Node>{}
  );

  auto routing_settings = root.count("routing_settings")
    ? ParseRoutingSettings(root.at("routing_settings").AsMap())
    : TransportGuide::RoutingSettings{};

  auto render_settings = root.count("render_settings")
    ? ParseRenderSettings(root.at("render_settings").AsMap())
    : RenderSettings{};

  auto serialization_settings = ParseSerializationSettings(
    root.count("serialization_settings") ? root.at("serialization_settings").AsMap() : map<string, Node>{}
  );

  return TransportManagerCommands{
    base_requests,
    stat_requests,
    routing_settings,
    render_settings,
    serialization_settings,
  };
}

static std::string InsertEscapeCharacter(std::string str) {
  for (auto it = begin(str); it != end(str); ++it) {
    if (*it == '"' || *it == '\\') {
      it = str.insert(it, '\\');
      if (it != end(str)) {
        ++it;
      }
    }
  }
  return str;
}

InCommandHandler::InCommandHandler(TransportGuide::TransportManager& manager)
  : manager_(manager)
{
}

void InCommandHandler::operator()(const NewStopCommand &new_stop_command) {
  manager_.AddStop(new_stop_command.Name(), new_stop_command.Latitude(),
                   new_stop_command.Longitude(),
                   new_stop_command.Distances());
}

void InCommandHandler::operator()(const NewBusCommand &new_bus_command ) {
  manager_.AddBus(new_bus_command.Name(), new_bus_command.Stops(),
                  new_bus_command.IsCyclic());
}

OutCommandHandler::OutCommandHandler(TransportGuide::TransportManager& manager)
  : manager_(manager)
{
}

void OutCommandHandler::operator()(const StopDescriptionCommand &c) {
  results.push_back(manager_.GetStopInfo(c.Name(), c.RequestId()));
}

void OutCommandHandler::operator()(const BusDescriptionCommand &c) {
  results.push_back(manager_.GetBusInfo(c.Name(), c.RequestId()));
}

void OutCommandHandler::operator()(const RouteCommand &c) {
  results.push_back(manager_.GetRouteInfo(c.From(), c.To(), c.RequestId()));
}

void OutCommandHandler::operator()(const MapCommand &c) {
  results.push_back(manager_.GetMap(c.RequestId()));
}

void OutCommandHandler::PrintResults(std::ostream& output) const {
  JsonArgs::ResultPrinter printer(manager_);
  vector<Json::Node> result_node;

  result_node.reserve(results.size());
  transform(begin(results), end(results),
            back_inserter(result_node),
            [&printer](const Result& res) { return visit(printer, res); });

  Json::Document doc{Json::Node{move(result_node)}};
  output << doc << std::endl;
}

ResultPrinter::ResultPrinter(const TransportGuide::TransportManager& manager)
  : manager_(manager)
{
}

Json::Node ResultPrinter::operator()(const StopInfo& stop) {
  map<string, Node> stop_dict = {
    {"request_id", Node(stop.request_id)},
  };

  if (stop.error_message.has_value()) {
    stop_dict["error_message"] = Node(stop.error_message.value());
  }
  else {
    vector<Node> buses;
    buses.reserve(stop.buses.size());
    transform(begin(stop.buses), end(stop.buses),
              back_inserter(buses),
              [](const string& route_number) { return Node(route_number); });
    stop_dict["buses"] = Node(move(buses));
  }

  return Node(move(stop_dict));
}

Json::Node ResultPrinter::operator()(const BusInfo& bus) {
  map<string, Node> bus_dict = {
    {"request_id", Node(bus.request_id)},
  };

  if (bus.error_message) {
    bus_dict["error_message"] = Node(bus.error_message.value());
  }
  else {
    bus_dict["route_length"] = Node(static_cast<int>(bus.route_length));
    bus_dict["curvature"] = Node(bus.curvature);
    bus_dict["stop_count"] = Node(static_cast<int>(bus.stop_count));
    bus_dict["unique_stop_count"] = Node(static_cast<int>(bus.unique_stop_count));
  }

  return Node(move(bus_dict));
}

Json::Node ResultPrinter::operator()(const RouteDescription& route) {
  map<string, Node> route_dict = {
    {"request_id", Node(route.request_id)},
  };

  if (route.error_message) {
    route_dict["error_message"] = Node(route.error_message.value());
  }
  else {
    vector<Node> items;
    items.reserve(route.items.size());
    for (const auto& item : route.items) {
      map<string, Node> activity_node;
      if (holds_alternative<TransportGuide::WaitActivity>(item)) {
        auto wait_activity = get<TransportGuide::WaitActivity>(item);
        activity_node["type"] = string("Wait");
        activity_node["time"] = static_cast<int>(manager_.base_.router().settings().bus_wait_time());
        activity_node["stop_name"] = wait_activity.stop_name();
      }
      else {
        auto bus_activity = get<TransportGuide::BusActivity>(item);
        activity_node["type"] = string("Bus");
        activity_node["time"] = static_cast<double>(bus_activity.time());
        activity_node["bus"] = bus_activity.bus();
        activity_node["span_count"] = static_cast<int>(bus_activity.span_count());
      }
      items.push_back(activity_node);
    }
    route_dict["items"] = move(items);
    route_dict["total_time"] = route.total_time;
    route_dict["map"] = Node(InsertEscapeCharacter(route.svg_map));
  }

  return Node(move(route_dict));
}

Json::Node ResultPrinter::operator()(const MapDescription& map_description) {
  return Node(map<string, Node> {
    {"request_id", Node(map_description.request_id)},
    {"map", Node(InsertEscapeCharacter(map_description.svg_map))},
  });
}

} // namespace JsonArgs
} // namespace TransportGuide
