#include <iostream>

#include "Check.hpp"
#include "Engine.hpp"
#include "Tests.hpp"
#include "policies/FcfsPolicy.hpp"
#include "policies/SjfPolicy.hpp"

using namespace scheduler;
using namespace scheduler::policies;
using testing::check;
using testing::checkEqual;

namespace {

// A workload used by several tests below, so the algorithms can be compared
// on identical input.
//   P1 arrives at 0 and needs 5 ticks
//   P2 arrives at 1 and needs 3
//   P3 arrives at 2 and needs 1
const std::vector<Process> kMixedWorkload = {
    {"P1", 0, 5, 0},
    {"P2", 1, 3, 0},
    {"P3", 2, 1, 0},
};

void testFcfsRunsInArrivalOrder() {
    std::cout << "FCFS runs processes in arrival order\n";

    SimulationResult result = runSimulation(kMixedWorkload, FcfsPolicy());

    // Expected: P1 [0,5)  P2 [5,8)  P3 [8,9)
    checkEqual(result.timeline.slices().size(), size_t{3}, "one uninterrupted block each");
    checkEqual(result.timeline.slices()[0].processId, std::string("P1"), "P1 first");
    checkEqual(result.timeline.slices()[1].processId, std::string("P2"), "P2 second");
    checkEqual(result.timeline.slices()[2].processId, std::string("P3"), "P3 last");

    checkEqual(result.metrics[0].completionTime, 5, "P1 finishes at 5");
    checkEqual(result.metrics[1].completionTime, 8, "P2 finishes at 8");
    checkEqual(result.metrics[2].completionTime, 9, "P3 finishes at 9");

    checkEqual(result.metrics[0].waitingTime, 0, "P1 never waits");
    checkEqual(result.metrics[1].waitingTime, 4, "P2 waits 4");
    checkEqual(result.metrics[2].waitingTime, 6, "P3 waits 6 - the convoy effect");
}

void testFcfsIdlesBetweenProcesses() {
    std::cout << "FCFS idles when nothing has arrived yet\n";

    const std::vector<Process> workload = {{"P1", 0, 2, 0}, {"P2", 5, 2, 0}};
    SimulationResult result = runSimulation(workload, FcfsPolicy());

    checkEqual(result.timeline.slices().size(), size_t{3}, "run, idle, run");
    check(result.timeline.slices()[1].kind == SliceKind::Idle, "middle slice is idle");
    checkEqual(result.timeline.totalTime(), 7, "finishes at 7");
    checkEqual(result.timeline.busyTime(), 4, "only 4 ticks of real work");
}

void testSjfPrefersShorterJobs() {
    std::cout << "SJF picks the shortest waiting job\n";

    SimulationResult result = runSimulation(kMixedWorkload, SjfPolicy());

    // P1 is alone at tick 0 so it starts and, being non-preemptive, finishes.
    // At tick 5 both P2 (3 ticks) and P3 (1 tick) wait, so P3 goes first.
    // Expected: P1 [0,5)  P3 [5,6)  P2 [6,9)
    checkEqual(result.timeline.slices()[0].processId, std::string("P1"), "P1 first");
    checkEqual(result.timeline.slices()[1].processId, std::string("P3"), "P3 jumps ahead");
    checkEqual(result.timeline.slices()[2].processId, std::string("P2"), "P2 last");

    checkEqual(result.metrics[1].waitingTime, 5, "P2 waits 5");
    checkEqual(result.metrics[2].waitingTime, 3, "P3 waits only 3");
}

void testSjfDoesNotInterruptARunningProcess() {
    std::cout << "SJF never interrupts a process that is already running\n";

    // P2 is much shorter but arrives after P1 has started, so it must wait.
    const std::vector<Process> workload = {{"P1", 0, 6, 0}, {"P2", 1, 1, 0}};
    SimulationResult result = runSimulation(workload, SjfPolicy());

    checkEqual(result.timeline.slices().size(), size_t{2}, "P1 runs as one unbroken block");
    checkEqual(result.metrics[0].completionTime, 6, "P1 still finishes at 6");
    checkEqual(result.metrics[1].waitingTime, 5, "P2 waits for the whole of P1");
}

void testSjfBreaksTiesByArrival() {
    std::cout << "SJF breaks equal-length ties by who waited longest\n";

    const std::vector<Process> workload = {{"P1", 0, 4, 0}, {"P2", 0, 4, 0}};
    SimulationResult result = runSimulation(workload, SjfPolicy());

    checkEqual(result.timeline.slices()[0].processId, std::string("P1"), "P1 goes first");
    checkEqual(result.metrics[0].completionTime, 4, "P1 finishes at 4");
    checkEqual(result.metrics[1].completionTime, 8, "P2 finishes at 8");
}

void testSjfBeatsFcfsOnWaitingTime() {
    std::cout << "SJF gives a lower average waiting time than FCFS\n";

    // One long process and a stream of short ones - the case SJF is built for.
    const std::vector<Process> workload = {
        {"LONG", 0, 5, 0},
        {"S1", 0, 1, 0},
        {"S2", 1, 1, 0},
        {"S3", 2, 1, 0},
    };

    SimulationResult fcfs = runSimulation(workload, FcfsPolicy());
    SimulationResult sjf = runSimulation(workload, SjfPolicy());

    checkEqual(fcfs.averages.waitingTime, 3.75, "FCFS makes the short jobs queue up");
    checkEqual(sjf.averages.waitingTime, 0.75, "SJF clears them first");
    check(sjf.averages.waitingTime < fcfs.averages.waitingTime, "SJF wins on average waiting");

    // Both do the same amount of work, so they take the same total time.
    checkEqual(fcfs.timeline.totalTime(), sjf.timeline.totalTime(), "same total runtime");
}

}  // namespace

void runAlgorithmTests() {
    testFcfsRunsInArrivalOrder();
    testFcfsIdlesBetweenProcesses();
    testSjfPrefersShorterJobs();
    testSjfDoesNotInterruptARunningProcess();
    testSjfBreaksTiesByArrival();
    testSjfBeatsFcfsOnWaitingTime();
}
