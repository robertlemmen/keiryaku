#include "interp.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

struct interp_env {
    struct interp_env *outer;
    value name;
    value value;
};

struct interp_ctx {
    struct allocator *alloc;
    struct interp_env *env;
};

value env_lookup(struct interp_env *env, value symbol) {
    while (env) {
        assert(value_type(env->name) == TYPE_SYMBOL);
        if (strcmp(symbol(env->name), symbol(symbol)) == 0) {
            return env->value;
        }
        env = env->outer;
    }
    fprintf(stderr, "Could not find symbol '%s' in environment\n", symbol(symbol));
    return VALUE_NIL;
}

struct interp_env* env_bind(struct interp_env *env, value symbol, value value) {
    assert(value_type(symbol) == TYPE_SYMBOL);
    struct interp_env *ret = malloc(sizeof(struct interp_env));
    ret->outer = env;
    ret->name = symbol;
    ret->value = value;
    return ret;
}

value builtin_plus(struct allocator *alloc, value a, value b) {
    assert(value_type(a) == TYPE_INT);
    assert(value_type(b) == TYPE_INT);
   
    return make_int(alloc, intval(a) + intval(b));
}

struct interp_ctx* interp_new(struct allocator *alloc) {
    struct interp_ctx *ret = malloc(sizeof(struct interp_ctx));
    ret->alloc = alloc;
    ret->env = NULL;

    // XXX create basic environment
    ret->env = env_bind(ret->env, make_symbol(ret->alloc, "+"), make_builtin2(ret->alloc, &builtin_plus));

    return ret;
}

void interp_free(struct interp_ctx *i) {
    assert(i != NULL);
    free(i);
}

value interp_eval(struct interp_ctx *i, value expr) {
    assert(i != NULL);

    switch (value_type(expr)) {
        case TYPE_INT:
        case TYPE_FLOAT:
        case TYPE_ENUM:
            return expr;
            break;

        case TYPE_SYMBOL:
            return env_lookup(i->env, expr);
            break;

        case TYPE_CONS:;
            value op = interp_eval(i, car(expr));
            if (op == VALUE_NIL) {
                return op;
            }
            switch (value_type(op)) {
                case TYPE_BUILTIN2:;
                    // collect arguments
                    int arg_count = 0;
                    value pos_args[2];
                    value ca = cdr(expr);
                    while (ca && (value_type(ca) == TYPE_CONS)) {
                        if (arg_count < 2) {
                            pos_args[arg_count] = car(ca);
                        }
                        arg_count++;
                        ca = cdr(ca);
                    }
                    if (arg_count != 2) {
                        fprintf(stderr, "Trying to call builtin with arity 2 with %i arg_count args\n",
                            arg_count);
                        return VALUE_NIL;    
                    }
                    t_builtin2 funcptr = builtin2_ptr(op);
                    return funcptr(i->alloc, pos_args[0], pos_args[1]);
                    break;
                default:
                    fprintf(stderr, "No idea how to apply operator of type 0x%lX\n", value_type(op));
                    return VALUE_NIL;
            }
            break;

        default:
            fprintf(stderr, "Unexpected value type 0x%lX in interp_eval\n", value_type(expr));
            return VALUE_NIL;
    }

    return expr;
}
