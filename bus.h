#pragma once

#include "bus.pb.h"

#include <utility>
#include <optional>
#include <string>

namespace TransportGuide {

std::pair<std::string, std::optional<std::string>> Endpoints(const Bus& bus);

} // namespace TransportGuide
