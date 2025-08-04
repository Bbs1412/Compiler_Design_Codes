#include <bits/stdc++.h>
using namespace std;

// Node for syntax tree
struct TreeNode
{
    char value;
    TreeNode *left;
    TreeNode *right;
    bool isNullable;
    set<int> startPos;
    set<int> endPos;
    int position;
    int id;

    TreeNode(char ch)
        : value(ch), left(nullptr), right(nullptr), isNullable(false),
          position(0), id(0) {}
};

// Global trackers
static int posCounter = 0;
static int idCounter = 0;
static map<int, set<int>> nextPositions;
static map<int, char> positionSymbol;
static vector<TreeNode *> nodesPool;

// Merge two sets
static set<int> mergeSets(const set<int> &a, const set<int> &b)
{
    set<int> result(a);
    result.insert(b.begin(), b.end());
    return result;
}

// Build syntax tree from postfix regex
TreeNode *constructSyntaxTree(const string &postfix)
{
    stack<TreeNode *> st;
    for (char ch : postfix)
    {
        if (ch == '*' || ch == '|' || ch == '.')
        {
            TreeNode *node = new TreeNode(ch);
            node->id = ++idCounter;
            nodesPool.push_back(node);
            if (ch == '*')
            {
                node->left = st.top();
                st.pop();
            }
            else
            {
                node->right = st.top();
                st.pop();
                node->left = st.top();
                st.pop();
            }
            st.push(node);
        }
        else
        {
            TreeNode *leaf = new TreeNode(ch);
            leaf->id = ++idCounter;
            leaf->position = ++posCounter;
            positionSymbol[leaf->position] = ch;
            nodesPool.push_back(leaf);
            st.push(leaf);
        }
    }
    return st.top();
}

// Compute nullable, firstPos, lastPos
void analyzeTree(TreeNode *root)
{
    if (!root)
        return;
    if (!root->left && !root->right)
    {
        root->isNullable = false;
        root->startPos = {root->position};
        root->endPos = {root->position};
        return;
    }
    if (root->value == '|')
    {
        analyzeTree(root->left);
        analyzeTree(root->right);
        root->isNullable = root->left->isNullable || root->right->isNullable;
        root->startPos = mergeSets(root->left->startPos, root->right->startPos);
        root->endPos = mergeSets(root->left->endPos, root->right->endPos);
    }
    else if (root->value == '.')
    {
        analyzeTree(root->left);
        analyzeTree(root->right);
        root->isNullable = root->left->isNullable && root->right->isNullable;
        root->startPos = root->left->isNullable ? mergeSets(root->left->startPos, root->right->startPos) : root->left->startPos;
        root->endPos = root->right->isNullable ? mergeSets(root->right->endPos, root->left->endPos) : root->right->endPos;
    }
    else if (root->value == '*')
    {
        analyzeTree(root->left);
        root->isNullable = true;
        root->startPos = root->left->startPos;
        root->endPos = root->left->endPos;
    }
}

// Populate followpos (nextPositions)
void buildNextPositions(TreeNode *root)
{
    if (!root)
        return;
    buildNextPositions(root->left);
    buildNextPositions(root->right);
    if (root->value == '.')
    {
        for (int p : root->left->endPos)
        {
            nextPositions[p] = mergeSets(nextPositions[p], root->right->startPos);
        }
    }
    else if (root->value == '*')
    {
        for (int p : root->endPos)
        {
            nextPositions[p] = mergeSets(nextPositions[p], root->startPos);
        }
    }
}

// Operator precedence for infix to postfix
int opPrec(char c)
{
    if (c == '*')
        return 3;
    if (c == '.')
        return 2;
    if (c == '|')
        return 1;
    return 0;
}

// Insert explicit '.' for concatenation
string insertConcatOperators(const string &expr)
{
    string out;
    for (size_t i = 0; i < expr.size(); ++i)
    {
        char a = expr[i];
        out += a;
        if (a == '(' || a == '|')
            continue;
        if (i + 1 < expr.size())
        {
            char b = expr[i + 1];
            if (b == '*' || b == '|' || b == ')')
                continue;
            out += '.';
        }
    }
    return out;
}

// Convert regex from infix to postfix
string infixToPostfix(const string &expr)
{
    string res;
    stack<char> stk;
    for (char c : expr)
    {
        if (isalnum(c) || c == '#')
        {
            res += c;
        }
        else if (c == '(')
        {
            stk.push(c);
        }
        else if (c == ')')
        {
            while (!stk.empty() && stk.top() != '(')
            {
                res += stk.top();
                stk.pop();
            }
            if (!stk.empty())
                stk.pop();
        }
        else
        {
            while (!stk.empty() && opPrec(stk.top()) >= opPrec(c))
            {
                res += stk.top();
                stk.pop();
            }
            stk.push(c);
        }
    }
    while (!stk.empty())
    {
        res += stk.top();
        stk.pop();
    }
    return res;
}

// Stringify set
string showSet(const set<int> &s)
{
    if (s.empty())
        return "{}";
    stringstream ss;
    ss << "{ ";
    for (int x : s)
        ss << x << ' ';
    ss << "}";
    return ss.str();
}

