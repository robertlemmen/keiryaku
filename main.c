#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include "types.h"
#include "heap.h"
#include "eval.h"
#include "parse.h"

#define BUFSIZE 4096

int main(int argc, char **argv) {
    if (isatty(fileno(stdin))) {
        printf("-=[ keiryaku ]=-\n\n");
        printf("> ");
    }

    struct allocator *a = allocator_new();
    struct parser *p = parser_new(a);

    char buffer[BUFSIZE];
    int pos = 0;
    while (fgets(&buffer[pos], BUFSIZE-1-pos, stdin)) {
        pos = parser_consume(p, buffer);
    }
/*

    value t = make_cons(a, make_int(a, 42), VALUE_EMPTY_LIST);
    value v = make_cons(a, VALUE_TRUE, t);
    assert(value_type(v) == TYPE_CONS);
    assert(car(v) == VALUE_TRUE);
    assert(value_type(cdr(v)) == TYPE_CONS);
    dump_value(v);
    printf("\n");

    v = make_symbol(a, "test234");
    assert(strcmp(symbol(v), "test234") == 0);
    dump_value(v);
    printf("\n");
*/
    parser_free(p);
    allocator_free(a);

    return 0;
}
