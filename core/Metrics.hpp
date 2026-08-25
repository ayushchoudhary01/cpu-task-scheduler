#pragma once

#include <string>
#include <vector>

#include "Process.hpp"
#include "Timeline.hpp"

namespace scheduler {

// How one process fared in a simulation.
struct ProcessMetrics {
    std::string id;
    int arrivalTime = 0;
    int burstTime = 0;
    int completionTime = 0;   // tick at which it finished
    int turnaroundTime = 0;   // completion - arrival  (total time in the system)
    int waitingTime = 0;      // turnaround - burst    (time spent not running)
    int responseTime = 0;     // firstRun - arrival    (wait before it first ran)
};

// Summary figures across the whole simulation.
struct Averages {
    double waitingTime = 0.0;
    double turnaroundTime = 0.0;
    double responseTime = 0.0;
    double cpuUtilization = 0.0;   // percentage of ticks the CPU was busy
    double throughput = 0.0;       // processes completed per tick
};

// Build the metrics for a single process.
// `firstRunTime` is the tick it first got the CPU, `completionTime` the tick it finished.
ProcessMetrics makeMetrics(const Process& process, int firstRunTime, int completionTime);

// Average the per-process figures and add the whole-run figures.
Averages computeAverages(const std::vector<ProcessMetrics>& metrics, const Timeline& timeline);

}  // namespace scheduler
