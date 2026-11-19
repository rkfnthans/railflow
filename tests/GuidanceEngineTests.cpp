#include <array>
#include <iostream>
#include <string>

#include "GuidanceEngine.hpp"

int main()
{
    struct TestCase
    {
        double crowding;
        const char* expected;
    };

    const std::array<TestCase, 9> tests{{
        {0.2, "여유"},
        {0.5, "보통"},
        {0.8, "혼잡"},
        {1.0, "혼잡"},
        {1.01, "초과혼잡"},
        {1.1, "초과혼잡"},
        {1.2, "초과혼잡"},
        {1.5, "초과혼잡"},
        {2.0, "초과혼잡"}
    }};

    int passed = 0;
    for (const auto& test : tests)
    {
        const std::string actual = getCrowdingStatus(test.crowding);
        const bool matches = actual == test.expected;
        std::cout << (matches ? "[PASS] " : "[FAIL] ")
                  << test.crowding << " crowding status";
        if (!matches)
        {
            std::cout << " | expected: " << test.expected << ", actual: " << actual;
        }
        std::cout << '\n';
        passed += matches;
    }

    const int total = static_cast<int>(tests.size());
    std::cout << "Checks passed: " << passed << '/' << total << '\n';
    std::cout << (passed == total
        ? "GuidanceEngine tests: ALL PASSED\n"
        : "GuidanceEngine tests: FAILED\n");
    return passed == total ? 0 : 1;
}
