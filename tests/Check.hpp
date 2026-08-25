#pragma once

#include <iostream>
#include <string>

// A minimal test helper. No framework, no dependencies - just enough to say
// "this value should equal that value" and report what went wrong.
namespace testing {

inline int totalChecks = 0;
inline int failedChecks = 0;

inline void check(bool condition, const std::string& what) {
    ++totalChecks;
    if (!condition) {
        ++failedChecks;
        std::cout << "  FAIL: " << what << "\n";
    }
}

template <typename Actual, typename Expected>
void checkEqual(const Actual& actual, const Expected& expected, const std::string& what) {
    ++totalChecks;
    if (!(actual == expected)) {
        ++failedChecks;
        std::cout << "  FAIL: " << what
                  << "  (got " << actual << ", expected " << expected << ")\n";
    }
}

// Print the tally. Returns the process exit code: 0 when everything passed.
inline int report() {
    std::cout << "\n" << (totalChecks - failedChecks) << "/" << totalChecks
              << " checks passed\n";
    return failedChecks == 0 ? 0 : 1;
}

}  // namespace testing
