#include "CommandLine.hpp"

#include <charconv>
#include <sstream>

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

std::vector<std::string> splitOnCommas(const std::string& text) {
    std::vector<std::string> parts;
    std::istringstream input(text);
    std::string part;
    while (std::getline(input, part, ',')) {
        if (!part.empty()) {
            parts.push_back(part);
        }
    }
    return parts;
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
        } else if (arg == "-c" || arg == "--compare") {
            if (!takeValue(args, i, value)) {
                parsed.errors.push_back(arg + " needs a list of algorithms, or 'all'");
            } else if (value == "all") {
                parsed.compare = scheduler::availablePolicies();
            } else {
                for (const std::string& name : splitOnCommas(value)) {
                    if (scheduler::makePolicy(name) == nullptr) {
                        parsed.errors.push_back("unknown algorithm '" + name + "'");
                    } else {
                        parsed.compare.push_back(name);
                    }
                }
                if (parsed.compare.size() == 1) {
                    parsed.errors.push_back("--compare needs at least two algorithms");
                }
            }
        } else if (arg == "-f" || arg == "--format") {
            if (!takeValue(args, i, value)) {
                parsed.errors.push_back(arg + " needs 'text' or 'json'");
            } else if (value == "text") {
                parsed.format = OutputFormat::Text;
            } else if (value == "json") {
                parsed.format = OutputFormat::Json;
            } else {
                parsed.errors.push_back("unknown format '" + value + "', expected text or json");
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
        } else if (arg == "--generate") {
            if (!takeValue(args, i, value)) {
                parsed.errors.push_back(arg + " needs a number of processes");
            } else if (!parseInt(value, parsed.generateCount)) {
                parsed.errors.push_back("process count '" + value + "' is not a whole number");
            } else if (parsed.generateCount < 1) {
                parsed.errors.push_back("--generate needs at least 1 process");
            }
        } else if (arg == "--seed") {
            int seed = 0;
            if (!takeValue(args, i, value)) {
                parsed.errors.push_back(arg + " needs a number");
            } else if (!parseInt(value, seed) || seed < 0) {
                parsed.errors.push_back("seed '" + value + "' is not a positive whole number");
            } else {
                parsed.seed = static_cast<unsigned>(seed);
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
    return
        "CPU scheduling simulator\n"
        "\n"
        "Usage:\n"
        "  scheduler [options]\n"
        "\n"
        "Options:\n"
        "  -a, --algorithm NAME  algorithm to run (default: FCFS)\n"
        "  -c, --compare LIST    run several algorithms on the same workload,\n"
        "                        comma separated, or 'all'\n"
        "  -f, --format FORMAT   text or json (default: text)\n"
        "  -q, --quantum N       time slice for Round Robin (default: 2)\n"
        "  -g, --aging N         priority gained per N ticks waited (default: 0, off)\n"
        "  -i, --input FILE      workload file (default: standard input)\n"
        "      --generate N      make up a workload of N processes instead\n"
        "      --seed S          seed for --generate; the same seed always\n"
        "                        produces the same workload\n"
        "  -l, --list            list the available algorithms\n"
        "  -h, --help            show this message\n"
        "\n"
        "Workload format - one process per line, '#' starts a comment:\n"
        "  ID  ARRIVAL  BURST  [PRIORITY]\n"
        "\n"
        "Examples:\n"
        "  scheduler --algorithm RR --quantum 3 --input examples/sample.txt\n"
        "  scheduler --compare all --input examples/sample.txt\n"
        "  scheduler --compare FCFS,SJF --format json --input examples/sample.txt\n"
        "  scheduler --generate 8 --seed 42 --compare all\n";
}

}  // namespace cli
