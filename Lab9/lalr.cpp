#include <bits/stdc++.h>
using namespace std;

struct Production
{
    string lhs;
    vector<string> rhs;
};

struct LR1Item
{
    int productionIndex;   // index into productions
    int dotPosition;       // 0..rhs.size()
    set<string> lookahead; // lookahead symbols

    bool operator==(const LR1Item &o) const
    {
        return productionIndex == o.productionIndex &&
               dotPosition == o.dotPosition &&
               lookahead == o.lookahead;
    }
};

struct LR1ItemHash
{
    size_t operator()(const LR1Item &x) const noexcept
    {
        size_t h = (static_cast<size_t>(x.productionIndex) << 16) ^ static_cast<size_t>(x.dotPosition);
        for (const string &s : x.lookahead)
        {
            h ^= hash<string>{}(s) + 0x9e3779b9 + (h << 6) + (h >> 2);
        }
        return h;
    }
};

class LALRParser
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
    vector<vector<LR1Item>> clrStates;          // original CLR states
    vector<vector<LR1Item>> lalrStates;         // merged LALR states
    map<pair<int, string>, int> clrTransition;  // CLR transitions
    map<pair<int, string>, int> lalrTransition; // LALR transitions
    map<int, int> stateMapping;                 // CLR state -> LALR state mapping

    // Parsing tables
    // ACTION[state][terminal] = string like "sN", "rK", "acc", or "" for empty
    map<int, map<string, string>> action;
    // GOTO[state][nonterminal] = next state index, or -1 if empty
    map<int, map<string, int>> gotoTable;

    // Utility: stringify a production item
    string itemToString(const LR1Item &it) const
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
        out += " , {";
        bool first = true;
        for (const string &la : it.lookahead)
        {
            if (!first)
                out += ", ";
            out += la;
            first = false;
        }
        out += "}";
        return out;
    }

    // Normalize state representation: sort and unique
    vector<LR1Item> normalize(const vector<LR1Item> &items) const
    {
        vector<LR1Item> v = items;
        sort(v.begin(), v.end(), [&](const LR1Item &a, const LR1Item &b)
             {
            if (a.productionIndex != b.productionIndex) return a.productionIndex < b.productionIndex;
            if (a.dotPosition != b.dotPosition) return a.dotPosition < b.dotPosition;
            return a.lookahead < b.lookahead; });
        v.erase(unique(v.begin(), v.end(), [&](const LR1Item &a, const LR1Item &b)
                       { return a.productionIndex == b.productionIndex &&
                                a.dotPosition == b.dotPosition &&
                                a.lookahead == b.lookahead; }),
                v.end());
        return v;
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

    // Compute FIRST(βa) where β is a string and a is a terminal
    set<string> computeFirstBeta(const vector<string> &beta, const string &a) const
    {
        map<string, set<string>> first = computeFirstSets();
        set<string> result;

        if (beta.empty())
        {
            result.insert(a);
            return result;
        }

        bool allNullable = true;
        for (const string &symbol : beta)
        {
            if (isTerminal(symbol))
            {
                result.insert(symbol);
                allNullable = false;
                break;
            }
            else if (isNonTerminal(symbol))
            {
                for (const string &firstSymbol : first[symbol])
                {
                    result.insert(firstSymbol);
                }
                if (first[symbol].count("$") == 0)
                {
                    allNullable = false;
                    break;
                }
            }
        }

        if (allNullable)
        {
            result.insert(a);
        }

        return result;
    }

    // closure(I) for LR(1) items
    vector<LR1Item> closure(vector<LR1Item> I) const
    {
        unordered_set<LR1Item, LR1ItemHash> S(I.begin(), I.end());
        bool changed = true;
        while (changed)
        {
            changed = false;
            for (const LR1Item &it : vector<LR1Item>(S.begin(), S.end()))
            {
                const Production &p = productions[it.productionIndex];
                if (it.dotPosition < (int)p.rhs.size())
                {
                    string X = p.rhs[it.dotPosition];
                    if (isNonTerminal(X))
                    {
                        // Compute FIRST(βa) where β is the string after X
                        vector<string> beta;
                        for (int i = it.dotPosition + 1; i < (int)p.rhs.size(); i++)
                        {
                            beta.push_back(p.rhs[i]);
                        }

                        set<string> firstBetaA;
                        for (const string &la : it.lookahead)
                        {
                            set<string> firstBeta = computeFirstBeta(beta, la);
                            for (const string &symbol : firstBeta)
                            {
                                firstBetaA.insert(symbol);
                            }
                        }

                        // Add all productions with X as LHS
                        for (int i = 0; i < (int)productions.size(); i++)
                        {
                            if (productions[i].lhs == X)
                            {
                                LR1Item add{i, 0, firstBetaA};
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
        vector<LR1Item> out(S.begin(), S.end());
        return normalize(out);
    }

    // goto(I, X) for LR(1) items
    vector<LR1Item> goTo(const vector<LR1Item> &I, const string &X) const
    {
        vector<LR1Item> J;
        for (const LR1Item &it : I)
        {
            const Production &p = productions[it.productionIndex];
            if (it.dotPosition < (int)p.rhs.size() && p.rhs[it.dotPosition] == X)
            {
                J.push_back({it.productionIndex, it.dotPosition + 1, it.lookahead});
            }
        }
        return closure(normalize(J));
    }

    // Find or add a CLR state, returning its index
    int getCLRStateIndex(const vector<LR1Item> &I)
    {
        for (int i = 0; i < (int)clrStates.size(); i++)
        {
            if (clrStates[i] == I)
                return i;
        }
        clrStates.push_back(I);
        return (int)clrStates.size() - 1;
    }

    // Find or add a LALR state, returning its index
    int getLALRStateIndex(const vector<LR1Item> &I)
    {
        for (int i = 0; i < (int)lalrStates.size(); i++)
        {
            if (lalrStates[i] == I)
                return i;
        }
        lalrStates.push_back(I);
        return (int)lalrStates.size() - 1;
    }

    // Get LR(0) core of an LR(1) item (remove lookahead)
    pair<int, int> getLR0Core(const LR1Item &item) const
    {
        return {item.productionIndex, item.dotPosition};
    }

    // Get LR(0) core of a state
    set<pair<int, int>> getLR0Core(const vector<LR1Item> &state) const
    {
        set<pair<int, int>> core;
        for (const LR1Item &item : state)
        {
            core.insert(getLR0Core(item));
        }
        return core;
    }

    // Build canonical collection and transitions (CLR first)
    void buildCLRCollection()
    {
        vector<LR1Item> I0 = closure({LR1Item{0, 0, {"$"}}}); // [S' -> . S, {$}]
        getCLRStateIndex(I0);
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
                vector<LR1Item> J = goTo(clrStates[i], X);
                if (J.empty())
                    continue;
                int j = getCLRStateIndex(J);
                if (!clrTransition.count({i, X}))
                {
                    clrTransition[{i, X}] = j;
                    if (j == (int)clrStates.size() - 1)
                    {
                        q.push(j);
                    }
                }
            }
        }
    }

    // Merge CLR states with same LR(0) cores to form LALR states
    void mergeStatesToLALR()
    {
        map<set<pair<int, int>>, vector<int>> coreToStates;

        // Group CLR states by their LR(0) cores
        for (int i = 0; i < (int)clrStates.size(); i++)
        {
            set<pair<int, int>> core = getLR0Core(clrStates[i]);
            coreToStates[core].push_back(i);
        }

        // Create LALR states by merging states with same cores
        for (auto it = coreToStates.begin(); it != coreToStates.end(); ++it)
        {
            const set<pair<int, int>> &core = it->first;
            const vector<int> &stateIndices = it->second;

            // Merge all items from states with same core
            vector<LR1Item> mergedState;
            for (int clrStateIndex : stateIndices)
            {
                for (const LR1Item &item : clrStates[clrStateIndex])
                {
                    mergedState.push_back(item);
                }
            }

            // Normalize and deduplicate
            mergedState = normalize(mergedState);

            // Create LALR state
            int lalrStateIndex = getLALRStateIndex(mergedState);

            // Map CLR states to LALR state
            for (int clrStateIndex : stateIndices)
            {
                stateMapping[clrStateIndex] = lalrStateIndex;
            }
        }

        // Build LALR transitions
        for (auto &transition : clrTransition)
        {
            int clrFrom = transition.first.first;
            string symbol = transition.first.second;
            int clrTo = transition.second;

            int lalrFrom = stateMapping[clrFrom];
            int lalrTo = stateMapping[clrTo];

            lalrTransition[{lalrFrom, symbol}] = lalrTo;
        }
    }

    // Build LALR parsing table
    void buildParsingTable()
    {
        // Initialize GOTO entries to -1 for nonterminals
        for (int i = 0; i < (int)lalrStates.size(); i++)
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

        // Fill SHIFT and GOTO from LALR transitions
        for (auto &e : lalrTransition)
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

        // REDUCE and ACCEPT using lookahead symbols from merged states
        for (int i = 0; i < (int)lalrStates.size(); i++)
        {
            for (const LR1Item &it : lalrStates[i])
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
                // LALR: reduce on lookahead symbols from merged states
                for (const string &a : it.lookahead)
                {
                    setAction(i, a, reduceStr);
                }
            }
        }
    }

    string itemsToString(const vector<LR1Item> &I) const
    {
        string s;
        for (const LR1Item &it : I)
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
    LALRParser()
    {
        // Build productions in fixed order; indices will be printed in table
        productions.push_back({"S'", {"S"}});     // 0
        productions.push_back({"S", {"C", "C"}}); // 1
        productions.push_back({"C", {"c", "C"}}); // 2
        productions.push_back({"C", {"d"}});      // 3
    }

    void run()
    {
        cout << "LALR PARSER IMPLEMENTATION" << "\n";
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

        // Build CLR collection first
        buildCLRCollection();

        cout << "CANONICAL COLLECTION OF LR(1) ITEM SETS (CLR):" << "\n";
        for (int i = 0; i < (int)clrStates.size(); i++)
        {
            cout << "I" << i << ":\n";
            cout << itemsToString(clrStates[i]);
        }
        cout << "\n";

        // Merge states to form LALR
        mergeStatesToLALR();

        cout << "MERGED LALR STATES:" << "\n";
        for (int i = 0; i < (int)lalrStates.size(); i++)
        {
            cout << "I" << i << " (merged from CLR states: ";
            vector<int> mergedFrom;
            for (auto &pair : stateMapping)
            {
                if (pair.second == i)
                {
                    mergedFrom.push_back(pair.first);
                }
            }
            sort(mergedFrom.begin(), mergedFrom.end());
            for (int j = 0; j < (int)mergedFrom.size(); j++)
            {
                cout << mergedFrom[j];
                if (j + 1 < (int)mergedFrom.size())
                    cout << ", ";
            }
            cout << "):\n";
            cout << itemsToString(lalrStates[i]);
        }
        cout << "\n";

        // DFA transitions (text only)
        cout << "DFA STATE TRANSITIONS (text):" << "\n";
        vector<tuple<int, string, int>> edges;
        for (auto &e : lalrTransition)
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
        cout << "LALR PARSING TABLE (ACTION | GOTO)" << "\n";
        cout << setw(6) << "state";
        for (const string &a : termOrder)
            cout << setw(8) << a;
        cout << " | ";
        for (const string &A : nonTermOrder)
            cout << setw(6) << A;
        cout << "\n";
        cout << string(6 + 8 * termOrder.size() + 3 + 6 * nonTermOrder.size(), '-') << "\n";
        for (int i = 0; i < (int)lalrStates.size(); i++)
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

    LALRParser parser;
    parser.run();
    return 0;
}
