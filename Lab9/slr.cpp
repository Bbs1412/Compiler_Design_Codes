#include <bits/stdc++.h>
using namespace std;

struct Production
{
    string lhs;
    vector<string> rhs;
};

struct LR0Item
{
    int productionIndex; // index into productions
    int dotPosition;     // 0..rhs.size()

    bool operator==(const LR0Item &o) const
    {
        return productionIndex == o.productionIndex && dotPosition == o.dotPosition;
    }
};

struct LR0ItemHash
{
    size_t operator()(const LR0Item &x) const noexcept
    {
        return (static_cast<size_t>(x.productionIndex) << 16) ^ static_cast<size_t>(x.dotPosition);
    }
};

class SLRParser
{
private:
    // Grammar for the assignment (augmented):
    // S' -> S
    // S  -> C C
    // C  -> c C | d
    vector<Production> productions;
    string startSymbol = "S'";
    set<string> nonTerminals{"S'", "S", "C"};
    set<string> terminals{"c", "d", "$"}; // include $ for table building/printing

    // FOLLOW sets for SLR parsing
    map<string, set<string>> followSets;

    // Helpers for symbol detection
    bool isTerminal(const string &sym) const
    {
        return terminals.count(sym) > 0;
    }
    bool isNonTerminal(const string &sym) const
    {
        return nonTerminals.count(sym) > 0;
    }

    // Canonical collection
    vector<vector<LR0Item>> states;         // each state is a vector of items (deduped & sorted)
    map<pair<int, string>, int> transition; // (state, symbol) -> state

    // Parsing tables
    // ACTION[state][terminal] = string like "sN", "rK", "acc", or "" for empty
    map<int, map<string, string>> action;
    // GOTO[state][nonterminal] = next state index, or -1 if empty
    map<int, map<string, int>> gotoTable;

    // Utility: stringify a production item
    string itemToString(const LR0Item &it) const
    {
        const Production &p = productions[it.productionIndex];
        string out = p.lhs + " -> ";
        for (int i = 0; i <= (int)p.rhs.size(); i++)
        {
            if (i == it.dotPosition)
                out += ". ";
            if (i < (int)p.rhs.size())
            {
                out += p.rhs[i];
                if (i + 1 < (int)p.rhs.size())
                    out += ' ';
            }
        }
        return out;
    }

    // Normalize state representation: sort and unique
    vector<LR0Item> normalize(const vector<LR0Item> &items) const
    {
        vector<LR0Item> v = items;
        sort(v.begin(), v.end(), [&](const LR0Item &a, const LR0Item &b)
             {
            if (a.productionIndex != b.productionIndex) return a.productionIndex < b.productionIndex;
            return a.dotPosition < b.dotPosition; });
        v.erase(unique(v.begin(), v.end(), [&](const LR0Item &a, const LR0Item &b)
                       { return a.productionIndex == b.productionIndex && a.dotPosition == b.dotPosition; }),
                v.end());
        return v;
    }

    // closure(I) - same as LR(0)
    vector<LR0Item> closure(vector<LR0Item> I) const
    {
        unordered_set<LR0Item, LR0ItemHash> S(I.begin(), I.end());
        bool changed = true;
        while (changed)
        {
            changed = false;
            for (const LR0Item &it : vector<LR0Item>(S.begin(), S.end()))
            {
                const Production &p = productions[it.productionIndex];
                if (it.dotPosition < (int)p.rhs.size())
                {
                    string X = p.rhs[it.dotPosition];
                    if (isNonTerminal(X))
                    {
                        for (int i = 0; i < (int)productions.size(); i++)
                        {
                            if (productions[i].lhs == X)
                            {
                                LR0Item add{i, 0};
                                if (!S.count(add))
                                {
                                    S.insert(add);
                                    changed = true;
                                }
                            }
                        }
                    }
                }
            }
        }
        vector<LR0Item> out(S.begin(), S.end());
        return normalize(out);
    }

    // goto(I, X) - same as LR(0)
    vector<LR0Item> goTo(const vector<LR0Item> &I, const string &X) const
    {
        vector<LR0Item> J;
        for (const LR0Item &it : I)
        {
            const Production &p = productions[it.productionIndex];
            if (it.dotPosition < (int)p.rhs.size() && p.rhs[it.dotPosition] == X)
            {
                J.push_back({it.productionIndex, it.dotPosition + 1});
            }
        }
        return closure(normalize(J));
    }

