#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>

#include "CommandLine.hpp"
#include "Engine.hpp"
#include "PolicyRegistry.hpp"
#include "WorkloadParser.hpp"

using namespace scheduler;

namespace {

void printErrors(const std::string& heading, const std::vector<std::string>& errors) {
    std::cerr << heading << "\n";
    for (const std::string& error : errors) {
        std::cerr << "  " << error << "\n";
    }
}

void printResult(const SimulationResult& result) {
    std::cout << result.algorithm << "\n\n";

    std::cout << "Timeline:\n";
    for (const TimeSlice& slice : result.timeline.slices()) {
        std::cout << "  [" << slice.start << ", " << slice.end << ")  "
                  << (slice.kind == SliceKind::Idle ? "idle" : slice.processId) << "\n";
    }

    std::cout << "\nPer process:\n";
    for (const ProcessMetrics& m : result.metrics) {
        std::cout << "  " << m.id << "  completed=" << m.completionTime
                  << "  turnaround=" << m.turnaroundTime << "  waiting=" << m.waitingTime
                  << "  response=" << m.responseTime << "\n";
    }

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "\nAverages:\n";
    std::cout << "  waiting time:    " << result.averages.waitingTime << "\n";
    std::cout << "  turnaround time: " << result.averages.turnaroundTime << "\n";
    std::cout << "  response time:   " << result.averages.responseTime << "\n";
    std::cout << "  CPU utilization: " << result.averages.cpuUtilization << "%\n";
}

}  // namespace

int main(int argc, char** argv) {
    const std::vector<std::string> args(argv + 1, argv + argc);
    const cli::CommandLine options = cli::parseCommandLine(args);

    if (options.showHelp) {
        std::cout << cli::helpText();
        return 0;
    }

    if (options.listAlgorithms) {
        for (const std::string& name : availablePolicies()) {
            std::cout << name << "\n";
        }
        return 0;
    }

    if (!options.ok()) {
        printErrors("Bad arguments:", options.errors);
        std::cerr << "\nRun with --help for usage.\n";
        return 1;
    }

    // Read the workload from a file if one was named, otherwise from stdin.
    std::ifstream file;
    if (!options.inputPath.empty()) {
        file.open(options.inputPath);
        if (!file) {
            std::cerr << "Cannot open workload file: " << options.inputPath << "\n";
            return 1;
        }
    }
    std::istream& input = options.inputPath.empty() ? std::cin : file;

    const cli::WorkloadParseResult workload = cli::parseWorkload(input);
    if (!workload.ok()) {
        printErrors("Problems in the workload:", workload.errors);
        return 1;
    }

    PolicyOptions policyOptions;
    policyOptions.quantum = options.quantum;
    policyOptions.agingRate = options.agingRate;

    const auto policy = makePolicy(options.algorithm, policyOptions);
    printResult(runSimulation(workload.processes, *policy));

    return 0;
}
