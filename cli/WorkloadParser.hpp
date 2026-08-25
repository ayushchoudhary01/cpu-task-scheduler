#pragma once

#include <istream>
#include <string>
#include <vector>

#include "Process.hpp"

namespace cli {

// The outcome of reading a workload: either the processes, or a list of
// complaints. Errors are collected rather than thrown on the first problem, so
// a file with three mistakes reports all three in one go.
struct WorkloadParseResult {
    std::vector<scheduler::Process> processes;
    std::vector<std::string> errors;

    bool ok() const { return errors.empty(); }
};

// Read a workload.
//
// Format - one process per line:
//     ID  ARRIVAL  BURST  [PRIORITY]
//
// Priority is optional and defaults to 0. Blank lines are skipped, and
// everything after a '#' is treated as a comment.
WorkloadParseResult parseWorkload(std::istream& input);

}  // namespace cli
