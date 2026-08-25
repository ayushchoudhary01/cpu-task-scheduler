#include "policies/FcfsPolicy.hpp"

#include "policies/Select.hpp"

namespace scheduler::policies {

std::size_t FcfsPolicy::choose(const std::vector<ReadyProcess>& ready, int currentTime) const {
    (void)currentTime;

    // FCFS has no preference of its own - arrival order *is* the rule, and
    // that is exactly what the shared tie-break already does.
    return selectBest(ready, [](const ReadyProcess&, const ReadyProcess&) { return false; });
}

}  // namespace scheduler::policies
