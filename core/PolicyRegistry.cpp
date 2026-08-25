#include "PolicyRegistry.hpp"

#include <algorithm>
#include <cctype>

#include "policies/FcfsPolicy.hpp"
#include "policies/PriorityPolicy.hpp"
#include "policies/RoundRobinPolicy.hpp"
#include "policies/SjfPolicy.hpp"
#include "policies/SrtfPolicy.hpp"

namespace scheduler {
namespace {

std::string toUpper(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return text;
}

}  // namespace

std::unique_ptr<SchedulingPolicy> makePolicy(const std::string& name,
                                             const PolicyOptions& options) {
    const std::string key = toUpper(name);

    if (key == "FCFS") return std::make_unique<policies::FcfsPolicy>();
    if (key == "SJF") return std::make_unique<policies::SjfPolicy>();
    if (key == "SRTF") return std::make_unique<policies::SrtfPolicy>();
    if (key == "RR") return std::make_unique<policies::RoundRobinPolicy>(options.quantum);
    if (key == "PRIORITY") return std::make_unique<policies::PriorityPolicy>(options.agingRate);

    return nullptr;
}

std::vector<std::string> availablePolicies() {
    return {"FCFS", "SJF", "SRTF", "RR", "Priority"};
}

}  // namespace scheduler
