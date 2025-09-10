// lr0_parser.cpp
// LR(0) parser implementation for the grammar:
// S' -> S
// S  -> C C
// C  -> c C | d
//
// Builds canonical LR(0) items, DFA, parsing table, and parses input "ccdd".

#include <bits/stdc++.h>
using namespace std;

// ------------------------------------------------------------------------------
// Production structure
// ------------------------------------------------------------------------------
struct Prod
{
    int id;
    string lhs;
    vector<string> rhs;
};

// LR(0) Item: production id and dot position (0..rhs.size())
struct Item
{
    int pid;
    int dot;
    bool operator<(Item const &o) const
    {
        if (pid != o.pid)
            return pid < o.pid;
        return dot < o.dot;
    }
    bool operator==(Item const &o) const
    {
        return pid == o.pid && dot == o.dot;
    }
};

// Convert item to readable string
string itemToStr(const Item &it, const vector<Prod> &prods)
{
    const Prod &p = prods[it.pid];
    string s = p.lhs + " -> ";
    for (size_t i = 0; i < p.rhs.size(); ++i)
    {
        if ((int)i == it.dot)
            s += ". ";
        s += p.rhs[i];
        if (i + 1 < p.rhs.size())
            s += " ";
    }
    if (it.dot == (int)p.rhs.size())
        s += " .";
    return s;
}

// set of items type
using ItemSet = set<Item>;

// Utilities to check if symbol is terminal
bool isNonTerminal(const string &sym)
{
    if (sym.empty())
        return false;
    // in our grammar: Non-terminals are uppercase (S, C, S')
    // terminals are single lowercase 'c','d' or $
    // treat any string whose first char is uppercase or contains '\'' as non-terminal
    char ch = sym[0];
    return (isupper(ch) || sym.find('\'') != string::npos);
}

