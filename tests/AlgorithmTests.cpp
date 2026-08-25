#include <iostream>

#include "Check.hpp"
#include "Engine.hpp"
#include "Tests.hpp"
#include "policies/FcfsPolicy.hpp"
#include "policies/RoundRobinPolicy.hpp"
#include "policies/SjfPolicy.hpp"
#include "policies/SrtfPolicy.hpp"

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


void testSrtfPreemptsForAShorterJob() {
    std::cout << "SRTF hands the CPU to a shorter job the moment it arrives\n";

    SimulationResult result = runSimulation(kMixedWorkload, SrtfPolicy());

    // P1 starts alone. P2 arrives at 1 needing 3 < P1's remaining 4, so it
    // takes over. P3 arrives at 2 needing 1 and takes over again.
    // Expected: P1 [0,1)  P2 [1,2)  P3 [2,3)  P2 [3,5)  P1 [5,9)
    checkEqual(result.timeline.slices().size(), size_t{5}, "five slices - P1 and P2 both split");
    checkEqual(result.timeline.slices()[0].processId, std::string("P1"), "P1 starts");
    checkEqual(result.timeline.slices()[1].processId, std::string("P2"), "P2 takes over at 1");
    checkEqual(result.timeline.slices()[2].processId, std::string("P3"), "P3 takes over at 2");
    checkEqual(result.timeline.slices()[3].processId, std::string("P2"), "P2 resumes");
    checkEqual(result.timeline.slices()[4].processId, std::string("P1"), "P1 finishes last");

    checkEqual(result.metrics[0].completionTime, 9, "P1 finishes at 9");
    checkEqual(result.metrics[1].completionTime, 5, "P2 finishes at 5");
    checkEqual(result.metrics[2].completionTime, 3, "P3 finishes at 3");
    checkEqual(result.averages.responseTime, 0.0, "everything ran the tick it arrived");
}

void testSrtfTerminatesWhenAProcessFinishesEarly() {
    std::cout << "SRTF terminates when a short process finishes before a long one\n";

    // This exact workload sends the reference project into an infinite loop:
    // once P2 finishes, its zero remaining time makes it look like the best
    // choice forever. Here a finished process is retired by the engine and
    // cannot come back, so the run simply ends.
    const std::vector<Process> workload = {{"P1", 0, 5, 0}, {"P2", 1, 2, 0}};
    SimulationResult result = runSimulation(workload, SrtfPolicy());

    checkEqual(result.timeline.totalTime(), 7, "finishes at 7, no wasted ticks");
    checkEqual(result.timeline.busyTime(), 7, "exactly the work required");
    checkEqual(result.metrics[0].completionTime, 7, "P1 finishes at 7");
    checkEqual(result.metrics[1].completionTime, 3, "P2 finishes at 3");
}

void testSrtfDoesNotSwitchOnATie() {
    std::cout << "SRTF keeps the CPU when a tie arrives\n";

    // P2 arrives needing exactly what P1 has left. Switching would gain
    // nothing, so P1 should run straight through.
    const std::vector<Process> workload = {{"P1", 0, 4, 0}, {"P2", 2, 2, 0}};
    SimulationResult result = runSimulation(workload, SrtfPolicy());

    checkEqual(result.timeline.slices().size(), size_t{2}, "no needless context switch");
    checkEqual(result.metrics[0].completionTime, 4, "P1 runs to completion");
}

void testSrtfBeatsSjfOnWaitingTime() {
    std::cout << "SRTF gives a lower average waiting time than SJF\n";

    SimulationResult sjf = runSimulation(kMixedWorkload, SjfPolicy());
    SimulationResult srtf = runSimulation(kMixedWorkload, SrtfPolicy());

    check(srtf.averages.waitingTime < sjf.averages.waitingTime,
          "preemption pays off on average waiting time");
}

