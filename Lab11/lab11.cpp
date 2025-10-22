#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
using namespace std;

// Structure to represent a three-address instruction
struct Instruction
{
    int address;
    string op;
    string arg1;
    string arg2;
    string result;

    Instruction(int addr, string operation, string a1, string a2, string res)
        : address(addr), op(operation), arg1(a1), arg2(a2), result(res) {}
};

// Structure to represent a list of instruction addresses
struct List
{
    vector<int> addresses;

    List() {}
    List(int addr) { addresses.push_back(addr); }

    void add(int addr) { addresses.push_back(addr); }
    bool empty() const { return addresses.empty(); }
    int size() const { return addresses.size(); }
};

// Global variables
vector<Instruction> instructions;
int nextInstr = 1;

// Function to create a new list containing a single instruction address
List makelist(int address)
{
    return List(address);
}

// Function to merge two lists
List merge(List list1, List list2)
{
    List result;
    for (int addr : list1.addresses)
    {
        result.add(addr);
    }
    for (int addr : list2.addresses)
    {
        result.add(addr);
    }
    return result;
}

// Function to backpatch a list of instructions with a target address
void backpatch(List list, int target)
{
    for (int addr : list.addresses)
    {
        if (addr < instructions.size())
        {
            instructions[addr - 1].result = to_string(target);
        }
    }
}

// Function to emit a three-address instruction
int emit(string op, string arg1, string arg2, string result)
{
    instructions.push_back(Instruction(nextInstr, op, arg1, arg2, result));
    return nextInstr++;
}

// Function to print all instructions in quadruple format
void printInstructionsQuadruple()
{
    cout << "\nThree-Address Code Instructions (Quadruple Format):\n";
    cout << "---------------------------------------------------\n";
    cout << setw(3) << "No." << setw(8) << "Op" << setw(8) << "Arg1" << setw(8) << "Arg2" << setw(8) << "Result" << endl;
    cout << "---------------------------------------------------\n";

    for (const auto &instr : instructions)
    {
        cout << setw(3) << instr.address << setw(8) << instr.op
             << setw(8) << instr.arg1 << setw(8) << instr.arg2
             << setw(8) << instr.result << endl;
    }
}

// Function to print all instructions in if-goto format
void printInstructionsIfGoto(int true_exit, int false_exit)
{
    cout << "\nThree-Address Code Instructions (If-Goto Format):\n";
    cout << "------------------------------------------------\n";

    for (const auto &instr : instructions)
    {
        cout << setw(3) << instr.address << ": ";

        if (instr.op == "j<")
        {
            cout << "if " << instr.arg1 << " < " << instr.arg2 << " goto " << instr.result;
        }
        else if (instr.op == "j==")
        {
            cout << "if " << instr.arg1 << " == " << instr.arg2 << " goto " << instr.result;
        }
        else if (instr.op == "j>")
        {
            cout << "if " << instr.arg1 << " > " << instr.arg2 << " goto " << instr.result;
        }
        else if (instr.op == "j")
        {
            if (instr.arg1.empty() && instr.arg2.empty())
            {
                if (instr.result == "0")
                {
                    // Check if this is the true or false exit
                    if (instr.address == true_exit)
                    {
                        cout << "true";
                    }
                    else if (instr.address == false_exit)
                    {
                        cout << "false";
                    }
                    else
                    {
                        cout << "goto " << instr.result;
                    }
                }
                else
                {
                    cout << "goto " << instr.result;
                }
            }
            else
            {
                cout << "goto " << instr.result;
            }
        }
        cout << endl;
    }
}

// Function to print a list
void printList(const List &list, const string &name)
{
    cout << name << ": ";
    if (list.empty())
    {
        cout << "empty";
    }
    else
    {
        for (int i = 0; i < list.addresses.size(); i++)
        {
            cout << list.addresses[i];
            if (i < list.addresses.size() - 1)
                cout << ", ";
        }
    }
    cout << endl;
}

