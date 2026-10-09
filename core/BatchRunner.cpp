#include "BatchRunner.hpp"

#include <thread>

namespace scheduler {
namespace {

// Run one algorithm into `slot`. Written as a free function so the thread body
// stays small and obvious.
void runOne(const std::vector<Process>& workload,
            const std::string& name,
            const PolicyOptions& policyOptions,
            const SimulationOptions& simulationOptions,
            SimulationResult& slot) {
    const auto policy = makePolicy(name, policyOptions);
    if (policy) {
        slot = runSimulation(workload, *policy, simulationOptions);
    }
}

}  // namespace

std::vector<SimulationResult> runAlgorithms(const std::vector<Process>& workload,
                                            const std::vector<std::string>& names,
                                            const PolicyOptions& policyOptions,
                                            const SimulationOptions& simulationOptions) {
    std::vector<SimulationResult> results(names.size());

    // One algorithm is the common case, and starting a thread to do one thing
    // costs more than it saves.
    if (names.size() <= 1) {
        if (names.size() == 1) {
            runOne(workload, names[0], policyOptions, simulationOptions, results[0]);
        }
        return results;
    }

    // No mutex, and none needed.
    //
    // `workload`, `policyOptions` and `simulationOptions` are only ever read,
    // and each thread writes to a different element of `results`. Two threads
    // writing to different elements of the same vector is safe as long as
    // nothing resizes it - and nothing does, because it was sized up front.
    //
    // The lock you do not take is the one that cannot deadlock. Sharing only
    // immutable data is what makes that possible here, and it is also why the
    // engine takes its workload by const reference and never modifies it.
    std::vector<std::thread> threads;
    threads.reserve(names.size());

    for (std::size_t i = 0; i < names.size(); ++i) {
        threads.emplace_back([&, i]() {
            runOne(workload, names[i], policyOptions, simulationOptions, results[i]);
        });
    }

    for (std::thread& thread : threads) {
        thread.join();
    }

    return results;
}

}  // namespace scheduler
