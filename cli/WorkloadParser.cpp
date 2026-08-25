#include "WorkloadParser.hpp"

#include <charconv>
#include <set>
#include <sstream>

namespace cli {
namespace {

// Parse a whole token as an integer. Rejects "12abc" and "" alike.
bool parseInt(const std::string& text, int& out) {
    if (text.empty()) {
        return false;
    }
    const char* begin = text.data();
    const char* end = begin + text.size();
    const std::from_chars_result result = std::from_chars(begin, end, out);
    return result.ec == std::errc() && result.ptr == end;
}

std::string withoutComment(const std::string& line) {
    const std::size_t hash = line.find('#');
    return hash == std::string::npos ? line : line.substr(0, hash);
}

std::string describeLine(int lineNumber) {
    return "line " + std::to_string(lineNumber) + ": ";
}

// Windows editors often save UTF-8 files with a byte order mark. Left alone it
// becomes part of the first process id, which then fails to match anything.
void stripByteOrderMark(std::string& line) {
    static const std::string kBom = "\xEF\xBB\xBF";
    if (line.rfind(kBom, 0) == 0) {
        line.erase(0, kBom.size());
    }
}

}  // namespace

WorkloadParseResult parseWorkload(std::istream& input) {
    WorkloadParseResult result;
    std::set<std::string> seenIds;

    std::string line;
    int lineNumber = 0;

    while (std::getline(input, line)) {
        ++lineNumber;
        if (lineNumber == 1) {
            stripByteOrderMark(line);
        }

        std::istringstream fields(withoutComment(line));
        std::vector<std::string> tokens;
        std::string token;
        while (fields >> token) {
            tokens.push_back(token);
        }

        if (tokens.empty()) {
            continue;  // blank line or comment only
        }

        if (tokens.size() < 3 || tokens.size() > 4) {
            result.errors.push_back(describeLine(lineNumber) +
                                    "expected 'ID ARRIVAL BURST [PRIORITY]', got " +
                                    std::to_string(tokens.size()) + " values");
            continue;
        }

        scheduler::Process process;
        process.id = tokens[0];

        if (!seenIds.insert(process.id).second) {
            result.errors.push_back(describeLine(lineNumber) + "duplicate process id '" +
                                    process.id + "'");
            continue;
        }

        if (!parseInt(tokens[1], process.arrivalTime)) {
            result.errors.push_back(describeLine(lineNumber) + "arrival time '" + tokens[1] +
                                    "' is not a whole number");
            continue;
        }
        if (!parseInt(tokens[2], process.burstTime)) {
            result.errors.push_back(describeLine(lineNumber) + "burst time '" + tokens[2] +
                                    "' is not a whole number");
            continue;
        }
        if (tokens.size() == 4 && !parseInt(tokens[3], process.priority)) {
            result.errors.push_back(describeLine(lineNumber) + "priority '" + tokens[3] +
                                    "' is not a whole number");
            continue;
        }

        if (process.arrivalTime < 0) {
            result.errors.push_back(describeLine(lineNumber) + "arrival time cannot be negative");
            continue;
        }
        if (process.burstTime < 1) {
            result.errors.push_back(describeLine(lineNumber) +
                                    "burst time must be at least 1 tick");
            continue;
        }

        result.processes.push_back(process);
    }

    if (result.processes.empty() && result.errors.empty()) {
        result.errors.push_back("no processes found - the workload is empty");
    }

    return result;
}

}  // namespace cli
