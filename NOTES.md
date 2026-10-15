# Notes

## Assumptions (things the brief did not say, and what I chose)

1. **Who can join, and when.** Members can be added only while no payment has been recorded in the current cycle. The first recorded payment locks the group. It unlocks only after *every* member has received the pot and the next cycle has been started. This is the rule "no new member can join until the whole group is paid". Adding a member after week 1 has started is refused with a clear message.
2. **Group size.** At most 10 members (the size of the group in the brief). At least 2 are needed before payments can be recorded or a week closed, because a susu with one member is meaningless. Both numbers are constructor parameters of `Group`.
3. **Only the current week accepts payments.** A week cannot close until everyone has paid, so earlier weeks are always settled, and paying for future weeks is not allowed. The week number is still part of the `recordContribution` call and is validated.
4. **A member who has already paid in full** cannot pay again that week. This is reported as an overpayment (they owe nothing).
5. **The pot** is the weekly contribution multiplied by the number of members. The recipient in week *n* is the *n*-th member in join order.
6. **Order of joining** is the order of the lines in `members.txt` (or the order members were added in the menu). A member who joins in a later cycle is paid last in that cycle.
7. **Next cycle.** When every member has been paid, the cycle is complete: no more payments or closes until the next cycle is started. Starting it resets the week to 1 and the order starts again. Payout history from earlier cycles is kept.
8. **Member identity.** A member is a unique name or ID. Comparison ignores case and surrounding spaces (`ama` and `AMA ` are the same member). `|` and `,` are not allowed in IDs because they separate fields in `members.txt`.
9. **Currency.** Rand by default (`R`), as in the example. The brief mentions GHS in one place, so the currency symbol and weekly amount can be given on the command line: `./susu 50 GHS`.
10. **Amounts** may have at most 2 decimal places and at most 9 digits before the decimal point.

## Decisions

- **Money is whole cents in a `long long`.** No `double`, so no rounding errors. Amounts are parsed from text by hand (digits only) instead of using `stod`.
- **Rules are separate from the menu.** `Group` (in `susu.h/.cpp`) never prints or reads anything. `main.cpp` is a thin menu. This satisfies "a command line, a simple menu or a small API", makes automated testing possible, and no UI was built.
- **Failures are returned, not thrown.** Each action returns a `Result` with a `Status` code and a message, and changes nothing when it fails. Tests check the status code rather than the wording. The only exception is the constructor, which throws `std::invalid_argument` for a zero or negative weekly contribution.
- **Closing a week is all-or-nothing.** If anyone owes, nothing is paid, nothing is reset, and the message lists each member and the exact amount owed.
- **Tests use a tiny built-in runner** (a `TEST` macro and `CHECK` / `CHECK_EQ`) so the project has no dependency and builds anywhere with one `g++` command.
- **`members.txt` is only appended to**, never rewritten, so comments and anything else in the file are preserved. If the file does not end with a newline, one is added first.
- **Development order.** The code was built in small stages (money, members, contributions, closing a week, joining rule, menu, docs). Each commit compiled and passed all tests that existed at that point.

## Testing notes

- 22 automated tests, 139 checks. Besides running them, I confirmed they actually catch mistakes by temporarily breaking two rules (letting a week close while someone owes, and allowing overpayment). Both made tests fail, and restoring the code made them pass again.
- The menu (`main.cpp`) was checked by running a scripted session by hand, but it has no automated tests.

## What I would improve with more time

1. **Save the state.** Payments, the current week and the payout history exist only while the program runs; only `members.txt` persists. A state file (or small database) would fix that.
2. **Test the menu layer** too, for example by feeding input through a stream instead of `cin`/`cout`.
3. **Use a standard test framework** (Catch2 or GoogleTest) and run the tests automatically on every push with GitHub Actions.
4. **More flexible weeks.** Allow late payments for earlier weeks, a payment deadline, and what to do about a member who never pays (skip, penalty, or removal) instead of the group waiting.
5. **Members leaving or being replaced** mid-cycle, and a configurable payout order (for example, by draw) instead of join order only.
6. **Better member data.** Store names next to IDs and validate the members file format more strictly.
7. **A ledger export** (CSV) of every payment and payout for record keeping.
