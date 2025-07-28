// Implement a lexical analyzer that scans a source program and builds a symbol table for identifiers, numbers, and literals, according to the specified lexical rules.

#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <unordered_set>
#include <iomanip>
#include <fstream>
#include <sstream>

using namespace std;

struct SymbolTableEntry {
    int entryNo;
    string lexeme;
    string tokenType;
    int lineDeclared;
    vector<int> linesUsed;
};

class LexicalAnalyzer {
private:
  
    unordered_set<string> keywords = {
        "int", "float", "char", "double", "bool", "void", "return",
        "if", "else", "while", "for", "do", "switch", "case", "break",
        "continue", "class", "struct", "public", "private", "protected",
        "new", "delete", "this", "const", "static", "using", "namespace"
    };

    vector<SymbolTableEntry> symbolTable;
    int entryCounter = 1;

    
    bool isOperator(char c) {
        string ops = "+-*/%<>=!&|";
        return ops.find(c) != string::npos;
    }

   
    bool isSpecialSymbol(char c) {
        string symbols = "(){}[];,:.#";
        return symbols.find(c) != string::npos;
    }

    
    void addToSymbolTable(const string& lexeme, const string& type, int line) {
        
        if (type == "Identifier") {
            for (auto& entry : symbolTable) {
                if (entry.lexeme == lexeme && entry.tokenType == "Identifier") {
                    entry.linesUsed.push_back(line);
                    return; 
                }
            }
        }

        
        SymbolTableEntry newEntry;
        newEntry.entryNo = entryCounter++;
        newEntry.lexeme = lexeme;
        newEntry.tokenType = type;
        newEntry.lineDeclared = line;
        newEntry.linesUsed.push_back(line);
        symbolTable.push_back(newEntry);
    }


public:
    
    void analyze(const string& sourceCode) {
        int currentPos = 0;
        int line = 1;

        while (currentPos < sourceCode.length()) {
            char currentChar = sourceCode[currentPos];

            
            if (isspace(currentChar)) {
                if (currentChar == '\n') {
                    line++;
                }
                currentPos++;
                continue;
            }

            
            if (currentChar == '/' && currentPos + 1 < sourceCode.length() && sourceCode[currentPos + 1] == '/') {
                while (currentPos < sourceCode.length() && sourceCode[currentPos] != '\n') {
                    currentPos++;
                }
                continue;
            }
            
            if (currentChar == '/' && currentPos + 1 < sourceCode.length() && sourceCode[currentPos + 1] == '*') {
                int startPos = currentPos;
                currentPos += 2;
                while (currentPos + 1 < sourceCode.length() &&
                       !(sourceCode[currentPos] == '*' && sourceCode[currentPos + 1] == '/')) {
                    if (sourceCode[currentPos] == '\n') {
                        line++;
                    }
                    currentPos++;
                }
                if (currentPos + 1 >= sourceCode.length()) {
                     cerr << "Lexical Error: Unterminated multi-line comment starting at line " << line << endl;
                     return;
                }
                currentPos += 2; 
                continue;
            }

            
            if (isalpha(currentChar) || currentChar == '_') {
                string lexeme;
                while (currentPos < sourceCode.length() && (isalnum(sourceCode[currentPos]) || sourceCode[currentPos] == '_')) {
                    lexeme += sourceCode[currentPos];
                    currentPos++;
                }

                if (keywords.count(lexeme)) {
                    
                    cout << "Token: Keyword, Lexeme: " << lexeme << ", Line: " << line << endl;
                } else {
                    
                    cout << "Token: Identifier, Lexeme: " << lexeme << ", Line: " << line << endl;
                    addToSymbolTable(lexeme, "Identifier", line);
                }
                continue;
            }

            
            if (isdigit(currentChar)) {
                string lexeme;
                bool isFloat = false;
                while (currentPos < sourceCode.length() && (isdigit(sourceCode[currentPos]) || sourceCode[currentPos] == '.')) {
                    if (sourceCode[currentPos] == '.') {
                        if (isFloat) break; 
                        isFloat = true;
                    }
                    lexeme += sourceCode[currentPos];
                    currentPos++;
                }
                string type = isFloat ? "Float" : "Integer";
                cout << "Token: " << type << ", Lexeme: " << lexeme << ", Line: " << line << endl;
                addToSymbolTable(lexeme, type, line);
                continue;
            }

          
            if (currentChar == '"') {
                string lexeme;
                lexeme += currentChar;
                currentPos++;
                while (currentPos < sourceCode.length() && sourceCode[currentPos] != '"') {
                    if (sourceCode[currentPos] == '\n') line++; 
                    lexeme += sourceCode[currentPos];
                    currentPos++;
                }
                if (currentPos < sourceCode.length() && sourceCode[currentPos] == '"') {
                    lexeme += sourceCode[currentPos];
                    currentPos++;
                    cout << "Token: Literal, Lexeme: " << lexeme << ", Line: " << line << endl;
                    addToSymbolTable(lexeme, "Literal", line);
                } else {
                    cerr << "Lexical Error: Unterminated string literal at line " << line << endl;
                }
                continue;
            }

           
            if (isOperator(currentChar)) {
                string lexeme;
                lexeme += currentChar;
                
                if (currentPos + 1 < sourceCode.length() && isOperator(sourceCode[currentPos+1])) {
                     lexeme += sourceCode[currentPos+1];
                     currentPos++;
                }
                currentPos++;
                cout << "Token: Operator, Lexeme: " << lexeme << ", Line: " << line << endl;
                
                continue;
            }


            if (isSpecialSymbol(currentChar)) {
                cout << "Token: Special Symbol, Lexeme: " << string(1, currentChar) << ", Line: " << line << endl;
                currentPos++;
                continue;
            }

            
            cerr << "Lexical Error: Unrecognized token '" << currentChar << "' at line " << line << endl;
            currentPos++;
        }
    }

    
    void printSymbolTable() {
        cout << "\n\n--- Symbol Table ---\n";
        cout << left << setw(10) << "Entry No."
                  << setw(25) << "Lexeme (Name/Value)"
                  << setw(15) << "Token Type"
                  << setw(15) << "Line Declared"
                  << "Lines Used" << endl;
        cout << string(80, '-') << endl;

        for (const auto& entry : symbolTable) {
            cout << left << setw(10) << entry.entryNo
                      << setw(25) << entry.lexeme
                      << setw(15) << entry.tokenType
                      << setw(15) << entry.lineDeclared;
            
            string usedLinesStr;
            for(size_t i = 0; i < entry.linesUsed.size(); ++i) {
                usedLinesStr += to_string(entry.linesUsed[i]) + (i == entry.linesUsed.size() - 1 ? "" : ", ");
            }
            cout << usedLinesStr << endl;
        }
    }
};

int main() {
    string filename;
    cout << "Enter the source code filename: ";
    cin >> filename;

    
    ifstream inputFile(filename);
    if (!inputFile.is_open()) {
        cerr << "Error: Could not open file '" << filename << "'" << endl;
        return 1; 
    }

    
    stringstream buffer;
    buffer << inputFile.rdbuf();
    string sourceCode = buffer.str();
    inputFile.close();

    
    LexicalAnalyzer analyzer;
    cout << "\n--- Analyzing Code from " << filename << " ---\n" << endl;
    analyzer.analyze(sourceCode);
    analyzer.printSymbolTable();

 

    return 0;
}
