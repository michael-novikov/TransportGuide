#pragma once

#include "svg.h"

#include "stop.pb.h"
#include "bus.pb.h"
#include "router.pb.h"

#include <algorithm>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>
#include <limits>
#include <optional>
#include <memory>
#include <variant>
#include <map>

namespace TransportGuide {

template <typename T>
using Dict = std::map<std::string, T*>;
using StopDict = Dict<Stop>;
using BusDict = Dict<Bus>;

struct SerializationSettings {
  std::string file;
 };

struct NewStopCommand {
public:
  NewStopCommand(std::string name, double latitude, double longitude, std::unordered_map<std::string, unsigned int> distances)
    : name_(move(name))
    , latitude_(latitude)
    , longitude_(longitude)
    , distances_(move(distances))
  {
  }

  std::string Name() const { return name_; }
  double Latitude() const { return latitude_; }
  double Longitude() const { return longitude_; }
  const auto& Distances() const { return distances_; }

private:
  std::string name_;
  double latitude_;
  double longitude_;
  std::unordered_map<std::string, unsigned int> distances_;
};

struct NewBusCommand {
public:
  NewBusCommand(std::string name, std::vector<std::string> stops, bool is_cyclic)
    : name_(move(name))
    , stops_(move(stops))
    , cyclic_(is_cyclic)
  {
  }

  std::string Name() const { return name_; }
  std::vector<std::string> Stops() const { return stops_; }
  bool IsCyclic() const { return cyclic_; }

private:
  std::string name_;
  std::vector<std::string> stops_;
  bool cyclic_;
};

struct OutCommandBase {
public:
  OutCommandBase(int request_id) : request_id_(request_id) {}
  int RequestId() const { return request_id_; }
protected:
  int request_id_;
};

struct StopDescriptionCommand : public OutCommandBase {
public:
  StopDescriptionCommand(std::string name, int request_id)
    : OutCommandBase(request_id)
    , name_(move(name))
  {
  }

  std::string Name() const { return name_; }

private:
  std::string name_;
};

struct BusDescriptionCommand : public OutCommandBase {
public:
  BusDescriptionCommand(std::string name, int request_id)
    : OutCommandBase(request_id)
    , name_(move(name))
  {
  }

  std::string Name() const { return name_; }

private:
  std::string name_;
};

struct RouteCommand : public OutCommandBase {
public:
  RouteCommand(std::string from, std::string to, int request_id)
    : OutCommandBase(request_id)
    , from_(move(from))
    , to_(move(to))
  {
  }

  std::string From() const { return from_; }
  std::string To() const { return to_; }

private:
  std::string from_;
  std::string to_;
};

struct MapCommand : public OutCommandBase {
public:
  MapCommand(int request_id) : OutCommandBase(request_id) {}
};

using InCommand = std::variant<NewStopCommand, NewBusCommand>;
using OutCommand = std::variant<StopDescriptionCommand, BusDescriptionCommand, RouteCommand, MapCommand>;

struct TransportManagerCommands {
  std::vector<InCommand> input_commands;
  std::vector<OutCommand> output_commands;
  TransportGuide::RoutingSettings routing_settings;
  RenderSettings render_settings = {}; // TODO: remove default initialization
  SerializationSettings serialization_settings;
};

struct StopInfo {
  std::vector<std::string> buses;
  int request_id;
  std::optional<std::string> error_message;
};

struct BusInfo {
  size_t route_length;
  int request_id;
  double curvature;
  size_t stop_count;
  size_t unique_stop_count;
  std::optional<std::string> error_message;
};

struct RouteDescription {
  using Route = std::vector<std::variant<TransportGuide::WaitActivity, TransportGuide::BusActivity>>;

  int request_id;
  double total_time;
  Route items;
  std::optional<std::string> error_message;
  std::string svg_map;
};

struct MapDescription {
  int request_id;
  std::string svg_map;
};

using Result = std::variant<
  StopInfo,
  BusInfo,
  RouteDescription,
  MapDescription
>;

}

