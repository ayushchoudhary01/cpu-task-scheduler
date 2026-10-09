#pragma once

#include <string>
#include <vector>

#include "Engine.hpp"
#include "PolicyRegistry.hpp"
#include "Process.hpp"

namespace scheduler {

// Run one workload through several algorithms.
//
// The simulations are independent of each other, so they are run on separate
// threads and joined. Results come back in the same order as `names`.
//
// Unknown names are skipped rather than aborting the batch; callers validate
// names before getting here, so this is a backstop.
std::vector<SimulationResult> runAlgorithms(const std::vector<Process>& workload,
                                            const std::vector<std::string>& names,
                                            const PolicyOptions& policyOptions = {},
                                            const SimulationOptions& simulationOptions = {});

}  // namespace scheduler
