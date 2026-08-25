#pragma once

#include "SchedulingPolicy.hpp"

namespace scheduler::policies {

// First Come First Served.
//
// The simplest possible scheduler: whoever has been waiting longest runs, and
// runs to completion. Easy to reason about, but one long process arriving
// first makes everyone behind it wait - the "convoy effect".
class FcfsPolicy : public SchedulingPolicy {
public:
    std::string name() const override { return "FCFS"; }

    std::size_t choose(const std::vector<ReadyProcess>& ready, int currentTime) const override;
};

}  // namespace scheduler::policies
