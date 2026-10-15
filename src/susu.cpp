// susu.cpp - implementation of the susu rules. See susu.h.
#include "susu.h"

#include <cctype>
#include <cstdlib>
#include <sstream>
#include <stdexcept>

namespace susu {

// ------------------------------------------------------------------ money

std::string trim(const std::string& text) {
    std::size_t a = text.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    std::size_t b = text.find_last_not_of(" \t\r\n");
    return text.substr(a, b - a + 1);
}

std::string normalise(const std::string& text) {
    std::string s = trim(text);
    for (std::size_t i = 0; i < s.size(); ++i)
        s[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(s[i])));
    return s;
}

bool parseMoney(const std::string& raw, Cents& cents, std::string& error) {
    std::string text = trim(raw);
    if (text.empty())   { error = "Amount is empty."; return false; }
    if (text[0] == '-') { error = "Amount must be positive."; return false; }

    std::size_t dot = text.find('.');
    if (dot != std::string::npos && text.find('.', dot + 1) != std::string::npos) {
        error = "Not a valid amount.";
        return false;
    }
    std::string whole = (dot == std::string::npos) ? text : text.substr(0, dot);
    std::string fraction = (dot == std::string::npos) ? "" : text.substr(dot + 1);

    if (whole.empty()) { error = "Not a valid amount."; return false; }
    for (std::size_t i = 0; i < whole.size(); ++i)
        if (!std::isdigit(static_cast<unsigned char>(whole[i]))) { error = "Not a valid amount."; return false; }
    for (std::size_t i = 0; i < fraction.size(); ++i)
        if (!std::isdigit(static_cast<unsigned char>(fraction[i]))) { error = "Not a valid amount."; return false; }
    if (fraction.size() > 2) { error = "Use at most 2 decimal places."; return false; }
    if (whole.size() > 9)    { error = "Amount is too large."; return false; }

    while (fraction.size() < 2) fraction += '0';
    Cents value = std::atoll(whole.c_str()) * 100 + std::atoll(fraction.c_str());
    if (value <= 0) { error = "Amount must be greater than zero."; return false; }
    cents = value;
    return true;
}

std::string formatMoney(Cents cents, const std::string& currency) {
    std::ostringstream out;
    if (cents < 0) {
        out << "-";
        cents = -cents;
    }
    out << currency;
    if (currency.size() > 1) out << " ";
    out << cents / 100 << "." << (cents % 100 < 10 ? "0" : "") << cents % 100;
    return out.str();
}

// ------------------------------------------------------------------ group

namespace {
Result make(Status status, const std::string& message) {
    Result r;
    r.status = status;
    r.message = message;
    return r;
}
}  // namespace

Group::Group(Cents weeklyContribution, const std::string& currency,
             std::size_t maxMembers, std::size_t minToStart)
    : contribution_(weeklyContribution),
      currency_(currency),
      maxMembers_(maxMembers),
      minToStart_(minToStart) {
    if (weeklyContribution <= 0)
        throw std::invalid_argument("The weekly contribution must be greater than zero.");
    week_ = 1;
    cycle_ = 1;
}

int Group::indexOf(const std::string& id) const {
    std::string wanted = normalise(id);
    for (std::size_t i = 0; i < members_.size(); ++i)
        if (normalise(members_[i].id) == wanted) return static_cast<int>(i);
    return -1;
}

Result Group::addMember(const std::string& rawId) {
    std::string id = trim(rawId);
    if (id.empty())
        return make(INVALID_ID, "A member ID or name cannot be empty.");
    if (id.find('|') != std::string::npos || id.find(',') != std::string::npos)
        return make(INVALID_ID, "A member ID or name cannot contain '|' or ','.");
    if (members_.size() >= maxMembers_)
        return make(GROUP_FULL, "The group is full.");
    if (indexOf(id) >= 0)
        return make(DUPLICATE_MEMBER, "'" + id + "' is already a member.");

    Member m;
    m.id = id;
    m.paid = 0;
    members_.push_back(m);
    return make(OK, "Added " + id + ".");
}

Cents Group::owedBy(const std::string& id) const {
    int i = indexOf(id);
    if (i < 0) return -1;
    return contribution_ - members_[i].paid;
}

std::vector<Debt> Group::debts() const {
    std::vector<Debt> result;
    for (std::size_t i = 0; i < members_.size(); ++i) {
        Cents owed = contribution_ - members_[i].paid;
        if (owed > 0) {
            Debt d;
            d.memberId = members_[i].id;
            d.owed = owed;
            result.push_back(d);
        }
    }
    return result;
}

Result Group::recordContribution(const std::string& id, int week, Cents amount) {
    if (members_.size() < minToStart_)
        return make(NOT_ENOUGH_MEMBERS, "The group needs at least " +
                    std::to_string(minToStart_) + " members before payments start.");
    int i = indexOf(id);
    if (i < 0)
        return make(UNKNOWN_MEMBER, "Unknown member: " + trim(id) + ".");
    if (week != week_)
        return make(WRONG_WEEK, "Payments are accepted for the current week only (week " +
                    std::to_string(week_) + ").");
    if (amount <= 0)
        return make(INVALID_AMOUNT, "The amount must be greater than zero.");

    Member& m = members_[i];
    Cents owed = contribution_ - m.paid;
    if (owed == 0)
        return make(OVERPAYMENT, m.id + " has already paid in full this week.");
    if (amount > owed)
        return make(OVERPAYMENT, "That is more than " + m.id + " owes (" +
                    formatMoney(owed, currency_) + ").");

    m.paid += amount;
    return make(OK, "Recorded " + formatMoney(amount, currency_) + " for " + m.id +
                ". Still owes " + formatMoney(contribution_ - m.paid, currency_) + ".");
}




}  // namespace susu
