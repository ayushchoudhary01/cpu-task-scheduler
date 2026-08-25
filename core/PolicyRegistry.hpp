#pragma once

#include <memory>
#include <string>
#include <vector>

#include "SchedulingPolicy.hpp"

namespace scheduler {

// Settings that individual algorithms may need. Anything that does not apply
// to the chosen algorithm is simply ignored.
struct PolicyOptions {
    int quantum = 2;      // Round Robin: ticks per turn
    int agingRate = 0;    // Priority: ticks of waiting per step of priority
};

// Build a policy from its name, e.g. "RR". Matching ignores case.
// Returns nullptr if the name is not one we know, so the caller can report the
// problem with whatever context it has.
std::unique_ptr<SchedulingPolicy> makePolicy(const std::string& name,
                                             const PolicyOptions& options = {});

// Every name makePolicy() accepts, in a sensible order for help text.
std::vector<std::string> availablePolicies();

}  // namespace scheduler
