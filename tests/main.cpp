#include <iostream>

#include "Check.hpp"
#include "Metrics.hpp"
#include "Timeline.hpp"

using namespace scheduler;
using testing::check;
using testing::checkEqual;

static void testTimelineMergesRepeatedTicks() {
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

static void testTimelineSplitsOnChange() {
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

static void testEmptyTimeline() {
    std::cout << "Empty timeline reports zeroes\n";

    Timeline timeline;
    checkEqual(timeline.totalTime(), 0, "no total time");
    checkEqual(timeline.busyTime(), 0, "no busy time");
}

static void testProcessMetrics() {
    std::cout << "Per-process metrics\n";

    // P1 arrives at 2, needs 4 ticks, first runs at 5, finishes at 9.
    Process p{"P1", 2, 4, 0};
    ProcessMetrics m = makeMetrics(p, 5, 9);

    checkEqual(m.turnaroundTime, 7, "turnaround = 9 - 2");
    checkEqual(m.waitingTime, 3, "waiting = 7 - 4");
    checkEqual(m.responseTime, 3, "response = 5 - 2");
}

static void testAverages() {
    std::cout << "Averages across processes\n";

    Timeline timeline;
    timeline.runProcess(0, "P1");
    timeline.runProcess(1, "P1");
    timeline.runIdle(2);
    timeline.runProcess(3, "P2");

    std::vector<ProcessMetrics> metrics = {
        makeMetrics({"P1", 0, 2, 0}, 0, 2),
        makeMetrics({"P2", 3, 1, 0}, 3, 4),
    };

    Averages averages = computeAverages(metrics, timeline);

    checkEqual(averages.waitingTime, 0.0, "neither process waited");
    checkEqual(averages.turnaroundTime, 1.5, "turnarounds of 2 and 1 average to 1.5");
    checkEqual(averages.cpuUtilization, 75.0, "busy 3 ticks out of 4");
}

static void testAveragesOfNothing() {
    std::cout << "Averages of an empty run do not divide by zero\n";

    Timeline timeline;
    Averages averages = computeAverages({}, timeline);

    checkEqual(averages.waitingTime, 0.0, "waiting time is zero");
    checkEqual(averages.cpuUtilization, 0.0, "utilization is zero");
}

int main() {
    testTimelineMergesRepeatedTicks();
    testTimelineSplitsOnChange();
    testEmptyTimeline();
    testProcessMetrics();
    testAverages();
    testAveragesOfNothing();
    return testing::report();
}
