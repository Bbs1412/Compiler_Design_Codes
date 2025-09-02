// Expected to be a `Operator Precedence Parser` implementation in C++

#include <iostream>
#include <vector>
#include <string>
#include <stack>
#include <map>
#include <set>
#include <iomanip>
#include <algorithm>
using namespace std;

const int SYMBOL_WIDTH = 8;
const int STACK_WIDTH = 20;
const int INPUT_WIDTH = 15;
const int ACTION_WIDTH = 10;
const int RELATION_WIDTH = 5;
const int TABLE_CELL_WIDTH = 6;

class OperatorPrecedenceParser
{
private:
    vector<string> grammar;
    map<char, set<char>> leading;
    map<char, set<char>> trailing;
    map<pair<char, char>, char> precedenceTable;
    vector<char> terminals;
    vector<char> operators;

public:
    OperatorPrecedenceParser()
    {
        // Initialize grammar rules
        grammar = {
            "E → E + E",
            "E → E - E",
            "E → E * E",
            "E → E / E",
            "E → ( E )",
            "E → id"};

        // Initialize terminals and operators
        terminals = {'i', '+', '-', '*', '/', '(', ')', '$'}; // 'i' represents 'id'
        operators = {'+', '-', '*', '/', '(', ')', '$'};

        computeLeadingTrailing();
        buildPrecedenceTable();
    }

    void computeLeadingTrailing()
    {
        cout << "\n"
             << string(60, '=') << endl;
        cout << "COMPUTING LEADING AND TRAILING SETS" << endl;
        cout << string(60, '=') << endl;

        // For this grammar, E is the only non-terminal
        // LEADING(E) = {i, (} - terminals that can appear at the beginning
        leading['E'] = {'i', '('};

        // TRAILING(E) = {i, )} - terminals that can appear at the end
        trailing['E'] = {'i', ')'};

        cout << "LEADING(E) = { ";
        for (char c : leading['E'])
        {
            cout << (c == 'i' ? "id" : string(1, c)) << " ";
        }
        cout << "}" << endl;

        cout << "TRAILING(E) = { ";
        for (char c : trailing['E'])
        {
            cout << (c == 'i' ? "id" : string(1, c)) << " ";
        }
        cout << "}" << endl;
    }

