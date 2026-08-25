#pragma once

#include <string>
#include <vector>

namespace cli {

enum class OutputFormat {
    Text,   // Gantt chart and tables for a terminal
    Json    // machine readable, used by the web interface
};

// Everything the user asked for on the command line.
struct CommandLine {
    std::string algorithm = "FCFS";
    int quantum = 2;                       // Round Robin only
    int agingRate = 0;                     // Priority only, 0 disables aging
    std::string inputPath;                 // empty means read standard input
    int generateCount = 0;                 // >0 means make up a workload instead
    unsigned seed = 1;                     // seed for --generate
    OutputFormat format = OutputFormat::Text;
    std::vector<std::string> compare;      // empty unless --compare was given
    bool showHelp = false;
    bool listAlgorithms = false;

    std::vector<std::string> errors;

    bool ok() const { return errors.empty(); }
};

// Parse arguments, excluding the program name.
// Like the workload parser, this collects every problem rather than stopping
// at the first one.
CommandLine parseCommandLine(const std::vector<std::string>& args);

// The text shown by --help.
std::string helpText();

}  // namespace cli
