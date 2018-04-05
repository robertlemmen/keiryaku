#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>
#include <pwd.h>

#include "linenoise/linenoise.h"
#include "types.h"
#include "heap.h"
#include "eval.h"
#include "parse.h"
#include "version.h"
#include "global.h"

#define BUFSIZE 4096

// XXX getopt for ags, e.g. not to load base env and/or compiler

static struct option long_options[] = {
    {"help", no_argument, 0, 'h'},
    {"version", no_argument, 0, 'v'},
    {"debug", no_argument, 0, 'd'},
    {0, 0, 0, 0}
};

void usage(char *prog_name) {
    fprintf(stderr, "Usage: %s [options]\n"
        "Options:\n"
        "  --help -h -?     print this text\n"
        "  --version -v     print the program version\n"
        "  --debug -d       switch on code helpful in debug environments\n\n",
        prog_name);
}

void consume_stream(struct parser *p, FILE *f) {
    char buffer[BUFSIZE];
    int pos = 0;
    while (fgets(&buffer[pos], BUFSIZE-1-pos, f)) {
        pos = parser_consume(p, buffer, false);
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

int main(int argc, char **argv) {
    int option_index = 0;
    int c;

    char *history_file = NULL;

    while (1) {
        c = getopt_long(argc, argv, "?hvd", long_options, &option_index);
        if (c == -1) {
            break;
        }

        switch(c) {
            case 'v':
                printf("keiryaku %s\n", software_version());
                return 0;
                break;
            case 'd':
                arg_debug = 1;
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

    if (option_index < (argc-1)) {
        fprintf(stderr, "useless argument \"%s\"\n", argv[option_index]);
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
        history_file = malloc(strlen(homedir) + strlen("/.keiryaky_history") + 1);
        sprintf(history_file, "%s/.keiryaky_history", homedir);
        linenoiseHistoryLoad(history_file);
    }

    struct allocator *a = allocator_new();
    struct parser *p = parser_new(a);

    consume_file(p, "compiler.ss");
    if (isatty(fileno(stdin))) {
        char *input;
        while ((input = linenoise("> ")) != NULL) {
            parser_consume(p, input, true);
            linenoiseHistoryAdd(input);
            linenoiseFree(input);
        }
        linenoiseHistorySave(history_file);
    }
    else {
        consume_stream(p, stdin);
    }

    if (arg_debug) {
        printf("cleaning up all remaining memory..\n");
        struct allocator_gc_ctx *gc = allocator_gc_new(a);
        allocator_gc_perform(gc);
    }

    parser_free(p);
    allocator_free(a);

    if (history_file) {
        free(history_file);
    }

    return 0;
}
