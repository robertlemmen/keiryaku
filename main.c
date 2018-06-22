#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>
#include <pwd.h>

#include "linenoise/linenoise.c"
#include "heap.h"
#include "parse.h"
#include "interp.h"
#include "version.h"
#include "global.h"

// XXX experiments
#include "ports.h"

#define BUFSIZE 4096

static struct option long_options[] = {
    {"help", no_argument, 0, 'h'},
    {"version", no_argument, 0, 'v'},
    {"debug", no_argument, 0, 'd'},
    {"debug-compiler", no_argument, 0, 'D'},
    {"no-compiler", no_argument, 0, 'N'},
    {0, 0, 0, 0}
};

void usage(char *prog_name) {
    fprintf(stderr, "Usage: %s [options]\n"
        "Options:\n"
        "  --help -h -?         print this text\n"
        "  --version -v         print the program version\n"
        "  --debug -d           switch on code helpful in debug environments\n"
        "  --debug-compiler -D  show input and output of compile stage\n"
        "  --no-compiler -N     do not load compiler, bare interpreter\n\n",
        prog_name);
}

void consume_stream(struct parser *p, FILE *f) {
    char buffer[BUFSIZE];
    int pos = 0;
    while (fgets(&buffer[pos], BUFSIZE-1-pos, f)) {
        pos = parser_consume(p, buffer);
    }
    parser_eof(p);
}

void consume_file(struct parser *p, char *fname) {
    FILE *fs = fopen(fname, "r");
    if (!fs) {
        fprintf(stderr, "Could not open file '%s': %s\n", fname, strerror(errno));
        exit(1);
    }
    consume_stream(p, fs);
    fclose(fs);
}

struct parser_cb_args {
    struct interp *i;
    struct allocator *a;
};

// XXX with some of this in here, some configs may not be globals anymore
static void parser_callback(value expr, void *arg) {
    struct parser_cb_args *pargs = (struct parser_cb_args*)arg;

    if (arg_debug_compiler) {
        printf("// parsed expression: ");
        dump_value(expr, stdout);
        printf("\n");
    }
    value comp_expr = make_cons(pargs->a, make_symbol(pargs->a, "quote"),
                                          make_cons(pargs->a, expr, VALUE_EMPTY_LIST));
    comp_expr = make_cons(pargs->a, make_symbol(pargs->a, "_compile"), 
                                    make_cons(pargs->a, comp_expr, VALUE_EMPTY_LIST));
    expr = interp_eval(pargs->i, comp_expr);
    if (arg_debug_compiler) {
        printf("// compiled expression: ");
        dump_value(expr, stdout);
        printf("\n");
    }
    // we now have something potentially executable, so evaluate it
    value result = interp_eval(pargs->i, expr);
    if (result != VALUE_NIL) {
        dump_value(result, stdout);
        printf("\n");
    }
}

int main(int argc, char **argv) {
    int c;
    char *history_file = NULL;
    bool load_compiler = true;

    while (1) {
        c = getopt_long(argc, argv, "?hvdND", long_options, NULL);
        if (c == -1) {
            break;
        }

        switch(c) {
            case 'v':
                printf("keiryaku %s\n", software_version());
                return 0;
                break;
            case 'd':
                arg_debug = true;
                break;
            case 'D':
                arg_debug_compiler = true;
                break;
            case 'N':
                load_compiler = false;
                break;
            case 'h':
            case '?':
                usage(argv[0]);
                return 0;
            default:
                usage(argv[0]);
                return 1;
        }
    }

    if (arg_debug_compiler && ! load_compiler) {
        fprintf(stderr, "arguments --debug-compiler and --no-compiler are mutually exclusive\n");
        usage(argv[0]);
        return 1;
    }

    if (optind < argc) {
        fprintf(stderr, "useless argument \"%s\"\n", argv[optind]);
        usage(argv[0]);
        return 1;
    }

    if (isatty(fileno(stdin))) {
        printf("-=[ keiryaku %s ]=-\n\n", software_version());
        linenoiseSetMultiLine(1);
        linenoiseHistorySetMaxLen(1024);
        const char *homedir;
        if ((homedir = getenv("HOME")) == NULL) {
            homedir = getpwuid(getuid())->pw_dir;
        }
        history_file = malloc(strlen(homedir) + strlen("/.keiryaku_history") + 1);
        sprintf(history_file, "%s/.keiryaku_history", homedir);
        linenoiseHistoryLoad(history_file);
    }

    struct allocator *a = allocator_new();
    struct interp *i = interp_new(a);
    struct parser_cb_args pargs;
    pargs.i = i;
    pargs.a = a;
    struct parser *p = parser_new(a, &parser_callback, &pargs);

    // XXX debug/experiments
    env_bind(a, interp_top_env(i), 
        make_symbol(a, "stdin"),
        port_new(a, stdin, true, false, true, false));
    env_bind(a, interp_top_env(i), 
        make_symbol(a, "stdout"),
        port_new(a, stdout, false, true, true, false));
    env_bind(a, interp_top_env(i), 
        make_symbol(a, "stderr"),
        port_new(a, stderr, false, true, true, false));

    if (load_compiler) {
        consume_file(p, "compiler.ss");
    }

    if (isatty(fileno(stdin))) {
        char *input;
        char buffer[BUFSIZE];
        int pos = 0;
        while ((input = linenoise("> ")) != NULL) {
            strncpy(&buffer[pos], input, BUFSIZE-pos-1);
            pos = parser_consume(p, buffer);
            strncpy(&buffer[pos], "\n", BUFSIZE-pos-1);
            pos = parser_consume(p, buffer);
            linenoiseHistoryAdd(input);
            free(input);
        }
        linenoiseHistorySave(history_file);
    }
    else {
        consume_stream(p, stdin);
    }

    if (arg_debug) {
        interp_gc(i);
    }

    parser_free(p);
    interp_free(i);
    allocator_free(a);

    if (history_file) {
        free(history_file);
    }

    return 0;
}
