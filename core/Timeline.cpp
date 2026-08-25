#include "Timeline.hpp"

namespace scheduler {

void Timeline::runProcess(int tick, const std::string& processId) {
    append(tick, SliceKind::Running, processId);
}

void Timeline::runIdle(int tick) {
    append(tick, SliceKind::Idle, "");
}

void Timeline::append(int tick, SliceKind kind, const std::string& processId) {
    // If this tick continues what the previous slice was doing, just stretch
    // that slice instead of adding a new one.
    if (!slices_.empty()) {
        TimeSlice& last = slices_.back();
        if (last.end == tick && last.kind == kind && last.processId == processId) {
            last.end = tick + 1;
            return;
        }
    }
    slices_.push_back({tick, tick + 1, kind, processId});
}

int Timeline::totalTime() const {
    return slices_.empty() ? 0 : slices_.back().end;
}

int Timeline::busyTime() const {
    int busy = 0;
    for (const TimeSlice& slice : slices_) {
        if (slice.kind == SliceKind::Running) {
            busy += slice.duration();
        }
    }
    return busy;
}

}  // namespace scheduler
