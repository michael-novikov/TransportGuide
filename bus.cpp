#include "bus.h"

namespace TransportGuide {

std::pair<std::string, std::optional<std::string>> Endpoints(const Bus& bus) {
  return {
    bus.stops(0),
    bus.is_roundtrip() ? std::optional<std::string>{}: bus.stops(bus.stops_size() / 2)
  };
}

} // namespace TransportGuide
