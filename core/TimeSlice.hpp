#pragma once

#include <string>

namespace scheduler {

// What the CPU was doing during one stretch of time.
enum class SliceKind {
    Running,         // some process held the CPU
    Idle,            // nothing was ready to run
    ContextSwitch    // changing from one process to another
};

// A context switch is time the CPU spends but gets no work out of: saving one
// process's registers and loading another's. Real hardware pays this on every
// switch, which is why operating systems use time slices of tens of
// milliseconds rather than one. It is only simulated when asked for - see
// SimulationOptions in Engine.hpp.

// One block of the Gantt chart.
//
// The range is half-open: [start, end). A slice with start=0 and end=3 covers
// ticks 0, 1 and 2 - so `end` is also the `start` of whatever comes next, and
// durations are a plain subtraction with no off-by-one.
struct TimeSlice {
    int start = 0;
    int end = 0;
    SliceKind kind = SliceKind::Idle;
    std::string processId;   // empty when kind == Idle

    int duration() const { return end - start; }
};

}  // namespace scheduler
