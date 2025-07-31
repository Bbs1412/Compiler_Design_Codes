// To implement a program to identify and count the number of  Keywords, Identifiers, Operators and Constants for a given input Program.

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

using namespace std;

const unordered_set<string> keywords = {
    "alignas", "alignof", "and", "and_eq", "asm", "auto", "bitand", "bitor", "bool",
    "break", "case", "catch", "char", "char8_t", "char16_t", "char32_t", "class",
    "compl", "concept", "const", "consteval", "constexpr", "constinit", "const_cast",
    "continue", "co_await", "co_return", "co_yield", "decltype", "default", "delete",
    "do", "double", "dynamic_cast", "else", "enum", "explicit", "export", "extern",
    "false", "float", "for", "friend", "goto", "if", "inline", "int", "long", "mutable",
    "namespace", "new", "noexcept", "not", "not_eq", "nullptr", "operator", "or", "or_eq",
    "private", "protected", "public", "reflexpr", "register", "reinterpret_cast", "requires",
    "return", "short", "signed", "sizeof", "static", "static_assert", "static_cast", "struct",
    "switch", "synchronized", "template", "this", "thread_local", "throw", "true", "try",
    "typedef", "typeid", "typename", "union", "unsigned", "using", "virtual", "void", "volatile",
    "wchar_t", "while", "xor", "xor_eq", "include", "main", "cout", "cin", "std"};

const unordered_set<char> single_char_operators = {
    '+', '-', '*', '/', '%', '=', '>', '<', '!', '&', '|', '^', '~', '?', ':'};

bool isKeyword(const string &token)
{
    return keywords.count(token);
}

bool isOperator(char ch)
{
    return single_char_operators.count(ch);
}

bool isNumericConstant(const string &token)
{
    if (token.empty())
    {
        return false;
    }

    bool hasDecimal = false;
    for (char c : token)
    {
        if (!isdigit(c))
        {
            if (c == '.' && !hasDecimal)
            {
                hasDecimal = true;
            }
            else
            {
                return false;
            }
        }
    }
    return true;
}

bool isIdentifier(const string &token)
{
    if (token.empty() || isKeyword(token) || isdigit(token[0]))
    {
        return false;
    }

    for (char ch : token)
    {
        if (!isalnum(ch) && ch != '_')
        {
            return false;
        }
    }

    return true;
}

void analyzeFile(const string &filename)
{
    ifstream file(filename);
    if (!file.is_open())
    {
        cerr << "Error: Could not open file '" << filename << "' for analysis." << endl;
        return;
    }

    cout << "Analyzing file: " << filename << "\n";

    int keywordCount = 0;
    int identifierCount = 0;
    int operatorCount = 0;
    int constantCount = 0;

    string line;
    string currentToken = "";
    bool in_multiline_comment = false;

    while (getline(file, line))
    {
        for (size_t i = 0; i < line.length(); ++i)
        {
            char ch = line[i];

            if (in_multiline_comment)
            {
                if (i + 1 < line.length() && ch == '*' && line[i + 1] == '/')
                {
                    in_multiline_comment = false;
                }
                continue;
            }

            if (i + 1 < line.length() && ch == '/' && line[i + 1] == '*')
            {
                in_multiline_comment = true;
                continue;
            }

            if (i + 1 < line.length() && ch == '/' && line[i + 1] == '/')
            {
                // single line comment, skip the rest of the line
            }

            if (ch == '"')
            {
                constantCount++;
                size_t end_quote = line.find('"', i + 1);
                if (end_quote != string::npos)
                {
                    i = end_quote;
                }
                else
                {
                }
                continue;
            }

            if (ch == '\'')
            {
                constantCount++;
                if (i + 2 < line.length() && line[i + 2] == '\'')
                {
                }
                else if (i + 3 < line.length() && line[i + 1] == '\\' && line[i + 3] == '\'')
                {
                }
                continue;
            }

            auto processToken = [&]()
            {
                if (!currentToken.empty())
                {
                    if (isKeyword(currentToken))
                        keywordCount++;
                    else if (isNumericConstant(currentToken))
                        constantCount++;
                    else if (isIdentifier(currentToken))
                        identifierCount++;
                    currentToken = "";
                }
            };

            if (isOperator(ch))
            {
                processToken();
                operatorCount++;
                if (i + 1 < line.length())
                {
                    char next_ch = line[i + 1];
                    if ((ch == '=' && next_ch == '=') || (ch == '!' && next_ch == '=') ||
                        (ch == '>' && next_ch == '=') || (ch == '<' && next_ch == '=') ||
                        (ch == '&' && next_ch == '&') || (ch == '|' && next_ch == '|'))
                    {
                    }
                }
                continue;
            }

            if (isspace(ch) || ch == '(' || ch == ')' || ch == '{' ||
                ch == '}' || ch == '[' || ch == ']' || ch == ';' || ch == ',' ||
                ch == '#')
            {
                processToken();
            }
            else
            {
                currentToken += ch;
            }
        }
        if (!currentToken.empty())
        {
            if (isKeyword(currentToken))
                keywordCount++;
            else if (isNumericConstant(currentToken))
                constantCount++;
            else if (isIdentifier(currentToken))
                identifierCount++;
            currentToken = "";
        }
    }

    file.close();

    cout << "\nLexical Analysis Report:\n";
    cout << "Keywords    : " << keywordCount << endl;
    cout << "Identifiers : " << identifierCount << endl;
    cout << "Operators   : " << operatorCount << endl;
    cout << "Constants   : " << constantCount << endl;
    cout << endl;
}

int main()
{
    const string filename = "./test_program.cpp";
    analyzeFile(filename);
    return 0;
}
