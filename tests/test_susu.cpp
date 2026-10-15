// test_susu.cpp - automated tests for the susu rules.
// Build and run with:  make test      (see README.md for the plain g++ commands)
// A tiny test runner is included below, so no extra library is needed.
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "../src/susu.h"

using namespace susu;

// ------------------------------------------------------------ mini test runner
namespace {
int g_checks = 0;
int g_failures = 0;

struct TestCase {
    const char* name;
    void (*run)();
};

std::vector<TestCase>& registry() {
    static std::vector<TestCase> tests;
    return tests;
}

struct Register {
    Register(const char* name, void (*run)()) {
        TestCase t;
        t.name = name;
        t.run = run;
        registry().push_back(t);
    }
};
}  // namespace

#define TEST(name)                                  \
    static void name();                             \
    static Register register_##name(#name, name);   \
    static void name()

#define CHECK(condition)                                                         \
    do {                                                                         \
        ++g_checks;                                                              \
        if (!(condition)) {                                                      \
            ++g_failures;                                                        \
            std::cerr << "    FAILED line " << __LINE__ << ": " << #condition << "\n"; \
        }                                                                        \
    } while (0)

#define CHECK_EQ(actual, expected)                                               \
    do {                                                                         \
        ++g_checks;                                                              \
        auto a_ = (actual);                                                      \
        auto e_ = (expected);                                                    \
        if (!(a_ == e_)) {                                                       \
            ++g_failures;                                                        \
            std::cerr << "    FAILED line " << __LINE__ << ": " << #actual       \
                      << "  (got " << a_ << ", expected " << e_ << ")\n";        \
        }                                                                        \
    } while (0)

// ------------------------------------------------------------------ money tests

TEST(money_parses_valid_amounts) {
    Cents c = 0;
    std::string err;
    CHECK(parseMoney("100", c, err));     CHECK_EQ(c, 10000);
    CHECK(parseMoney("100.5", c, err));   CHECK_EQ(c, 10050);
    CHECK(parseMoney("100.50", c, err));  CHECK_EQ(c, 10050);
    CHECK(parseMoney("0.05", c, err));    CHECK_EQ(c, 5);
    CHECK(parseMoney("  25.00 ", c, err)); CHECK_EQ(c, 2500);
}

TEST(money_rejects_invalid_amounts) {
    const char* bad[] = {"", "abc", "-5", "1.234", "1.2.3", "0", "0.00", ".5", "1,5", "12abc"};
    for (std::size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); ++i) {
        Cents c = 123;
        std::string err;
        CHECK(!parseMoney(bad[i], c, err));
        CHECK(!err.empty());
        CHECK_EQ(c, 123);  // left untouched on failure
    }
}

TEST(money_is_formatted_exactly) {
    CHECK_EQ(formatMoney(10000), "R100.00");
    CHECK_EQ(formatMoney(5), "R0.05");
    CHECK_EQ(formatMoney(12345), "R123.45");
    CHECK_EQ(formatMoney(7, "GHS"), "GHS 0.07");
}

TEST(money_has_no_rounding_errors) {
    // With floating point, 0.1 + 0.2 != 0.3. With whole cents it is exact.
    Cents a = 0, b = 0, c = 0;
    std::string err;
    parseMoney("0.10", a, err);
    parseMoney("0.20", b, err);
    parseMoney("0.30", c, err);
    CHECK_EQ(a + b, c);

    // Three odd payments add up to exactly R100.00.
    Cents x = 0, y = 0, z = 0;
    parseMoney("33.33", x, err);
    parseMoney("33.33", y, err);
    parseMoney("33.34", z, err);
    CHECK_EQ(x + y + z, 10000);
}





// ------------------------------------------------------------------ runner

int main() {
    int failedTests = 0;
    std::vector<TestCase>& tests = registry();
    for (std::size_t i = 0; i < tests.size(); ++i) {
        int before = g_failures;
        try {
            tests[i].run();
        } catch (const std::exception& e) {
            ++g_failures;
            std::cerr << "    unexpected exception: " << e.what() << "\n";
        }
        bool passed = (g_failures == before);
        if (!passed) ++failedTests;
        std::cout << (passed ? "[ PASS ] " : "[ FAIL ] ") << tests[i].name << "\n";
    }
    std::cout << "\n" << (tests.size() - failedTests) << " of " << tests.size()
              << " tests passed (" << g_checks << " checks).\n";
    return failedTests == 0 ? 0 : 1;
}
