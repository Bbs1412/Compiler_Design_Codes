#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define MAX_LEN 200
#define LEFT_SPACE 20

const char *keywords[] = {"int", "float", "if", "else", "while", "return", "for", "break", "continue", "char", "double", "void"};

int isKeyword(char *word)
{
    for (int i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++)
    {
        if (strcmp(word, keywords[i]) == 0)
            return 1;
    }
    return 0;
}

int isSpecialSymbol(char c) { return c == '(' || c == ')' || c == '{' || c == '}' || c == ';' || c == ','; }

int isOperator(char c) { return c == '+' || c == '-' || c == '*' || c == '/' || c == '=' || c == '<' || c == '>' || c == '!'; }

void printToken(char *type, char *value) { printf("%*s : %s\n", LEFT_SPACE, type, value); }

void printError(char *message, char *value, int line)
{
    // char typeFormatted[100];
    // sprintf(typeFormatted, "\033[1;31mError (line %d):\033[0m Invalid token :", line);
    // printf("\033[1;31mError:\033[0m %s '%s'\n", message, value);

    printf("\033[1;31mError in (line %d):\033[0m\n", line);
    printf("\033[1;31m%*s :\033[0m %s\n", LEFT_SPACE, message, value);
}

int main()
{
    // printf("\033[1;31mError:\033[0m Invalid token '123_new'\n");

    FILE *file = fopen("test_code.c", "r");
    if (!file)
    {
        printf("Error opening file!\n");
        return 1;
    }

    char ch, buffer[MAX_LEN];
    int i = 0;
    int line = 1;

    while ((ch = fgetc(file)) != EOF)
    {
        if (isspace(ch))
            continue;

        // Handle preprocessor line
        if (ch == '#')
        {
            i = 0;
            buffer[i++] = ch;
            while ((ch = fgetc(file)) != EOF && ch != '\n')
            {
                buffer[i++] = ch;
            }
            buffer[i] = '\0';
            printToken("Preprocessor", buffer);
            line++;
            continue;
        }

        // Handle comments
        if (ch == '/')
        {
            char next = fgetc(file);
            if (next == '/')
            {
                // Single-line comment
                i = 0;
                buffer[i++] = '/';
                buffer[i++] = '/';
                while ((ch = fgetc(file)) != EOF && ch != '\n')
                {
                    buffer[i++] = ch;
                }
                buffer[i] = '\0';
                printToken("Single-line Comment", buffer);
                line++;
                continue;
            }
            else if (next == '*')
            {
                // Multi-line comment
                i = 0;
                buffer[i++] = '/';
                buffer[i++] = '*';
                int prev = 0;

                while ((ch = fgetc(file)) != EOF)
                {
                    if (ch == '\n')
                    {
                        line++;
                        // buffer[i++] = '\\';
                        // buffer[i++] = 'n';
                        buffer[i++] = '\n';
                        for (int j = 0; j < LEFT_SPACE + 1; j++)
                        {
                            buffer[i++] = ' ';
                        }
                        buffer[i++] = ':';
                    }
                    else
                    {
                        buffer[i++] = ch;
                    }

                    if (prev == '*' && ch == '/')
                        break;

                    prev = ch;

                    // Prevent overflow
                    if (i >= MAX_LEN - 2)
                    {
                        buffer[i++] = '.';
                        buffer[i++] = '.';
                        buffer[i++] = '.';
                        break;
                    }
                }
                buffer[i] = '\0';
                printToken("Multi-line Comment", buffer);
                continue;
            }
            else
            {
                // It's just an operator /
                ungetc(next, file);
                buffer[0] = ch;
                buffer[1] = '\0';
                printToken("Operator", buffer);
                continue;
            }
        }

        // Handle string literals
        if (ch == '"')
        {
            i = 0;
            buffer[i++] = ch;
            while ((ch = fgetc(file)) != EOF && ch != '"')
            {
                buffer[i++] = ch;
                if (ch == '\n')
                    line++;
            }
            buffer[i++] = '"'; // add closing quote
            buffer[i] = '\0';
            printToken("String Literal", buffer);
            continue;
        }

        // Unified block for identifiers, keywords, and numbers
        if (isalpha(ch) || ch == '_' || isdigit(ch))
        {
            i = 0;
            buffer[i++] = ch;

            int isFirstDigit = isdigit(ch);
            int hasDot = 0;
            int hasAlpha = isalpha(ch);
            int isValid = 1;

            while ((ch = fgetc(file)) != EOF && (isalnum(ch) || ch == '_' || ch == '.'))
            {
                if (ch == '.')
                {
                    if (hasDot || !isFirstDigit) // already has a dot OR dot without starting as number
                    {
                        isValid = 0;
                    }
                    hasDot = 1;
                }
                else if (isalpha(ch) || ch == '_')
                {
                    hasAlpha = 1;
                    if (isFirstDigit) // starts as number but has alpha → invalid
                        isValid = 0;
                }
                buffer[i++] = ch;
            }

            buffer[i] = '\0';
            ungetc(ch, file);

            if (!isValid)
            {
                printError("Invalid token", buffer, line);
            }
            else if (isKeyword(buffer))
            {
                printToken("Keyword", buffer);
            }
            else if (isdigit(buffer[0]))
            {
                if (hasDot)
                    printToken("Float", buffer);
                else
                    printToken("Integer", buffer);
            }
            else if (isalpha(buffer[0]) || buffer[0] == '_')
            {
                printToken("Identifier", buffer);
            }
            else
            {
                printError("Invalid token", buffer, line);
            }

            continue;
        }

        // Two-character operators
        else if (isOperator(ch))
        {
            char next = fgetc(file);
            buffer[0] = ch;
            if (next == '=' || (ch == next && (ch == '+' || ch == '-' || ch == '&' || ch == '|')))
            {
                buffer[1] = next;
                buffer[2] = '\0';
            }
            else
            {
                buffer[1] = '\0';
                ungetc(next, file);
            }
            printToken("Operator", buffer);
        }

        // Special symbols
        else if (isSpecialSymbol(ch))
        {
            // if (ch == '{' || ch == '}' || ch == ')' || ch == ';') line++;
            if (ch == '{' || ch == '}' || ch == ')' || ch == ';')
                line++;

            buffer[0] = ch;
            buffer[1] = '\0';
            printToken("Special Symbol", buffer);
        }

        // Ignore newline
        else if (ch == '\n')
        {
            line++;
            continue;
        }

        // Unknown character
        else
        {
            buffer[0] = ch;
            buffer[1] = '\0';
            line++;
            printError("Invalid token", buffer, line);
        }
    }

    fclose(file);
    return 0;
}
