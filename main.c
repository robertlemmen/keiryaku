#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>
#include <pwd.h>
// XXX just for the elapsed time
#include <time.h>

#include "heap.h"
#include "parse.h"
#include "interp.h"
#include "version.h"
#include "global.h"
#include "ports.h"

#define BUFSIZE 4096

static struct option long_options[] = {
    {"help", no_argument, 0, 'h'},
    {"version", no_argument, 0, 'v'},
    {"debug", no_argument, 0, 'd'},
    {"debug-compiler", no_argument, 0, 'D'},
    {"no-compiler", no_argument, 0, 'N'},
    {"gc-threshold", required_argument, 0, 'g'},
    {0, 0, 0, 0}
};

void usage(char *prog_name) {
    fprintf(stderr, "Usage: %s [options] [--] [script [arguments]]\n"
        "Options:\n"
        "  --help -h -?         print this text\n"
        "  --version -v         print the program version\n"
        "  --debug -d           switch on code helpful in debug environments\n"
        "  --debug-compiler -D  show input and output of compile stage\n"
        "  --no-compiler -N     do not load compiler, bare interpreter\n"
        "  --gc-threshold <int> set the threshold for GC pressure to specified number\n\n",
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
    value comp_expr = make_cons(pargs->a, make_symbol(pargs->a, "quote"),
                                          make_cons(pargs->a, expr, VALUE_EMPTY_LIST));
    comp_expr = make_cons(pargs->a, make_symbol(pargs->a, "_compile"), 
                                    make_cons(pargs->a, comp_expr, VALUE_EMPTY_LIST));
    expr = interp_eval(pargs->i, comp_expr);
    // execute it!
    interp_eval(pargs->i, expr);
}

int main(int argc, char **argv) {
    int c;
    char *history_file = NULL;
    bool load_compiler = true;
    char *script_file = NULL;

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    if (getenv("KEIRYAKU_GC_THRESHOLD")) {
        arg_gc_threshold = atoi(getenv("KEIRYAKU_GC_THRESHOLD"));
    }

    while (1) {
        c = getopt_long(argc, argv, "?hvdNDg:", long_options, NULL);
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
            case 'g':
                arg_gc_threshold = atoi(optarg);
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

    for (; optind < argc; optind++) {
        // XXX currenty we only grab the first extra argument, need to work out
        // how to pass more aguments to a scheme script
        if (!script_file) {
            script_file = argv[optind];
        }
    }

    struct allocator *a = allocator_new();
    struct interp *i = interp_new(a);
    struct parser_cb_args pargs;
    pargs.i = i;
    pargs.a = a;
    struct parser *p = parser_new(a, &parser_callback, &pargs);

    env_bind(a, interp_top_env(i), 
        make_symbol(a, "stdout"),
        port_new(a, stdout, false, true, true, false));
    env_bind(a, interp_top_env(i), 
        make_symbol(a, "stderr"),
        port_new(a, stderr, false, true, true, false));
    env_bind(a, interp_top_env(i),
        make_symbol(a, "_arg-debug-compiler"),
        arg_debug_compiler ? VALUE_TRUE : VALUE_FALSE);

    if (script_file) {
        // script mode, load the file directly
        env_bind(a, interp_top_env(i), 
            make_symbol(a, "stdin"),
            port_new(a, stdin, true, false, true, false));

        if (load_compiler) {
            consume_file(p, "compiler.ss");
        }
        consume_file(p, script_file);
    }
    else {
        // repl or stdin mode
        if (isatty(fileno(stdin))) {
            printf("-=[ keiryaku %s ]=-\n\n", software_version());
        }

        if (isatty(fileno(stdin))) {
            env_bind(a, interp_top_env(i), 
                make_symbol(a, "stdin"),
                port_new_tty(a));
        }
        else {
            env_bind(a, interp_top_env(i), 
                make_symbol(a, "stdin"),
                port_new(a, stdin, true, false, true, false));
        }

        if (load_compiler) {
            consume_file(p, "compiler.ss");
        }
        consume_file(p, "repl.ss");
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

    clock_gettime(CLOCK_MONOTONIC, &end);
    long elapsed_us =   (end.tv_sec * 1000000 + end.tv_nsec / 1000)
                      - (start.tv_sec * 1000000 + start.tv_nsec / 1000);
    fprintf(stderr, "# total elapsed: %7lius\n", elapsed_us);
    fprintf(stderr, "# total GC time: %7lius\n", total_gc_time_us);

    return 0;
}
