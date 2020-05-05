#include "parse.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include "types.h"
#include "interp.h"
#include "global.h"

struct parser {
    struct allocator *alloc;
    struct expr_lnk *exp_stack_top;
    int tokenizer_state;
    void (*callback)(value exp, void *arg);
    void *cb_arg;
};

/* more parser states means you need to widen the field in expr_lnk below! */
#define P_LPAREN     0
#define P_RPAREN     1
#define P_LBRACKET   2
#define P_RBRACKET   3
#define P_INTNUM     4
#define P_FLONUM     5
#define P_DOT        6
#define P_IDENT      7
#define P_BOOL       8
#define P_QUOTE      9
#define P_VECTOR    10
#define P_STRING    11
#define P_EOF       12

#define PP_CAR       0
#define PP_MID       1
#define PP_CDR       2
#define PP_DONE      3

struct expr_lnk {
    value content;
    struct expr_lnk *outer;
    unsigned int implicit : 1;
    unsigned int quote : 1;
    unsigned int bracket : 1;
    unsigned int parse_pos : 3;
};

void string_deescape(char *str, int len) {
    char *loc = str;
    while ((loc = strchr(loc, '\\')) != NULL) {
        switch (*(loc+1)) {
            case 'a':
                *loc = '\a';
                break;
            case 'b':
                *loc = '\b';
                break;
            case 't':
                *loc = '\t';
                break;
            case 'n':
                *loc = '\n';
                break;
            case 'r':
                *loc = '\r';
                break;
            case '"':
                *loc = '"';
                break;
            case '\\':
                *loc = '\\';
                break;
            case '|':
                *loc = '|';
                break;
        }
        strncpy(loc + 1, loc + 2, len - 1);
        loc++;
    }
}

// store the passed value in the current position of the top element of
// the exp_stack
void parser_store_cell(struct parser *p, value cv) {
    struct expr_lnk *cl;
    if (p->exp_stack_top) {
        if (p->exp_stack_top->parse_pos == PP_CAR) {
            set_car(p->alloc, p->exp_stack_top->content, cv);
            p->exp_stack_top->parse_pos = PP_MID;
        }
        else if (p->exp_stack_top->parse_pos == PP_CDR) {
            set_cdr(p->alloc, p->exp_stack_top->content, cv);
            p->exp_stack_top->parse_pos = PP_DONE;
        }
        else if (p->exp_stack_top->parse_pos == PP_MID) {
            // short notation, create new cell in cdr and store in car of that
            set_cdr(p->alloc, p->exp_stack_top->content, make_cons(p->alloc, cv, VALUE_EMPTY_LIST));
            cv = cdr(p->exp_stack_top->content);
            cl = malloc(sizeof(struct expr_lnk));
            cl->content = cv;
            cl->outer = p->exp_stack_top;
            cl->implicit = 1;
            cl->quote = cl->outer->quote;
            cl->bracket = 0;
            p->exp_stack_top = cl;
            p->exp_stack_top->parse_pos = PP_MID;
        }
        else {
            fprintf(stderr, "trying to store after end of cons cell\n");
        }
    }
}

