#include "WorkloadGenerator.hpp"

#include <algorithm>
#include <random>

namespace scheduler {
namespace {

// Pick a number in [low, high].

// This does the modulo itself rather than using std::uniform_int_distribution,
// whose output is allowed to differ between standard libraries. The tiny bias
// that introduces does not matter for made-up test data, and in exchange a
// given seed produces the same workload wherever the project is built.
int pick(std::mt19937& rng, int low, int high) {
    if (high <= low) {
        return low;
    }
    const unsigned span = static_cast<unsigned>(high - low + 1);
    return low + static_cast<int>(rng() % span);
}

}  // namespace

std::vector<Process> generateWorkload(const WorkloadSpec& spec) {
    std::vector<Process> processes;
    if (spec.count <= 0) {
        return processes;
    }

    std::mt19937 rng(spec.seed);
    processes.reserve(static_cast<std::size_t>(spec.count));

    for (int i = 0; i < spec.count; ++i) {
        Process process;
        process.id = "P" + std::to_string(i + 1);
        process.arrivalTime = pick(rng, 0, std::max(0, spec.maxArrival));
        process.burstTime = pick(rng, std::max(1, spec.minBurst), std::max(1, spec.maxBurst));
        process.priority = pick(rng, spec.minPriority, spec.maxPriority);
        processes.push_back(process);
    }

    // Sorted by arrival so the printed workload reads in the order things
    // happen. Ties keep their numbering.
    std::stable_sort(processes.begin(), processes.end(),
                     [](const Process& a, const Process& b) {
                         return a.arrivalTime < b.arrivalTime;
                     });
    return processes;
}

}  // namespace scheduler
