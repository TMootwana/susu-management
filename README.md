# Susu Manager

A small C++ console program that manages **one rotating savings group (a susu)**.

A group of members each pay the same fixed amount every week. Each week the whole pot is paid to one member, in the order the members joined, one member per week.

Default: **R100.00 per member per week**, up to **10 members**. The amount and currency can be changed when you start the program (see below).

## Project layout

```
src/susu.h        Rules of the susu (Group class, money handling). No console I/O.
src/susu.cpp      Implementation of the rules.
src/main.cpp      Console menu (reads input, prints results).
tests/test_susu.cpp   Automated tests (own tiny test runner, no libraries needed).
members.txt       Member IDs, one per line (example included).
Makefile          make / make test
NOTES.md          Assumptions, decisions, and what I would improve.
```

The rules are kept separate from the menu on purpose: `Group` can be called directly (a small API) and tested without typing anything.

## Requirements

A C++11 compiler (tested with g++ 13, built with `-std=c++11 -Wall -Wextra -pedantic`, no warnings). Code::Blocks with MinGW works too.

## Build and run

With `make`:

```
make            # builds ./susu
./susu          # R100.00 per week
./susu 50 GHS   # or choose the weekly amount and currency
```

Without `make` (any terminal):

```
g++ -std=c++11 -Wall -Wextra -pedantic -o susu src/main.cpp src/susu.cpp
./susu
```

In Code::Blocks: create a console project, add `src/main.cpp`, `src/susu.cpp` and `src/susu.h`, and press **F9**. Put `members.txt` in the folder the program runs from (Project > Properties > Build targets > *Execution working dir*). Windows users run `susu.exe` instead of `./susu`.

## Using the program

```
1) Add member
2) Record a contribution
3) Close the week
4) Report
5) Start the next cycle
6) Import members from a txt file
0) Exit
```

- **Add member** - type a unique name or ID, or press Enter for an automatic code (`M001`, `M002`, ...). New members are also appended to `members.txt`.
- **Record a contribution** - member ID, week (Enter = current week), amount such as `25.00`. Partial payments are allowed.
- **Close the week** - if everyone has paid in full, the pot goes to the next member in order and the week advances. Otherwise nothing is paid and it lists who still owes how much.
- **Report** - current week, who has paid in full, who owes what, and the payout history.
- **Start the next cycle** - only possible once every member has been paid. Payout order starts again.
- **Import members** - reads member IDs from another txt file and merges them in; duplicates are skipped and reported.

### members.txt

One member per line. The first member listed is paid first.

```
# comments and blank lines are ignored
M001
M002
M003
```

`M001|Ama` or `M001,Ama` also work: only the part before `|` or `,` is used as the ID. IDs are case-insensitive and must be unique. The file is read when the program starts.

## Rules

- Rejected: unknown members, duplicate members, zero or negative amounts, payments above what the member still owes this week, payments for a week other than the current one.
- Money is held as whole **cents** in a `long long` (R100.00 = `10000`), so there are no rounding errors. Amounts with more than 2 decimal places are rejected.
- **No new member can join until the whole group is paid.** The group locks when the first payment of a cycle is recorded. It opens again only after every member has received the pot and the next cycle has been started.
- A week cannot close while anyone owes money; nothing changes when it fails.

More detail on these choices is in [NOTES.md](NOTES.md).

## Running the tests

```
make test
```

or without `make`:

```
g++ -std=c++11 -Wall -Wextra -pedantic -o run_tests tests/test_susu.cpp src/susu.cpp
./run_tests
```

The program prints `[ PASS ]` or `[ FAIL ]` for each test, then a summary, and exits with code 0 only if everything passed. Expected ending:

```
22 of 22 tests passed (139 checks).
```

The 22 tests cover, among other things:

- partial payments being accepted and the balance tracked (`partial_payments_are_accepted_and_the_balance_is_tracked`)
- a week that cannot close while someone owes, with nobody paid and nothing changed (`week_cannot_close_while_someone_owes`)
- unknown members, duplicates, zero/negative amounts, overpayment, wrong week
- exact money (`0.10 + 0.20 == 0.30`, and `33.33 + 33.33 + 33.34 == 100.00`)
- payout order, the pot size, and the cycle completing
- the "no joining until the whole group is paid" rule and starting the next cycle

In Code::Blocks, make a second console project containing `tests/test_susu.cpp`, `src/susu.cpp` and `src/susu.h` (do not mix `main.cpp` and `test_susu.cpp` in one project: each has its own `main`).
