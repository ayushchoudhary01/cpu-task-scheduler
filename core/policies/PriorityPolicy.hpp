#pragma once

#include "SchedulingPolicy.hpp"

namespace scheduler::policies {

// Priority scheduling, optionally with aging. Non-preemptive.
//
// A LOWER priority number means MORE important (see Process.hpp). When the CPU
// frees up, the most important waiting process runs, and runs to completion.
//
// The catch is starvation: a steady supply of important work means an
// unimportant process may never run at all. Aging is the standard fix - the
// longer a process waits, the more important it becomes, so it eventually
// wins no matter how modest its original priority.
//
// Aging is expressed as a rate: one step of priority gained per `agingRate`
// ticks of waiting. A rate of 0 disables it. Because the boost is derived from
// how long the process has been queued, it is recalculated rather than stored -
// which means a process that runs and is later re-queued starts aging afresh.
class PriorityPolicy : public SchedulingPolicy {
public:
    explicit PriorityPolicy(int agingRate = 0);

    std::string name() const override;

    std::size_t choose(const std::vector<ReadyProcess>& ready, int currentTime) const override;

    int agingRate() const { return agingRate_; }

    // Priority a process counts as having right now, boost included.
    int effectivePriority(const ReadyProcess& process, int currentTime) const;

private:
    int agingRate_;
};

}  // namespace scheduler::policies
