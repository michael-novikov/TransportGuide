#pragma once

#include <algorithm>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>
#include <limits>
#include <optional>
#include <memory>
#include <variant>

struct RoutingSettingsCommand {
  unsigned int bus_wait_time;
  double bus_velocity;
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

struct StopDescriptionCommand {
public:
  StopDescriptionCommand(std::string name)
    : name_(move(name))
  {
  }

  StopDescriptionCommand(std::string name, size_t request_id)
    : name_(move(name))
    , request_id_(request_id)
  {
  }

  std::string Name() const { return name_; }
  size_t RequestId() const { return request_id_; }

private:
  std::string name_;
  size_t request_id_{std::numeric_limits<size_t>::max()};
};

struct BusDescriptionCommand {
public:
  BusDescriptionCommand(std::string name)
    : name_(move(name))
  {
  }

  BusDescriptionCommand(std::string name, size_t request_id)
    : name_(move(name))
    , request_id_(request_id)
  {
  }

  std::string Name() const { return name_; }
  size_t RequestId() const { return request_id_; }

private:
  std::string name_;
  size_t request_id_{std::numeric_limits<size_t>::max()};
};

struct RouteCommand {
public:
  RouteCommand(std::string from, std::string to)
    : from_(move(from))
    , to_(move(to))
  {
  }

  RouteCommand(std::string from, std::string to, size_t request_id)
    : from_(move(from))
    , to_(move(to))
    , request_id_(request_id)
  {
  }

  std::string From() const { return from_; }
  std::string To() const { return to_; }
  size_t RequestId() const { return request_id_; }

private:
  std::string from_;
  std::string to_;
  size_t request_id_{std::numeric_limits<size_t>::max()};
};

using InCommand = std::variant<NewStopCommand, NewBusCommand>;
using OutCommand = std::variant<StopDescriptionCommand, BusDescriptionCommand, RouteCommand>;

struct TransportManagerCommands {
  std::vector<InCommand> input_commands;
  std::vector<OutCommand> output_commands;
  RoutingSettingsCommand routing_settings;
};

struct StopInfo {
  std::vector<std::string> buses;
  size_t request_id;
  std::optional<std::string> error_message;
};

struct BusInfo {
  size_t route_length;
  size_t request_id;
  double curvature;
  size_t stop_count;
  size_t unique_stop_count;
  std::optional<std::string> error_message;
};

struct WaitActivity {
  std::string type;
  unsigned int time;
  std::string stop_name;
};

struct BusActivity {
  std::string type;
  double time;
  std::string bus;
  unsigned int span_count;
};

struct RouteInfo {
  size_t request_id;
  double total_time;
  std::vector<std::variant<WaitActivity, BusActivity>> items;
  std::optional<std::string> error_message;
};
