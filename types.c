#include "types.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>

value make_cons(struct allocator *a, value car, value cdr) {
    assert(sizeof(struct cons) == 16);
    struct cons *cp = allocator_alloc(a, sizeof(struct cons));
    cp->car = car;
    cp->cdr = cdr;
    return (uint64_t)cp | TYPE_CONS;
}

value make_symbol(struct allocator *a, char *s) {
    char *sp = allocator_alloc(a, strlen(s) + 1);
    strcpy(sp, s);
    return (uint64_t)sp | TYPE_SYMBOL;
}

void dump_value(value v) {
    switch (value_type(v)) {
        case TYPE_INT:
            printf("%i", intval(v));
            break;
        case TYPE_FLOAT:
            printf("%f", floatval(v));
            break;
        case TYPE_ENUM:
            switch (v) {
                case VALUE_NIL:
                    // XXX
                    break;
                case VALUE_TRUE:
                    printf("#t");
                    break;
                case VALUE_FALSE:
                    printf("#f");
                    break;
                case VALUE_EMPTY_LIST:
                    printf("()");
                    break;
                default:
                    printf("<?enum %li>", v);
                    //assert(0 && "unsupported enum value");
            }
            break;
        case TYPE_SYMBOL:
            printf("%s", value_to_symbol(v));
            break;
        case TYPE_CONS:
            printf("(");
            dump_value(car(v));
            while (   (cdr(v) != VALUE_EMPTY_LIST) 
                   && (value_type(cdr(v)) == TYPE_CONS) ) {
                printf(" ");
                v = cdr(v);
                dump_value(car(v));
            }
            if (cdr(v) != VALUE_EMPTY_LIST) {
                printf(" . ");
                dump_value(cdr(v));
            }
            printf(")");
            break;
        default:
            printf("<?type %li>", value_type(v));
//            assert(0 && "unsupported value type");
    }
}

value make_builtin2(struct allocator *a, t_builtin2 funcptr) {
    // XXX why can't this be an immediate?
    t_builtin2 *p = allocator_alloc(a, sizeof(t_builtin2));
    *p = funcptr;
    value ret = (uint64_t)p | TYPE_BUILTIN2;
    return ret;
}

t_builtin2 builtin2_ptr(value v) {
    assert(value_type(v) == TYPE_BUILTIN2);
    t_builtin2 *p = (t_builtin2*)value_to_cell(v);
    return *p;
}