int main()
{
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    // ------------------------------------------------------------------------------
    //  1. Define grammar
    // ------------------------------------------------------------------------------

    // productions vector
    vector<Prod> prods;
    // Augmented grammar: 0: S' -> S
    prods.push_back({0, "S'", {"S"}});
    prods.push_back({1, "S", {"C", "C"}});
    prods.push_back({2, "C", {"c", "C"}});
    prods.push_back({3, "C", {"d"}});

    // Terminals and nonterminals (explicit)
    vector<string> terminals = {"c", "d", "$"};
    vector<string> nonterminals = {"S", "C", "S'"}; // keep S' for printing GOTO if present

    // ------------------------------------------------------------------------------
    // 2. closure()
    // ------------------------------------------------------------------------------

    auto closure = [&](const ItemSet &I)
    {
        ItemSet C = I;
        bool added = true;
        while (added)
        {
            added = false;
            vector<Item> toAdd;
            for (auto &it : C)
            {
                // get symbol after dot
                const Prod &p = prods[it.pid];
                if (it.dot < (int)p.rhs.size())
                {
                    string X = p.rhs[it.dot];
                    if (isNonTerminal(X))
                    {
                        // add all productions of X with dot at 0
                        for (const Prod &q : prods)
                        {
                            if (q.lhs == X)
                            {
                                Item newIt{q.id, 0};
                                if (C.find(newIt) == C.end())
                                {
                                    toAdd.push_back(newIt);
                                }
                            }
                        }
                    }
                }
            }
            if (!toAdd.empty())
            {
                for (auto &x : toAdd)
                {
                    C.insert(x);
                    added = true;
                }
            }
        }
        return C;
    };

    // ------------------------------------------------------------------------------
    // 3. goto()
    // ------------------------------------------------------------------------------

    auto goto_fn = [&](const ItemSet &I, const string &X)
    {
        ItemSet J;
        for (auto &it : I)
        {
            const Prod &p = prods[it.pid];
            if (it.dot < (int)p.rhs.size() && p.rhs[it.dot] == X)
            {
                Item moved{it.pid, it.dot + 1};
                J.insert(moved);
            }
        }
        return closure(J);
    };

    // ------------------------------------------------------------------------------
    // 4. Construct canonical collection of LR(0) items
    // ------------------------------------------------------------------------------
    vector<ItemSet> C;         // states
    map<ItemSet, int> stateId; // map set -> id
    queue<ItemSet> q;

    // start state: closure({ S' -> . S })
    ItemSet start;
    start.insert({0, 0}); // prod 0 dot at 0
    ItemSet I0 = closure(start);
    C.push_back(I0);
    stateId[I0] = 0;
    q.push(I0);

    // all grammar symbols set (terminals + nonterminals)
    set<string> symbols;
    for (auto &t : terminals)
        symbols.insert(t);
    for (auto &nt : nonterminals)
        symbols.insert(nt);

    // BFS
    while (!q.empty())
    {
        ItemSet I = q.front();
        q.pop();
        int idI = stateId[I];
        // for every grammar symbol X, compute goto(I,X)
        for (auto &X : symbols)
        {
            // skip $ as it's not a grammar symbol here for transitions
            if (X == "$")
                continue;
            ItemSet J = goto_fn(I, X);
            if (J.empty())
                continue;
            if (!stateId.count(J))
            {
                int newid = (int)C.size();
                C.push_back(J);
                stateId[J] = newid;
                q.push(J);
            }
        }
    }

    // ------------------------------------------------------------------------------
    // 5. Print canonical collection
    // ------------------------------------------------------------------------------
    cout << "---------- [#] Canonical Collection of LR(0) Items [#] ----------\n\n";
    for (size_t i = 0; i < C.size(); ++i)
    {
        cout << "State I" << i << ":\n";
        for (auto &it : C[i])
        {
            cout << "  " << itemToStr(it, prods) << "\n";
        }
        cout << "\n";
    }

    // ------------------------------------------------------------------------------
    // 6. Build DFA transitions
    // ------------------------------------------------------------------------------

    // transitions[state][symbol] = nextState
    map<int, map<string, int>> transitions;
    for (size_t i = 0; i < C.size(); ++i)
    {
        for (auto &sym : symbols)
        {
            if (sym == "$")
                continue;
            ItemSet J = goto_fn(C[i], sym);
            if (!J.empty())
            {
                int j = stateId[J];
                transitions[(int)i][sym] = j;
            }
        }
    }

    cout << "---------- [#] DFA (state transitions) [#] ----------\n\n";
    for (auto &p : transitions)
    {
        int s = p.first;
        for (auto &edge : p.second)
        {
            cout << "  I" << s << " -- " << edge.first << " --> I" << edge.second << "\n";
        }
    }
    cout << "\n";

    // ------------------------------------------------------------------------------
    // 7. Construct LR(0) Parsing Table (ACTION & GOTO)
    // ------------------------------------------------------------------------------

    // GOTO as map[state][nonterminal] = nextstate
    map<int, map<string, string>> ACTION;
    map<int, map<string, int>> GOTO;

    for (size_t i = 0; i < C.size(); ++i)
    {
        // For shifts: items A->alpha·a beta (a is terminal)
        for (auto &it : C[i])
        {
            const Prod &p = prods[it.pid];
            if (it.dot < (int)p.rhs.size())
            {
                string a = p.rhs[it.dot];
                // if a is terminal and goto exists
                if (!isNonTerminal(a))
                {
                    // check transition
                    if (transitions.count((int)i) && transitions[(int)i].count(a))
                    {
                        int j = transitions[(int)i][a];
                        ACTION[(int)i][a] = "s" + to_string(j); // shift j
                    }
                }
                else
                {
                    // if X is nonterminal, set GOTO
                    if (transitions.count((int)i) && transitions[(int)i].count(a))
                    {
                        int j = transitions[(int)i][a];
                        GOTO[(int)i][a] = j;
                    }
                }
            }
            else
            {
                // Dot at end: A -> alpha .
                if (p.lhs == "S'")
                {
                    // Accept when S' -> S .
                    ACTION[(int)i]["$"] = "acc";
                }
                else
                {
                    // Reduce by production p.id on ALL terminals (LR(0) rule)
                    string red = "r" + to_string(p.id);
                    for (auto &t : terminals)
                    {
                        if (!ACTION[(int)i].count(t))
                        {
                            ACTION[(int)i][t] = red;
                        }
                        else
                        {
                            // conflict detection (not expected for this grammar)
                            if (ACTION[(int)i][t] != red)
                            {
                                // mark as conflict
                                ACTION[(int)i][t] = ACTION[(int)i][t] + " | " + red;
                            }
                        }
                    }
                }
            }
        }
        // Also set GOTO entries already collected via transitions for nonterminals that appear
    }

    // ------------------------------------------------------------------------------
    // 8. Print the LR(0) Parsing Table
    // ------------------------------------------------------------------------------

    cout << "---------- [#] LR(0) Parsing Table [#] ----------\n\n";

    // Header
    vector<string> termHdr = {"c", "d", "$"};
    vector<string> nontermHdr = {"S", "C"};
    // Print header
    cout << left << setw(8) << "State";
    for (auto &t : termHdr)
        cout << left << setw(8) << t;
    for (auto &nt : nontermHdr)
        cout << left << setw(8) << nt;
    cout << "\n";

    for (size_t i = 0; i < C.size(); ++i)
    {
        cout << left << setw(8) << i;
        for (auto &t : termHdr)
        {
            string cell = "-";
            if (ACTION[(int)i].count(t))
                cell = ACTION[(int)i][t];
            cout << left << setw(8) << cell;
        }
        for (auto &nt : nontermHdr)
        {
            string cell = "-";
            if (GOTO[(int)i].count(nt))
                cell = to_string(GOTO[(int)i][nt]);
            cout << left << setw(8) << cell;
        }
        cout << "\n";
    }
    cout << "\n";

    // ------------------------------------------------------------------------------
    // 9. Parsing the input string "ccdd"
    // ------------------------------------------------------------------------------

    string inputRaw = "ccdd";
    // token vector with $ at end
    vector<string> inputTokens;
    for (char ch : inputRaw)
    {
        string s(1, ch);
        inputTokens.push_back(s);
    }
    inputTokens.push_back("$");

    cout << "---------- [#] Parsing Trace (input = " << inputRaw << ") [#] ----------\n\n";
    // Parser stack: state stack (ints) and symbol stack (strings) for readability
    vector<int> stateStack;
    vector<string> symStack;
    stateStack.push_back(0);
    symStack.push_back("$");

    size_t ip = 0;
    bool accepted = false;
    int step = 0;

    auto printStep = [&](const string &actionDesc)
    {
        // Print step number, stack content (states and symbols), input buffer, action
        cout << setw(3) << left << step << " | ";
        // Format states
        stringstream ssStates;
        ssStates << "[";
        for (size_t i = 0; i < stateStack.size(); ++i)
        {
            if (i)
                ssStates << " ";
            ssStates << stateStack[i];
        }
        ssStates << "]";
        cout << setw(18) << left << ssStates.str() << " | ";

        // Format symbols
        stringstream ssSyms;
        ssSyms << "[";
        for (size_t i = 0; i < symStack.size(); ++i)
        {
            if (i)
                ssSyms << " ";
            ssSyms << symStack[i];
        }
        ssSyms << "]";
        cout << setw(13) << left << ssSyms.str() << " | ";

        // Format input
        stringstream ssInput;
        for (size_t k = ip; k < inputTokens.size(); ++k)
        {
            ssInput << inputTokens[k];
        }
        cout << setw(7) << left << ssInput.str() << " | ";

        // Action
        cout << left << actionDesc << "\n";
    };

    cout << "Step| Stack (states)     | Symbols       | Input   | Action\n";
    cout << "------------------------------------------------------------------------------------------\n";

    while (true)
    {
        step++;
        int s = stateStack.back();
        string a = inputTokens[ip];

        string action = "-";
        if (ACTION[s].count(a))
            action = ACTION[s][a];
        else
            action = "error";

        if (action == "error")
        {
            printStep("Error: no action -> reject");
            break;
        }

        if (action == "acc" || action == "acc")
        {
            printStep("Accept");
            accepted = true;
            break;
        }

        if (action.size() > 0 && action[0] == 's')
        {
            // shift
            int t = stoi(action.substr(1));
            // push symbol and state
            symStack.push_back(a);
            stateStack.push_back(t);
            ip++; // consume input token
            printStep("Shift and go to state " + to_string(t));
            continue;
        }

        if (action.size() > 0 && action[0] == 'r')
        {
            int prodId = stoi(action.substr(1));
            const Prod &rp = prods[prodId];
            // pop |rhs| symbols and states
            int popCount = (int)rp.rhs.size();
            // If production RHS is epsilon, pop 0
            for (int k = 0; k < popCount; k++)
            {
                if (!symStack.empty())
                    symStack.pop_back();
                if (!stateStack.empty())
                    stateStack.pop_back();
            }
            // push LHS symbol
            symStack.push_back(rp.lhs);
            // goto from current state
            int topState = stateStack.back();
            int gotoState = -1;
            if (GOTO[topState].count(rp.lhs))
            {
                gotoState = GOTO[topState][rp.lhs];
            }
            else
            {
                // If GOTO not found, that is an error
                printStep("Error: no GOTO from state " + to_string(topState) + " for " + rp.lhs);
                break;
            }
            stateStack.push_back(gotoState);
            string ad = "Reduce by [" + to_string(prodId) + "] " + rp.lhs + " ->";
            for (size_t i = 0; i < rp.rhs.size(); ++i)
            {
                if (i)
                    ad += " ";
                ad += rp.rhs[i];
            }
            printStep(ad + ", then GOTO " + to_string(gotoState));
            continue;
        }

        // If none matched
        printStep("Unhandled action: " + action);
        break;
    }

    cout << "\nParsing result: " << (accepted ? "ACCEPTED" : "REJECTED") << "\n\n";

    // Helpful: Print productions mapping for r# references
    cout << "Productions (for reference):\n";
    for (auto &p : prods)
    {
        cout << "  " << p.id << ": " << p.lhs << " ->";
        if (p.rhs.empty())
            cout << " epsilon";
        for (auto &s : p.rhs)
            cout << " " << s;
        cout << "\n";
    }

    cout << "\n(DONE)\n";
    return 0;
}
