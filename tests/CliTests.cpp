#include <iostream>
#include <sstream>

#include "Check.hpp"
#include "CommandLine.hpp"
#include "Tests.hpp"
#include "WorkloadParser.hpp"

using namespace cli;
using testing::check;
using testing::checkEqual;

namespace {

WorkloadParseResult parseText(const std::string& text) {
    std::istringstream input(text);
    return parseWorkload(input);
}

void testParsesAWorkload() {
    std::cout << "Workload parser reads processes\n";

    WorkloadParseResult result = parseText("P1 0 5 3\nP2 1 3 1\n");

    check(result.ok(), "no errors");
    checkEqual(result.processes.size(), size_t{2}, "two processes");
    checkEqual(result.processes[0].id, std::string("P1"), "first id");
    checkEqual(result.processes[0].burstTime, 5, "first burst");
    checkEqual(result.processes[1].priority, 1, "second priority");
}

void testPriorityIsOptional() {
    std::cout << "Workload parser defaults a missing priority to 0\n";

    WorkloadParseResult result = parseText("P1 0 5\n");

    check(result.ok(), "no errors");
    checkEqual(result.processes[0].priority, 0, "priority defaults to 0");
}

void testSkipsBlankLinesAndComments() {
    std::cout << "Workload parser skips blank lines and comments\n";

    WorkloadParseResult result = parseText(
        "# a comment\n"
        "\n"
        "P1 0 5   # trailing comment\n"
        "   \n"
        "P2 1 3\n");

    check(result.ok(), "no errors");
    checkEqual(result.processes.size(), size_t{2}, "two processes");
}

void testReportsBadNumbers() {
    std::cout << "Workload parser rejects values that are not numbers\n";

    WorkloadParseResult result = parseText("P1 0 five\n");

    check(!result.ok(), "reports an error");
    checkEqual(result.errors.size(), size_t{1}, "one error");
    check(result.errors[0].find("line 1") != std::string::npos, "error names the line");
    check(result.errors[0].find("five") != std::string::npos, "error quotes the value");
}

void testReportsEveryProblemAtOnce() {
    std::cout << "Workload parser reports every bad line, not just the first\n";

    WorkloadParseResult result = parseText(
        "P1 0 5\n"
        "P2 -1 3\n"
        "P3 0 0\n"
        "P4 0\n");

    checkEqual(result.errors.size(), size_t{3}, "three problems found");
    check(result.errors[0].find("line 2") != std::string::npos, "negative arrival on line 2");
    check(result.errors[1].find("line 3") != std::string::npos, "zero burst on line 3");
    check(result.errors[2].find("line 4") != std::string::npos, "too few values on line 4");
}

void testRejectsDuplicateIds() {
    std::cout << "Workload parser rejects duplicate process ids\n";

    WorkloadParseResult result = parseText("P1 0 5\nP1 1 3\n");

    check(!result.ok(), "reports an error");
    check(result.errors[0].find("duplicate") != std::string::npos, "says duplicate");
}

void testRejectsAnEmptyWorkload() {
    std::cout << "Workload parser rejects an empty workload\n";

    WorkloadParseResult result = parseText("# nothing but comments\n");

    check(!result.ok(), "reports an error");
    check(result.errors[0].find("empty") != std::string::npos, "explains the file is empty");
}

void testCommandLineDefaults() {
    std::cout << "Command line has sensible defaults\n";

    CommandLine parsed = parseCommandLine({});

    check(parsed.ok(), "no errors");
    checkEqual(parsed.algorithm, std::string("FCFS"), "defaults to FCFS");
    checkEqual(parsed.quantum, 2, "default quantum");
    checkEqual(parsed.agingRate, 0, "aging off by default");
    check(parsed.inputPath.empty(), "reads stdin by default");
}

void testCommandLineReadsOptions() {
    std::cout << "Command line reads long and short options\n";

    CommandLine longForm =
        parseCommandLine({"--algorithm", "RR", "--quantum", "5", "--input", "work.txt"});
    check(longForm.ok(), "long form parses");
    checkEqual(longForm.algorithm, std::string("RR"), "algorithm read");
    checkEqual(longForm.quantum, 5, "quantum read");
    checkEqual(longForm.inputPath, std::string("work.txt"), "input path read");

    CommandLine shortForm = parseCommandLine({"-a", "SJF", "-g", "3"});
    check(shortForm.ok(), "short form parses");
    checkEqual(shortForm.algorithm, std::string("SJF"), "algorithm read");
    checkEqual(shortForm.agingRate, 3, "aging rate read");
}

void testCommandLineRejectsBadInput() {
    std::cout << "Command line rejects bad input clearly\n";

    check(!parseCommandLine({"--algorithm", "MLFQ"}).ok(), "unknown algorithm");
    check(!parseCommandLine({"--quantum", "0"}).ok(), "quantum below 1");
    check(!parseCommandLine({"--quantum", "two"}).ok(), "quantum not a number");
    check(!parseCommandLine({"--wat"}).ok(), "unknown option");
    check(!parseCommandLine({"--quantum"}).ok(), "option with no value");
    check(!parseCommandLine({"--aging", "-2"}).ok(), "negative aging rate");
}

void testCommandLineCollectsSeveralProblems() {
    std::cout << "Command line reports every problem at once\n";

    CommandLine parsed = parseCommandLine({"--quantum", "0", "--algorithm", "NOPE"});
    checkEqual(parsed.errors.size(), size_t{2}, "both problems reported");
}

void testHelpAndListShortCircuit() {
    std::cout << "Help and list flags are recognised\n";

    check(parseCommandLine({"--help"}).showHelp, "--help");
    check(parseCommandLine({"-h"}).showHelp, "-h");
    check(parseCommandLine({"--list"}).listAlgorithms, "--list");
    check(helpText().find("--algorithm") != std::string::npos, "help mentions --algorithm");
}


void testStripsByteOrderMark() {
    std::cout << "Workload parser ignores a UTF-8 byte order mark\n";

    // Windows editors add these invisibly. Left in place, the first process
    // ends up called "<BOM>P1" and stops matching anything - including the
    // duplicate check below.
    WorkloadParseResult result = parseText("\xEF\xBB\xBFP1 0 5\nP1 1 3\n");

    checkEqual(result.processes.size(), size_t{1}, "only the first line is kept");
    checkEqual(result.processes[0].id, std::string("P1"), "id has no mark attached");
    check(!result.ok(), "the repeat is still caught");
    check(result.errors[0].find("duplicate") != std::string::npos, "reported as duplicate");
}
}  // namespace

void runCliTests() {
    testParsesAWorkload();
    testPriorityIsOptional();
    testSkipsBlankLinesAndComments();
    testReportsBadNumbers();
    testReportsEveryProblemAtOnce();
    testRejectsDuplicateIds();
    testRejectsAnEmptyWorkload();
    testStripsByteOrderMark();
    testCommandLineDefaults();
    testCommandLineReadsOptions();
    testCommandLineRejectsBadInput();
    testCommandLineCollectsSeveralProblems();
    testHelpAndListShortCircuit();
}
