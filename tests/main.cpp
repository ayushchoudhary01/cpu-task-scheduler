#include "Check.hpp"
#include "Tests.hpp"

int main() {
    runCoreTests();
    runAlgorithmTests();
    runPriorityTests();
    return testing::report();
}