    // Find or add a state, returning its index
    int getStateIndex(const vector<LR0Item> &I)
    {
        for (int i = 0; i < (int)states.size(); i++)
        {
            if (states[i] == I)
                return i;
        }
        states.push_back(I);
        return (int)states.size() - 1;
    }

    // Compute FIRST sets for non-terminals
    map<string, set<string>> computeFirstSets() const
    {
        map<string, set<string>> first;

        // Initialize FIRST sets
        for (const string &nt : nonTerminals)
        {
            first[nt] = set<string>();
        }

        bool changed = true;
        while (changed)
        {
            changed = false;
            for (const Production &p : productions)
            {
                const string &A = p.lhs;
                if (p.rhs.empty())
                    continue;

                string firstSymbol = p.rhs[0];
                if (isTerminal(firstSymbol))
                {
                    if (first[A].insert(firstSymbol).second)
                    {
                        changed = true;
                    }
                }
                else if (isNonTerminal(firstSymbol))
                {
                    for (const string &symbol : first[firstSymbol])
                    {
                        if (first[A].insert(symbol).second)
                        {
                            changed = true;
                        }
                    }
                }
            }
        }

        return first;
    }

    // Compute FOLLOW sets for non-terminals
    void computeFollowSets()
    {
        map<string, set<string>> first = computeFirstSets();

        // Initialize FOLLOW sets
        for (const string &nt : nonTerminals)
        {
            followSets[nt] = set<string>();
        }

        // Add $ to FOLLOW(S') where S' is the start symbol
        followSets[startSymbol].insert("$");

        bool changed = true;
        while (changed)
        {
            changed = false;

            for (const Production &p : productions)
            {
                const string &A = p.lhs;

                for (int i = 0; i < (int)p.rhs.size(); i++)
                {
                    string B = p.rhs[i];
                    if (!isNonTerminal(B))
                        continue;

                    // Case 1: B is followed by something
                    if (i + 1 < (int)p.rhs.size())
                    {
                        string nextSymbol = p.rhs[i + 1];

                        if (isTerminal(nextSymbol))
                        {
                            if (followSets[B].insert(nextSymbol).second)
                            {
                                changed = true;
                            }
                        }
                        else if (isNonTerminal(nextSymbol))
                        {
                            // Add FIRST(nextSymbol) to FOLLOW(B)
                            for (const string &symbol : first[nextSymbol])
                            {
                                if (followSets[B].insert(symbol).second)
                                {
                                    changed = true;
                                }
                            }
                        }
                    }
                    else
                    {
                        // Case 2: B is at the end of production
                        // Add FOLLOW(A) to FOLLOW(B)
                        for (const string &symbol : followSets[A])
                        {
                            if (followSets[B].insert(symbol).second)
                            {
                                changed = true;
                            }
                        }
                    }
                }
            }
        }
    }

    // Build canonical collection and transitions - same as LR(0)
    void buildCanonicalCollection()
    {
        vector<LR0Item> I0 = closure({LR0Item{0, 0}}); // [S' -> . S]
        getStateIndex(I0);
        queue<int> q;
        q.push(0);
        set<string> symbols;
        symbols.insert("c");
        symbols.insert("d");
        symbols.insert("S");
        symbols.insert("C"); // grammar symbols
        while (!q.empty())
        {
            int i = q.front();
            q.pop();
            for (const string &X : symbols)
            {
                vector<LR0Item> J = goTo(states[i], X);
                if (J.empty())
                    continue;
                int j = getStateIndex(J);
                if (!transition.count({i, X}))
                {
                    transition[{i, X}] = j;
                    if (j == (int)states.size() - 1)
                    {
                        q.push(j);
                    }
                }
            }
        }
    }

