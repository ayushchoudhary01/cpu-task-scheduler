#include <iostream>

#include "Check.hpp"
#include "Engine.hpp"
#include "Metrics.hpp"
#include "Tests.hpp"
#include "Timeline.hpp"

using namespace scheduler;
using testing::check;
using testing::checkEqual;

namespace {

void testTimelineMergesRepeatedTicks() {
    std::cout << "Timeline merges repeated ticks\n";

    Timeline timeline;
    timeline.runProcess(0, "P1");
    timeline.runProcess(1, "P1");
    timeline.runProcess(2, "P1");

    checkEqual(timeline.slices().size(), size_t{1}, "three ticks of P1 make one slice");
    checkEqual(timeline.slices()[0].start, 0, "slice starts at 0");
    checkEqual(timeline.slices()[0].end, 3, "slice ends at 3");
    checkEqual(timeline.slices()[0].duration(), 3, "slice lasts 3 ticks");
}

void testTimelineSplitsOnChange() {
    std::cout << "Timeline starts a new slice when the process changes\n";

    Timeline timeline;
    timeline.runProcess(0, "P1");
    timeline.runProcess(1, "P2");
    timeline.runIdle(2);
    timeline.runProcess(3, "P1");

    checkEqual(timeline.slices().size(), size_t{4}, "four separate slices");
    checkEqual(timeline.totalTime(), 4, "simulation ran for 4 ticks");
    checkEqual(timeline.busyTime(), 3, "CPU was busy for 3 of them");
    check(timeline.slices()[2].kind == SliceKind::Idle, "third slice is idle");
}

void testEmptyTimeline() {
    std::cout << "Empty timeline reports zeroes\n";

    Timeline timeline;
    checkEqual(timeline.totalTime(), 0, "no total time");
    checkEqual(timeline.busyTime(), 0, "no busy time");
}

void testProcessMetrics() {
    std::cout << "Per-process metrics\n";

    Process p{"P1", 2, 4, 0};
    ProcessMetrics m = makeMetrics(p, 5, 9);

    checkEqual(m.turnaroundTime, 7, "turnaround = 9 - 2");
    checkEqual(m.waitingTime, 3, "waiting = 7 - 4");
    checkEqual(m.responseTime, 3, "response = 5 - 2");
}

void testAveragesOfNothing() {
    std::cout << "Averages of an empty run do not divide by zero\n";

    Timeline timeline;
    Averages averages = computeAverages({}, timeline);

    checkEqual(averages.waitingTime, 0.0, "waiting time is zero");
    checkEqual(averages.cpuUtilization, 0.0, "utilization is zero");
}

// Stub policies. These are not real algorithms - they exist so the engine can
// be tested on its own, including paths no sensible algorithm would take.

// Always runs whoever has been ready longest, and never interrupts anyone.
class AlwaysFirst : public SchedulingPolicy {
public:
    std::string name() const override { return "STUB-FIRST"; }
    std::size_t choose(const std::vector<ReadyProcess>&, int) const override { return 0; }
};

// Interrupts the running process on every single tick.
class AlwaysPreempt : public SchedulingPolicy {
public:
    std::string name() const override { return "STUB-PREEMPT"; }
    std::size_t choose(const std::vector<ReadyProcess>&, int) const override { return 0; }
    bool shouldPreempt(const ReadyProcess&, const std::vector<ReadyProcess>&, int) const override {
        return true;
    }
};

void testEngineRunsOneProcess() {
    std::cout << "Engine runs a single process straight through\n";

    std::vector<Process> workload = {{"P1", 0, 3, 0}};
    SimulationResult result = runSimulation(workload, AlwaysFirst());

    checkEqual(result.timeline.slices().size(), size_t{1}, "one slice");
    checkEqual(result.timeline.totalTime(), 3, "finished at tick 3");
    checkEqual(result.metrics[0].completionTime, 3, "completed at 3");
    checkEqual(result.metrics[0].waitingTime, 0, "never waited");
    checkEqual(result.metrics[0].responseTime, 0, "ran immediately");
}

void testEngineIdlesUntilArrival() {
    std::cout << "Engine idles while nothing has arrived\n";

    std::vector<Process> workload = {{"P1", 2, 2, 0}};
    SimulationResult result = runSimulation(workload, AlwaysFirst());

    checkEqual(result.timeline.slices().size(), size_t{2}, "an idle slice then a running one");
    check(result.timeline.slices()[0].kind == SliceKind::Idle, "starts idle");
    checkEqual(result.timeline.totalTime(), 4, "finished at tick 4");
    checkEqual(result.timeline.busyTime(), 2, "busy for 2 ticks");
    checkEqual(result.averages.cpuUtilization, 50.0, "CPU used half the time");
}

void testEngineKeepsInputOrderInMetrics() {
    std::cout << "Engine reports metrics in input order, not completion order\n";

    std::vector<Process> workload = {{"P2", 5, 1, 0}, {"P1", 0, 1, 0}};
    SimulationResult result = runSimulation(workload, AlwaysFirst());

    checkEqual(result.metrics[0].id, std::string("P2"), "first row is P2");
    checkEqual(result.metrics[1].id, std::string("P1"), "second row is P1");
}

void testFinishedProcessIsNeverRescheduled() {
    std::cout << "A finished process is never scheduled again\n";

    // This is the shape of bug that hangs the reference project: a completed
    // process going back into the ready queue and being picked forever.
    std::vector<Process> workload = {{"P1", 0, 2, 0}, {"P2", 0, 2, 0}};
    SimulationResult result = runSimulation(workload, AlwaysPreempt());

    checkEqual(result.timeline.totalTime(), 4, "total time is exactly the work required");
    checkEqual(result.timeline.busyTime(), 4, "no tick was wasted on a finished process");
    checkEqual(result.metrics[0].completionTime, 3, "P1 finished at 3");
    checkEqual(result.metrics[1].completionTime, 4, "P2 finished at 4");
    checkEqual(result.averages.cpuUtilization, 100.0, "CPU fully used");
}

void testEmptyWorkload() {
    std::cout << "Engine handles an empty workload\n";

    SimulationResult result = runSimulation({}, AlwaysFirst());
    checkEqual(result.metrics.size(), size_t{0}, "no metrics");
    checkEqual(result.timeline.totalTime(), 0, "no time passed");
}

}  // namespace

void runCoreTests() {
    testTimelineMergesRepeatedTicks();
    testTimelineSplitsOnChange();
    testEmptyTimeline();
    testProcessMetrics();
    testAveragesOfNothing();
    testEngineRunsOneProcess();
    testEngineIdlesUntilArrival();
    testEngineKeepsInputOrderInMetrics();
    testFinishedProcessIsNeverRescheduled();
    testEmptyWorkload();
}
