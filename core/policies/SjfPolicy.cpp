#include "policies/SjfPolicy.hpp"

#include "policies/Select.hpp"

namespace scheduler::policies {

std::size_t SjfPolicy::choose(const std::vector<ReadyProcess>& ready, int currentTime) const {
    (void)currentTime;

    // Compare total burst time, not remaining time: this policy never
    // interrupts anyone, so a process only ever gets picked untouched.
    return selectBest(ready, [](const ReadyProcess& a, const ReadyProcess& b) {
        return a.process->burstTime < b.process->burstTime;
    });
}

}  // namespace scheduler::policies
