#include "bus.h"
#include "transport_manager.h"

#include "json_parser.h"
#include "stop_manager.h"
#include "transport_manager_command.h"

#include <iomanip>
#include <iostream>
#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>

using namespace std;

struct InCommandHandler {
  InCommandHandler(TransportManager& manager) : manager_(manager) {}

  TransportManager& manager_;

  void operator()(const NewStopCommand &new_stop_command) {
    manager_.AddStop(new_stop_command.Name(), new_stop_command.Latitude(),
                     new_stop_command.Longitude(),
                     new_stop_command.Distances());
  }
  void operator()(const NewBusCommand &new_bus_command ) {
    manager_.AddBus(new_bus_command.Name(), new_bus_command.Stops(),
                    new_bus_command.IsCyclic());
  }
};

struct OutCommandHandler {
  OutCommandHandler(TransportManager& manager) : manager_(manager) {}

  vector<StopInfo> stop_info_data;
  vector<BusInfo> bus_info_data;
  vector<RouteInfo> route_data;
  TransportManager& manager_;

  void operator()(const StopDescriptionCommand &c) {
    stop_info_data.push_back(manager_.GetStopInfo(c.Name(), c.RequestId()));
  }
  void operator()(const BusDescriptionCommand &c) {
    bus_info_data.push_back(manager_.GetBusInfo(c.Name(), c.RequestId()));
  }
  void operator()(const RouteCommand &c) {
    route_data.push_back(manager_.GetRouteInfo(c.From(), c.To(), c.RequestId()));
  }
};

int main() {
  auto& input = cin;
  auto& output = cout;

  TransportManagerCommands commands = JsonArgs::ReadCommands(input);

  TransportManager manager{
    RoutingSettings{commands.routing_settings.bus_wait_time, commands.routing_settings.bus_velocity}
  };

  InCommandHandler in_handler{manager};
  for (const auto& command : commands.input_commands) {
    visit(in_handler, command);
  }

  manager.CreateRoutes();
  OutCommandHandler out_handler{manager};

  for (const auto& command : commands.output_commands) {
    visit(out_handler, command);
  }

  JsonArgs::PrintResults(out_handler.stop_info_data, out_handler.bus_info_data, out_handler.route_data, output);
  output << endl;
}
