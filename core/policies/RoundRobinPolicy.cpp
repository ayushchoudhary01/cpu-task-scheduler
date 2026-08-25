#include "policies/RoundRobinPolicy.hpp"

#include <algorithm>

#include "policies/Select.hpp"

namespace scheduler::policies {

RoundRobinPolicy::RoundRobinPolicy(int quantum)
    // A quantum below 1 would mean a process never makes progress, so refuse
    // to build one. Input is validated before this point; this is a backstop.
    : quantum_(std::max(1, quantum)) {}

std::string RoundRobinPolicy::name() const {
    return "RR(q=" + std::to_string(quantum_) + ")";
}

std::size_t RoundRobinPolicy::choose(const std::vector<ReadyProcess>& ready,
                                     int currentTime) const {
    (void)currentTime;

    // Like FCFS, Round Robin just takes the front of the queue. What makes it
    // different is timeSlice(), which sends the running process to the back.
    return selectBest(ready, [](const ReadyProcess&, const ReadyProcess&) { return false; });
}

}  // namespace scheduler::policies
