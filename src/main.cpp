// main.cpp - console menu for the susu manager.
// All rules live in susu.cpp; this file only reads input and prints results.
//
// Usage:  susu [weekly_amount] [currency]      e.g.  susu 100.00 R   or   susu 50 GHS
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

#include "susu.h"

using namespace std;
using namespace susu;

namespace {

const string MEMBERS_FILE = "members.txt";

bool ask(const string& prompt, string& answer) {
    cout << prompt;
    return static_cast<bool>(getline(cin, answer));
}

bool parseWeek(const string& text, int& week) {
    string s = trim(text);
    if (s.empty() || s.size() > 6) return false;
    for (size_t i = 0; i < s.size(); ++i)
        if (s[i] < '0' || s[i] > '9') return false;
    week = atoi(s.c_str());
    return true;
}

// Adds one ID to the end of members.txt (and adds the missing newline first if the
// file does not end with one, so two IDs are never glued together).
void appendToMembersFile(const string& id) {
    bool needNewline = false;
    {
        ifstream in(MEMBERS_FILE.c_str(), ios::binary);
        if (in) {
            in.seekg(0, ios::end);
            if (in.tellg() > 0) {
                in.seekg(-1, ios::end);
                char last = 0;
                in.get(last);
                needNewline = (last != '\n');
            }
        }
    }
    ofstream out(MEMBERS_FILE.c_str(), ios::app);
    if (!out) {
        cout << "  (warning: could not write to " << MEMBERS_FILE << ")\n";
        return;
    }
    if (needNewline) out << "\n";
    out << id << "\n";
}

// Reads one member per line from a text file and adds them to the group.
// Blank lines and lines starting with '#' are skipped. For "M001|Ama" or "M001,Ama"
// only the text before the '|' or ',' is used as the ID. Duplicates and anything the
// group refuses are reported and skipped.
// Returns false if the file could not be opened.
bool addMembersFromFile(Group& group, const string& path, bool saveToMembersFile) {
    ifstream file(path.c_str());
    if (!file) return false;
    string line;
    int lineNo = 0, added = 0, skipped = 0;
    while (getline(file, line)) {
        ++lineNo;
        if (lineNo == 1 && line.size() >= 3 &&  // hidden UTF-8 marker some editors add
            static_cast<unsigned char>(line[0]) == 0xEF &&
            static_cast<unsigned char>(line[1]) == 0xBB &&
            static_cast<unsigned char>(line[2]) == 0xBF)
            line.erase(0, 3);
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        size_t cut = line.find_first_of("|,");
        if (cut != string::npos) line = trim(line.substr(0, cut));

        Result r = group.addMember(line);
        if (r.ok()) {
            ++added;
            if (saveToMembersFile) appendToMembersFile(line);
        } else {
            ++skipped;
            cout << "  " << path << " line " << lineNo << ": " << r.message << "\n";
        }
    }
    cout << path << ": added " << added << ", skipped " << skipped << ".\n";
    return true;
}

void printReport(const Group& g) {
    const string& cur = g.currency();
    cout << "\n=========== SUSU REPORT ===========\n";
    cout << "Weekly contribution: " << formatMoney(g.weeklyContribution(), cur) << " per member\n";
    cout << "Cycle " << g.cycle() << ", week " << g.week() << "  |  members: "
         << g.members().size() << "\n";
    if (g.isComplete())
        cout << "Status: cycle complete - every member has been paid.\n";
    else if (!g.nextRecipient().empty())
        cout << "This week's recipient: " << g.nextRecipient() << " (pot "
             << formatMoney(g.weeklyContribution() * static_cast<Cents>(g.members().size()), cur) << ")\n";
    cout << "New members: " << (g.isLocked() ? "CLOSED until the whole group has been paid" : "open") << "\n";

    cout << "\n-- Paid in full this week --\n";
    bool any = false;
    for (size_t i = 0; i < g.members().size(); ++i)
        if (g.owedBy(g.members()[i].id) == 0) {
            cout << "  " << g.members()[i].id << "\n";
            any = true;
        }
    if (!any) cout << "  (nobody yet)\n";

    cout << "\n-- Still owing this week --\n";
    vector<Debt> debts = g.debts();
    for (size_t i = 0; i < debts.size(); ++i)
        cout << "  " << debts[i].memberId << " owes " << formatMoney(debts[i].owed, cur) << "\n";
    if (debts.empty()) cout << "  (nobody)\n";

    cout << "\n-- Payout history --\n";
    const vector<Payout>& history = g.payouts();
    for (size_t i = 0; i < history.size(); ++i)
        cout << "  Cycle " << history[i].cycle << ", week " << history[i].week << ": "
             << history[i].memberId << " received " << formatMoney(history[i].amount, cur) << "\n";
    if (history.empty()) cout << "  (no payouts yet)\n";
    cout << "===================================\n";
}

}  // namespace

int main(int argc, char* argv[]) {
    Cents contribution = 10000;  // R100.00 by default
    string currency = "R";
    if (argc >= 2) {
        string error;
        if (!parseMoney(argv[1], contribution, error)) {
            cerr << "Invalid weekly amount '" << argv[1] << "': " << error << "\n";
            return 1;
        }
    }
    if (argc >= 3) currency = argv[2];

    Group group(contribution, currency);
    cout << "SUSU MANAGER - " << formatMoney(contribution, currency) << " per member per week\n";
    if (!addMembersFromFile(group, MEMBERS_FILE, false))
        cout << "No " << MEMBERS_FILE << " found: starting with an empty group.\n";

    string choice;
    while (true) {
        cout << "\n1) Add member\n2) Record a contribution\n3) Close the week\n4) Report\n"
             << "5) Start the next cycle\n6) Import members from a txt file\n0) Exit\n";
        if (!ask("Choose: ", choice)) break;
        choice = trim(choice);

        if (choice == "0") {
            break;
        } else if (choice == "1") {
            string id;
            if (!ask("Member ID or name (Enter = automatic code): ", id)) break;
            if (trim(id).empty()) id = group.nextFreeCode();
            Result r = group.addMember(id);
            cout << r.message << "\n";
            if (r.ok()) appendToMembersFile(trim(id));
        } else if (choice == "2") {
            string id, weekText, amountText;
            if (!ask("Member ID: ", id)) break;
            if (!ask("Week (Enter = current week " + to_string(group.week()) + "): ", weekText)) break;
            if (!ask("Amount: ", amountText)) break;
            int week = group.week();
            if (!trim(weekText).empty() && !parseWeek(weekText, week)) {
                cout << "Invalid week number.\n";
                continue;
            }
            Cents amount = 0;
            string error;
            if (!parseMoney(amountText, amount, error)) {
                cout << error << "\n";
                continue;
            }
            cout << group.recordContribution(id, week, amount).message << "\n";
        } else if (choice == "3") {
            cout << group.closeWeek().message << "\n";
        } else if (choice == "4") {
            printReport(group);
        } else if (choice == "5") {
            cout << group.startNextCycle().message << "\n";
        } else if (choice == "6") {
            string path;
            if (!ask("Path of txt file: ", path)) break;
            if (!addMembersFromFile(group, trim(path), trim(path) != MEMBERS_FILE))
                cout << "Cannot open file: " << trim(path) << "\n";
        } else {
            cout << "Please choose one of the listed options.\n";
        }
    }
    cout << "Goodbye.\n";
    return 0;
}
