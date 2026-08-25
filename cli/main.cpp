#include <fstream>
#include <iostream>
#include <vector>

#include "CommandLine.hpp"
#include "Engine.hpp"
#include "JsonReport.hpp"
#include "PolicyRegistry.hpp"
#include "TextReport.hpp"
#include "WorkloadParser.hpp"

using namespace scheduler;

namespace {

void printErrors(const std::string& heading, const std::vector<std::string>& errors) {
    std::cerr << heading << "\n";
    for (const std::string& error : errors) {
        std::cerr << "  " << error << "\n";
    }
}

// Run one workload through every named algorithm.
std::vector<SimulationResult> runAll(const std::vector<Process>& workload,
                                     const std::vector<std::string>& names,
                                     const PolicyOptions& options) {
    std::vector<SimulationResult> results;
    results.reserve(names.size());
    for (const std::string& name : names) {
        const auto policy = makePolicy(name, options);
        results.push_back(runSimulation(workload, *policy));
    }
    return results;
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

    const bool comparing = !options.compare.empty();
    const std::vector<std::string> names =
        comparing ? options.compare : std::vector<std::string>{options.algorithm};

    const std::vector<SimulationResult> results =
        runAll(workload.processes, names, policyOptions);

    if (options.format == cli::OutputFormat::Json) {
        std::cout << (comparing ? cli::renderJson(results) : cli::renderJson(results.front()));
        return 0;
    }

    if (comparing) {
        std::cout << "Comparison of " << results.size() << " algorithms on "
                  << workload.processes.size() << " processes\n\n";
        std::cout << cli::renderComparison(results);
    } else {
        std::cout << cli::renderReport(results.front());
    }
    return 0;
}
