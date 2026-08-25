#pragma once

#include <cstddef>
#include <vector>

#include "SchedulingPolicy.hpp"

namespace scheduler::policies {

// Has `a` been waiting longer than `b`?
// This is the tie-break every policy must use: longest wait first, and if even
// that is equal, the process listed first in the input.
inline bool waitedLonger(const ReadyProcess& a, const ReadyProcess& b) {
    if (a.readySince != b.readySince) {
        return a.readySince < b.readySince;
    }
    return a.index < b.index;
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
        } else if (!isBetter(incumbent, candidate) && waitedLonger(candidate, incumbent)) {
            best = i;
        }
    }
    return best;
}

}  // namespace scheduler::policies
