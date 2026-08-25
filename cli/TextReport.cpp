#include "TextReport.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace cli {
namespace {

using namespace scheduler;

const std::string kIdleLabel = "idle";
const std::string kIndent = "  ";

std::string labelFor(const TimeSlice& slice) {
    return slice.kind == SliceKind::Idle ? kIdleLabel : slice.processId;
}

// Put `text` in the middle of a field `width` characters wide.
std::string centred(const std::string& text, int width) {
    const int spare = width - static_cast<int>(text.size());
    const int left = spare / 2;
    const int right = spare - left;
    return std::string(left, ' ') + text + std::string(right, ' ');
}

// How many characters to draw per tick. Enough to fill the available width,
// but capped so a two-tick simulation does not become a banner.
int chooseScale(const Timeline& timeline, int maxWidth) {
    const int total = timeline.totalTime();
    if (total <= 0) {
        return 1;
    }
    return std::clamp(maxWidth / total, 1, 6);
}

}  // namespace

std::string renderGantt(const Timeline& timeline, int maxWidth) {
    const std::vector<TimeSlice>& slices = timeline.slices();
    if (slices.empty()) {
        return "";
    }

    const int scale = chooseScale(timeline, maxWidth);

    std::string border = "+";
    std::string labels = "|";

    // Character position of each block boundary, so the time axis below can
    // line up with the drawing rather than with the tick numbers.
    std::vector<int> boundary;
    boundary.push_back(0);

    for (const TimeSlice& slice : slices) {
        const std::string label = labelFor(slice);
        const int width =
            std::max(static_cast<int>(label.size()) + 2, slice.duration() * scale);

        border += std::string(width, '-') + "+";
        labels += centred(label, width) + "|";
        boundary.push_back(boundary.back() + width + 1);
    }

    // The axis: each boundary's tick number, skipping any that would collide
    // with the number before it.
    std::string axis;
    for (std::size_t i = 0; i < boundary.size(); ++i) {
        const int tick = (i < slices.size()) ? slices[i].start : slices.back().end;
        const std::string text = std::to_string(tick);
        const int column = boundary[i];

        if (static_cast<int>(axis.size()) > column) {
            continue;
        }
        axis += std::string(column - axis.size(), ' ');
        axis += text;
    }

    std::ostringstream out;
    out << kIndent << border << "\n"
        << kIndent << labels << "\n"
        << kIndent << border << "\n"
        << kIndent << axis << "\n";
    return out.str();
}

std::string renderMetricsTable(const std::vector<ProcessMetrics>& metrics) {
    if (metrics.empty()) {
        return "";
    }

    std::size_t idWidth = std::string("Process").size();
    for (const ProcessMetrics& row : metrics) {
        idWidth = std::max(idWidth, row.id.size());
    }

    std::ostringstream out;
    out << kIndent << std::left << std::setw(static_cast<int>(idWidth)) << "Process"
        << std::right << std::setw(10) << "Arrival" << std::setw(8) << "Burst"
        << std::setw(12) << "Completion" << std::setw(12) << "Turnaround"
        << std::setw(9) << "Waiting" << std::setw(10) << "Response" << "\n";

    for (const ProcessMetrics& row : metrics) {
        out << kIndent << std::left << std::setw(static_cast<int>(idWidth)) << row.id
            << std::right << std::setw(10) << row.arrivalTime << std::setw(8) << row.burstTime
            << std::setw(12) << row.completionTime << std::setw(12) << row.turnaroundTime
            << std::setw(9) << row.waitingTime << std::setw(10) << row.responseTime << "\n";
    }
    return out.str();
}

std::string renderAverages(const Averages& averages) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(2);
    out << kIndent << std::left << std::setw(22) << "Waiting time"
        << std::right << std::setw(8) << averages.waitingTime << "\n";
    out << kIndent << std::left << std::setw(22) << "Turnaround time"
        << std::right << std::setw(8) << averages.turnaroundTime << "\n";
    out << kIndent << std::left << std::setw(22) << "Response time"
        << std::right << std::setw(8) << averages.responseTime << "\n";
    out << kIndent << std::left << std::setw(22) << "CPU utilization"
        << std::right << std::setw(8) << averages.cpuUtilization << " %\n";
    out << kIndent << std::left << std::setw(22) << "Throughput"
        << std::right << std::setw(8) << averages.throughput << " processes/tick\n";
    return out.str();
}

std::string renderReport(const SimulationResult& result) {
    std::ostringstream out;
    out << "Algorithm: " << result.algorithm << "\n";
    out << "\nGantt chart\n" << renderGantt(result.timeline);
    out << "\nPer process\n" << renderMetricsTable(result.metrics);
    out << "\nAverages\n" << renderAverages(result.averages);
    return out.str();
}

std::string renderComparison(const std::vector<SimulationResult>& results) {
    if (results.empty()) {
        return "";
    }

    std::size_t nameWidth = std::string("Algorithm").size();
    for (const SimulationResult& result : results) {
        nameWidth = std::max(nameWidth, result.algorithm.size());
    }

    std::ostringstream out;
    out << kIndent << std::left << std::setw(static_cast<int>(nameWidth)) << "Algorithm"
        << std::right << std::setw(10) << "Waiting" << std::setw(12) << "Turnaround"
        << std::setw(10) << "Response" << std::setw(9) << "CPU %" << std::setw(8) << "Total"
        << "\n";

    out << std::fixed << std::setprecision(2);
    for (const SimulationResult& result : results) {
        out << kIndent << std::left << std::setw(static_cast<int>(nameWidth)) << result.algorithm
            << std::right << std::setw(10) << result.averages.waitingTime
            << std::setw(12) << result.averages.turnaroundTime
            << std::setw(10) << result.averages.responseTime
            << std::setw(9) << result.averages.cpuUtilization
            << std::setw(8) << result.timeline.totalTime() << "\n";
    }

    // Point out the winner on each measure. Every algorithm does the same
    // total work, so these are the differences that actually matter.
    const SimulationResult* bestWaiting = &results.front();
    const SimulationResult* bestResponse = &results.front();
    for (const SimulationResult& result : results) {
        if (result.averages.waitingTime < bestWaiting->averages.waitingTime) {
            bestWaiting = &result;
        }
        if (result.averages.responseTime < bestResponse->averages.responseTime) {
            bestResponse = &result;
        }
    }

    out << "\n" << kIndent << "Lowest average waiting time:  " << bestWaiting->algorithm
        << " (" << bestWaiting->averages.waitingTime << ")\n";
    out << kIndent << "Lowest average response time: " << bestResponse->algorithm
        << " (" << bestResponse->averages.responseTime << ")\n";
    return out.str();
}

}  // namespace cli
