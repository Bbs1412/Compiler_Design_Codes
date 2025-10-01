#include <iostream>
#include <string>
#include <vector>
#include <map>
using namespace std;

struct Instruction
{
    string op;
    string arg1;
    string arg2;
    string result;        // for jumps: label target; for assign: destination
    bool isLabel = false; // true if this is a label declaration
    string labelName;     // valid when isLabel == true
};

struct Emitter
{
    int tempCounter = 0;
    int labelCounter = 0;
    vector<Instruction> code;
    map<string, int> tempDefinedAtTripleIndex; // temp -> triple index (assigned during triple rendering)

    string newTemp() { return "t" + to_string(++tempCounter); }
    string newLabel() { return "L" + to_string(labelCounter++); }

    void emitBinary(const string &op, const string &a, const string &b, const string &t)
    {
        code.push_back({op, a, b, t, false, ""});
    }
    void emitAssign(const string &src, const string &dest)
    {
        code.push_back({"=", src, "-", dest, false, ""});
    }
    void emitIfGoto(const string &cond, const string &label)
    {
        code.push_back({"if", cond, "-", label, false, ""});
    }
    void emitGoto(const string &label)
    {
        code.push_back({"goto", "-", "-", label, false, ""});
    }
    void emitLabel(const string &label)
    {
        Instruction ins;
        ins.isLabel = true;
        ins.labelName = label;
        code.push_back(ins);
    }
};

// ---------- Generators for the three test cases ----------

// (A+B)*(C-D)/(E+F)
void generateArithmetic(Emitter &e)
{
    string t1 = e.newTemp();
    e.emitBinary("+", "A", "B", t1);
    string t2 = e.newTemp();
    e.emitBinary("-", "C", "D", t2);
    string t3 = e.newTemp();
    e.emitBinary("*", t1, t2, t3);
    string t4 = e.newTemp();
    e.emitBinary("+", "E", "F", t4);
    string t5 = e.newTemp();
    e.emitBinary("/", t3, t4, t5);
    e.emitAssign(t5, "result");
}

// if ((a<b) and (c!=d)) then x=1 else x=0
void generateBooleanIf(Emitter &e)
{
    string L0 = e.newLabel(); // else
    string L1 = e.newLabel(); // after first cond true
    string L2 = e.newLabel(); // then branch
    string L3 = e.newLabel(); // end

    string t1 = e.newTemp();
    e.emitBinary("<", "a", "b", t1);
    e.emitIfGoto(t1, L1);
    e.emitGoto(L0);
    e.emitLabel(L1);
    string t2 = e.newTemp();
    e.emitBinary("!=", "c", "d", t2);
    e.emitIfGoto(t2, L2);
    e.emitGoto(L0);
    e.emitLabel(L2);
    e.emitAssign("1", "x");
    e.emitGoto(L3);
    e.emitLabel(L0);
    e.emitAssign("0", "x");
    e.emitLabel(L3);
}

// while (i < n) { sum = sum + i; i = i + 1; }
void generateWhile(Emitter &e)
{
    string L0 = e.newLabel(); // start / condition
    string L1 = e.newLabel(); // body
    string L2 = e.newLabel(); // end

    e.emitLabel(L0);
    string t1 = e.newTemp();
    e.emitBinary("<", "i", "n", t1);
    e.emitIfGoto(t1, L1);
    e.emitGoto(L2);
    e.emitLabel(L1);
    string t2 = e.newTemp();
    e.emitBinary("+", "sum", "i", t2);
    e.emitAssign(t2, "sum");
    string t3 = e.newTemp();
    e.emitBinary("+", "i", "1", t3);
    e.emitAssign(t3, "i");
    e.emitGoto(L0);
    e.emitLabel(L2);
}

// ---------- Rendering utilities ----------

static void printStepByStep(const vector<Instruction> &code)
{
    for (const auto &ins : code)
    {
        if (ins.isLabel)
        {
            cout << ins.labelName << ": ";
            cout << "; label\n";
            continue;
        }
        if (ins.op == "=")
        {
            cout << ins.result << " = " << ins.arg1 << "\n";
        }
        else if (ins.op == "if")
        {
            cout << "if " << ins.arg1 << " goto " << ins.result << "\n";
        }
        else if (ins.op == "goto")
        {
            cout << "goto " << ins.result << "\n";
        }
        else
        {
            cout << ins.result << " = " << ins.arg1 << " " << ins.op << " " << ins.arg2 << "\n";
        }
    }
}

