#include <iomanip>
#include <iostream>
#include <vector>

#include "Engine.hpp"
#include "PolicyRegistry.hpp"

using namespace scheduler;

namespace {

void printResult(const SimulationResult& result) {
    std::cout << "\n" << result.algorithm << "\n";

    std::cout << "  timeline: ";
    for (const TimeSlice& slice : result.timeline.slices()) {
        std::cout << "[" << (slice.kind == SliceKind::Idle ? "idle" : slice.processId)
                  << " " << slice.start << "-" << slice.end << "] ";
    }
    std::cout << "\n";

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "  average waiting time:    " << result.averages.waitingTime << "\n";
    std::cout << "  average turnaround time: " << result.averages.turnaroundTime << "\n";
    std::cout << "  average response time:   " << result.averages.responseTime << "\n";
}

}  // namespace

// Temporary demo: a fixed workload run through every algorithm.
// Real input handling arrives with the command line interface.
int main() {
    const std::vector<Process> workload = {
        {"P1", 0, 5, 3},
        {"P2", 1, 3, 1},
        {"P3", 2, 1, 2},
    };

    std::cout << "Workload:\n";
    for (const Process& p : workload) {
        std::cout << "  " << p.id << "  arrival=" << p.arrivalTime
                  << "  burst=" << p.burstTime << "  priority=" << p.priority << "\n";
    }

    for (const std::string& name : availablePolicies()) {
        auto policy = makePolicy(name);
        printResult(runSimulation(workload, *policy));
    }

    return 0;
}
