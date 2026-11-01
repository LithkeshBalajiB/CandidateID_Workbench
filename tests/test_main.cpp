#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

// Phase 0 smoke test to verify test framework and C++17 compilation
TEST_CASE("Smoke Test: Harness and Environment Verification") {
    CHECK(1 + 1 == 2);
    
    // Verify C++17 standard is active
    #if __cplusplus >= 201703L
    CHECK(true);
    #else
    CHECK(false);
    #endif
}