void parser_parse(struct parser *p, int tok, int64_t inum, double fnum, char *str) {
    value cv = VALUE_NIL;
    struct expr_lnk *cl;
    switch (tok) {
        case P_LBRACKET:
        case P_LPAREN:
            cv = make_cons(p->alloc, VALUE_NIL, VALUE_NIL);
            parser_store_cell(p, cv);
            cl = malloc(sizeof(struct expr_lnk));
            cl->content = cv;
            cl->implicit = 0;
            cl->quote = 0;
            cl->bracket = tok == P_LBRACKET;
            cl->outer = p->exp_stack_top;
            p->exp_stack_top = cl;
            cl->parse_pos = PP_CAR;
            break;
        case P_RBRACKET:
        case P_RPAREN:
            cl = p->exp_stack_top;
            if (cl) {
                if (cl->parse_pos == PP_MID) {
                    set_cdr(p->alloc, p->exp_stack_top->content, VALUE_EMPTY_LIST);
                }
                if ((car(cl->content) == VALUE_NIL) && (cdr(cl->content) == VALUE_NIL)) {
                    cl->content = VALUE_EMPTY_LIST;
                    if (cl->outer) {
                        if (cl->outer->parse_pos == PP_MID) {
                            set_car(p->alloc, cl->outer->content, cl->content);
                        }
                    }
                }
                // reduce expression stack
                while (cl->implicit) {
                    struct expr_lnk *tmp = cl;
                    cl = cl->outer;
                    free(tmp);
                }
                p->exp_stack_top = cl->outer;
                cv = cl->content;
                if (cl->bracket != (tok == P_RBRACKET)) {
                    fprintf(stderr, "mismatched parenthesis and bracket\n");
                }
                free(cl);
            }
            else {
                fprintf(stderr, "syntax error: ')' without open s-expression\n");
            }
            // also reduce quote expressions on the stack
            cl = p->exp_stack_top;
            while (cl && cl->quote) {
                cv = cl->content;
                struct expr_lnk *tmp = cl;
                cl = cl->outer;
                free(tmp);
            }
            p->exp_stack_top = cl;
            break;
        case P_INTNUM:
            cv = make_int(p->alloc, inum);
            parser_store_cell(p, cv);
            // also reduce quote expressions on the stack
            cl = p->exp_stack_top;
            while (cl && cl->quote) {
                cv = cl->content;
                struct expr_lnk *tmp = cl;
                cl = cl->outer;
                free(tmp);
            }
            p->exp_stack_top = cl;
            break;
        case P_FLONUM:
            cv = make_float(p->alloc, fnum);
            parser_store_cell(p, cv);
            // also reduce quote expressions on the stack
            cl = p->exp_stack_top;
            while (cl && cl->quote) {
                cv = cl->content;
                struct expr_lnk *tmp = cl;
                cl = cl->outer;
                free(tmp);
            }
            p->exp_stack_top = cl;
            break;
        case P_BOOL:
            cv = inum ? VALUE_TRUE : VALUE_FALSE;
            parser_store_cell(p, cv);
            // also reduce quote expressions on the stack
            // XXX in three places, refactor
            cl = p->exp_stack_top;
            while (cl && cl->quote) {
                cv = cl->content;
                struct expr_lnk *tmp = cl;
                cl = cl->outer;
                free(tmp);
            }
            p->exp_stack_top = cl;
            break;
        case P_DOT:
            if (p->exp_stack_top->parse_pos != PP_CDR) {
                p->exp_stack_top->parse_pos = PP_CDR;
            }
            else {
                fprintf(stderr, "dot notation not supported here\n");
            }
            break;
        case P_IDENT:
            cv = make_symbol(p->alloc, str);
            parser_store_cell(p, cv);
            // also reduce quote expressions on the stack
            cl = p->exp_stack_top;
            while (cl && cl->quote) {
                cv = cl->content;
                struct expr_lnk *tmp = cl;
                cl = cl->outer;
                free(tmp);
            }
            p->exp_stack_top = cl;
            break;
        case P_STRING:
            cv = make_string(p->alloc, str);
            parser_store_cell(p, cv);
            // also reduce quote expressions on the stack
            cl = p->exp_stack_top;
            while (cl && cl->quote) {
                cv = cl->content;
                struct expr_lnk *tmp = cl;
                cl = cl->outer;
                free(tmp);
            }
            p->exp_stack_top = cl;
            break;
        case P_QUOTE:
            // XXX this is a combination of stuff above, could be refactored to
            // avoid duplication, also around reduction of quote expressions
            cv = make_cons(p->alloc, VALUE_NIL, VALUE_NIL);
            parser_store_cell(p, cv);
            cl = malloc(sizeof(struct expr_lnk));
            cl->content = cv;
            cl->implicit = 0;
            cl->quote = 1;
            cl->bracket = 0;
            cl->outer = p->exp_stack_top;
            p->exp_stack_top = cl;
            cl->parse_pos = PP_CAR;
            cv = make_symbol(p->alloc, "quote");
            parser_store_cell(p, cv);
            break;
        case P_VECTOR:
            // XXX more duplication, but subtly different. urgh
            cv = make_cons(p->alloc, VALUE_NIL, VALUE_NIL);
            parser_store_cell(p, cv);
            cl = malloc(sizeof(struct expr_lnk));
            cl->content = cv;
            cl->implicit = 0;
            cl->quote = 0;
            cl->bracket = 0;
            cl->outer = p->exp_stack_top;
            p->exp_stack_top = cl;
            cl->parse_pos = PP_CAR;
            cv = make_symbol(p->alloc, "vector");
            parser_store_cell(p, cv);
            break;
        case P_EOF:
            if (p->exp_stack_top) {
                fprintf(stderr, "end-of-file but open expression\n");
                // XXX this and other errors in parser are fatal, do something
                // about them
            }
            break;
        default:
            printf("?? %i\n", tok);
    }

    if (!p->exp_stack_top) {
        // we have a fully parsed expression, compile it
        p->callback(cv, p->cb_arg);
    }
}

