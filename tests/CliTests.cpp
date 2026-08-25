#include <iostream>
#include <sstream>

#include "Check.hpp"
#include "CommandLine.hpp"
#include "PolicyRegistry.hpp"
#include "TextReport.hpp"
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

// ---------------------------------------------------------------------------
// Text output
// ---------------------------------------------------------------------------

scheduler::SimulationResult runSample(const std::string& algorithm) {
    const std::vector<scheduler::Process> workload = {
        {"P1", 0, 5, 0},
        {"P2", 1, 3, 0},
        {"P3", 2, 1, 0},
    };
    auto policy = scheduler::makePolicy(algorithm);
    return scheduler::runSimulation(workload, *policy);
}

std::vector<std::string> linesOf(const std::string& text) {
    std::vector<std::string> lines;
    std::istringstream input(text);
    std::string line;
    while (std::getline(input, line)) {
        lines.push_back(line);
    }
    return lines;
}

void testGanttHasAlignedBorders() {
    std::cout << "Gantt chart borders line up with the labels\n";

    std::vector<std::string> lines = linesOf(renderGantt(runSample("FCFS").timeline));

    checkEqual(lines.size(), size_t{4}, "border, labels, border, axis");
    checkEqual(lines[0].size(), lines[1].size(), "top border matches label row");
    checkEqual(lines[0], lines[2], "both borders identical");
    check(lines[0].find("+---") != std::string::npos, "drawn with + and -");
}

void testGanttShowsEveryProcess() {
    std::cout << "Gantt chart shows every process in order\n";

    const std::string chart = renderGantt(runSample("FCFS").timeline);

    const std::size_t p1 = chart.find("P1");
    const std::size_t p2 = chart.find("P2");
    const std::size_t p3 = chart.find("P3");

    check(p1 != std::string::npos, "P1 appears");
    check(p1 < p2 && p2 < p3, "in the order they ran");
}

void testGanttAxisStartsAtZeroAndEndsAtTotal() {
    std::cout << "Gantt chart axis runs from 0 to the finish time\n";

    scheduler::SimulationResult result = runSample("FCFS");
    std::vector<std::string> lines = linesOf(renderGantt(result.timeline));
    const std::string& axis = lines[3];

    check(axis.find("0") != std::string::npos, "starts at 0");
    check(axis.find(std::to_string(result.timeline.totalTime())) != std::string::npos,
          "ends at the total time");
}

void testGanttLabelsIdleTime() {
    std::cout << "Gantt chart labels idle stretches\n";

    const std::vector<scheduler::Process> gap = {{"P1", 0, 2, 0}, {"P2", 6, 2, 0}};
    auto policy = scheduler::makePolicy("FCFS");
    const std::string chart = renderGantt(scheduler::runSimulation(gap, *policy).timeline);

    check(chart.find("idle") != std::string::npos, "idle block is labelled");
}

void testGanttHandlesAnEmptyTimeline() {
    std::cout << "Gantt chart of nothing is empty, not broken\n";

    scheduler::Timeline empty;
    checkEqual(renderGantt(empty), std::string(""), "no output");
}

void testGanttWidthTracksDuration() {
    std::cout << "Gantt blocks are wider for longer bursts\n";

    // P1 runs four times as long as P2, so its block must be wider.
    const std::vector<scheduler::Process> workload = {{"P1", 0, 8, 0}, {"P2", 0, 2, 0}};
    auto policy = scheduler::makePolicy("FCFS");
    std::vector<std::string> lines =
        linesOf(renderGantt(scheduler::runSimulation(workload, *policy).timeline));

    const std::string& labels = lines[1];
    const std::size_t firstBar = labels.find('|');
    const std::size_t secondBar = labels.find('|', firstBar + 1);
    const std::size_t thirdBar = labels.find('|', secondBar + 1);

    check(secondBar - firstBar > thirdBar - secondBar, "P1's block is the wider one");
}

void testMetricsTableHasARowPerProcess() {
    std::cout << "Metrics table has a header and one row per process\n";

    std::vector<std::string> lines = linesOf(renderMetricsTable(runSample("FCFS").metrics));

    checkEqual(lines.size(), size_t{4}, "header plus three rows");
    check(lines[0].find("Waiting") != std::string::npos, "header names the columns");
    check(lines[1].find("P1") != std::string::npos, "first row is P1");
}

void testMetricsTableColumnsLineUp() {
    std::cout << "Metrics table columns line up\n";

    // Ids of different lengths must not shift the numeric columns.
    const std::vector<scheduler::Process> workload = {{"A", 0, 2, 0}, {"LONGNAME", 0, 2, 0}};
    auto policy = scheduler::makePolicy("FCFS");
    std::vector<std::string> lines =
        linesOf(renderMetricsTable(scheduler::runSimulation(workload, *policy).metrics));

    checkEqual(lines[1].size(), lines[2].size(), "both rows the same width");
    checkEqual(lines[0].size(), lines[1].size(), "header matches the rows");
}

void testMetricsTableOfNothingIsEmpty() {
    std::cout << "Metrics table of nothing is empty\n";

    checkEqual(renderMetricsTable({}), std::string(""), "no output");
}

void testAveragesAreRoundedToTwoPlaces() {
    std::cout << "Averages are shown to two decimal places\n";

    const std::string text = renderAverages(runSample("FCFS").averages);

    check(text.find("3.33") != std::string::npos, "waiting time rounded to 3.33");
    check(text.find("100.00 %") != std::string::npos, "utilization shown as a percentage");
    check(text.find("processes/tick") != std::string::npos, "throughput has units");
}

void testFullReportHasEverySection() {
    std::cout << "Full report contains all three sections\n";

    const std::string report = renderReport(runSample("SRTF"));

    check(report.find("Algorithm: SRTF") != std::string::npos, "names the algorithm");
    check(report.find("Gantt chart") != std::string::npos, "has the chart");
    check(report.find("Per process") != std::string::npos, "has the table");
    check(report.find("Averages") != std::string::npos, "has the averages");
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
    testGanttHasAlignedBorders();
    testGanttShowsEveryProcess();
    testGanttAxisStartsAtZeroAndEndsAtTotal();
    testGanttLabelsIdleTime();
    testGanttHandlesAnEmptyTimeline();
    testGanttWidthTracksDuration();
    testMetricsTableHasARowPerProcess();
    testMetricsTableColumnsLineUp();
    testMetricsTableOfNothingIsEmpty();
    testAveragesAreRoundedToTwoPlaces();
    testFullReportHasEverySection();
    testCommandLineDefaults();
    testCommandLineReadsOptions();
    testCommandLineRejectsBadInput();
    testCommandLineCollectsSeveralProblems();
    testHelpAndListShortCircuit();
}
