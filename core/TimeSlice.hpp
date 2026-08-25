#pragma once

#include <string>

namespace scheduler {

// What the CPU was doing during one stretch of time.
enum class SliceKind {
    Running,   // some process held the CPU
    Idle       // nothing was ready to run
};

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