/* the tokenizer takes in input string and tokenizes it, calling parse() for
 * each token. it is based on a simple state machine */
#define S_INIT       0
#define S_COMMENT    1
#define S_INTNUM     2
#define S_FLONUM     3
#define S_IDENT      4
#define S_HASH       5
#define S_PLUSMINUS  6
#define S_STRING     7
#define S_HASHIDENT  8
#define S_ESCAPE     9
#define S_DOT       10

// XXX we should be able to treat things as ".+" as identifiers, see
// 15-little-shadows.t
int parser_tokenize(struct parser *p, char *data) {
    char *cp = data;
    char *mark = NULL;
    if (       (p->tokenizer_state == S_IDENT)
            || (p->tokenizer_state == S_INTNUM)
            || (p->tokenizer_state == S_FLONUM)) {
        mark = data;
    }
    // read the data char-by-char
    while (*cp != '\0') {
        if (p->tokenizer_state == S_INIT) {
            if ((*cp == ' ') || (*cp == '\t') || (*cp == '\r') || (*cp == '\n')) {
                // ignore whitespace
            }
            else if (*cp == '(') {
                parser_parse(p, P_LPAREN, 0, 0.0, NULL);
            }
            else if (*cp == ')') {
                parser_parse(p, P_RPAREN, 0, 0.0, NULL);
            }
            else if (*cp == '[') {
                parser_parse(p, P_LBRACKET, 0, 0.0, NULL);
            }
            else if (*cp == ']') {
                parser_parse(p, P_RBRACKET, 0, 0.0, NULL);
            }
            else if (*cp == '\'') {
                parser_parse(p, P_QUOTE, 0, 0.0, NULL);
            }
            else if (*cp == ';') {
                p->tokenizer_state = S_COMMENT;
            }
            else if (*cp == '#') {
                mark = cp;
                p->tokenizer_state = S_HASH;
            }
            else if (*cp == '"') {
                mark = cp;
                p->tokenizer_state = S_STRING;
            }
            else if (*cp == '.') {
                mark = cp;
                p->tokenizer_state = S_DOT;
            }
            else if ((*cp >= '0') && (*cp <= '9')) {
                mark = cp;
                p->tokenizer_state = S_INTNUM;
            }
            else if ((*cp == '-') || (*cp == '+')) {
                mark = cp;
                p->tokenizer_state = S_PLUSMINUS;
            }
            else if (   (strchr("!$%&*/:<=>?^_~+", *cp) != NULL)
                     || ((*cp >= 'a') && (*cp <= 'z'))
                     || ((*cp >= 'A') && (*cp <= 'Z')) ) {
                mark = cp;
                p->tokenizer_state = S_IDENT;
            }
            else {
                fprintf(stderr,
                    "Unexpected character '%c' in tokenizer state: %i\n",
                    *cp, p->tokenizer_state);
                exit(1);
            }
        }
        else if (p->tokenizer_state == S_COMMENT) {
            if (*cp == '\n') {
                p->tokenizer_state = S_INIT;
            }
            else {
                // comment, ignore
            }
        }
        else if (p->tokenizer_state == S_INTNUM) {
            if ((*cp >= '0') && (*cp <= '9')) {
                // still in number
            }
            else if (*cp == '.') {
                p->tokenizer_state = S_FLONUM;
            }
            else if (   (strchr("!$%&*/:<=>?^_~", *cp) != NULL)
                || ((*cp >= 'a') && (*cp <= 'z'))
                || ((*cp >= '0') && (*cp <= '9'))
                || ((*cp >= 'A') && (*cp <= 'Z'))
                || (strchr("+-.@", *cp) != NULL) ) {
                p->tokenizer_state = S_IDENT;
            }
            else {
                // end of number
                p->tokenizer_state = S_INIT;
                char *ep = cp-1;
                int64_t num_literal = strtoll(mark, &ep, 10);
                parser_parse(p, P_INTNUM, num_literal, 0.0, NULL);
                cp--;
                mark = NULL;
            }
        }
        else if (p->tokenizer_state == S_DOT) {
            if ((*cp >= '0') && (*cp <= '9')) {
                p->tokenizer_state = S_FLONUM;
            }
            // XXX could switch to ident in some cases?
            else {
                // it was just a dot after all
                p->tokenizer_state = S_INIT;
                char *ep = cp-1;
                float num_literal = strtof(mark, &ep);
                parser_parse(p, P_DOT, 0, 0.0, NULL);
                cp--;
                mark = NULL;
            }
        }
        else if (p->tokenizer_state == S_FLONUM) {
            if ((*cp >= '0') && (*cp <= '9')) {
                // still in float
            }
            else {
                // end of float
                p->tokenizer_state = S_INIT;
                char *ep = cp-1;
                float num_literal = strtof(mark, &ep);
                parser_parse(p, P_FLONUM, 0, num_literal, NULL);
                cp--;
                mark = NULL;
            }
        }
        else if (p->tokenizer_state == S_STRING) {
            if (*cp == '\\') {
                p->tokenizer_state = S_ESCAPE;
            }
            else if (*cp == '"') {
                p->tokenizer_state = S_INIT;
                char *str = malloc(cp - mark);
                strncpy(str, mark+1, cp - mark - 1);
                str[cp - mark - 1] = '\0';
                string_deescape(str, cp - mark - 1);
                parser_parse(p, P_STRING, 0, 0.0, str);
                free(str);
                mark = NULL;
            }
        }
        else if (p->tokenizer_state == S_ESCAPE) {
            // just read one char and fall back to string state
            p->tokenizer_state = S_STRING;
        }
        else if (p->tokenizer_state == S_HASH) {
            if (*cp == '(') {
                parser_parse(p, P_VECTOR, 0, 0.0, NULL);
                p->tokenizer_state = S_INIT;
            }
            else if (*cp == '!') {
                // this is a bit weird and perhaps wrong, but allows she-bang
                // lines. we could be stricter and only accept these at the very
                // beginning of the file...
                p->tokenizer_state = S_COMMENT;
            }
            else if (   (strchr("!$%&*/:<=>?^_~+", *cp) != NULL)
                     || ((*cp >= 'a') && (*cp <= 'z'))
                     || ((*cp >= 'A') && (*cp <= 'Z')) ) {
                mark = cp;
                p->tokenizer_state = S_HASHIDENT;
            }
            else {
                fprintf(stderr,
                    "Unexpected character '%c' in tokenizer state: %i\n",
                    *cp, p->tokenizer_state);
                exit(1);
            }
        }
        else if (p->tokenizer_state == S_IDENT) {
            if (   (strchr("!$%&*/:<=>?^_~", *cp) != NULL)
                || ((*cp >= 'a') && (*cp <= 'z'))
                || ((*cp >= '0') && (*cp <= '9'))
                || ((*cp >= 'A') && (*cp <= 'Z'))
                || (strchr("+-.@", *cp) != NULL) ) {
                // still in identifier
            }
            else {
                // end of identifier
                p->tokenizer_state = S_INIT;
                // XXX if we could pass through the string differently, we could
                // avoid allocating another copy...
                char *str = malloc(cp - mark + 1);
                strncpy(str, mark, cp - mark);
                str[cp - mark] = '\0';
                parser_parse(p, P_IDENT, 0, 0.0, str);
                free(str);
                cp--;
                mark = NULL;
            }
        }
        else if (p->tokenizer_state == S_HASHIDENT) {
            if (   (strchr("!$%&*/:<=>?^_~", *cp) != NULL)
                || ((*cp >= 'a') && (*cp <= 'z'))
                || ((*cp >= '0') && (*cp <= '9'))
                || ((*cp >= 'A') && (*cp <= 'Z'))
                || (strchr("+-.@", *cp) != NULL) ) {
                // still in hash identifier
            }
            else {
                // end of identifier
                if (((cp - mark) == 1) && (mark[0] == 't')) {
                    parser_parse(p, P_BOOL, 1, 0.0, NULL);
                }
                else if (((cp - mark) == 1) && (mark[0] == 'f')) {
                    parser_parse(p, P_BOOL, 0, 0.0, NULL);
                }
                else if (((cp - mark) == 4) && (strncmp(mark, "true", 4) == 0)) {
                    parser_parse(p, P_BOOL, 1, 0.0, NULL);
                }
                else if (((cp - mark) == 5) && (strncmp(mark, "false", 5) == 0)) {
                    parser_parse(p, P_BOOL, 0, 0.0, NULL);
                }
                else {
                    char *str = malloc(cp - mark + 1);
                    strncpy(str, mark, cp - mark);
                    str[cp - mark] = '\0';
                    parser_parse(p, P_IDENT, 0, 0.0, str);
                    fprintf(stderr,
                        "Unexpected hash identifier '#%s' in tokenizer state: %i\n",
                        str, p->tokenizer_state);
                    free(str);
                    exit(1);
                }
                p->tokenizer_state = S_INIT;
                cp--;
                mark = NULL;
            }
        }
        else if (p->tokenizer_state == S_PLUSMINUS) {
            if ((*cp >= '0') && (*cp <= '9')) {
                p->tokenizer_state = S_INTNUM;
            }
            else if (*cp >= '.') {
                p->tokenizer_state = S_FLONUM;
            }
            else if (   (strchr("!$%&*/:<=>?^_~", *cp) != NULL)
                || ((*cp >= 'a') && (*cp <= 'z'))
                || ((*cp >= '0') && (*cp <= '9'))
                || ((*cp >= 'A') && (*cp <= 'Z'))
                || (strchr("+-.@", *cp) != NULL) ) {
                p->tokenizer_state = S_IDENT;
            }
            else {
                // end, treat as identifier
                p->tokenizer_state = S_INIT;
                // XXX if we could pass through the string differently, we could
                // avoid allocating another copy...
                char *str = malloc(cp - mark + 1);
                strncpy(str, mark, cp - mark);
                str[cp - mark] = '\0';
                parser_parse(p, P_IDENT, 0, 0.0, str);
                free(str);
                cp--;
                mark = NULL;
            }
        }
        else {
            fprintf(stderr, "Unexpected tokenizer state: %i\n", p->tokenizer_state);
            exit(1);
        }
        cp++;
    }
    if (mark) {
        memmove(data, mark, cp - mark + 1);
        return cp - mark;
    }
    return 0;
}

struct parser* parser_new(struct allocator *alloc, void (*callback)(value exp, void *arg), void *cb_arg) {
    struct parser *ret = malloc(sizeof(struct parser));
    ret->alloc = alloc;
    ret->tokenizer_state = S_INIT;
    ret->exp_stack_top = NULL;
    ret->callback = callback;
    ret->cb_arg = cb_arg;
    return ret;
}

void parser_set_cb_arg(struct parser *p, void *cb_arg) {
    p->cb_arg = cb_arg;
}

void parser_free(struct parser *p) {
    assert(p != NULL);
    free(p);
}

int parser_consume(struct parser *p, char *data) {
    assert(p != NULL);
    assert(data != NULL);

    return parser_tokenize(p, data);
}

void parser_eof(struct parser *p) {
    parser_parse(p, P_EOF, 0, 0.0, NULL);
}

