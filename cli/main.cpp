#include <iostream>
#include <vector>

#include "Metrics.hpp"
#include "Process.hpp"
#include "Timeline.hpp"

using namespace scheduler;

// Still a placeholder. It builds a timeline by hand to show the shape of the
// output we are working towards - the engine will produce these for real.
int main() {
    std::vector<Process> workload = {
        {"P1", 0, 3, 2},
        {"P2", 1, 2, 1},
    };

    Timeline timeline;
    timeline.runProcess(0, "P1");
    timeline.runProcess(1, "P1");
    timeline.runProcess(2, "P1");
    timeline.runProcess(3, "P2");
    timeline.runProcess(4, "P2");

    std::cout << "Timeline:\n";
    for (const TimeSlice& slice : timeline.slices()) {
        std::cout << "  [" << slice.start << ", " << slice.end << ")  "
                  << (slice.kind == SliceKind::Idle ? "idle" : slice.processId) << "\n";
    }

    std::vector<ProcessMetrics> metrics = {
        makeMetrics(workload[0], 0, 3),
        makeMetrics(workload[1], 3, 5),
    };
    Averages averages = computeAverages(metrics, timeline);

    std::cout << "\nAverage waiting time:    " << averages.waitingTime << "\n";
    std::cout << "Average turnaround time: " << averages.turnaroundTime << "\n";
    std::cout << "CPU utilization:         " << averages.cpuUtilization << "%\n";

    return 0;
}
