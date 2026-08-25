#include "CommandLine.hpp"

#include <charconv>

#include "PolicyRegistry.hpp"

namespace cli {
namespace {

bool parseInt(const std::string& text, int& out) {
    if (text.empty()) {
        return false;
    }
    const char* begin = text.data();
    const char* end = begin + text.size();
    const std::from_chars_result result = std::from_chars(begin, end, out);
    return result.ec == std::errc() && result.ptr == end;
}

// Read the value that follows an option, e.g. the "5" in "--quantum 5".
// Advances `i` past the value. Returns false if there was nothing to read.
bool takeValue(const std::vector<std::string>& args, std::size_t& i, std::string& out) {
    if (i + 1 >= args.size()) {
        return false;
    }
    out = args[++i];
    return true;
}

}  // namespace

CommandLine parseCommandLine(const std::vector<std::string>& args) {
    CommandLine parsed;

    for (std::size_t i = 0; i < args.size(); ++i) {
        const std::string& arg = args[i];
        std::string value;

        if (arg == "-h" || arg == "--help") {
            parsed.showHelp = true;
        } else if (arg == "-l" || arg == "--list") {
            parsed.listAlgorithms = true;
        } else if (arg == "-a" || arg == "--algorithm") {
            if (!takeValue(args, i, value)) {
                parsed.errors.push_back(arg + " needs an algorithm name");
            } else if (scheduler::makePolicy(value) == nullptr) {
                parsed.errors.push_back("unknown algorithm '" + value + "'");
            } else {
                parsed.algorithm = value;
            }
        } else if (arg == "-q" || arg == "--quantum") {
            if (!takeValue(args, i, value)) {
                parsed.errors.push_back(arg + " needs a number");
            } else if (!parseInt(value, parsed.quantum)) {
                parsed.errors.push_back("quantum '" + value + "' is not a whole number");
            } else if (parsed.quantum < 1) {
                parsed.errors.push_back("quantum must be at least 1");
            }
        } else if (arg == "-g" || arg == "--aging") {
            if (!takeValue(args, i, value)) {
                parsed.errors.push_back(arg + " needs a number");
            } else if (!parseInt(value, parsed.agingRate)) {
                parsed.errors.push_back("aging rate '" + value + "' is not a whole number");
            } else if (parsed.agingRate < 0) {
                parsed.errors.push_back("aging rate cannot be negative");
            }
        } else if (arg == "-i" || arg == "--input") {
            if (!takeValue(args, i, parsed.inputPath)) {
                parsed.errors.push_back(arg + " needs a file path");
            }
        } else {
            parsed.errors.push_back("unknown option '" + arg + "'");
        }
    }

    return parsed;
}

std::string helpText() {
    std::string text =
        "CPU scheduling simulator\n"
        "\n"
        "Usage:\n"
        "  scheduler [options]\n"
        "\n"
        "Options:\n"
        "  -a, --algorithm NAME  algorithm to run (default: FCFS)\n"
        "  -q, --quantum N       time slice for Round Robin (default: 2)\n"
        "  -g, --aging N         priority gained per N ticks waited (default: 0, off)\n"
        "  -i, --input FILE      workload file (default: standard input)\n"
        "  -l, --list            list the available algorithms\n"
        "  -h, --help            show this message\n"
        "\n"
        "Workload format - one process per line, '#' starts a comment:\n"
        "  ID  ARRIVAL  BURST  [PRIORITY]\n"
        "\n"
        "Example:\n"
        "  scheduler --algorithm RR --quantum 3 --input examples/sample.txt\n";
    return text;
}

}  // namespace cli
