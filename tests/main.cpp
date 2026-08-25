#include "Check.hpp"
#include "Tests.hpp"

int main() {
    runCoreTests();
    runAlgorithmTests();
    runPriorityTests();
    runCliTests();
    return testing::report();
}
