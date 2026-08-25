#pragma once

#include <cstddef>
#include <vector>

#include "SchedulingPolicy.hpp"

namespace scheduler::policies {

// Did `a` join the ready queue before `b`?
// This is the tie-break every policy must use, and it is a total order, so the
// result of a simulation never depends on vector ordering.
inline bool joinedQueueFirst(const ReadyProcess& a, const ReadyProcess& b) {
    return a.queueOrder < b.queueOrder;
}

// Pick the best ready process.
//
// `isBetter(a, b)` should say whether `a` is a strictly better choice than `b`
// by the algorithm's own rule - shorter burst, higher priority, and so on.
// Anything it considers equal falls through to the shared tie-break above, so
// each policy only has to express what makes it different.
template <typename IsBetter>
std::size_t selectBest(const std::vector<ReadyProcess>& ready, IsBetter isBetter) {
    std::size_t best = 0;
    for (std::size_t i = 1; i < ready.size(); ++i) {
        const ReadyProcess& candidate = ready[i];
        const ReadyProcess& incumbent = ready[best];

        if (isBetter(candidate, incumbent)) {
            best = i;
        } else if (!isBetter(incumbent, candidate) && joinedQueueFirst(candidate, incumbent)) {
            best = i;
        }
    }
    return best;
}

}  // namespace scheduler::policies
