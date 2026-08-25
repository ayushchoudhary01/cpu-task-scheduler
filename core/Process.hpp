#pragma once

#include <string>

namespace scheduler {

// A process exactly as the user describes it.
//
// These four values are the *input* to a simulation. They are never modified
// while the simulation runs, so a Process can be safely copied and reused
// (for example, to run the same workload through several algorithms).
struct Process {
    std::string id;         // short label shown in output, e.g. "P1"
    int arrivalTime = 0;    // tick at which the process becomes ready
    int burstTime = 0;      // total CPU time the process needs
    int priority = 0;       // LOWER number means MORE important
};

}  // namespace scheduler
