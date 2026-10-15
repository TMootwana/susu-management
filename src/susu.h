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


}  // namespace susu

#endif
