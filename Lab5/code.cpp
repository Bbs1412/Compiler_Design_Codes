#include <bits/stdc++.h>
using namespace std;

static const string EPS = "ε";
static const string END_MARKER = "$";

struct Grammar
{
    unordered_map<string, vector<vector<string>>> productions;
    vector<string> nonterminals_list;
    unordered_set<string> nonterminals;
    unordered_set<string> terminals;
    string start;
    unordered_map<string, unordered_set<string>> FIRST;
    unordered_map<string, unordered_set<string>> FOLLOW;
    unordered_map<string, unordered_map<string, vector<string>>> table;
};

string trim(const string &s)
{
    size_t i = 0, j = s.size();
    while (i < j && isspace((unsigned char)s[i]))
        ++i;
    while (j > i && isspace((unsigned char)s[j - 1]))
        --j;
    return s.substr(i, j - i);
}

bool is_epsilon_token(const string &t)
{
    string low;
    low.reserve(t.size());
    for (char c : t)
        low.push_back((char)tolower((unsigned char)c));
    return (t == EPS) || (low == "eps") || (low == "epsilon");
}

vector<string> split_symbols(const string &rhs)
{
    vector<string> out;
    stringstream ss(rhs);
    string tok;
    while (ss >> tok)
    {
        if (is_epsilon_token(tok))
            out.push_back(EPS);
        else
            out.push_back(tok);
    }

    return out;
}

void collect_terminals(Grammar &G)
{
    for (auto it = G.productions.begin(); it != G.productions.end(); ++it)
    {
        const string &A = it->first;
        auto &alts = it->second;
        for (size_t i = 0; i < alts.size(); ++i)
        {
            for (size_t j = 0; j < alts[i].size(); ++j)
            {
                const string &sym = alts[i][j];
                if (sym == EPS)
                    continue;
                if (!G.nonterminals.count(sym))
                {
                    G.terminals.insert(sym);
                }
            }
        }
    }

    G.terminals.insert(END_MARKER);
}

bool add_to_set(unordered_set<string> &s, const unordered_set<string> &add)
{
    size_t before = s.size();
    for (auto it = add.begin(); it != add.end(); ++it)
        s.insert(*it);
    return s.size() != before;
}

bool add_to_set_excluding_epsilon(unordered_set<string> &dest, const unordered_set<string> &src)
{
    size_t before = dest.size();
    for (auto it = src.begin(); it != src.end(); ++it)
        if (*it != EPS)
            dest.insert(*it);
    return dest.size() != before;
}

const unordered_set<string> &FIRST_of_symbol(Grammar &G, const string &X)
{
    static unordered_map<string, unordered_set<string>> cacheTerm;
    if (!G.nonterminals.count(X))
    {
        if (cacheTerm.count(X))
            return cacheTerm[X];
        unordered_set<string> tmp;
        tmp.insert(X);
        cacheTerm[X] = tmp;
        return cacheTerm[X];
    }

    return G.FIRST[X];
}

void compute_FIRST(Grammar &G)
{
    for (size_t i = 0; i < G.nonterminals_list.size(); ++i)
        G.FIRST[G.nonterminals_list[i]];
    bool changed = true;
    while (changed)
    {
        changed = false;
        for (size_t idx = 0; idx < G.nonterminals_list.size(); ++idx)
        {
            const string &A = G.nonterminals_list[idx];
            for (size_t p = 0; p < G.productions[A].size(); ++p)
            {
                const vector<string> &alpha = G.productions[A][p];
                bool allNullable = true;
                for (size_t i = 0; i < alpha.size(); ++i)
                {
                    string X = alpha[i];
                    const auto &F_X = FIRST_of_symbol(G, X);
                    if (add_to_set_excluding_epsilon(G.FIRST[A], F_X))
                        changed = true;
                    if (!F_X.count(EPS))
                    {
                        allNullable = false;
                        break;
                    }
                }

                if (allNullable)
                {
                    if (!G.FIRST[A].count(EPS))
                    {
                        G.FIRST[A].insert(EPS);
                        changed = true;
                    }
                }
            }
        }
    }
}

