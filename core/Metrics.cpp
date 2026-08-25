#include "Metrics.hpp"

namespace scheduler {

ProcessMetrics makeMetrics(const Process& process, int firstRunTime, int completionTime) {
    ProcessMetrics m;
    m.id = process.id;
    m.arrivalTime = process.arrivalTime;
    m.burstTime = process.burstTime;
    m.completionTime = completionTime;
    m.turnaroundTime = completionTime - process.arrivalTime;
    m.waitingTime = m.turnaroundTime - process.burstTime;
    m.responseTime = firstRunTime - process.arrivalTime;
    return m;
}

Averages computeAverages(const std::vector<ProcessMetrics>& metrics, const Timeline& timeline) {
    Averages averages;
    if (metrics.empty()) {
        return averages;
    }

    int totalWaiting = 0;
    int totalTurnaround = 0;
    int totalResponse = 0;
    for (const ProcessMetrics& m : metrics) {
        totalWaiting += m.waitingTime;
        totalTurnaround += m.turnaroundTime;
        totalResponse += m.responseTime;
    }

    const double count = static_cast<double>(metrics.size());
    averages.waitingTime = totalWaiting / count;
    averages.turnaroundTime = totalTurnaround / count;
    averages.responseTime = totalResponse / count;

    const int total = timeline.totalTime();
    if (total > 0) {
        averages.cpuUtilization = (timeline.busyTime() * 100.0) / total;
        averages.throughput = count / total;
    }
    return averages;
}

}  // namespace scheduler
