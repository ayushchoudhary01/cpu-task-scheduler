#pragma once

#include <vector>

#include "Metrics.hpp"
#include "Process.hpp"
#include "SchedulingPolicy.hpp"
#include "Timeline.hpp"

namespace scheduler {

// Everything one simulation produces.
struct SimulationResult {
    std::string algorithm;
    Timeline timeline;
    std::vector<ProcessMetrics> metrics;   // same order as the input processes
    Averages averages;
};

// Run `processes` through `policy`, one tick at a time.
//
// The input is not modified, so the same workload can be run through several
// policies and compared.
SimulationResult runSimulation(const std::vector<Process>& processes,
                               const SchedulingPolicy& policy);

}  // namespace scheduler
