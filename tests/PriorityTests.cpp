#include <iostream>

#include "Check.hpp"
#include "Engine.hpp"
#include "PolicyRegistry.hpp"
#include "Tests.hpp"
#include "policies/PriorityPolicy.hpp"

using namespace scheduler;
using namespace scheduler::policies;
using testing::check;
using testing::checkEqual;

namespace {

// One unimportant process competing against a steady stream of important ones.
// Whenever the CPU frees up there is always something more urgent waiting,
// which is exactly the situation that starves LOW.
const std::vector<Process> kStarvationWorkload = {
    {"LOW", 0, 3, 5},
    {"H1", 0, 2, 1},
    {"H2", 1, 2, 1},
    {"H3", 3, 2, 1},
    {"H4", 5, 2, 1},
};

void testPriorityRunsMostImportantFirst() {
    std::cout << "Priority runs the lowest priority number first\n";

    const std::vector<Process> workload = {
        {"P1", 0, 4, 3},
        {"P2", 0, 4, 1},
        {"P3", 0, 4, 2},
    };
    SimulationResult result = runSimulation(workload, PriorityPolicy());

    checkEqual(result.timeline.slices()[0].processId, std::string("P2"), "P2 is most important");
    checkEqual(result.timeline.slices()[1].processId, std::string("P3"), "then P3");
    checkEqual(result.timeline.slices()[2].processId, std::string("P1"), "P1 last");
}

void testPriorityDoesNotInterruptARunningProcess() {
    std::cout << "Priority lets a running process finish\n";

    // P2 is far more important but arrives after P1 has started.
    const std::vector<Process> workload = {{"P1", 0, 5, 5}, {"P2", 1, 1, 1}};
    SimulationResult result = runSimulation(workload, PriorityPolicy());

    checkEqual(result.timeline.slices().size(), size_t{2}, "P1 runs as one block");
    checkEqual(result.metrics[0].completionTime, 5, "P1 finishes at 5");
    checkEqual(result.metrics[1].completionTime, 6, "P2 waits its turn");
}

void testPriorityStarvesLowPriorityWork() {
    std::cout << "Priority without aging starves an unimportant process\n";

    SimulationResult result = runSimulation(kStarvationWorkload, PriorityPolicy());

    // Every time the CPU frees up, another important process is waiting.
    checkEqual(result.metrics[0].id, std::string("LOW"), "first row is LOW");
    checkEqual(result.metrics[0].responseTime, 8, "LOW does not run until tick 8");
    checkEqual(result.metrics[0].waitingTime, 8, "LOW waits 8 ticks");
    checkEqual(result.metrics[0].completionTime, 11, "LOW finishes last, at 11");
}

void testAgingRescuesStarvedWork() {
    std::cout << "Aging rescues the starved process\n";

    // Same workload, but waiting now earns priority: one step per tick.
    SimulationResult result = runSimulation(kStarvationWorkload, PriorityPolicy(1));

    checkEqual(result.metrics[0].responseTime, 6, "LOW now runs at tick 6");
    checkEqual(result.metrics[0].waitingTime, 6, "LOW waits 6 instead of 8");
    checkEqual(result.metrics[0].completionTime, 9, "LOW finishes at 9 instead of 11");

    // The work still has to happen, so the run takes just as long overall -
    // aging redistributes waiting rather than removing it.
    SimulationResult without = runSimulation(kStarvationWorkload, PriorityPolicy());
    checkEqual(result.timeline.totalTime(), without.timeline.totalTime(), "same total runtime");
    check(result.metrics[0].waitingTime < without.metrics[0].waitingTime, "LOW waits less");
}

void testAgingIsOffByDefault() {
    std::cout << "Aging is off unless asked for\n";

    PriorityPolicy plain;
    checkEqual(plain.agingRate(), 0, "default rate is 0");
    checkEqual(plain.name(), std::string("Priority"), "plain name");

    PriorityPolicy aged(4);
    checkEqual(aged.name(), std::string("Priority(aging=4)"), "name shows the rate");
}

void testRegistryBuildsEveryAlgorithm() {
    std::cout << "Registry builds every algorithm by name\n";

    for (const std::string& name : availablePolicies()) {
        auto policy = makePolicy(name);
        check(policy != nullptr, "makePolicy handles " + name);
    }
    checkEqual(availablePolicies().size(), size_t{5}, "five algorithms available");
}

void testRegistryIgnoresCase() {
    std::cout << "Registry matches names regardless of case\n";

    check(makePolicy("fcfs") != nullptr, "lowercase works");
    check(makePolicy("SrTf") != nullptr, "mixed case works");
    checkEqual(makePolicy("rr")->name(), std::string("RR(q=2)"), "default quantum applied");
}

void testRegistryPassesOptionsThrough() {
    std::cout << "Registry passes options to the algorithms that need them\n";

    PolicyOptions options;
    options.quantum = 7;
    options.agingRate = 3;

    checkEqual(makePolicy("RR", options)->name(), std::string("RR(q=7)"), "quantum used");
    checkEqual(makePolicy("Priority", options)->name(), std::string("Priority(aging=3)"),
               "aging rate used");
    checkEqual(makePolicy("FCFS", options)->name(), std::string("FCFS"), "FCFS ignores both");
}

void testRegistryRejectsUnknownNames() {
    std::cout << "Registry returns nothing for an unknown name\n";

    check(makePolicy("MLFQ") == nullptr, "unknown algorithm");
    check(makePolicy("") == nullptr, "empty name");
}

}  // namespace

void runPriorityTests() {
    testPriorityRunsMostImportantFirst();
    testPriorityDoesNotInterruptARunningProcess();
    testPriorityStarvesLowPriorityWork();
    testAgingRescuesStarvedWork();
    testAgingIsOffByDefault();
    testRegistryBuildsEveryAlgorithm();
    testRegistryIgnoresCase();
    testRegistryPassesOptionsThrough();
    testRegistryRejectsUnknownNames();
}