    void buildPrecedenceTable()
    {
        cout << "\n"
             << string(60, '=') << endl;
        cout << "BUILDING OPERATOR PRECEDENCE TABLE" << endl;
        cout << string(60, '=') << endl;

        // Initialize all relations as undefined (' ')
        for (char a : operators)
        {
            for (char b : operators)
            {
                precedenceTable[{a, b}] = ' ';
            }
        }

        // Define precedence relations
        // Higher precedence operators
        vector<char> highPrec = {'*', '/'};
        vector<char> lowPrec = {'+', '-'};

        // Relations for arithmetic operators
        for (char high : highPrec)
        {
            for (char low : lowPrec)
            {
                precedenceTable[{high, low}] = '>'; // * > +, / > +, etc.
                precedenceTable[{low, high}] = '<'; // + < *, + < /, etc.
            }
        }

        // Same precedence relations
        for (int i = 0; i < highPrec.size(); i++)
        {
            for (int j = 0; j < highPrec.size(); j++)
            {
                if (i != j)
                {
                    precedenceTable[{highPrec[i], highPrec[j]}] = '>';
                }
            }
        }

        for (int i = 0; i < lowPrec.size(); i++)
        {
            for (int j = 0; j < lowPrec.size(); j++)
            {
                if (i != j)
                {
                    precedenceTable[{lowPrec[i], lowPrec[j]}] = '>';
                }
            }
        }

        // Parentheses relations
        precedenceTable[{'(', '('}] = '<';
        precedenceTable[{'(', '+'}] = '<';
        precedenceTable[{'(', '-'}] = '<';
        precedenceTable[{'(', '*'}] = '<';
        precedenceTable[{'(', '/'}] = '<';
        precedenceTable[{'(', 'i'}] = '<';
        precedenceTable[{'(', ')'}] = '=';

        precedenceTable[{')', '+'}] = '>';
        precedenceTable[{')', '-'}] = '>';
        precedenceTable[{')', '*'}] = '>';
        precedenceTable[{')', '/'}] = '>';
        precedenceTable[{')', ')'}] = '>';
        precedenceTable[{')', '$'}] = '>';

        precedenceTable[{'+', '('}] = '<';
        precedenceTable[{'-', '('}] = '<';
        precedenceTable[{'*', '('}] = '<';
        precedenceTable[{'/', '('}] = '<';

        precedenceTable[{'+', 'i'}] = '<';
        precedenceTable[{'-', 'i'}] = '<';
        precedenceTable[{'*', 'i'}] = '<';
        precedenceTable[{'/', 'i'}] = '<';

        precedenceTable[{'i', '+'}] = '>';
        precedenceTable[{'i', '-'}] = '>';
        precedenceTable[{'i', '*'}] = '>';
        precedenceTable[{'i', '/'}] = '>';
        precedenceTable[{'i', ')'}] = '>';
        precedenceTable[{'i', '$'}] = '>';

        // $ relations
        precedenceTable[{'$', '('}] = '<';
        precedenceTable[{'$', '+'}] = '<';
        precedenceTable[{'$', '-'}] = '<';
        precedenceTable[{'$', '*'}] = '<';
        precedenceTable[{'$', '/'}] = '<';
        precedenceTable[{'$', 'i'}] = '<';
        precedenceTable[{'$', '$'}] = '=';

        // Same precedence for same operators
        precedenceTable[{'+', '+'}] = '>';
        precedenceTable[{'-', '-'}] = '>';
        precedenceTable[{'*', '*'}] = '>';
        precedenceTable[{'/', '/'}] = '>';

        displayPrecedenceTable();
    }

    void displayPrecedenceTable()
    {
        cout << "\nOperator Precedence Table:" << endl;
        cout << setw(TABLE_CELL_WIDTH) << " ";
        for (char op : operators)
        {
            cout << setw(TABLE_CELL_WIDTH) << (op == 'i' ? "id" : string(1, op));
        }
        cout << endl;

        for (char row : operators)
        {
            cout << setw(TABLE_CELL_WIDTH) << (row == 'i' ? "id" : string(1, row));
            for (char col : operators)
            {
                char relation = precedenceTable[{row, col}];
                cout << setw(TABLE_CELL_WIDTH) << (relation == ' ' ? "-" : string(1, relation));
            }
            cout << endl;
        }
    }

    string preprocessInput(const string &input)
    {
        string processed = "";
        for (int i = 0; i < input.length(); i++)
        {
            if (input.substr(i, 2) == "id")
            {
                processed += "i"; // Replace "id" with "i"
                i++;              // Skip the 'd'
            }
            else
            {
                processed += input[i];
            }
        }
        return processed + "$";
    }