// Display node details
void displayNodeDetails(TreeNode *node, int lvl = 0)
{
    if (!node)
        return;
    string pad(lvl * 2, ' ');
    cout << pad << "Node " << node->id << " (" << node->value << ")";
    if (node->position)
        cout << " [pos=" << node->position << "]";
    cout << ":\n";
    cout << pad << " nullable: " << (node->isNullable ? "true" : "false") << "\n";
    cout << pad << " firstpos: " << showSet(node->startPos) << "\n";
    cout << pad << " lastpos: " << showSet(node->endPos) << "\n";
    displayNodeDetails(node->left, lvl + 1);
    displayNodeDetails(node->right, lvl + 1);
}

// DFA representation
struct DFAState
{
    vector<set<int>> sets;
    map<pair<int, char>, int> transitions;
    map<int, bool> isAccept;
    int start;
};

// Construct DFA
DFAState buildDFA(TreeNode *root)
{
    DFAState dfa;
    vector<set<int>> unmarked;
    map<set<int>, int> idMap;

    unmarked.push_back(root->startPos);
    idMap[root->startPos] = 0;
    dfa.start = 0;

    size_t idx = 0;
    while (idx < unmarked.size())
    {
        auto S = unmarked[idx];
        map<char, set<int>> moves;
        for (int p : S)
        {
            char sym = positionSymbol[p];
            if (sym == '#')
                continue;
            moves[sym] = mergeSets(moves[sym], nextPositions[p]);
        }
        for (auto &pr : moves)
        {
            char sym = pr.first;
            auto &tgt = pr.second;
            if (tgt.empty())
                continue;
            if (!idMap.count(tgt))
            {
                idMap[tgt] = unmarked.size();
                unmarked.push_back(tgt);
            }
            dfa.transitions[{(int)idx, sym}] = idMap[tgt];
        }
        ++idx;
    }

    int hashPos = -1;
    for (auto &pr : positionSymbol)
    {
        if (pr.second == '#')
            hashPos = pr.first;
    }
    for (int i = 0; i < (int)unmarked.size(); ++i)
    {
        dfa.isAccept[i] = (unmarked[i].count(hashPos) > 0);
    }
    dfa.sets = unmarked;
    return dfa;
}

// Clean up memory
void releaseAll()
{
    for (auto ptr : nodesPool)
        delete ptr;
    nodesPool.clear();
}

// Run a test scenario
void runTest(const string &name, const string &expr)
{
    cout << "\n"
         << string(60, '=') << "\n";
    cout << "TEST CASE: " << name << "\n";
    cout << "Regular Expression: " << expr << "\n";
    cout << string(60, '=') << "\n\n";

    posCounter = 0;
    idCounter = 0;
    positionSymbol.clear();
    nextPositions.clear();
    releaseAll();

    string withConcat = insertConcatOperators(expr);
    cout << "Regex with explicit concatenation: " << withConcat << "\n";
    string post = infixToPostfix(withConcat);
    cout << "Postfix regex: " << post << "\n\n";

    TreeNode *root = constructSyntaxTree(post);
    analyzeTree(root);
    buildNextPositions(root);

    cout << "FIRSTPOS AND LASTPOS FOR ALL NODES:\n";
    cout << string(40, '-') << "\n";
    displayNodeDetails(root);

    cout << "\nFOLLOWPOS TABLE:\n";
    cout << string(40, '-') << "\n";
    for (int i = 1; i <= posCounter; ++i)
    {
        cout << "Position " << i << " (" << positionSymbol[i] << ") : "
             << showSet(nextPositions[i]) << "\n";
    }

    auto dfa = buildDFA(root);
    cout << "\nDFA STATES AND TRANSITIONS:\n";
    cout << string(40, '-') << "\n";
    for (int i = 0; i < (int)dfa.sets.size(); ++i)
    {
        cout << "State " << char('A' + i) << " : "
             << showSet(dfa.sets[i]);
        if (dfa.isAccept[i])
            cout << " [Accepting]";
        if (i == dfa.start)
            cout << " [Start]";
        cout << "\n";
    }

    cout << "\nTRANSITION TABLE:\n";
    cout << string(40, '-') << "\n";
    cout << "State\t|\ta\t|\tb\n";
    cout << string(40, '-') << "\n";
    for (int i = 0; i < (int)dfa.sets.size(); ++i)
    {
        cout << char('A' + i) << "\t|\t";
        auto itA = dfa.transitions.find({i, 'a'});
        cout << (itA != dfa.transitions.end() ? char('A' + itA->second) : '-') << "\t|\t";
        auto itB = dfa.transitions.find({i, 'b'});
        cout << (itB != dfa.transitions.end() ? char('A' + itB->second) : '-') << "\n";
    }
    cout << "\n";
}

int main()
{
    cout << "DFA CONSTRUCTION FROM REGULAR EXPRESSION\n";
    cout << "Using Syntax-Tree-Based Method\n";
    cout << string(60, '=') << "\n";

    runTest("Test Case 1", "(a|b)*abb#");
    runTest("Test Case 2", "a*b*a(a|b)*b*a#");

    cout << "\n"
         << string(60, '=') << "\n";
    cout << "ALL TEST CASES COMPLETED\n";
    cout << string(60, '=') << "\n";
    return 0;
}