    // Build SLR parsing table using FOLLOW sets
    void buildParsingTable()
    {
        // Initialize GOTO entries to -1 for nonterminals
        for (int i = 0; i < (int)states.size(); i++)
        {
            for (const string &A : nonTerminals)
                if (A != startSymbol)
                    gotoTable[i][A] = -1;
        }

        auto setAction = [&](int i, const string &a, const string &val)
        {
            string &cell = action[i][a];
            if (!cell.empty() && cell != val)
            {
                cout << "[Conflict] ACTION[" << i << "][" << a << "] already '" << cell << "', new '" << val << "'\n";
            }
            if (cell.empty())
                cell = val; // keep first, report conflicts
        };

        // Fill SHIFT and GOTO from transitions
        for (auto &e : transition)
        {
            int i = e.first.first;
            const string &X = e.first.second;
            int j = e.second;
            if (isTerminal(X))
            {
                setAction(i, X, string("s") + to_string(j));
            }
            else if (isNonTerminal(X))
            {
                gotoTable[i][X] = j;
            }
        }

        // REDUCE and ACCEPT using FOLLOW sets (SLR specific)
        for (int i = 0; i < (int)states.size(); i++)
        {
            for (const LR0Item &it : states[i])
            {
                const Production &p = productions[it.productionIndex];
                bool atEnd = (it.dotPosition == (int)p.rhs.size());
                if (!atEnd)
                    continue;

                if (p.lhs == startSymbol)
                {
                    setAction(i, "$", "acc");
                    continue;
                }

                string reduceStr = string("r") + to_string(it.productionIndex);
                // SLR: reduce only on FOLLOW(p.lhs)
                for (const string &a : followSets[p.lhs])
                {
                    setAction(i, a, reduceStr);
                }
            }
        }
    }

    string itemsToString(const vector<LR0Item> &I) const
    {
        string s;
        for (const LR0Item &it : I)
        {
            s += "  " + itemToString(it) + "\n";
        }
        return s;
    }

    // Tokenize input: 'c' and 'd' letters only
    vector<string> tokenize(const string &s) const
    {
        vector<string> out;
        for (char ch : s)
        {
            if (isspace((unsigned char)ch))
                continue;
            if (ch == 'c' || ch == 'd')
                out.push_back(string(1, ch));
        }
        out.push_back("$");
        return out;
    }

    // Apply a reduce action rK on the parse stack using production K
    bool applyReduce(vector<int> &stateStack, vector<string> &symbolStack, int prodIndex)
    {
        const Production &p = productions[prodIndex];
        int betaSize = (int)p.rhs.size();
        for (int i = 0; i < betaSize; i++)
        {
            if (symbolStack.empty() || stateStack.empty())
                return false;
            symbolStack.pop_back();
            stateStack.pop_back();
        }
        int t = stateStack.back();
        int g = gotoTable[t][p.lhs];
        if (g < 0)
            return false;
        symbolStack.push_back(p.lhs);
        stateStack.push_back(g);
        return true;
    }

public:
    SLRParser()
    {
        // Build productions in fixed order; indices will be printed in table
        productions.push_back({"S'", {"S"}});     // 0
        productions.push_back({"S", {"C", "C"}}); // 1
        productions.push_back({"C", {"c", "C"}}); // 2
        productions.push_back({"C", {"d"}});      // 3
    }

