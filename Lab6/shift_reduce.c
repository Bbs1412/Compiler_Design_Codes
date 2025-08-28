/* shift_reduce.c
    Shift-Reduce parser for:
        E -> E + T | T
        T -> T * F | F
        F -> ( E ) | id

    Prints an aligned 3-column trace:
        Stack | Input | Action
        Allows customization for column widths.

    Policy to respect precedence & associativity:
        - Always reduce: id->F, (E)->F, T*F->T
        - Reduce F->T always (safe)
        - Reduce T->E only if lookahead != '*'
        - Reduce E+T->E only if lookahead != '*'
    This defers E-level reductions when '*' is upcoming, giving '*' higher precedence.

    Build:
        gcc -std=c99 -O2 shift_reduce.c -o sr_parser

    Run:
        ./sr_parser
        enter input like:
            `id + id `      -> Accept
            `id + id * id`  -> Accept
            ` id + ( id`    -> Error
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define MAX_TOKENS 256
#define MAX_STACK 256
#define MAX_STEPS 1024
#define BUF_LEN 512

#define COL_STACK 30  // 50
#define COL_INPUT 30  // 42
#define COL_ACTION 30 // 30
#define TOTAL_WIDTH (COL_STACK + COL_INPUT + COL_ACTION + 2 * 3)

typedef enum
{
    SYM_ERR = -1,
    SYM_DOLLAR = 0,
    SYM_E,
    SYM_T,
    SYM_F,
    SYM_PLUS,
    SYM_MUL,
    SYM_LPAREN,
    SYM_RPAREN,
    SYM_ID
} Sym;

const char *sym_to_str(Sym s)
{
    switch (s)
    {
    case SYM_DOLLAR:
        return "$";
    case SYM_E:
        return "E";
    case SYM_T:
        return "T";
    case SYM_F:
        return "F";
    case SYM_PLUS:
        return "+";
    case SYM_MUL:
        return "*";
    case SYM_LPAREN:
        return "(";
    case SYM_RPAREN:
        return ")";
    case SYM_ID:
        return "id";
    default:
        return "?";
    }
}

typedef struct
{
    Sym stack[MAX_STACK];
    int top; /* index of next free slot (size) */
} Stack;

static void st_init(Stack *st) { st->top = 0; }
static int st_size(const Stack *st) { return st->top; }
static int st_empty(const Stack *st) { return st->top == 0; }
static Sym st_peek(const Stack *st, int k)
{
    /* k=1 topmost, k=2 next, ... */
    if (k <= 0 || k > st->top)
        return SYM_ERR;
    return st->stack[st->top - k];
}
static void st_push(Stack *st, Sym s)
{
    if (st->top < MAX_STACK)
        st->stack[st->top++] = s;
}
static Sym st_pop(Stack *st)
{
    if (st->top <= 0)
        return SYM_ERR;
    return st->stack[--st->top];
}

/* Trace storage */
typedef struct
{
    char stack_s[BUF_LEN];
    char input_s[BUF_LEN];
    char action_s[BUF_LEN];
} Step;

typedef struct
{
    Step steps[MAX_STEPS];
    int count;
} Trace;

static void join_stack(const Stack *st, char *out, size_t cap)
{
    out[0] = '\0';
    for (int i = 0; i < st->top; ++i)
    {
        const char *s = sym_to_str(st->stack[i]);
        if (i)
            strncat(out, " ", cap - strlen(out) - 1);
        strncat(out, s, cap - strlen(out) - 1);
    }
}

static void join_input(const Sym *tokens, int pos, int n, char *out, size_t cap)
{
    out[0] = '\0';
    for (int i = pos; i < n; ++i)
    {
        if (i > pos)
            strncat(out, " ", cap - strlen(out) - 1);
        strncat(out, sym_to_str(tokens[i]), cap - strlen(out) - 1);
    }
}

static void add_step(Trace *tr, const Stack *st, const Sym *tokens, int pos, int n, const char *action)
{
    if (tr->count >= MAX_STEPS)
        return;
    Step *s = &tr->steps[tr->count++];
    join_stack(st, s->stack_s, sizeof(s->stack_s));
    join_input(tokens, pos, n, s->input_s, sizeof(s->input_s));
    strncpy(s->action_s, action, sizeof(s->action_s) - 1);
    s->action_s[sizeof(s->action_s) - 1] = '\0';
}

