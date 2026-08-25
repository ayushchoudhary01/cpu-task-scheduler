#include "policies/SrtfPolicy.hpp"

#include "policies/Select.hpp"

namespace scheduler::policies {

std::size_t SrtfPolicy::choose(const std::vector<ReadyProcess>& ready, int currentTime) const {
    (void)currentTime;

    return selectBest(ready, [](const ReadyProcess& a, const ReadyProcess& b) {
        return a.remainingTime < b.remainingTime;
    });
}

bool SrtfPolicy::shouldPreempt(const ReadyProcess& running,
                               const std::vector<ReadyProcess>& ready,
                               int currentTime) const {
    (void)currentTime;

    // Step aside only for a process that would genuinely finish sooner.
    // Using a strict "<" matters: on a tie the running process keeps the CPU,
    // which avoids pointless context switches between equal processes.
    for (const ReadyProcess& candidate : ready) {
        if (candidate.remainingTime < running.remainingTime) {
            return true;
        }
    }
    return false;
}

}  // namespace scheduler::policies