int main()
{
    cout << "Three-Address Code Generation with Backpatching\n";
    cout << "===============================================\n";
    cout << "Expression: ((a<b) or (a==b)) and (c>d)\n\n";

    // Step-by-step generation of three-address code
    cout << "Step-by-step generation:\n";
    cout << "------------------------\n";

    cout << "\n=== STEP 1: Generating code for (a<b) ===\n";
    cout << "Using makelist() to create lists:\n";

    // Generate code for (a<b)
    cout << "\n1. Generating code for (a<b):\n";
    int instr1 = emit("j<", "a", "b", "0"); // Jump if a<b is true
    int instr2 = emit("j", "", "", "0");    // Jump if a<b is false

    cout << "   Instruction " << instr1 << ": j< a, b, 0 (jump if a<b is true)\n";
    cout << "   Instruction " << instr2 << ": j _, _, 0 (jump if a<b is false)\n";

    cout << "\n   makelist(" << instr1 << ") creates true list:\n";
    List true1 = makelist(instr1);
    printList(true1, "   True list");

    cout << "\n   makelist(" << instr2 << ") creates false list:\n";
    List false1 = makelist(instr2);
    printList(false1, "   False list");

    // Generate code for (a==b)
    cout << "\n=== STEP 2: Generating code for (a==b) ===\n";
    cout << "\n2. Generating code for (a==b):\n";
    int instr3 = emit("j==", "a", "b", "0"); // Jump if a==b is true
    int instr4 = emit("j", "", "", "0");     // Jump if a==b is false

    cout << "   Instruction " << instr3 << ": j== a, b, 0 (jump if a==b is true)\n";
    cout << "   Instruction " << instr4 << ": j _, _, 0 (jump if a==b is false)\n";

    cout << "\n   makelist(" << instr3 << ") creates true list:\n";
    List true2 = makelist(instr3);
    printList(true2, "   True list");

    cout << "\n   makelist(" << instr4 << ") creates false list:\n";
    List false2 = makelist(instr4);
    printList(false2, "   False list");

    // Handle OR operation: (a<b) or (a==b)
    cout << "\n=== STEP 3: Handling OR operation using backpatch() and merge() ===\n";
    cout << "\n3. Handling OR operation: (a<b) or (a==b)\n";

    cout << "\n   backpatch(false1, " << instr3 << ") - filling false list of (a<b) with target " << instr3 << ":\n";
    cout << "   Before backpatch: ";
    printList(false1, "");
    backpatch(false1, instr3); // Backpatch false list of (a<b) to start of (a==b)
    cout << "   After backpatch: false list of (a<b) now points to instruction " << instr3 << "\n";

    cout << "\n   merge(true1, true2) - combining true lists of both operands:\n";
    cout << "   true1: ";
    printList(true1, "");
    cout << "   true2: ";
    printList(true2, "");
    List true_or = merge(true1, true2); // Merge true lists
    cout << "   Result after merge: ";
    printList(true_or, "");

    List false_or = false2; // False list of OR is false list of second operand
    cout << "\n   False list after OR: ";
    printList(false_or, "");

    // Generate code for (c>d)
    cout << "\n=== STEP 4: Generating code for (c>d) ===\n";
    cout << "\n4. Generating code for (c>d):\n";
    int instr5 = emit("j>", "c", "d", "0"); // Jump if c>d is true
    int instr6 = emit("j", "", "", "0");    // Jump if c>d is false

    cout << "   Instruction " << instr5 << ": j> c, d, 0 (jump if c>d is true)\n";
    cout << "   Instruction " << instr6 << ": j _, _, 0 (jump if c>d is false)\n";

    cout << "\n   makelist(" << instr5 << ") creates true list:\n";
    List true3 = makelist(instr5);
    printList(true3, "   True list");

    cout << "\n   makelist(" << instr6 << ") creates false list:\n";
    List false3 = makelist(instr6);
    printList(false3, "   False list");

    // Handle AND operation: ((a<b) or (a==b)) and (c>d)
    cout << "\n=== STEP 5: Handling AND operation using backpatch() and merge() ===\n";
    cout << "\n5. Handling AND operation: ((a<b) or (a==b)) and (c>d)\n";

    cout << "\n   backpatch(true_or, " << instr5 << ") - filling true list of OR with target " << instr5 << ":\n";
    cout << "   Before backpatch: ";
    printList(true_or, "");
    backpatch(true_or, instr5); // Backpatch true list of OR to start of (c>d)
    cout << "   After backpatch: true list of OR now points to instruction " << instr5 << "\n";

    cout << "\n   merge(false_or, false3) - combining false lists of both operands:\n";
    cout << "   false_or: ";
    printList(false_or, "");
    cout << "   false3: ";
    printList(false3, "");
    List false_and = merge(false_or, false3); // Merge false lists
    cout << "   Result after merge: ";
    printList(false_and, "");

    List true_and = true3; // True list of AND is true list of second operand
    cout << "\n   True list after AND: ";
    printList(true_and, "");

    // Final backpatching
    cout << "\n=== STEP 6: Final backpatching ===\n";
    cout << "\n6. Final backpatching:\n";
    int true_exit = nextInstr;
    int false_exit = nextInstr + 1;
    emit("j", "", "", "0"); // True exit
    emit("j", "", "", "0"); // False exit

    cout << "\n   backpatch(true_and, " << true_exit << ") - filling true list with final true exit:\n";
    cout << "   Before backpatch: ";
    printList(true_and, "");
    backpatch(true_and, true_exit);
    cout << "   After backpatch: true list now points to instruction " << true_exit << " (true exit)\n";

    cout << "\n   backpatch(false_and, " << false_exit << ") - filling false list with final false exit:\n";
    cout << "   Before backpatch: ";
    printList(false_and, "");
    backpatch(false_and, false_exit);
    cout << "   After backpatch: false list now points to instruction " << false_exit << " (false exit)\n";

    // Print final instructions in both formats
    // printInstructionsQuadruple();
    printInstructionsIfGoto(true_exit, false_exit);

    // Print final control flow
    cout << "\nFinal Control Flow:\n";
    cout << "===================\n";
    cout << "True list: ";
    for (int addr : true_and.addresses)
    {
        cout << addr << " ";
    }
    cout << "-> " << true_exit << " (exit true)\n";

    cout << "False list: ";
    for (int addr : false_and.addresses)
    {
        cout << addr << " ";
    }
    cout << "-> " << false_exit << " (exit false)\n";

    cout << "\nControl Flow Explanation:\n";
    cout << "========================\n";
    cout << "1. Instructions " << instr1 << " and " << instr3 << " jump to true exit if conditions are met\n";
    cout << "2. Instructions " << instr2 << " and " << instr4 << " jump to next condition or false exit\n";
    cout << "3. Instruction " << instr5 << " jumps to true exit if c>d is true\n";
    cout << "4. Instruction " << instr6 << " jumps to false exit if c>d is false\n";
    cout << "5. Instructions " << true_exit << " and " << false_exit << " are the final exit points\n";

    return 0;
}
