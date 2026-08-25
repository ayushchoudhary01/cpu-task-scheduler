#include <iomanip>
#include <iostream>
#include <vector>

#include "Engine.hpp"
#include "policies/FcfsPolicy.hpp"
#include "policies/SjfPolicy.hpp"

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
}

}  // namespace

// Temporary demo: a fixed workload run through the algorithms built so far.
// Real input handling arrives with the command line interface.
int main() {
    const std::vector<Process> workload = {
        {"P1", 0, 5, 0},
        {"P2", 1, 3, 0},
        {"P3", 2, 1, 0},
    };

    std::cout << "Workload:\n";
    for (const Process& p : workload) {
        std::cout << "  " << p.id << "  arrival=" << p.arrivalTime
                  << "  burst=" << p.burstTime << "\n";
    }

    printResult(runSimulation(workload, policies::FcfsPolicy()));
    printResult(runSimulation(workload, policies::SjfPolicy()));

    return 0;
}