unordered_set<string> FIRST_of_sequence(Grammar &G, const vector<string> &alpha)
{
    unordered_set<string> res;
    if (alpha.empty())
    {
        res.insert(EPS);
        return res;
    }

    bool allNullable = true;
    for (size_t i = 0; i < alpha.size(); ++i)
    {
        const auto &FX = FIRST_of_symbol(G, alpha[i]);
        for (auto it = FX.begin(); it != FX.end(); ++it)
            if (*it != EPS)
                res.insert(*it);
        if (!FX.count(EPS))
        {
            allNullable = false;
            break;
        }
    }

    if (allNullable)
        res.insert(EPS);
    return res;
}

void compute_FOLLOW(Grammar &G)
{
    for (size_t i = 0; i < G.nonterminals_list.size(); ++i)
        G.FOLLOW[G.nonterminals_list[i]];
    G.FOLLOW[G.start].insert(END_MARKER);
    bool changed = true;
    while (changed)
    {
        changed = false;
        for (auto it = G.productions.begin(); it != G.productions.end(); ++it)
        {
            const string &A = it->first;
            auto &alts = it->second;
            for (size_t p = 0; p < alts.size(); ++p)
            {
                const vector<string> &alpha = alts[p];
                for (size_t i = 0; i < alpha.size(); ++i)
                {
                    string B = alpha[i];
                    if (!G.nonterminals.count(B))
                        continue;
                    vector<string> beta;
                    for (size_t j = i + 1; j < alpha.size(); ++j)
                        beta.push_back(alpha[j]);
                    auto FIRST_beta = FIRST_of_sequence(G, beta);
                    if (add_to_set_excluding_epsilon(G.FOLLOW[B], FIRST_beta))
                        changed = true;
                    if (FIRST_beta.count(EPS))
                    {
                        if (add_to_set(G.FOLLOW[B], G.FOLLOW[A]))
                            changed = true;
                    }
                }
            }
        }
    }
}

bool build_LL1_table(Grammar &G, vector<string> &conflicts)
{
    bool ok = true;
    for (size_t idx = 0; idx < G.nonterminals_list.size(); ++idx)
    {
        const string &A = G.nonterminals_list[idx];
        for (size_t p = 0; p < G.productions[A].size(); ++p)
        {
            auto alpha = G.productions[A][p];
            auto FIRST_alpha = FIRST_of_sequence(G, alpha);
            for (auto it = FIRST_alpha.begin(); it != FIRST_alpha.end(); ++it)
            {
                string a = *it;
                if (a == EPS)
                    continue;
                if (!G.table[A][a].empty() && G.table[A][a] != alpha)
                {
                    ok = false;
                    conflicts.push_back("Conflict at M[" + A + "," + a + "]");
                }

                G.table[A][a] = alpha;
            }

            if (FIRST_alpha.count(EPS))
            {
                for (auto it2 = G.FOLLOW[A].begin(); it2 != G.FOLLOW[A].end(); ++it2)
                {
                    string b = *it2;
                    if (!G.table[A][b].empty() && G.table[A][b] != alpha)
                    {
                        ok = false;
                        conflicts.push_back("Conflict at M[" + A + "," + b + "]");
                    }

                    G.table[A][b] = alpha;
                }
            }
        }
    }

    return ok;
}

template <typename T>
vector<T> set_to_sorted_vec(const unordered_set<T> &S)
{
    vector<T> v(S.begin(), S.end());
    sort(v.begin(), v.end());
    return v;
}

