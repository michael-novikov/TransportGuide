#pragma once

#include "transport_manager.h"
#include "transport_manager_command.h"
#include "json.h"

#include <memory>
#include <string_view>
#include <iostream>
#include <vector>
#include <variant>

namespace TransportGuide {
namespace JsonArgs {

TransportManagerCommands ReadCommands(std::istream& s);

class InCommandHandler {
public:
  InCommandHandler(TransportManager& manager);

  void operator()(const NewStopCommand &new_stop_command);
  void operator()(const NewBusCommand &new_bus_command );

private:
  TransportManager& manager_;
};

class OutCommandHandler {
public:
  OutCommandHandler(TransportManager& manager);

  void operator()(const StopDescriptionCommand &c);
  void operator()(const BusDescriptionCommand &c);
  void operator()(const RouteCommand &c);
  void operator()(const MapCommand &c);

  void PrintResults(std::ostream& output) const;

private:
  std::vector<Result> results;
  TransportManager& manager_;
};

class ResultPrinter {
public:
  ResultPrinter(const TransportManager& manager);

  static void Print(const Result& res);
  static void Print(const std::vector<Result>& res);

  Json::Node operator()(const StopInfo& res);
  Json::Node operator()(const BusInfo& res);
  Json::Node operator()(const RouteDescription& res);
  Json::Node operator()(const MapDescription& res);

private:
  const TransportManager& manager_;
};

} // namespace JsonArgs
} // namespace TransportGuide
