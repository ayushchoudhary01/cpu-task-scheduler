#pragma once

#include <vector>

#include "Process.hpp"

namespace scheduler {

// What kind of workload to make up.
struct WorkloadSpec {
    int count = 5;          // how many processes
    unsigned seed = 1;      // same seed gives the same workload every time
    int maxArrival = 10;    // arrivals are spread over 0..maxArrival
    int minBurst = 1;
    int maxBurst = 10;
    int minPriority = 0;
    int maxPriority = 5;
};

// Build a random workload. Processes come back sorted by arrival time and are
// named P1, P2, and so on.

// The same seed always produces the same workload, which matters: a comparison
// between algorithms is only meaningful if they ran on identical input, and a
// surprising result is only worth investigating if it can be reproduced.
std::vector<Process> generateWorkload(const WorkloadSpec& spec);

}  // namespace scheduler
