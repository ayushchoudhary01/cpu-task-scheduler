#pragma once

#include "SchedulingPolicy.hpp"

namespace scheduler::policies {

// Shortest Remaining Time First - the preemptive form of SJF.
//
// Whenever a process arrives that needs less CPU time than whatever is
// running, it takes over immediately. This gives the best average waiting time
// of any of these algorithms, at the cost of frequent context switches and
// even worse starvation than SJF.
class SrtfPolicy : public SchedulingPolicy {
public:
    std::string name() const override { return "SRTF"; }

    std::size_t choose(const std::vector<ReadyProcess>& ready, int currentTime) const override;

    bool shouldPreempt(const ReadyProcess& running,
                       const std::vector<ReadyProcess>& ready,
                       int currentTime) const override;
};

}  // namespace scheduler::policies
