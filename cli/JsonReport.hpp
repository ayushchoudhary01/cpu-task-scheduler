#pragma once

#include <string>
#include <vector>

#include "Engine.hpp"

namespace cli {

// JSON for one simulation. This is the shape the web interface reads, so the
// field names match what the browser expects.
std::string renderJson(const scheduler::SimulationResult& result);

// JSON for several simulations of the same workload, as {"runs": [...]}.
std::string renderJson(const std::vector<scheduler::SimulationResult>& results);

}  // namespace cli
