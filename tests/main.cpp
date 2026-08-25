#include "Check.hpp"
#include "Tests.hpp"

int main() {
    runCoreTests();
    runAlgorithmTests();
    return testing::report();
}
