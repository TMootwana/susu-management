// susu.h - the rules of a susu (rotating savings group).
//
// There is no console input/output in this file or in susu.cpp, so every rule
// can be tested automatically. main.cpp is only a thin menu on top of it.
#ifndef SUSU_H
#define SUSU_H

#include <cstddef>
#include <string>
#include <vector>

namespace susu {

// All money is a whole number of cents (R100.00 is 10000). No floating point.
typedef long long Cents;

std::string trim(const std::string& text);
std::string normalise(const std::string& text);  // trim + UPPER CASE, used to compare IDs

// Accepts "100", "100.5" and "100.50". On failure returns false and fills 'error'.
bool parseMoney(const std::string& text, Cents& cents, std::string& error);

// 10050 -> "R100.50". A currency longer than one character gets a space ("GHS 100.50").
std::string formatMoney(Cents cents, const std::string& currency = "R");

// Why an action was refused. OK means it worked.
enum Status {
    OK,
    INVALID_ID,
    DUPLICATE_MEMBER,
    GROUP_FULL,
    NOT_ENOUGH_MEMBERS,
    UNKNOWN_MEMBER,
    WRONG_WEEK,
    INVALID_AMOUNT,
    OVERPAYMENT,
    CYCLE_COMPLETE,
    WEEK_NOT_PAID,
    GROUP_LOCKED,
    CYCLE_NOT_COMPLETE
};

// The outcome of an action. When status is not OK nothing was changed.
struct Result {
    Status status;
    std::string message;
    bool ok() const { return status == OK; }
};

struct Member {
    std::string id;
    Cents paid;  // paid so far in the current week
};

struct Debt {
    std::string memberId;
    Cents owed;
};

struct Payout {
    int cycle;
    int week;
    std::string memberId;
    Cents amount;
};

class Group {
public:
    // Every member pays 'weeklyContribution' cents each week.
    // Throws std::invalid_argument if the contribution is not positive.
    explicit Group(Cents weeklyContribution, const std::string& currency = "R",
                   std::size_t maxMembers = 10, std::size_t minToStart = 2);

    // --- actions ---
    Result addMember(const std::string& id);
    Result recordContribution(const std::string& id, int week, Cents amount);
    Result closeWeek();
    Result startNextCycle();

    // --- read-only views ---
    Cents weeklyContribution() const { return contribution_; }
    const std::string& currency() const { return currency_; }
    const std::vector<Member>& members() const { return members_; }
    int week() const { return week_; }
    int cycle() const { return cycle_; }
    Cents owedBy(const std::string& id) const;  // -1 if there is no such member
    std::vector<Debt> debts() const;            // everyone who still owes this week
    const std::vector<Payout>& payouts() const { return payouts_; }
    bool isComplete() const { return complete_; }
    std::string nextRecipient() const;          // "" when nobody is due a payout
    bool isLocked() const { return locked_; }
    std::string nextFreeCode() const;           // M001, M002, ... first unused

private:
    int indexOf(const std::string& id) const;  // -1 if unknown

    Cents contribution_;
    std::string currency_;
    std::size_t maxMembers_;
    std::size_t minToStart_;
    std::vector<Member> members_;
    int week_;
    int cycle_;
    std::vector<Payout> payouts_;
    bool complete_;
    bool locked_;
};

}  // namespace susu

#endif
