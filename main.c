#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>

#include "types.h"
#include "heap.h"
#include "eval.h"
#include "parse.h"
#include "version.h"

#define BUFSIZE 4096

// XXX getopt for ags, e.g. not to load base env and/or compiler

void consume_stream(struct parser *p, FILE *f) {
    char buffer[BUFSIZE];
    int pos = 0;
    while (fgets(&buffer[pos], BUFSIZE-1-pos, f)) {
        pos = parser_consume(p, buffer, isatty(fileno(f)));
    }
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
    if (isatty(fileno(stdin))) {
        printf("-=[ keiryaku %s ]=-\n\n", software_version());
        printf("> ");
    }

    struct allocator *a = allocator_new();
    struct parser *p = parser_new(a);

    consume_file(p, "compiler.ss");
    consume_stream(p, stdin);

    parser_free(p);
    allocator_free(a);

    return 0;
}