static void print_line_of(char ch, int n)
{
    for (int i = 0; i < n; ++i)
        putchar(ch);
    putchar('\n');
}

static void print_fixed(const char *s, int width)
{
    int len = (int)strlen(s);
    if (len <= width)
    {
        /* left align, pad spaces */
        printf("%-*s", width, s);
    }
    else
    {
        /* truncate on the left for stack/input to keep tail visible; show "..." */
        if (width >= 3)
        {
            const char *tail = s + (len - (width - 3));
            printf("...%.*s", width - 3, tail);
        }
        else
        {
            /* degenerate case */
            for (int i = 0; i < width; ++i)
                putchar('.');
        }
    }
}

static void print_trace(const Trace *tr)
{
    print_line_of('-', TOTAL_WIDTH);
    print_fixed("Stack", COL_STACK);
    printf(" | ");
    print_fixed("Input", COL_INPUT);
    printf(" | ");
    print_fixed("Action", COL_ACTION);
    printf("\n");
    print_line_of('-', TOTAL_WIDTH);
    for (int i = 0; i < tr->count; ++i)
    {
        print_fixed(tr->steps[i].stack_s, COL_STACK);
        printf(" | ");
        print_fixed(tr->steps[i].input_s, COL_INPUT);
        printf(" | ");
        print_fixed(tr->steps[i].action_s, COL_ACTION);
        printf("\n");
    }
    print_line_of('-', TOTAL_WIDTH);
}

/* Tokenizer: accepts "id", '+', '*', '(', ')' and ignores whitespace.
   Appends SYM_DOLLAR at end. Returns number of tokens (including $), or -1 on error. */
static int tokenize(const char *line, Sym *tokens, int cap)
{
    int n = 0;
    for (int i = 0; line[i];)
    {
        if (isspace((unsigned char)line[i]))
        {
            ++i;
            continue;
        }
        if (line[i] == '+')
        {
            if (n < cap)
                tokens[n++] = SYM_PLUS;
            ++i;
            continue;
        }
        if (line[i] == '*')
        {
            if (n < cap)
                tokens[n++] = SYM_MUL;
            ++i;
            continue;
        }
        if (line[i] == '(')
        {
            if (n < cap)
                tokens[n++] = SYM_LPAREN;
            ++i;
            continue;
        }
        if (line[i] == ')')
        {
            if (n < cap)
                tokens[n++] = SYM_RPAREN;
            ++i;
            continue;
        }
        if (line[i] == 'i' && line[i + 1] == 'd')
        {
            if (n < cap)
                tokens[n++] = SYM_ID;
            i += 2;
            continue;
        }
        /* invalid token */
        return -1;
    }
    if (n < cap)
        tokens[n++] = SYM_DOLLAR;
    return n;
}

static int is_accept(const Stack *st, Sym lookahead)
{
    return (st->top == 2 && st->stack[0] == SYM_DOLLAR && st->stack[1] == SYM_E && lookahead == SYM_DOLLAR);
}