    void parse(const string &input)
    {
        string processedInput = preprocessInput(input);
        stack<char> parseStack;
        parseStack.push('$');

        cout << "\n"
             << string(80, '=') << endl;
        cout << "PARSING: " << input << endl;
        cout << string(80, '=') << endl;

        cout << setw(5) << "Step" << setw(STACK_WIDTH) << "Stack"
             << setw(INPUT_WIDTH) << "Input" << setw(ACTION_WIDTH) << "Action"
             << setw(RELATION_WIDTH) << "Relation" << endl;
        cout << string(55, '-') << endl;

        int step = 1;
        int inputIndex = 0;

        while (!(parseStack.size() == 1 && parseStack.top() == '$' &&
                 inputIndex >= processedInput.length() - 1))
        {

            // Display current state
            string stackStr = "";
            stack<char> tempStack = parseStack;
            vector<char> stackVec;
            while (!tempStack.empty())
            {
                stackVec.push_back(tempStack.top());
                tempStack.pop();
            }
            reverse(stackVec.begin(), stackVec.end());
            for (char c : stackVec)
            {
                stackStr += (c == 'i' ? "id" : string(1, c));
            }

            string inputStr = processedInput.substr(inputIndex);
            // Convert back 'i' to 'id' for display
            string displayInput = "";
            for (char c : inputStr)
            {
                displayInput += (c == 'i' ? "id" : string(1, c));
            }

            char stackTop = parseStack.top();
            char currentInput = processedInput[inputIndex];
            char relation = precedenceTable[{stackTop, currentInput}];

            cout << setw(5) << step << setw(STACK_WIDTH) << stackStr
                 << setw(INPUT_WIDTH) << displayInput;

            if (relation == '<' || relation == '=')
            {
                // Shift
                parseStack.push(currentInput);
                inputIndex++;
                cout << setw(ACTION_WIDTH) << "SHIFT" << setw(RELATION_WIDTH) << relation << endl;
            }
            else if (relation == '>')
            {
                // Reduce
                cout << setw(ACTION_WIDTH) << "REDUCE" << setw(RELATION_WIDTH) << relation << endl;

                // Find handle and reduce
                vector<char> handle;
                stack<char> tempStack;

                // Pop until we find the start of handle
                while (!parseStack.empty())
                {
                    char popped = parseStack.top();
                    parseStack.pop();
                    handle.push_back(popped);

                    if (!parseStack.empty())
                    {
                        char below = parseStack.top();
                        if (precedenceTable[{below, popped}] == '<')
                        {
                            break;
                        }
                    }
                }

                // Push E (non-terminal) back
                parseStack.push('E');
            }
            else
            {
                // Error
                cout << setw(ACTION_WIDTH) << "ERROR" << setw(RELATION_WIDTH) << "-" << endl;
                cout << "\nParsing FAILED! No relation defined between '"
                     << (stackTop == 'i' ? "id" : string(1, stackTop))
                     << "' and '"
                     << (currentInput == 'i' ? "id" : string(1, currentInput))
                     << "'" << endl;
                return;
            }

            step++;

            // Check for successful completion
            if (parseStack.size() == 2 && parseStack.top() == 'E' &&
                inputIndex >= processedInput.length() - 1)
            {
                break;
            }
        }

        cout << "\nParsing SUCCESSFUL! Input string is ACCEPTED." << endl;
    }

    void runTests()
    {
        cout << "\n"
             << string(80, '*') << endl;
        cout << "OPERATOR PRECEDENCE PARSER IMPLEMENTATION" << endl;
        cout << string(80, '*') << endl;

        cout << "\nGrammar Rules:" << endl;
        for (const string &rule : grammar)
        {
            cout << rule << endl;
        }

        // Test case 1
        cout << "\n"
             << string(40, '+') << " TEST CASE 1 " << string(40, '+') << endl;
        parse("id+id*id");

        // Test case 2
        cout << "\n"
             << string(40, '+') << " TEST CASE 2 " << string(40, '+') << endl;
        parse("(id+id)*id");
    }
};

int main()
{
    cout << "Sometimes deadlines are tighter than dedication to quality :)" << endl;

    OperatorPrecedenceParser parser;
    parser.runTests();

    // Optional: Parse additional strings
    cout << "\n"
         << string(60, '=') << endl;
    cout << "Enter additional expressions to parse (or 'quit' to exit):" << endl;
    cout << string(60, '=') << endl;

    string input;
    while (true)
    {
        cout << "\nEnter expression: ";
        getline(cin, input);

        if (input == "quit" || input == "exit")
        {
            break;
        }

        if (!input.empty())
        {
            parser.parse(input);
        }
    }

    return 0;
}
