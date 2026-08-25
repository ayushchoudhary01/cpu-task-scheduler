#pragma once

#include <string>
#include <vector>

#include "TimeSlice.hpp"

namespace scheduler {

// Records what the CPU did, one tick at a time.
//
// The engine calls runProcess()/runIdle() once per tick. Consecutive ticks
// doing the same thing are merged into a single slice, so a process that runs
// for 4 ticks produces one block in the Gantt chart rather than four.
class Timeline {
public:
    // Record that `processId` held the CPU for the single tick starting at `tick`.
    void runProcess(int tick, const std::string& processId);

    // Record that nothing ran during the single tick starting at `tick`.
    void runIdle(int tick);

    const std::vector<TimeSlice>& slices() const { return slices_; }

    // Tick at which the simulation finished (0 if nothing was recorded).
    int totalTime() const;

    // Number of ticks the CPU was actually running something.
    int busyTime() const;

private:
    void append(int tick, SliceKind kind, const std::string& processId);

    std::vector<TimeSlice> slices_;
};

}  // namespace scheduler
