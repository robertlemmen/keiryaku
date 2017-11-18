#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "types.h"
#include "heap.h"

int main(int argc, char **argv) {
    printf("-=[ kenbou ]=-\n");

    struct allocator *a = allocator_new();

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

    allocator_free(a);

    return 0;
}
