#include <iostream>
#include <vector>

#include "Process.hpp"
#include "TimeSlice.hpp"

using namespace scheduler;

// Still a placeholder, but now it exercises the real domain types so we can
// see that they compile and behave the way we expect.
int main() {
    std::vector<Process> workload = {
        {"P1", 0, 5, 2},
        {"P2", 1, 3, 1},
    };

    std::cout << "Workload:\n";
    for (const Process& p : workload) {
        std::cout << "  " << p.id
                  << "  arrival=" << p.arrivalTime
                  << "  burst=" << p.burstTime
                  << "  priority=" << p.priority << "\n";
    }

    // A hand-made timeline, just to show the shape of the output we are
    // building towards. The engine will produce these for real.
    std::vector<TimeSlice> timeline = {
        {0, 1, SliceKind::Running, "P1"},
        {1, 4, SliceKind::Running, "P2"},
        {4, 5, SliceKind::Idle, ""},
    };

    std::cout << "\nExample timeline:\n";
    for (const TimeSlice& s : timeline) {
        std::cout << "  [" << s.start << ", " << s.end << ")  "
                  << (s.kind == SliceKind::Idle ? "idle" : s.processId)
                  << "  (" << s.duration() << " ticks)\n";
    }

    return 0;
}