void print_sets(const Grammar &G)
{
    cout << "\nFIRST sets:\n";
    for (size_t i = 0; i < G.nonterminals_list.size(); ++i)
    {
        const string &A = G.nonterminals_list[i];
        auto v = set_to_sorted_vec(G.FIRST.at(A));
        cout << "FIRST(" << A << ") = { ";
        for (size_t j = 0; j < v.size(); ++j)
        {
            cout << v[j] << (j + 1 < v.size() ? ", " : "");
        }

        cout << " }\n";
    }

    cout << "\nFOLLOW sets:\n";
    for (size_t i = 0; i < G.nonterminals_list.size(); ++i)
    {
        const string &A = G.nonterminals_list[i];
        auto v = set_to_sorted_vec(G.FOLLOW.at(A));
        cout << "FOLLOW(" << A << ") = { ";

        for (size_t j = 0; j < v.size(); ++j)
        {
            cout << v[j] << (j + 1 < v.size() ? ", " : "");
        }

        cout << " }\n";
    }
}

void print_table(const Grammar &G)
{
    vector<string> terms(G.terminals.begin(), G.terminals.end());
    sort(terms.begin(), terms.end());
    cout << "\nLL(1) Parsing Table M[A, a]:\n";
    cout << setw(12) << " ";
    for (size_t i = 0; i < terms.size(); ++i)
        cout << setw(12) << terms[i];
    cout << "\n";
    for (size_t idx = 0; idx < G.nonterminals_list.size(); ++idx)
    {
        const string &A = G.nonterminals_list[idx];
        cout << setw(12) << A;
        for (size_t j = 0; j < terms.size(); ++j)
        {
            const string &a = terms[j];
            auto itA = G.table.find(A);
            if (itA != G.table.end())
            {
                auto itB = itA->second.find(a);
                if (itB != itA->second.end() && !itB->second.empty())
                {
                    string rhs;
                    for (size_t k = 0; k < itB->second.size(); ++k)
                        rhs += (rhs.empty() ? "" : " ") + itB->second[k];
                    cout << setw(12) << (A + "->" + (rhs.empty() ? EPS : rhs));
                }

                else
                {
                    cout << setw(12) << "-";
                }
            }

            else
                cout << setw(12) << "-";
        }

        cout << "\n";
    }
}

vector<string> lex_input(const string &s)
{
    vector<string> tokens;
    for (size_t i = 0; i < s.size();)
    {
        if (isspace((unsigned char)s[i]))
        {
            ++i;
            continue;
        }

        if (s.compare(i, 2, "id") == 0)
        {
            tokens.push_back("id");
            i += 2;
            continue;
        }

        char c = s[i];
        if (c == '+' || c == '*' || c == '(' || c == ')')
        {
            tokens.push_back(string(1, c));
            ++i;
            continue;
        }

        tokens.push_back(string(1, c));
        ++i;
    }

    return tokens;
}

string join_tokens(const vector<string> &v, size_t from = 0)
{
    string out;
    for (size_t i = from; i < v.size(); ++i)
    {
        out += v[i];
        if (i + 1 < v.size())
            out += " ";
    }

    return out;
}

string stack_to_string(const vector<string> &st)
{
    string s;
    for (size_t i = 0; i < st.size(); ++i)
    {
        s += st[i];
        if (i + 1 < st.size())
            s += " ";
    }

    return s;
}

