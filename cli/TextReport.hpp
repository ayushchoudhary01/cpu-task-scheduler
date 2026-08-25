#pragma once

#include <string>
#include <vector>

#include "Engine.hpp"

namespace cli {

// Rendering returns strings rather than printing, so the tests can check the
// output and main.cpp stays free of formatting details.

// An ASCII Gantt chart. Block widths are proportional to how long each process
// held the CPU, scaled to fit roughly `maxWidth` characters.
std::string renderGantt(const scheduler::Timeline& timeline, int maxWidth = 76);

// One row per process, columns aligned.
std::string renderMetricsTable(const std::vector<scheduler::ProcessMetrics>& metrics);

// The summary figures.
std::string renderAverages(const scheduler::Averages& averages);

// Everything above, with headings.
std::string renderReport(const scheduler::SimulationResult& result);

// One row per algorithm, for comparing several runs of the same workload.
std::string renderComparison(const std::vector<scheduler::SimulationResult>& results);

}  // namespace cli
