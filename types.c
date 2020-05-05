#include "types.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "heap.h"

struct fvalu {
    union {
        uint64_t v;
        float f;
    };
};

value make_float(struct allocator *a, float x) {
    struct fvalu fuv;
    fuv.f = x;
    return (fuv.v << 32) | TYPE_FLOAT;
}

float floatval(value v) {
    assert(value_is_float(v));
    struct fvalu fuv;
    fuv.v = v >> 32;
    return fuv.f;
}

value make_cons(struct allocator *a, value car, value cdr) {
    assert(sizeof(struct cons) == 16);
    struct cons *cp = allocator_alloc(a, sizeof(struct cons));
    cp->car = car;
    cp->cdr = cdr;
    return (uint64_t)cp | TYPE_CONS;
}

void set_car(struct allocator *a, value c, value n) {
    ((struct cons*)value_to_cell(c))->car = n;
    write_barrier(a, c, &((struct cons*)value_to_cell(c))->car);
}

void set_cdr(struct allocator *a, value c, value n) {
    ((struct cons*)value_to_cell(c))->cdr = n;
    write_barrier(a, c, &((struct cons*)value_to_cell(c))->cdr);
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

value make_stringn(struct allocator *a, char *s, int n) {
    if (n < 7) {
        value ret = 0;
        char *sr = (char*)&ret;
        memcpy(&sr[1], s, n);
        ret |= (n << 4) | TYPE_SHORT_STRING;
        return ret;
    }
    else {
        char *sp = allocator_alloc(a, n + 1);
        strncpy(sp, s, n);
        sp[n] = '\0';
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

value make_lookup_vector(struct allocator *a, uint16_t envs, uint16_t entries) {
    return ((uint64_t)envs << 32) | ((uint64_t)entries << 16) | TYPE_LOOKUP_VECTOR;
}

uint16_t lookup_vector_envs(value l) {
    assert(value_type(l) == TYPE_LOOKUP_VECTOR);
    return (l >> 32) & 0xffff;
}

uint16_t lookup_vector_entries(value l) {
    assert(value_type(l) == TYPE_LOOKUP_VECTOR);
    return (l >> 16) & 0xffff;
}

// XXX this needs to move to ports
void dump_value(value v, FILE *f) {
    switch (value_type(v)) {
        case TYPE_INT:
            fprintf(f, "%li", intval(v));
            break;
        case TYPE_FLOAT:
            fprintf(f, "%g", floatval(v));
            break;
        case TYPE_ENUM:
            switch (v) {
                case VALUE_NIL:
                    break;
                case VALUE_TRUE:
                    fprintf(f, "#t");
                    break;
                case VALUE_FALSE:
                    fprintf(f, "#f");
                    break;
                case VALUE_EMPTY_LIST:
                    fprintf(f, "()");
                    break;
                case VALUE_EOF:
                    fprintf(f, "<end-of-file>");
                    break;
                default:
                    fprintf(f, "<?enum %li>", v);
            // XXX we should really have a type->string function to make this
            // kind of output neater
                    //assert(0 && "unsupported enum value");
            }
            break;
        case TYPE_SYMBOL:
        case TYPE_SHORT_SYMBOL:
            fprintf(f, "%s", value_to_symbol(&v));
            break;
        case TYPE_STRING:
        case TYPE_SHORT_STRING:
            fprintf(f, "\"%s\"", value_to_string(&v));
            break;
        case TYPE_CONS:
            fprintf(f, "(");
            if (1 || (car(v) != VALUE_NIL) || (cdr(v) != VALUE_NIL)) {
                dump_value(car(v), f);
                while (   (cdr(v) != VALUE_EMPTY_LIST)
                       && (cdr(v) != VALUE_NIL)
                       && (value_type(cdr(v)) == TYPE_CONS) ) {
                    fprintf(f, " ");
                    v = cdr(v);
                    dump_value(car(v), f);
                }
                if ((cdr(v) != VALUE_EMPTY_LIST) && (cdr(v) != VALUE_NIL)) {
                    fprintf(f, " . ");
                    dump_value(cdr(v), f);
                }
            }
            fprintf(f, ")");
            break;
        case TYPE_VECTOR:
            fprintf(f, "#(");
            int length = vector_length(v);
            for (int i = 0; i < length; i++) {
                if (i) {
                    fprintf(f, " ");
                }
                dump_value(vector_ref(v, i), f);
            }
            fprintf(f, ")");
            break;
        default:
            // XXX we should really have a type->string function to make this
            // kind of output neater
            if (value_type(v) == TYPE_BOXED) {
                fprintf(f, "<?type boxed 0x%X>", value_subtype(v));
            }
            else {
                fprintf(f, "<?type 0x%lX>", value_type(v));
            }
    }
    // XXX this isn't right, we should really check if this is a tty and then
    // add to linenoise rather than just print and flush
    fflush(f);
}

void dump_string_value(value v, FILE *f) {
    assert(value_is_string(v));
    fprintf(f, "%s", value_to_string(&v));
    // XXX this isn't right, we should really check if this is a tty and then
    // add to linenoise rather than just print and flush
    fflush(f);
}

struct builtin_ref {
    union {
        t_builtin0 funcptr0;
        t_builtin1 funcptr1;
        t_builtin2 funcptr2;
        t_builtin3 funcptr3;
    };
    int arity;
    char *name;
};

int builtin_arity(value v) {
    struct builtin_ref *p = (struct builtin_ref*)value_to_cell(v);
    return p->arity;
}

char* builtin_name(value v) {
    struct builtin_ref *p = (struct builtin_ref*)value_to_cell(v);
    return p->name;
}

value make_builtin0(struct allocator *a, t_builtin0 funcptr, char *name) {
    struct builtin_ref *p = allocator_alloc(a, sizeof(struct builtin_ref));
    p->funcptr0 = funcptr;
    p->arity = 0;
    p->name = name;
    return (uint64_t)p | TYPE_BUILTIN;
}

t_builtin0 builtin0_ptr(value v) {
    assert(value_type(v) == TYPE_BUILTIN);
    struct builtin_ref *p = (struct builtin_ref*)value_to_cell(v);
    assert(p->arity == 0);
    return p->funcptr0;
}

value make_builtin1(struct allocator *a, t_builtin1 funcptr, char *name) {
    struct builtin_ref *p = allocator_alloc(a, sizeof(struct builtin_ref));
    p->funcptr1 = funcptr;
    p->arity = 1;
    p->name = name;
    return (uint64_t)p | TYPE_BUILTIN;
}

t_builtin1 builtin1_ptr(value v) {
    assert(value_type(v) == TYPE_BUILTIN);
    struct builtin_ref *p = (struct builtin_ref*)value_to_cell(v);
    assert(p->arity == 1);
    return p->funcptr1;
}

value make_builtin2(struct allocator *a, t_builtin2 funcptr, char *name) {
    struct builtin_ref *p = allocator_alloc(a, sizeof(struct builtin_ref));
    p->funcptr2 = funcptr;
    p->arity = 2;
    p->name = name;
    return (uint64_t)p | TYPE_BUILTIN;
}

t_builtin2 builtin2_ptr(value v) {
    assert(value_type(v) == TYPE_BUILTIN);
    struct builtin_ref *p = (struct builtin_ref*)value_to_cell(v);
    assert(p->arity == 2);
    return p->funcptr2;
}

value make_builtin3(struct allocator *a, t_builtin3 funcptr, char *name) {
    struct builtin_ref *p = allocator_alloc(a, sizeof(struct builtin_ref));
    p->funcptr3 = funcptr;
    p->arity = 3;
    p->name = name;
    return (uint64_t)p | TYPE_BUILTIN;
}

t_builtin3 builtin3_ptr(value v) {
    assert(value_type(v) == TYPE_BUILTIN);
    struct builtin_ref *p = (struct builtin_ref*)value_to_cell(v);
    assert(p->arity == 3);
    return p->funcptr3;
}

value make_builtinv(struct allocator *a, t_builtinv funcptr, char *name) {
    struct builtin_ref *p = allocator_alloc(a, sizeof(struct builtin_ref));
    p->funcptr1 = funcptr;
    p->arity = BUILTIN_ARITY_VARIADIC;
    p->name = name;
    value ret = (uint64_t)p | TYPE_BUILTIN;
    return ret;
}

t_builtinv builtinv_ptr(value v) {
    assert(value_type(v) == TYPE_BUILTIN);
    struct builtin_ref *p = (struct builtin_ref*)value_to_cell(v);
    assert(p->arity == BUILTIN_ARITY_VARIADIC);
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
        allocator_gc_add_root_fp(gc, &body[i+1]);
    }
}

value make_environment(struct allocator *a, struct interp_env *env) {
    assert(*(uint8_t*)env == SUBTYPE_ENV); // XXX we should have a reusable define for this
    return (uint64_t)env | TYPE_BOXED;
}

struct interp_env* value_to_environment(value v) {
    assert(value_type(v) == TYPE_BOXED);
    struct interp_env *ret = value_to_cell(v);
    assert(*(uint8_t*)ret == SUBTYPE_ENV);
    return ret;
}

value make_env_entry(struct allocator *a, struct interp_env_entry *entry) {
    assert(*(uint8_t*)entry == SUBTYPE_ENV_ENTRY);
    return (uint64_t)entry | TYPE_BOXED;
}

struct interp_env_entry* value_to_env_entry(value v) {
    assert(value_type(v) == TYPE_BOXED);
    struct interp_env_entry *ret = value_to_cell(v);
    assert(*(uint8_t*)ret == SUBTYPE_ENV_ENTRY);
    return ret;
}

value make_dyn_frame(struct allocator *a, struct dynamic_frame *df) {
    assert(*(uint8_t*)df == SUBTYPE_DYN_FRAME);
    return (uint64_t)df | TYPE_BOXED;
}

struct dynamic_frame* value_to_dyn_frame(value v) {
    assert(value_type(v) == TYPE_BOXED);
    struct dynamic_frame *ret = value_to_cell(v);
    assert(*(uint8_t*)ret == SUBTYPE_DYN_FRAME);
    return ret;
}

struct param_box {
    uint8_t sub_type;
    struct param param;
};

value make_parameter(struct allocator *a, value init, value convert) {
    struct param_box *ret = allocator_alloc(a, sizeof(struct param_box));
    ret->sub_type = SUBTYPE_PARAM;
    ret->param.init = init;
    ret->param.convert = convert;
    return (uint64_t)ret | TYPE_BOXED;
}

struct param* value_to_parameter(value v) {
    assert(value_type(v) == TYPE_BOXED);
    struct param_box *box = (struct param_box*)value_to_cell(v);
    assert(box->sub_type == SUBTYPE_PARAM);
    return &box->param;
}