static void printQuadruples(const vector<Instruction> &code)
{
    for (const auto &ins : code)
    {
        if (ins.isLabel)
        {
            cout << "(label, -, -, " << ins.labelName << ")\n";
            continue;
        }
        if (ins.op == "=")
        {
            cout << "(=, " << ins.arg1 << ", -, " << ins.result << ")\n";
        }
        else if (ins.op == "if")
        {
            cout << "(if, " << ins.arg1 << ", -, " << ins.result << ")\n";
        }
        else if (ins.op == "goto")
        {
            cout << "(goto, -, -, " << ins.result << ")\n";
        }
        else
        {
            cout << "(" << ins.op << ", " << ins.arg1 << ", " << ins.arg2 << ", " << ins.result << ")\n";
        }
    }
}

static void printTriples(const vector<Instruction> &code)
{
    // Assign triple indices to non-label instructions
    map<int, int> instToTriple; // instruction index -> triple idx
    int idx = 0;
    for (size_t i = 0; i < code.size(); ++i)
    {
        if (!code[i].isLabel)
            instToTriple[(int)i] = idx++;
    }

    // Map temps to where they are defined (their triple index)
    map<string, int> tempDef;
    for (size_t i = 0; i < code.size(); ++i)
    {
        const auto &ins = code[i];
        if (!ins.isLabel && !ins.result.empty())
        {
            if (ins.op != "if" && ins.op != "goto" && ins.op != "=")
            {
                tempDef[ins.result] = instToTriple[(int)i];
            }
        }
    }

    auto ref = [&](const string &s) -> string
    {
        auto it = tempDef.find(s);
        if (it != tempDef.end())
            return string("r") + to_string(it->second);
        return s;
    };

    for (size_t i = 0; i < code.size(); ++i)
    {
        const auto &ins = code[i];
        if (ins.isLabel)
        {
            cout << ins.labelName << ":\n";
            continue;
        }
        int t = instToTriple[(int)i];
        if (ins.op == "=")
        {
            cout << t << ": (=, " << ref(ins.arg1) << ", " << ins.result << ")\n";
        }
        else if (ins.op == "if")
        {
            cout << t << ": (if, " << ref(ins.arg1) << ", " << ins.result << ")\n";
        }
        else if (ins.op == "goto")
        {
            cout << t << ": (goto, " << ins.result << ", -)\n";
        }
        else
        {
            cout << t << ": (" << ins.op << ", " << ref(ins.arg1) << ", " << ref(ins.arg2) << ")\n";
        }
    }
}

int main()
{
    // 1) Arithmetic Expression
    {
        Emitter e;
        cout << "[Test 1] Arithmetic Expression: (A + B) * (C - D) / (E + F)\n\n";
        generateArithmetic(e);

        cout << "Step-by-step temporaries (TAC):\n";
        printStepByStep(e.code);
        cout << "\n";

        cout << "Quadruples (op, arg1, arg2, result):\n";
        printQuadruples(e.code);
        cout << "\n";

        cout << "Triples [index: (op, arg1, arg2)]\n";
        printTriples(e.code);
        cout << "\n\n";
    }

    // 2) Boolean If-Else
    {
        Emitter e;
        cout << "[Test 2] Boolean Expression: if ((a < b) and (c != d)) then x = 1 else x = 0\n\n";
        generateBooleanIf(e);

        cout << "Step-by-step temporaries and control flow (TAC):\n";
        printStepByStep(e.code);
        cout << "\n";

        cout << "Quadruples (op, arg1, arg2, result/target):\n";
        printQuadruples(e.code);
        cout << "\n";

        cout << "Triples [index: (op, arg1, arg2)] with labels:\n";
        printTriples(e.code);
        cout << "\n\n";
    }

    // 3) While Loop
    {
        Emitter e;
        cout << "[Test 3] Loop: while (i < n) { sum = sum + i; i = i + 1; }\n\n";
        generateWhile(e);

        cout << "Step-by-step temporaries and control flow (TAC):\n";
        printStepByStep(e.code);
        cout << "\n";

        cout << "Quadruples (op, arg1, arg2, result/target):\n";
        printQuadruples(e.code);
        cout << "\n";

        cout << "Triples [index: (op, arg1, arg2)] with labels:\n";
        printTriples(e.code);
        cout << "\n";
    }
    return 0;
}
