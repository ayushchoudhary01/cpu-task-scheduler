#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "Process.hpp"

namespace scheduler {

// A process that is waiting for the CPU, as a scheduling policy sees it.
//
// The engine owns these. A policy only reads them - it never changes anything,
// which is why every method below is const.
struct ReadyProcess {
    const Process* process = nullptr;   // the original, unmodified input
    int remainingTime = 0;              // CPU time still needed
    int readySince = 0;                 // tick it last joined the ready queue
    long long queueOrder = 0;           // strictly increasing; see below
    int firstRunTime = -1;              // tick it first got the CPU, -1 if never
    std::size_t index = 0;              // position in the original input list
};

// The one thing that differs between scheduling algorithms: who runs next.
//
// TIE-BREAKING: when two processes look equally good, policies must prefer the
// smaller `queueOrder` - that is, whoever joined the ready queue first.
//
// `queueOrder` exists because `readySince` is only a tick number, and several
// processes can join the queue during the same tick. The engine admits new
// arrivals before it re-queues a preempted process, so ordering by
// `queueOrder` gives the conventional behaviour: an arriving process queues
// ahead of one whose time slice just ran out.
class SchedulingPolicy {
public:
    virtual ~SchedulingPolicy() = default;

    // Name used in output, e.g. "FCFS".
    virtual std::string name() const = 0;

    // Pick which ready process runs next. `ready` is never empty.
    // Returns an index into `ready`.
    virtual std::size_t choose(const std::vector<ReadyProcess>& ready,
                               int currentTime) const = 0;

    // How many ticks a process may hold the CPU before it must give it up.
    // 0 means no limit: it runs until it finishes or is preempted.
    // Only Round Robin overrides this.
    virtual int timeSlice() const { return 0; }

    // Should the running process be interrupted right now in favour of
    // something in the ready list? `ready` is never empty when this is called.
    // Non-preemptive algorithms use the default.
    virtual bool shouldPreempt(const ReadyProcess& running,
                               const std::vector<ReadyProcess>& ready,
                               int currentTime) const {
        (void)running;
        (void)ready;
        (void)currentTime;
        return false;
    }
};

}  // namespace scheduler
