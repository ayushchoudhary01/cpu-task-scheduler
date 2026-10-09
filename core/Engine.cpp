#include "Engine.hpp"

#include <algorithm>

namespace scheduler {
namespace {

// Input order does not have to be arrival order, so work out the order in
// which processes show up. Ties keep their original position.
std::vector<std::size_t> arrivalOrder(const std::vector<Process>& processes) {
    std::vector<std::size_t> order(processes.size());
    for (std::size_t i = 0; i < order.size(); ++i) {
        order[i] = i;
    }
    std::stable_sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) {
        return processes[a].arrivalTime < processes[b].arrivalTime;
    });
    return order;
}

}  // namespace

SimulationResult runSimulation(const std::vector<Process>& processes,
                               const SchedulingPolicy& policy,
                               const SimulationOptions& options) {
    SimulationResult result;
    result.algorithm = policy.name();
    result.metrics.resize(processes.size());

    if (processes.empty()) {
        return result;
    }

    const std::vector<std::size_t> order = arrivalOrder(processes);

    std::vector<ReadyProcess> ready;   // waiting for the CPU
    ReadyProcess running;              // currently on the CPU
    bool cpuBusy = false;

    int currentTime = 0;
    int ticksOnCpu = 0;                // how long `running` has held the CPU
    long long nextQueueOrder = 0;      // stamped on each process as it queues
    std::size_t nextArrival = 0;
    std::size_t completed = 0;

    // Context switching. `switchTicksLeft` counts down the overhead owed before
    // the chosen process may actually start, and `lastRunId` is whoever held
    // the CPU most recently - switching back to the same process costs nothing.
    const int switchCost = std::max(0, options.contextSwitchCost);
    int switchTicksLeft = 0;
    std::string lastRunId;

    while (completed < processes.size()) {
        // 1. Admit everything that has arrived by now.
        while (nextArrival < order.size() &&
               processes[order[nextArrival]].arrivalTime <= currentTime) {
            const std::size_t i = order[nextArrival];
            ready.push_back(
                {&processes[i], processes[i].burstTime, currentTime, nextQueueOrder++, -1, i});
            ++nextArrival;
        }

        // 2. Does the running process have to give up the CPU?
        //
        // Skipped while a context switch is in progress: once the CPU has
        // started changing hands, letting the scheduler change its mind would
        // mean paying the cost and getting nothing for it.
        if (cpuBusy && switchTicksLeft == 0) {
            const bool sliceUsedUp =
                policy.timeSlice() > 0 && ticksOnCpu >= policy.timeSlice();

            if (!ready.empty() &&
                (sliceUsedUp || policy.shouldPreempt(running, ready, currentTime))) {
                // Queued after any process admitted above, which is what puts
                // a fresh arrival ahead of one that just used up its slice.
                running.readySince = currentTime;
                running.queueOrder = nextQueueOrder++;
                ready.push_back(running);
                cpuBusy = false;
            } else if (sliceUsedUp) {
                // Nothing else wants the CPU, so it simply gets another slice.
                ticksOnCpu = 0;
            }
        }

        // 3. Hand the CPU to whoever the policy picks.
        if (!cpuBusy && switchTicksLeft == 0 && !ready.empty()) {
            const std::size_t chosen = policy.choose(ready, currentTime);
            running = ready[chosen];
            ready.erase(ready.begin() + static_cast<std::ptrdiff_t>(chosen));
            cpuBusy = true;
            ticksOnCpu = 0;

            // Taking the CPU from a different process costs time. Nothing is
            // owed for the very first dispatch, or for resuming whoever was
            // already running.
            if (switchCost > 0 && !lastRunId.empty() && lastRunId != running.process->id) {
                switchTicksLeft = switchCost;
            }
        }

        // 4. Run for exactly one tick.
        if (switchTicksLeft > 0) {
            // The CPU is busy changing hands and gets no work done.
            result.timeline.runContextSwitch(currentTime);
            switchTicksLeft -= 1;
        } else if (cpuBusy) {
            // Response time is measured from when a process actually runs, so
            // it includes any switch overhead paid to get it started.
            if (running.firstRunTime < 0) {
                running.firstRunTime = currentTime;
            }
            lastRunId = running.process->id;
            result.timeline.runProcess(currentTime, running.process->id);
            running.remainingTime -= 1;
            ticksOnCpu += 1;

            // A finished process is retired here and is never put back into
            // `ready`, so it can never be scheduled again.
            if (running.remainingTime <= 0) {
                result.metrics[running.index] =
                    makeMetrics(*running.process, running.firstRunTime, currentTime + 1);
                cpuBusy = false;
                ++completed;
            }
        } else {
            result.timeline.runIdle(currentTime);
        }

        currentTime += 1;
    }

    result.averages = computeAverages(result.metrics, result.timeline);
    return result;
}

}  // namespace scheduler
