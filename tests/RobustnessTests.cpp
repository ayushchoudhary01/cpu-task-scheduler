#include <iostream>
#include <set>

#include "Check.hpp"
#include "Engine.hpp"
#include "PolicyRegistry.hpp"
#include "Tests.hpp"
#include "WorkloadGenerator.hpp"

using namespace scheduler;
using testing::check;
using testing::checkEqual;

namespace {

int totalBurst(const std::vector<Process>& processes) {
    int total = 0;
    for (const Process& process : processes) {
        total += process.burstTime;
    }
    return total;
}

// Things that must hold after any simulation, whatever the algorithm.
// Returns a description of the first broken rule, or an empty string.
std::string findBrokenInvariant(const std::vector<Process>& workload,
                                const SimulationResult& result) {
    if (result.metrics.size() != workload.size()) {
        return "not every process was reported";
    }

    // The CPU must do exactly the work asked of it - no more, no less.
    if (result.timeline.busyTime() != totalBurst(workload)) {
        return "busy time does not match the total burst time";
    }
    if (result.timeline.totalTime() < result.timeline.busyTime()) {
        return "total time is less than busy time";
    }

    // The timeline must be one continuous stretch starting at zero.
    const std::vector<TimeSlice>& slices = result.timeline.slices();
    if (!slices.empty() && slices.front().start != 0) {
        return "timeline does not start at 0";
    }
    for (std::size_t i = 1; i < slices.size(); ++i) {
        if (slices[i - 1].end != slices[i].start) {
            return "gap or overlap between timeline slices";
        }
    }

    for (std::size_t i = 0; i < result.metrics.size(); ++i) {
        const ProcessMetrics& m = result.metrics[i];
        if (m.id != workload[i].id) {
            return "metrics are not in input order";
        }
        if (m.completionTime <= m.arrivalTime) {
            return "process finished before it arrived";
        }
        if (m.waitingTime < 0 || m.responseTime < 0) {
            return "negative waiting or response time";
        }
        if (m.responseTime > m.waitingTime) {
            return "response time longer than waiting time";
        }
        if (m.turnaroundTime != m.waitingTime + m.burstTime) {
            return "turnaround does not equal waiting plus burst";
        }
    }
    return "";
}

void testInvariantsHoldForEveryAlgorithm() {
    std::cout << "Every algorithm satisfies the basic invariants\n";

    // Twenty different random workloads through all five algorithms. Cheap to
    // run, and it covers shapes nobody would think to write by hand.
    int runs = 0;
    std::string firstFailure;

    for (unsigned seed = 1; seed <= 20; ++seed) {
        WorkloadSpec spec;
        spec.count = 8;
        spec.seed = seed;
        const std::vector<Process> workload = generateWorkload(spec);

        for (const std::string& name : availablePolicies()) {
            PolicyOptions options;
            options.agingRate = 2;
            const SimulationResult result = runSimulation(workload, *makePolicy(name, options));
            ++runs;

            const std::string broken = findBrokenInvariant(workload, result);
            if (!broken.empty() && firstFailure.empty()) {
                firstFailure = name + " on seed " + std::to_string(seed) + ": " + broken;
            }
        }
    }

    checkEqual(runs, 100, "ran 20 workloads through 5 algorithms");
    checkEqual(firstFailure, std::string(""), "no invariant was broken");
}

void testGeneratorIsReproducible() {
    std::cout << "The same seed always makes the same workload\n";

    WorkloadSpec spec;
    spec.count = 6;
    spec.seed = 99;

    const std::vector<Process> first = generateWorkload(spec);
    const std::vector<Process> second = generateWorkload(spec);

    checkEqual(first.size(), second.size(), "same size");
    bool identical = true;
    for (std::size_t i = 0; i < first.size(); ++i) {
        identical = identical && first[i].id == second[i].id &&
                    first[i].arrivalTime == second[i].arrivalTime &&
                    first[i].burstTime == second[i].burstTime &&
                    first[i].priority == second[i].priority;
    }
    check(identical, "identical workloads");

    spec.seed = 100;
    const std::vector<Process> different = generateWorkload(spec);
    bool anyDifference = false;
    for (std::size_t i = 0; i < first.size(); ++i) {
        anyDifference = anyDifference || first[i].burstTime != different[i].burstTime ||
                        first[i].arrivalTime != different[i].arrivalTime;
    }
    check(anyDifference, "a different seed makes a different workload");
}

void testGeneratorRespectsItsBounds() {
    std::cout << "Generated workloads stay inside the requested ranges\n";

    WorkloadSpec spec;
    spec.count = 50;
    spec.seed = 7;
    spec.maxArrival = 4;
    spec.minBurst = 2;
    spec.maxBurst = 6;
    spec.minPriority = 1;
    spec.maxPriority = 3;

    const std::vector<Process> workload = generateWorkload(spec);

    checkEqual(workload.size(), size_t{50}, "asked for 50");

    bool inRange = true;
    bool sorted = true;
    std::set<std::string> ids;
    for (std::size_t i = 0; i < workload.size(); ++i) {
        const Process& p = workload[i];
        inRange = inRange && p.arrivalTime >= 0 && p.arrivalTime <= 4 && p.burstTime >= 2 &&
                  p.burstTime <= 6 && p.priority >= 1 && p.priority <= 3;
        ids.insert(p.id);
        if (i > 0 && workload[i - 1].arrivalTime > p.arrivalTime) {
            sorted = false;
        }
    }
    check(inRange, "every value is inside its range");
    check(sorted, "sorted by arrival time");
    checkEqual(ids.size(), size_t{50}, "every id is unique");
}

void testGeneratorHandlesSillyRequests() {
    std::cout << "Generator copes with nonsense specs\n";

    WorkloadSpec none;
    none.count = 0;
    checkEqual(generateWorkload(none).size(), size_t{0}, "zero processes");

    WorkloadSpec negative;
    negative.count = -5;
    checkEqual(generateWorkload(negative).size(), size_t{0}, "negative count");

    // Bounds the wrong way round must not hang or produce rubbish.
    WorkloadSpec backwards;
    backwards.count = 3;
    backwards.minBurst = 9;
    backwards.maxBurst = 2;
    const std::vector<Process> workload = generateWorkload(backwards);
    checkEqual(workload.size(), size_t{3}, "still produces processes");
    check(workload[0].burstTime >= 1, "burst is still usable");
}

void testInputOrderDoesNotMatter() {
    std::cout << "Processes listed out of arrival order still schedule correctly\n";

    const std::vector<Process> inOrder = {{"A", 0, 3, 0}, {"B", 1, 2, 0}, {"C", 2, 1, 0}};
    const std::vector<Process> shuffled = {{"C", 2, 1, 0}, {"A", 0, 3, 0}, {"B", 1, 2, 0}};

    const SimulationResult first = runSimulation(inOrder, *makePolicy("FCFS"));
    const SimulationResult second = runSimulation(shuffled, *makePolicy("FCFS"));

    checkEqual(first.timeline.slices().size(), second.timeline.slices().size(), "same timeline");
    checkEqual(first.averages.waitingTime, second.averages.waitingTime, "same averages");
    checkEqual(second.metrics[0].id, std::string("C"), "metrics still follow input order");
}

void testALargeWorkloadFinishes() {
    std::cout << "A large workload completes in reasonable time\n";

    WorkloadSpec spec;
    spec.count = 500;
    spec.seed = 3;
    spec.maxArrival = 200;
    spec.maxBurst = 20;
    const std::vector<Process> workload = generateWorkload(spec);

    const SimulationResult result = runSimulation(workload, *makePolicy("SRTF"));

    checkEqual(result.metrics.size(), size_t{500}, "all 500 reported");
    checkEqual(findBrokenInvariant(workload, result), std::string(""), "invariants hold");
}

void testSingleTickProcesses() {
    std::cout << "Processes that need a single tick each\n";

    const std::vector<Process> workload = {{"A", 0, 1, 0}, {"B", 0, 1, 0}, {"C", 0, 1, 0}};
    const SimulationResult result = runSimulation(workload, *makePolicy("RR"));

    checkEqual(result.timeline.totalTime(), 3, "three ticks in total");
    checkEqual(findBrokenInvariant(workload, result), std::string(""), "invariants hold");
}

void testNegativePrioritiesWork() {
    std::cout << "Negative priority numbers are allowed and mean more important\n";

    const std::vector<Process> workload = {{"NORMAL", 0, 2, 0}, {"URGENT", 0, 2, -5}};
    const SimulationResult result = runSimulation(workload, *makePolicy("Priority"));

    checkEqual(result.timeline.slices()[0].processId, std::string("URGENT"), "URGENT runs first");
}

void testLongIdleGap() {
    std::cout << "A long gap between processes is handled\n";

    const std::vector<Process> workload = {{"A", 0, 1, 0}, {"B", 1000, 1, 0}};
    const SimulationResult result = runSimulation(workload, *makePolicy("FCFS"));

    checkEqual(result.timeline.totalTime(), 1001, "runs to the late arrival");
    checkEqual(result.timeline.busyTime(), 2, "only 2 ticks of work");
    checkEqual(result.timeline.slices().size(), size_t{3}, "run, idle, run");
    check(result.averages.cpuUtilization < 1.0, "utilization is tiny");
}

}  // namespace

void runRobustnessTests() {
    testInvariantsHoldForEveryAlgorithm();
    testGeneratorIsReproducible();
    testGeneratorRespectsItsBounds();
    testGeneratorHandlesSillyRequests();
    testInputOrderDoesNotMatter();
    testALargeWorkloadFinishes();
    testSingleTickProcesses();
    testNegativePrioritiesWork();
    testLongIdleGap();
}
