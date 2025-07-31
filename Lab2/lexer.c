#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define MAX_LEN 100

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

int isSpecialSymbol(char c)
{
    return c == '(' || c == ')' || c == '{' || c == '}' || c == ';' || c == ',';
}

int isOperator(char c)
{
    return c == '+' || c == '-' || c == '*' || c == '/' || c == '=' || c == '<' || c == '>' || c == '!';
}

void printToken(char *type, char *value)
{
    printf("%20s : %s\n", type, value);
}

int main()
{
    FILE *file = fopen("test_code.c", "r");
    if (!file)
    {
        printf("Error opening file!\n");
        return 1;
    }

    char ch, buffer[MAX_LEN];
    int i = 0;
    int inSingleLineComment = 0, inMultiLineComment = 0;

    while ((ch = fgetc(file)) != EOF)
    {
        if (inSingleLineComment)
        {
            if (ch == '\n')
                inSingleLineComment = 0;
            continue;
        }

        if (inMultiLineComment)
        {
            if (ch == '*' && (ch = fgetc(file)) == '/')
            {
                inMultiLineComment = 0;
            }
            continue;
        }

        // Handle preprocessor line: #include <stdio.h>
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
            continue;
        }

        // Handle comments
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
                    buffer[i++] = ch;

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

        if (isspace(ch))
            continue;

        // Handle string literals
        if (ch == '"')
        {
            i = 0;
            buffer[i++] = ch;
            while ((ch = fgetc(file)) != EOF && ch != '"')
            {
                buffer[i++] = ch;
            }
            buffer[i++] = '"'; // add closing quote
            buffer[i] = '\0';
            printToken("String Literal", buffer);
            continue;
        }

        // Identifiers or keywords
        if (isalpha(ch) || ch == '_')
        {
            i = 0;
            buffer[i++] = ch;
            while ((ch = fgetc(file)) != EOF && (isalnum(ch) || ch == '_'))
            {
                buffer[i++] = ch;
            }
            buffer[i] = '\0';
            ungetc(ch, file);
            if (isKeyword(buffer))
                printToken("Keyword", buffer);
            else
                printToken("Identifier", buffer);
        }

        // Numbers (integers and floats)
        else if (isdigit(ch))
        {
            i = 0;
            int hasDot = 0;
            buffer[i++] = ch;
            while ((ch = fgetc(file)) != EOF && (isdigit(ch) || ch == '.'))
            {
                if (ch == '.')
                {
                    if (hasDot)
                        break;
                    hasDot = 1;
                }
                buffer[i++] = ch;
            }
            buffer[i] = '\0';
            ungetc(ch, file);
            if (hasDot)
                printToken("Float", buffer);
            else
                printToken("Integer", buffer);
        }

        // Two-character operators (optional extension)
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
            buffer[0] = ch;
            buffer[1] = '\0';
            printToken("Special Symbol", buffer);
        }

        // Ignore newline
        else if (ch == '\n')
        {
            continue;
        }

        // Unknown character
        else
        {
            buffer[0] = ch;
            buffer[1] = '\0';
            printf("Error: Invalid token '%s'\n", buffer);
        }
    }

    fclose(file);
    return 0;
}
