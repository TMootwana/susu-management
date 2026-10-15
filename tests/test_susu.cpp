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

// ------------------------------------------------------------ group helpers

const Cents R100 = 10000;

// A group of R100 contributors called M1, M2, ... Mn.
Group makeGroup(int members) {
    Group g(R100);
    for (int i = 1; i <= members; ++i) g.addMember("M" + std::to_string(i));
    return g;
}

// ------------------------------------------------------------ member tests

TEST(group_rejects_a_zero_or_negative_contribution) {
    bool threw = false;
    try { Group g(0); } catch (const std::invalid_argument&) { threw = true; }
    CHECK(threw);
    threw = false;
    try { Group g(-500); } catch (const std::invalid_argument&) { threw = true; }
    CHECK(threw);
}

TEST(member_ids_must_be_unique_ignoring_case_and_spaces) {
    Group g(R100);
    CHECK_EQ(g.addMember("Ama").status, OK);
    CHECK_EQ(g.addMember("ama").status, DUPLICATE_MEMBER);
    CHECK_EQ(g.addMember("  AMA  ").status, DUPLICATE_MEMBER);
    CHECK_EQ(g.addMember("Kofi").status, OK);
    CHECK_EQ(g.members().size(), 2u);
}

TEST(empty_or_invalid_ids_are_rejected) {
    Group g(R100);
    CHECK_EQ(g.addMember("").status, INVALID_ID);
    CHECK_EQ(g.addMember("   ").status, INVALID_ID);
    CHECK_EQ(g.addMember("A|B").status, INVALID_ID);
    CHECK_EQ(g.members().size(), 0u);
}

TEST(group_is_limited_to_its_maximum_size) {
    Group g(R100, "R", 3);
    CHECK_EQ(g.addMember("A").status, OK);
    CHECK_EQ(g.addMember("B").status, OK);
    CHECK_EQ(g.addMember("C").status, OK);
    CHECK_EQ(g.addMember("D").status, GROUP_FULL);
    CHECK_EQ(g.members().size(), 3u);
}

// ------------------------------------------------------------ contribution tests

TEST(unknown_member_is_rejected) {
    Group g = makeGroup(3);
    CHECK_EQ(g.recordContribution("NOBODY", 1, 5000).status, UNKNOWN_MEMBER);
    CHECK_EQ(g.debts().size(), 3u);  // nothing changed
}

TEST(zero_and_negative_amounts_are_rejected) {
    Group g = makeGroup(3);
    CHECK_EQ(g.recordContribution("M1", 1, 0).status, INVALID_AMOUNT);
    CHECK_EQ(g.recordContribution("M1", 1, -1).status, INVALID_AMOUNT);
    CHECK_EQ(g.recordContribution("M1", 1, -10000).status, INVALID_AMOUNT);
    CHECK_EQ(g.owedBy("M1"), R100);
}

TEST(payment_above_the_amount_owed_is_rejected) {
    Group g = makeGroup(3);
    CHECK_EQ(g.recordContribution("M1", 1, 10001).status, OVERPAYMENT);
    CHECK_EQ(g.owedBy("M1"), R100);
    CHECK_EQ(g.recordContribution("M1", 1, 6000).status, OK);
    CHECK_EQ(g.recordContribution("M1", 1, 4001).status, OVERPAYMENT);  // only 4000 left
    CHECK_EQ(g.owedBy("M1"), 4000);
}

TEST(partial_payments_are_accepted_and_the_balance_is_tracked) {
    Group g = makeGroup(3);
    CHECK_EQ(g.recordContribution("M1", 1, 3000).status, OK);
    CHECK_EQ(g.owedBy("M1"), 7000);
    CHECK_EQ(g.recordContribution("M1", 1, 2500).status, OK);
    CHECK_EQ(g.owedBy("M1"), 4500);
    CHECK_EQ(g.recordContribution("M1", 1, 4500).status, OK);
    CHECK_EQ(g.owedBy("M1"), 0);
    CHECK_EQ(g.recordContribution("M1", 1, 1).status, OVERPAYMENT);  // already paid in full
    CHECK_EQ(g.debts().size(), 2u);  // only M2 and M3 still owe
}

TEST(payment_for_the_wrong_week_is_rejected) {
    Group g = makeGroup(3);
    CHECK_EQ(g.recordContribution("M1", 2, 5000).status, WRONG_WEEK);
    CHECK_EQ(g.recordContribution("M1", 0, 5000).status, WRONG_WEEK);
    CHECK_EQ(g.owedBy("M1"), R100);
}

TEST(payments_need_enough_members) {
    Group g(R100);
    g.addMember("ONLY");
    CHECK_EQ(g.recordContribution("ONLY", 1, 5000).status, NOT_ENOUGH_MEMBERS);
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
