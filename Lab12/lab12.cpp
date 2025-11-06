#include <bits/stdc++.h>
using namespace std;

// TAC Instruction
struct Instruction
{
    string op, arg1, arg2, result;
    bool isLabel = false;
    string label;
};

// Emit TAC
struct Emitter
{
    int t = 0, L = 0;
    vector<Instruction> code;
    string newTemp() { return "t" + to_string(++t); }
    string newLabel() { return "L" + to_string(++L); }
    void emit(string op, string a1 = "-", string a2 = "-", string res = "-")
    {
        code.push_back({op, a1, a2, res, false, ""});
    }
    void emitLabel(string label)
    {
        code.push_back({"", "", "", "", true, label});
    }
    void emitSwap(string a, string b)
    {
        string t = newTemp();
        emit("=", a, "-", t);
        emit("=", b, "-", a);
        emit("=", t, "-", b);
    }
};

// PRINT TAC
void printTAC(const vector<Instruction> &code)
{
    for (auto &i : code)
    {
        if (i.isLabel)
        {
            cout << i.label << ":\n";
            continue;
        }
        if (i.op == "=")
            cout << i.result << " = " << i.arg1 << "\n";
        else if (i.op == "if")
            cout << "if " << i.arg1 << " goto " << i.result << "\n";
        else if (i.op == "goto")
            cout << "goto " << i.result << "\n";
        else
            cout << i.result << " = " << i.arg1 << " " << i.op << " " << i.arg2 << "\n";
    }
}

// GENERATE PARTITION TAC
void genPartition(Emitter &e)
{
    e.emitLabel("partition");
    string pivot = e.newTemp();
    e.emit("=", "A[r]", "-", pivot);
    string i = e.newTemp();
    string t1 = e.newTemp();
    e.emit("-", "l", "1", t1);
    e.emit("=", t1, "-", i);
    string j = e.newTemp();
    e.emit("=", "l", "-", j);
    string LOOP = e.newLabel();
    string BODY = e.newLabel();
    string END = e.newLabel();
    e.emitLabel(LOOP);
    string cond = e.newTemp();
    e.emit("<=", j, "r-1", cond);
    e.emit("if", cond, "-", BODY);
    e.emit("goto", "-", "-", END);
    e.emitLabel(BODY);
    string cmp = e.newTemp();
    e.emit("<=", "A[" + j + "]", pivot, cmp);
    string THEN = e.newLabel();
    string AFTER = e.newLabel();
    e.emit("if", cmp, "-", THEN);
    e.emit("goto", "-", "-", AFTER);
    e.emitLabel(THEN);
    string t2 = e.newTemp();
    e.emit("+", i, "1", t2);
    e.emit("=", t2, "-", i);
    e.emitSwap("A[" + i + "]", "A[" + j + "]");
    e.emitLabel(AFTER);
    string t3 = e.newTemp();
    e.emit("+", j, "1", t3);
    e.emit("=", t3, "-", j);
    e.emit("goto", "-", "-", LOOP);
    e.emitLabel(END);
    string ip1 = e.newTemp();
    e.emit("+", i, "1", ip1);
    e.emitSwap("A[" + ip1 + "]", "A[r]");
    e.emit("ret", ip1);
}

// GENERATE QUICKSORT TAC
void genQuickSort(Emitter &e)
{
    e.emitLabel("quicksort");
    string cond = e.newTemp();
    e.emit("<", "l", "r", cond);
    string THEN = e.newLabel();
    string END = e.newLabel();
    e.emit("if", cond, "-", THEN);
    e.emit("goto", "-", "-", END);
    e.emitLabel(THEN);
    e.emit("call", "partition(A,l,r)", "-", "p");
    e.emit("call", "quicksort(A,l,p-1)");
    e.emit("call", "quicksort(A,p+1,r)");
    e.emitLabel(END);
    e.emit("ret");
}

// OPTIMIZATION
void constantFold(vector<Instruction> &code)
{
    for (auto &i : code)
    {
        if ((i.op == "+" || i.op == "-") && isdigit(i.arg1[0]) && isdigit(i.arg2[0]))
        {
            int a = stoi(i.arg1), b = stoi(i.arg2);
            int r = (i.op == "+") ? a + b : a - b;
            i.op = "=";
            i.arg1 = to_string(r);
            i.arg2 = "-";
        }
    }
}

void deadCode(vector<Instruction> &code)
{
    set<string> used;
    for (int i = code.size() - 1; i >= 0; i--)
    {
        auto &x = code[i];
        if (x.isLabel)
            continue;
        used.insert(x.arg1);
        used.insert(x.arg2);
        if (x.result[0] == 't' && !used.count(x.result))
            code.erase(code.begin() + i);
    }
}

void optimize(vector<Instruction> &c)
{
    constantFold(c);
    deadCode(c);
}

// REAL QUICKSORT FOR ARRAY OUTPUT
int partition(int A[], int l, int r)
{
    int pivot = A[r], i = l - 1;
    for (int j = l; j < r; j++)
    {
        if (A[j] <= pivot)
        {
            i++;
            swap(A[i], A[j]);
        }
    }
    swap(A[i + 1], A[r]);
    return i + 1;
}

void quickSort(int A[], int l, int r)
{
    if (l < r)
    {
        int p = partition(A, l, r);
        quickSort(A, l, p - 1);
        quickSort(A, p + 1, r);
    }
}

// MAIN
int main()
{
    Emitter e;
    genPartition(e);
    genQuickSort(e);

    cout << "\n TAC BEFORE OPTIMIZATION\n";
    printTAC(e.code);
    vector<Instruction> opt = e.code;
    optimize(opt);

    cout << "\n TAC AFTER OPTIMIZATION\n";
    printTAC(opt);
    // REAL ARRAY EXECUTION
    int A[] = {10, 7, 8, 9, 1, 5};
    int n = 6;

    cout << "\nOriginal Array: ";
    for (int x : A)
        cout << x << " ";
    quickSort(A, 0, n - 1);

    cout << "\nSorted Array: ";
    for (int x : A)
        cout << x << " ";
    cout << endl;
    return 0;
}