    void run()
    {
        cout << "SLR PARSER IMPLEMENTATION" << "\n";
        cout << string(80, '=') << "\n\n";

        // Print grammar
        cout << "GRAMMAR (AUGMENTED):\n";
        for (int i = 0; i < (int)productions.size(); i++)
        {
            cout << setw(2) << i << ": " << productions[i].lhs << " -> ";
            for (int j = 0; j < (int)productions[i].rhs.size(); j++)
            {
                cout << productions[i].rhs[j] << (j + 1 < (int)productions[i].rhs.size() ? " " : "");
            }
            cout << "\n";
        }
        cout << "\n";

        // Compute FOLLOW sets
        computeFollowSets();

        cout << "FOLLOW SETS:\n";
        for (const string &nt : nonTerminals)
        {
            if (nt == startSymbol)
                continue; // Skip S' in output
            cout << "FOLLOW(" << nt << ") = {";
            bool first = true;
            for (const string &symbol : followSets[nt])
            {
                if (!first)
                    cout << ", ";
                cout << symbol;
                first = false;
            }
            cout << "}\n";
        }
        cout << "\n";

        // Canonical collection
        buildCanonicalCollection();

        cout << "CANONICAL COLLECTION OF LR(0) ITEM SETS:" << "\n";
        for (int i = 0; i < (int)states.size(); i++)
        {
            cout << "I" << i << ":\n";
            cout << itemsToString(states[i]);
        }
        cout << "\n";

        // DFA transitions (text only)
        cout << "DFA STATE TRANSITIONS (text):" << "\n";
        vector<tuple<int, string, int>> edges;
        for (auto &e : transition)
            edges.emplace_back(e.first.first, e.first.second, e.second);
        sort(edges.begin(), edges.end());
        for (auto &tup : edges)
        {
            int i, j;
            string X;
            tie(i, X, j) = tup;
            cout << "I" << i << " --" << X << "--> I" << j << "\n";
        }
        cout << "\n";

        // Parsing table
        buildParsingTable();

        // Print ACTION table header
        vector<string> termOrder = {"c", "d", "$"};
        vector<string> nonTermOrder = {"S", "C"};
        cout << "SLR PARSING TABLE (ACTION | GOTO)" << "\n";
        cout << setw(6) << "state";
        for (const string &a : termOrder)
            cout << setw(8) << a;
        cout << " | ";
        for (const string &A : nonTermOrder)
            cout << setw(6) << A;
        cout << "\n";
        cout << string(6 + 8 * termOrder.size() + 3 + 6 * nonTermOrder.size(), '-') << "\n";
        for (int i = 0; i < (int)states.size(); i++)
        {
            cout << setw(6) << i;
            for (const string &a : termOrder)
            {
                string cell = action[i][a];
                cout << setw(8) << (cell.empty() ? "." : cell);
            }
            cout << " | ";
            for (const string &A : nonTermOrder)
            {
                int g = gotoTable[i][A];
                cout << setw(6) << (g < 0 ? string(".") : to_string(g));
            }
            cout << "\n";
        }
        cout << "\n";

        // Parsing trace for input ccdd
        string input = "ccdd";
        vector<string> inBuf = tokenize(input);
        cout << string(80, '=') << "\n";
        cout << "PARSING TRACE FOR INPUT: " << input << "\n";
        cout << string(80, '=') << "\n";
        cout << setw(30) << "Stack" << " | "
             << setw(15) << "Input" << " | Action\n";
        cout << string(80, '-') << "\n";

        vector<int> stateStack;
        stateStack.push_back(0);
        vector<string> symbolStack;
        int ip = 0;
        int step = 1;

        auto stackStatesToString = [&]()
        { string s; for (int x : stateStack) { s += to_string(x); s += ' '; } return s; };
        auto stackSymbolsToString = [&]()
        { string s; for (auto &x : symbolStack) { s += x; s += ' '; } return s; };
        auto combinedStackToString = [&]()
        {
            string s;
            // Interleave: state0, sym0, state1, sym1, ..., stateN
            for (size_t i = 0; i < symbolStack.size(); i++)
            {
                s += to_string(stateStack[i]);
                s += ' ';
                s += symbolStack[i];
                s += ' ';
            }
            // last state
            s += to_string(stateStack.back());
            s += ' ';
            return s;
        };
        auto inputToString = [&](int pos)
        { string s; for (int i = pos; i < (int)inBuf.size(); i++) { s += inBuf[i]; s += ' '; } return s; };

        bool accepted = false;
        while (true)
        {
            int sTop = stateStack.back();
            string a = inBuf[ip];
            string act = action[sTop][a];
            cout << setw(30) << combinedStackToString() << " | "
                 << setw(15) << inputToString(ip) << " | ";
            if (act.empty())
            {
                cout << "Error (no ACTION)\n";
                break;
            }
            if (act == "acc")
            {
                cout << "Accept\n";
                accepted = true;
                break;
            }
            if (act[0] == 's')
            {
                int j = stoi(act.substr(1));
                cout << "Shift to I" << j << "\n";
                symbolStack.push_back(a);
                stateStack.push_back(j);
                ip++;
            }
            else if (act[0] == 'r')
            {
                int k = stoi(act.substr(1));
                const Production &p = productions[k];
                cout << "Reduce by [" << k << ": " << p.lhs << " -> ";
                for (int i = 0; i < (int)p.rhs.size(); i++)
                    cout << p.rhs[i] << (i + 1 < (int)p.rhs.size() ? " " : "");
                cout << "]\n";
                bool ok = applyReduce(stateStack, symbolStack, k);
                if (!ok)
                {
                    cout << "Error during reduce\n";
                    break;
                }
            }
            else
            {
                cout << "Error (unknown ACTION)\n";
                break;
            }
            if (step++ > 1000)
            {
                cout << "Error: step limit exceeded\n";
                break;
            }
        }

        cout << string(60, '=') << "\n";
        cout << "Final Result: " << (accepted ? "ACCEPTED" : "REJECTED") << "\n";
        cout << string(60, '=') << "\n";
    }
};

int main()
{
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    SLRParser parser;
    parser.run();
    return 0;
}
