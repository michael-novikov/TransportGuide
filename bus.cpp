#include "bus.h"

#include "stop.h"

#include <algorithm>
#include <iterator>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>
#include <sstream>
#include <unordered_set>
#include <vector>

using namespace std;

BusRoute::BusRoute(RouteNumber bus_no, vector<string> stops, bool is_roundtrip)
  : number_(move(bus_no))
  , stops_(move(stops))
  , stop_names_(begin(stops_), end(stops_))
  , is_roundtrip_(is_roundtrip)
{
  if (!is_roundtrip_) {
    vector<string> cyclic_route{begin(stops_), end(stops_)};
    cyclic_route.insert(end(cyclic_route), next(rbegin(stops_)), rend(stops_));
    stops_ = move(cyclic_route);
  }
}

std::pair<std::string, std::optional<std::string>> BusRoute::Endpoints() const {
  return {
    Stops().front(),
    IsRoundTrip() ? optional<string>{}: Stops()[Stops().size() / 2]
  };
}

BusRoute BusRoute::CreateRawBusRoute(RouteNumber bus_no,
                                     const std::vector<std::string>& stops) {
  return {bus_no, stops, false};
}

BusRoute BusRoute::CreateCyclicBusRoute(RouteNumber bus_no, const std::vector<std::string>& stops) {
  return {bus_no, stops, true};
}