struct ParseResult
{
    bool accepted;
    string errorMsg;
};
ParseResult predictive_parse(Grammar &G, const vector<string> &input_tokens)
{
    vector<string> stk;
    stk.push_back(END_MARKER);
    stk.push_back(G.start);
    vector<string> tokens = input_tokens;
    tokens.push_back(END_MARKER);
    size_t ip = 0;
    cout << "\nParsing Trace:\n";
    cout << left << setw(6) << "Step" << setw(35) << "Stack" << setw(30) << "Input" << "Action\n";
    cout << string(6 + 35 + 30 + 10, '-') << "\n";
    int step = 1;
    while (!stk.empty())
    {
        string X = stk.back();
        string a = tokens[ip];
        if (X == END_MARKER && a == END_MARKER)
        {
            cout << left << setw(6) << step++ << setw(35) << stack_to_string(stk) << setw(30) << join_tokens(tokens, ip) << "ACCEPT\n";
            return {true, ""};
        }

        if (!G.nonterminals.count(X))
        {
            if (X == a)
            {
                cout << left << setw(6) << step++ << setw(35) << stack_to_string(stk) << setw(30) << join_tokens(tokens, ip) << "match " + a << "\n";
                stk.pop_back();
                ++ip;
            }

            else
            {
                string msg = "ERROR: terminal mismatch. On stack: '" + X + "', lookahead: '" + a + "'";
                cout << left << setw(6) << step++ << setw(35) << stack_to_string(stk) << setw(30) << join_tokens(tokens, ip) << msg << "\n";
                return {false, msg};
            }
        }

        else
        {
            auto itA = G.table.find(X);
            if (itA == G.table.end() || itA->second.find(a) == itA->second.end() || itA->second.at(a).empty())
            {
                string msg = "ERROR: no rule for M[" + X + "," + a + "]";
                cout << left << setw(6) << step++ << setw(35) << stack_to_string(stk) << setw(30) << join_tokens(tokens, ip) << msg << "\n";
                return {false, msg};
            }

            auto rhs = itA->second[a];
            stk.pop_back();
            if (!(rhs.size() == 1 && rhs[0] == EPS))
            {
                for (int i = (int)rhs.size() - 1; i >= 0; --i)
                    stk.push_back(rhs[i]);
            }

            string rhs_str;
            for (size_t i = 0; i < rhs.size(); ++i)
                rhs_str += (i ? " " : "") + rhs[i];
            if (rhs_str.empty())
                rhs_str = EPS;
            cout << left << setw(6) << step++ << setw(35) << stack_to_string(stk) << setw(30) << join_tokens(tokens, ip) << "expand " + X + " -> " + rhs_str << "\n";
        }
    }

    string msg = "ERROR: stack emptied without acceptance.";
    return {false, msg};
}

void print_summary_and_table(Grammar &G, bool showTable = true)
{
    compute_FIRST(G);
    compute_FOLLOW(G);
    print_sets(G);
    vector<string> conflicts;
    bool ok = build_LL1_table(G, conflicts);
    if (!ok)
    {
        cout << "\nWARNING: Grammar is NOT LL(1) (conflicts found):\n";
        for (size_t i = 0; i < conflicts.size(); ++i)
            cout << "  - " << conflicts[i] << "\n";
    }

    else
    {
        cout << "\nGrammar appears LL(1): no table conflicts detected.\n";
    }

    if (showTable)
        print_table(G);
}

int main()
{
    ios::sync_with_stdio(false);
    cin.tie(NULL);
    Grammar G;
    // Hardcoded grammar for the given task
    G.start = "E";
    G.productions["E"] = {{"T", "E'"}};
    G.productions["E'"] = {{"+", "T", "E'"}, {EPS}};
    G.productions["T"] = {{"F", "T'"}};
    G.productions["T'"] = {{"*", "F", "T'"}, {EPS}};
    G.productions["F"] = {{"(", "E", ")"}, {"id"}};
    G.nonterminals = {"E", "E'", "T", "T'", "F"};
    G.nonterminals_list = {"E", "E'", "T", "T'", "F"};
    collect_terminals(G);
    // Print grammar summary
    print_summary_and_table(G, true);
    // Test cases
    vector<string> tests = {"id+id", "id+id*id"};
    for (const auto &test : tests)
    {
        cout << "\nParsing input: " << test << "\n";
        vector<string> tokens = lex_input(test);
        auto res = predictive_parse(G, tokens);
        if (!res.accepted)
        {
            cout << "\nResult: REJECTED\n"
                 << res.errorMsg << "\n";
        }

        else
        {
            cout << "\nResult: ACCEPTED\n";
        }

        cout << "\n";
    }

    return 0;
}
