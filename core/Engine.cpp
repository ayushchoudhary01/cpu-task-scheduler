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
                               const SchedulingPolicy& policy) {
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
    std::size_t nextArrival = 0;
    std::size_t completed = 0;

    while (completed < processes.size()) {
        // 1. Admit everything that has arrived by now.
        while (nextArrival < order.size() &&
               processes[order[nextArrival]].arrivalTime <= currentTime) {
            const std::size_t i = order[nextArrival];
            ready.push_back({&processes[i], processes[i].burstTime, currentTime, -1, i});
            ++nextArrival;
        }

        // 2. Does the running process have to give up the CPU?
        if (cpuBusy) {
            const bool sliceUsedUp =
                policy.timeSlice() > 0 && ticksOnCpu >= policy.timeSlice();

            if (!ready.empty() &&
                (sliceUsedUp || policy.shouldPreempt(running, ready, currentTime))) {
                running.readySince = currentTime;
                ready.push_back(running);
                cpuBusy = false;
            } else if (sliceUsedUp) {
                // Nothing else wants the CPU, so it simply gets another slice.
                ticksOnCpu = 0;
            }
        }

        // 3. Hand the CPU to whoever the policy picks.
        if (!cpuBusy && !ready.empty()) {
            const std::size_t chosen = policy.choose(ready, currentTime);
            running = ready[chosen];
            ready.erase(ready.begin() + static_cast<std::ptrdiff_t>(chosen));
            cpuBusy = true;
            ticksOnCpu = 0;
            if (running.firstRunTime < 0) {
                running.firstRunTime = currentTime;
            }
        }

        // 4. Run for exactly one tick.
        if (cpuBusy) {
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
