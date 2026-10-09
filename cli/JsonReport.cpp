#include "JsonReport.hpp"

#include <iomanip>
#include <sstream>

namespace cli {
namespace {

using namespace scheduler;

// Quote a string as JSON. Process ids come from user input, so anything in
// there has to survive being written out and read back.
std::string quoted(const std::string& text) {
    std::ostringstream out;
    out << '"';
    for (const char c : text) {
        switch (c) {
            case '"':  out << "\\\""; break;
            case '\\': out << "\\\\"; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    // Control characters have to be escaped numerically.
                    out << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                        << static_cast<int>(c) << std::dec << std::setfill(' ');
                } else {
                    out << c;
                }
        }
    }
    out << '"';
    return out.str();
}

// Fixed notation, never scientific - 1e-05 is valid JSON but awkward to read,
// and some parsers in the wild handle it badly.
std::string number(double value) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(4) << value;
    return out.str();
}

// "running", "idle" or "switch". A reader cannot infer this from processId
// alone any more, since both idle and switching have no process.
std::string kindName(SliceKind kind) {
    switch (kind) {
        case SliceKind::Idle:
            return "idle";
        case SliceKind::ContextSwitch:
            return "switch";
        case SliceKind::Running:
            break;
    }
    return "running";
}

std::string timelineJson(const Timeline& timeline) {
    std::ostringstream out;
    out << "[";
    const std::vector<TimeSlice>& slices = timeline.slices();
    for (std::size_t i = 0; i < slices.size(); ++i) {
        const TimeSlice& slice = slices[i];
        out << (i == 0 ? "" : ",") << "\n      {"
            << "\"start\": " << slice.start
            << ", \"end\": " << slice.end
            << ", \"kind\": " << quoted(kindName(slice.kind))
            << ", \"processId\": "
            << (slice.kind == SliceKind::Running ? quoted(slice.processId) : "null")
            << "}";
    }
    out << (slices.empty() ? "" : "\n    ") << "]";
    return out.str();
}

std::string metricsJson(const std::vector<ProcessMetrics>& metrics) {
    std::ostringstream out;
    out << "[";
    for (std::size_t i = 0; i < metrics.size(); ++i) {
        const ProcessMetrics& m = metrics[i];
        out << (i == 0 ? "" : ",") << "\n      {"
            << "\"id\": " << quoted(m.id)
            << ", \"arrivalTime\": " << m.arrivalTime
            << ", \"burstTime\": " << m.burstTime
            << ", \"completionTime\": " << m.completionTime
            << ", \"turnaroundTime\": " << m.turnaroundTime
            << ", \"waitingTime\": " << m.waitingTime
            << ", \"responseTime\": " << m.responseTime
            << "}";
    }
    out << (metrics.empty() ? "" : "\n    ") << "]";
    return out.str();
}

std::string averagesJson(const Averages& averages) {
    std::ostringstream out;
    out << "{"
        << "\"waitingTime\": " << number(averages.waitingTime)
        << ", \"turnaroundTime\": " << number(averages.turnaroundTime)
        << ", \"responseTime\": " << number(averages.responseTime)
        << ", \"cpuUtilization\": " << number(averages.cpuUtilization)
        << ", \"throughput\": " << number(averages.throughput)
        << "}";
    return out.str();
}

// One result, indented to sit inside an array.
std::string resultBody(const SimulationResult& result) {
    std::ostringstream out;
    out << "    \"algorithm\": " << quoted(result.algorithm) << ",\n"
        << "    \"totalTime\": " << result.timeline.totalTime() << ",\n"
        << "    \"busyTime\": " << result.timeline.busyTime() << ",\n"
        << "    \"switchTime\": " << result.timeline.switchTime() << ",\n"
        << "    \"timeline\": " << timelineJson(result.timeline) << ",\n"
        << "    \"processes\": " << metricsJson(result.metrics) << ",\n"
        << "    \"averages\": " << averagesJson(result.averages) << "\n";
    return out.str();
}

}  // namespace

std::string renderJson(const SimulationResult& result) {
    return "{\n" + resultBody(result) + "}\n";
}

std::string renderJson(const std::vector<SimulationResult>& results) {
    std::ostringstream out;
    out << "{\n  \"runs\": [\n";
    for (std::size_t i = 0; i < results.size(); ++i) {
        out << "  {\n" << resultBody(results[i]) << "  }" << (i + 1 == results.size() ? "" : ",")
            << "\n";
    }
    out << "  ]\n}\n";
    return out.str();
}

}  // namespace cli
