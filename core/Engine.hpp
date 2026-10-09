#pragma once

#include <vector>

#include "Metrics.hpp"
#include "Process.hpp"
#include "SchedulingPolicy.hpp"
#include "Timeline.hpp"

namespace scheduler {

// Settings that apply to the simulation itself rather than to any one
// algorithm.
struct SimulationOptions {
    // Ticks lost each time the CPU is handed from one process to a different
    // one. 0 - the default - treats switching as free, which is the usual
    // textbook simplification.
    //
    // Setting it above 0 is what makes Round Robin's quantum a real trade-off:
    // with free switching a quantum of 1 looks strictly best, which is the
    // opposite of what happens on real hardware.
    int contextSwitchCost = 0;
};

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
                               const SchedulingPolicy& policy,
                               const SimulationOptions& options = {});

}  // namespace scheduler
