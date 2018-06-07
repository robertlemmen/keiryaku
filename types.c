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
    if (strlen(s) < 7) {
        value ret = 0;
        char *sr = (char*)&ret;
        // XXX this is highly dependent on endianness
        memcpy(&sr[1], s, strlen(s));
        ret |= (strlen(s) << 4) | TYPE_SHORT_SYMBOL;
        return ret;
    }
    else {
        char *sp = allocator_alloc(a, strlen(s) + 1);
        strcpy(sp, s);
        return (uint64_t)sp | TYPE_SYMBOL;
    }
}

char* value_to_symbol(value *s) {
    assert(value_is_symbol(*s));
    if (value_type(*s) == TYPE_SYMBOL) {
        return ((char*)value_to_cell(*s));
    }
    else if (value_type(*s) == TYPE_SHORT_SYMBOL) {
        char *sr = (char*)s;
        return (char*)&sr[1];
    }
    else {
        return NULL;
    }
}

/* XXX same as above, refactor */
value make_string(struct allocator *a, char *s) {
    if (strlen(s) < 7) {
        value ret = 0;
        char *sr = (char*)&ret;
        // XXX this is highly dependent on endianness
        memcpy(&sr[1], s, strlen(s));
        ret |= (strlen(s) << 4) | TYPE_SHORT_STRING;
        return ret;
    }
    else {
        char *sp = allocator_alloc(a, strlen(s) + 1);
        strcpy(sp, s);
        return (uint64_t)sp | TYPE_STRING;
    }
}

char* value_to_string(value *s) {
    assert(value_is_string(*s));
    if (value_type(*s) == TYPE_STRING) {
        return ((char*)value_to_cell(*s));
    }
    else if (value_type(*s) == TYPE_SHORT_STRING) {
        char *sr = (char*)s;
        return (char*)&sr[1];
    }
    else {
        return NULL;
    }
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
        case TYPE_SHORT_SYMBOL:
            printf("%s", value_to_symbol(&v));
            break;
        case TYPE_STRING:
        case TYPE_SHORT_STRING:
            printf("\"%s\"", value_to_string(&v));
            break;
        case TYPE_CONS:
            printf("(");
            if ((car(v) != VALUE_NIL) || (cdr(v) != VALUE_NIL)) {
                dump_value(car(v));
                while (   (cdr(v) != VALUE_EMPTY_LIST) 
                       && (cdr(v) != VALUE_NIL)
                       && (value_type(cdr(v)) == TYPE_CONS) ) {
                    printf(" ");
                    v = cdr(v);
                    dump_value(car(v));
                }
                if ((cdr(v) != VALUE_EMPTY_LIST) && (cdr(v) != VALUE_NIL)) {
                    printf(" . ");
                    dump_value(cdr(v));
                }
            }
            printf(")");
            break;
        case TYPE_VECTOR:
            printf("#(");
            int length = vector_length(v);
            for (int i = 0; i < length; i++) {
                if (i) {
                    printf(" ");
                }
                dump_value(vector_ref(v, i));
            }
            printf(")");
            break;
        default:
            printf("<?type %li>", value_type(v));
//            assert(0 && "unsupported value type");
    }
}

struct builtin_ref {
    union {
        t_builtin1 funcptr1;
        t_builtin2 funcptr2;
        t_builtin3 funcptr3;
    };
    int arity;
};

int builtin_arity(value v) {
    struct builtin_ref *p = (struct builtin_ref*)value_to_cell(v);
    return p->arity;
}

value make_builtin1(struct allocator *a, t_builtin1 funcptr) {
    struct builtin_ref *p = allocator_alloc(a, sizeof(struct builtin_ref));
    p->funcptr1 = funcptr;
    p->arity = 1;
    value ret = (uint64_t)p | TYPE_BUILTIN;
    return ret;
}

t_builtin1 builtin1_ptr(value v) {
    assert(value_type(v) == TYPE_BUILTIN);
    struct builtin_ref *p = (struct builtin_ref*)value_to_cell(v);
    assert(p->arity == 1);
    return p->funcptr1;
}

value make_builtin2(struct allocator *a, t_builtin2 funcptr) {
    struct builtin_ref *p = allocator_alloc(a, sizeof(struct builtin_ref));
    p->funcptr2 = funcptr;
    p->arity = 2;
    value ret = (uint64_t)p | TYPE_BUILTIN;
    return ret;
}

t_builtin2 builtin2_ptr(value v) {
    assert(value_type(v) == TYPE_BUILTIN);
    struct builtin_ref *p = (struct builtin_ref*)value_to_cell(v);
    assert(p->arity == 2);
    return p->funcptr2;
}

value make_builtin3(struct allocator *a, t_builtin3 funcptr) {
    struct builtin_ref *p = allocator_alloc(a, sizeof(struct builtin_ref));
    p->funcptr3 = funcptr;
    p->arity = 3;
    value ret = (uint64_t)p | TYPE_BUILTIN;
    return ret;
}

t_builtin3 builtin3_ptr(value v) {
    assert(value_type(v) == TYPE_BUILTIN);
    struct builtin_ref *p = (struct builtin_ref*)value_to_cell(v);
    assert(p->arity == 3);
    return p->funcptr3;
}

value make_builtinv(struct allocator *a, t_builtinv funcptr) {
    struct builtin_ref *p = allocator_alloc(a, sizeof(struct builtin_ref));
    p->funcptr1 = funcptr;
    p->arity = 0;
    value ret = (uint64_t)p | TYPE_BUILTIN;
    return ret;
}

t_builtinv builtinv_ptr(value v) {
    assert(value_type(v) == TYPE_BUILTIN);
    struct builtin_ref *p = (struct builtin_ref*)value_to_cell(v);
    assert(p->arity == 0);
    return p->funcptr1;
}

value make_vector(struct allocator *a, int length, value fill) {
    value *ret = allocator_alloc(a, (length + 1) * sizeof(value));
    ret[0] = make_int(a, length);
    for (int i = 0; i < length; i++) {
        ret[i+1] = fill;
    }
    return (uint64_t)ret | TYPE_VECTOR;
}

int vector_length(value v) {
    assert(value_type(v) == TYPE_VECTOR);
    value *body = value_to_cell(v);
    return intval(body[0]);
}

value vector_ref(value v, int pos) {
    assert(value_type(v) == TYPE_VECTOR);
    value *body = value_to_cell(v);
    int length = intval(body[0]);
    assert(pos >= 0);
    assert(pos < length);
    return body[pos+1];
}

void vector_set(value v, int pos, value i) {
    assert(value_type(v) == TYPE_VECTOR);
    value *body = value_to_cell(v);
    int length = intval(body[0]);
    assert(pos >= 0);
    assert(pos < length);
    body[pos+1] = i;
}

void traverse_vector(struct allocator_gc_ctx *gc, value v) {
    assert(value_type(v) == TYPE_VECTOR);
    value *body = value_to_cell(v);
    int length = intval(body[0]);
    for (int i = 0; i < length; i++) {
        allocator_gc_add_root(gc, body[i+1]);
    }
}
