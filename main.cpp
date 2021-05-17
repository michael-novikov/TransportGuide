#include "bus.h"
#include "transport_manager.h"
#include "json_api.h"
#include "stop.h"
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
using namespace TransportGuide;
using namespace TransportGuide::JsonArgs;

void usage() {
  cerr << "Usage: transport_guide [make_base|process_requests]" << endl;
}

int main(int argc, const char *argv[]) {
  if (argc != 2) {
    usage();
    return 5;
  }

  auto& input = cin;
  auto& output = cout;

  TransportManagerCommands commands = ReadCommands(input);

  TransportManager manager{
    commands.routing_settings,
    commands.render_settings,
    commands.serialization_settings,
  };

  const string_view mode(argv[1]);
  if (mode == "make_base") {
    InCommandHandler in_handler{manager};
    for (const auto& command : commands.input_commands) {
      visit(in_handler, command);
    }
    manager.CreateRouter();
    manager.FillBase();
    manager.Serialize();
  }
  else if (mode == "process_requests") {
    manager.Deserialize();
    OutCommandHandler out_handler{manager};
    for (const auto& command : commands.output_commands) {
      visit(out_handler, command);
    }
    out_handler.PrintResults(output);
  }
  else {
    cerr << "invalid argument: run mode" << endl;
    usage();
    return 5;
  }
}
