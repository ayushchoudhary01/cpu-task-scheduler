#pragma once

#include "SchedulingPolicy.hpp"

namespace scheduler::policies {

// Shortest Job First (non-preemptive).
//
// When the CPU frees up, run whichever waiting process needs the least CPU
// time. This gives the lowest possible average waiting time - but a steady
// supply of short jobs can starve a long one indefinitely.
class SjfPolicy : public SchedulingPolicy {
public:
    std::string name() const override { return "SJF"; }

    std::size_t choose(const std::vector<ReadyProcess>& ready, int currentTime) const override;
};

}  // namespace scheduler::policies