void testRoundRobinTakesTurns() {
    std::cout << "Round Robin gives each process a fixed slice in turn\n";

    SimulationResult result = runSimulation(kMixedWorkload, RoundRobinPolicy(2));

    // Expected: P1 [0,2)  P2 [2,4)  P3 [4,5)  P1 [5,7)  P2 [7,8)  P1 [8,9)
    checkEqual(result.timeline.slices().size(), size_t{6}, "six turns");
    checkEqual(result.timeline.slices()[0].processId, std::string("P1"), "P1 first slice");
    checkEqual(result.timeline.slices()[1].processId, std::string("P2"), "then P2");
    checkEqual(result.timeline.slices()[2].processId, std::string("P3"), "then P3");
    checkEqual(result.timeline.slices()[3].processId, std::string("P1"), "back round to P1");

    checkEqual(result.metrics[0].completionTime, 9, "P1 finishes at 9");
    checkEqual(result.metrics[1].completionTime, 8, "P2 finishes at 8");
    checkEqual(result.metrics[2].completionTime, 5, "P3 finishes at 5");
}

void testRoundRobinQueuesArrivalsAheadOfPreemptedProcess() {
    std::cout << "Round Robin queues a new arrival ahead of the process it preempts\n";

    // P2 arrives at tick 2, the same tick P1's quantum runs out. The arrival
    // is admitted first, so P2 runs next and P1 goes behind it.
    const std::vector<Process> workload = {{"P1", 0, 4, 0}, {"P2", 2, 2, 0}};
    SimulationResult result = runSimulation(workload, RoundRobinPolicy(2));

    checkEqual(result.timeline.slices()[0].processId, std::string("P1"), "P1 uses its slice");
    checkEqual(result.timeline.slices()[1].processId, std::string("P2"), "P2 goes next");
    checkEqual(result.timeline.slices()[2].processId, std::string("P1"), "then P1 again");
}

void testRoundRobinWithLargeQuantumBehavesLikeFcfs() {
    std::cout << "Round Robin with a quantum bigger than any burst matches FCFS\n";

    SimulationResult rr = runSimulation(kMixedWorkload, RoundRobinPolicy(100));
    SimulationResult fcfs = runSimulation(kMixedWorkload, FcfsPolicy());

    checkEqual(rr.timeline.slices().size(), fcfs.timeline.slices().size(), "same slices");
    checkEqual(rr.averages.waitingTime, fcfs.averages.waitingTime, "same waiting time");
}

void testRoundRobinRespondsFasterThanFcfs() {
    std::cout << "Round Robin gives a better average response time than FCFS\n";

    SimulationResult rr = runSimulation(kMixedWorkload, RoundRobinPolicy(2));
    SimulationResult fcfs = runSimulation(kMixedWorkload, FcfsPolicy());

    checkEqual(rr.averages.responseTime, 1.0, "everyone runs within a turn or two");
    checkEqual(fcfs.averages.responseTime, 10.0 / 3.0, "FCFS makes later arrivals wait");
    check(rr.averages.responseTime < fcfs.averages.responseTime, "Round Robin is more responsive");
}

}  // namespace

void runAlgorithmTests() {
    testFcfsRunsInArrivalOrder();
    testFcfsIdlesBetweenProcesses();
    testSjfPrefersShorterJobs();
    testSjfDoesNotInterruptARunningProcess();
    testSjfBreaksTiesByArrival();
    testSjfBeatsFcfsOnWaitingTime();
    testSrtfPreemptsForAShorterJob();
    testSrtfTerminatesWhenAProcessFinishesEarly();
    testSrtfDoesNotSwitchOnATie();
    testSrtfBeatsSjfOnWaitingTime();
    testRoundRobinTakesTurns();
    testRoundRobinQueuesArrivalsAheadOfPreemptedProcess();
    testRoundRobinWithLargeQuantumBehavesLikeFcfs();
    testRoundRobinRespondsFasterThanFcfs();
}
