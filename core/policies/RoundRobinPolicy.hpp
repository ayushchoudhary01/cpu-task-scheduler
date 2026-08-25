#pragma once

#include "SchedulingPolicy.hpp"

namespace scheduler::policies {

// Round Robin.
//
// Everyone takes turns, each getting at most `quantum` ticks before going to
// the back of the queue. Nobody starves and short jobs get seen quickly, but
// the constant switching costs throughput. The quantum is the trade-off: small
// values feel responsive, large values behave more and more like FCFS.
class RoundRobinPolicy : public SchedulingPolicy {
public:
    explicit RoundRobinPolicy(int quantum);

    std::string name() const override;

    std::size_t choose(const std::vector<ReadyProcess>& ready, int currentTime) const override;

    int timeSlice() const override { return quantum_; }

    int quantum() const { return quantum_; }

private:
    int quantum_;
};

}  // namespace scheduler::policies
