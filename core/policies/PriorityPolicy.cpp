#include "policies/PriorityPolicy.hpp"

#include <algorithm>

#include "policies/Select.hpp"

namespace scheduler::policies {

PriorityPolicy::PriorityPolicy(int agingRate) : agingRate_(std::max(0, agingRate)) {}

std::string PriorityPolicy::name() const {
    if (agingRate_ == 0) {
        return "Priority";
    }
    return "Priority(aging=" + std::to_string(agingRate_) + ")";
}

int PriorityPolicy::effectivePriority(const ReadyProcess& process, int currentTime) const {
    if (agingRate_ == 0) {
        return process.process->priority;
    }
    const int ticksWaited = currentTime - process.readySince;
    return process.process->priority - (ticksWaited / agingRate_);
}

std::size_t PriorityPolicy::choose(const std::vector<ReadyProcess>& ready, int currentTime) const {
    return selectBest(ready, [&](const ReadyProcess& a, const ReadyProcess& b) {
        return effectivePriority(a, currentTime) < effectivePriority(b, currentTime);
    });
}

}  // namespace scheduler::policies