/* Try reductions according to policy.
   Returns:
     1 if a reduction happened (and writes action string),
     0 if no reduction possible,
    -1 on unrecoverable internal issue.
*/
static int try_reduce(Stack *st, Sym lookahead, char *action, size_t alen)
{
    /* Prefer longer/stronger handles first. */

    /* E + T -> E  (defer if next is '*') */
    if (st->top >= 3 &&
        st->stack[st->top - 3] == SYM_E &&
        st->stack[st->top - 2] == SYM_PLUS &&
        st->stack[st->top - 1] == SYM_T &&
        lookahead != SYM_MUL)
    {
        st_pop(st);
        st_pop(st);
        st_pop(st);
        st_push(st, SYM_E);
        snprintf(action, alen, "REDUCE E -> E + T");
        return 1;
    }

    /* T * F -> T */
    if (st->top >= 3 &&
        st->stack[st->top - 3] == SYM_T &&
        st->stack[st->top - 2] == SYM_MUL &&
        st->stack[st->top - 1] == SYM_F)
    {
        st_pop(st);
        st_pop(st);
        st_pop(st);
        st_push(st, SYM_T);
        snprintf(action, alen, "REDUCE T -> T * F");
        return 1;
    }

    /* ( E ) -> F */
    if (st->top >= 3 &&
        st->stack[st->top - 3] == SYM_LPAREN &&
        st->stack[st->top - 2] == SYM_E &&
        st->stack[st->top - 1] == SYM_RPAREN)
    {
        st_pop(st);
        st_pop(st);
        st_pop(st);
        st_push(st, SYM_F);
        snprintf(action, alen, "REDUCE F -> ( E )");
        return 1;
    }

    /* id -> F */
    if (st->top >= 1 && st_peek(st, 1) == SYM_ID)
    {
        st_pop(st);
        st_push(st, SYM_F);
        snprintf(action, alen, "REDUCE F -> id");
        return 1;
    }

    /* F -> T (always safe) */
    if (st->top >= 1 && st_peek(st, 1) == SYM_F)
    {
        st_pop(st);
        st_push(st, SYM_T);
        snprintf(action, alen, "REDUCE T -> F");
        return 1;
    }

    /* T -> E (defer if next is '*') */
    if (st->top >= 1 && st_peek(st, 1) == SYM_T && lookahead != SYM_MUL)
    {
        st_pop(st);
        st_push(st, SYM_E);
        snprintf(action, alen, "REDUCE E -> T");
        return 1;
    }

    return 0; /* no reduction */
}

/* Parse a token stream; produce trace; return 1 on accept, 0 on error */
static int parse_tokens(const Sym *tokens, int n, Trace *tr, char *errmsg, size_t elen)
{
    Stack st;
    st_init(&st);
    st_push(&st, SYM_DOLLAR);

    int pos = 0; /* index into tokens; tokens[n-1] is SYM_DOLLAR */

    /* Add initial starting point to trace */
    add_step(tr, &st, tokens, pos, n, "Starting point");

    while (1)
    {
        Sym lookahead = (pos < n) ? tokens[pos] : SYM_ERR;

        if (is_accept(&st, lookahead))
        {
            add_step(tr, &st, tokens, pos, n, "ACCEPT");
            return 1;
        }

        /* Try reductions (one at a time, each is recorded as a step) */
        char act[BUF_LEN];
        int red = try_reduce(&st, lookahead, act, sizeof(act));
        if (red < 0)
        {
            snprintf(errmsg, elen, "Internal reduction error.");
            return 0;
        }
        if (red == 1)
        {
            add_step(tr, &st, tokens, pos, n, act);
            continue;
        }

        /* No reduction: try SHIFT */
        if (lookahead == SYM_ERR)
        {
            snprintf(errmsg, elen, "Unexpected end of input.");
            return 0;
        }

        /* Basic sanity: do not shift trailing $ if stack is not reducible to $ E */
        if (lookahead == SYM_DOLLAR)
        {
            snprintf(errmsg, elen, "Cannot accept: remaining stack not reducible.");
            return 0;
        }

        st_push(&st, lookahead);
        ++pos;
        snprintf(act, sizeof(act), "SHIFT %s", sym_to_str(lookahead));
        add_step(tr, &st, tokens, pos, n, act);
    }
}

/* ------------- Main ------------- */
int main(void)
{
    char line[BUF_LEN];

    printf("Grammar:\n");
    // printf("E->E+T|T; T->T*F|F; F->(E)|id\n");
    printf("\t\tE -> E + T | T\n");
    printf("\t\tT -> T * F | F\n");
    printf("\t\tF -> ( E ) | id\n");

    printf("\nEnter input in one line (e.g., id+id or id+id*id):\n> ");

    if (!fgets(line, sizeof(line), stdin))
    {
        fprintf(stderr, "No input.\n");
        return 1;
    }

    Sym tokens[MAX_TOKENS];
    int n = tokenize(line, tokens, MAX_TOKENS);
    if (n < 0)
    {
        fprintf(stderr, "Lexical error: only 'id', '+', '*', '(', ')' are allowed.\n");
        return 1;
    }

    Trace tr;
    tr.count = 0;
    char errmsg[BUF_LEN];

    int ok = parse_tokens(tokens, n, &tr, errmsg, sizeof(errmsg));
    print_trace(&tr);

    if (ok)
    {
        printf("Result: ACCEPTED\n");
        return 0;
    }
    else
    {
        printf("Result: ERROR - %s\n", errmsg);
        return 2;
    }
}
