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

    ret->env = env_bind(ret->env, make_symbol(ret->alloc, "if"), VALUE_SP_IF);
    ret->env = env_bind(ret->env, make_symbol(ret->alloc, "begin"), VALUE_SP_BEGIN);
    ret->env = env_bind(ret->env, make_symbol(ret->alloc, "quote"), VALUE_SP_QUOTE);

    return ret;
}

void interp_free(struct interp_ctx *i) {
    assert(i != NULL);
    free(i);
}

#define INTERP_COLLECT_MAX_ARGS     5
value interp_collect_list_temp[INTERP_COLLECT_MAX_ARGS];
value* interp_collect_list(value expr, int count) {
    assert(count <= INTERP_COLLECT_MAX_ARGS);
    int pos = 0;
    value ca = expr;
    while (ca && (value_type(ca) == TYPE_CONS)) {
        if (pos < count) {
            interp_collect_list_temp[pos] = car(ca);
        }
        pos++;
        ca = cdr(ca);
    }
    if (pos > count) {
        fprintf(stderr, "interp_collect_list with arity %i, but found list of length %i\n",
            count, pos);
        return NULL;
    }
    return interp_collect_list_temp;
}

value interp_apply_special(struct interp_ctx *i, value special, value args) {
    value *pos_args;
    switch (special) {
        case VALUE_SP_IF:
            pos_args = interp_collect_list(args, 3);
            if (value_is_true(interp_eval(i, pos_args[0]))) {
                return interp_eval(i, pos_args[1]);
            }
            else {
                return interp_eval(i, pos_args[2]);
            }
            break;
        case VALUE_SP_BEGIN:;
            value cret = VALUE_NIL;
            while (value_type(args ) == TYPE_CONS) {
                cret = interp_eval(i, car(args));
                args = cdr(args);
            }
            if (args != VALUE_EMPTY_LIST) {
                fprintf(stderr, "arguments to BEGIN are nore well-formed list\n");
            }
            return cret;
        case VALUE_SP_QUOTE:
            pos_args = interp_collect_list(args, 1);
            return pos_args[0];
            break;
        default:
            fprintf(stderr, "Unknown special 0x%lX\n", special);
            return VALUE_NIL;
    }
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
            else if (value_is_special(op)) {
                return interp_apply_special(i, op, cdr(expr));
            }
            else if (value_type(op) == TYPE_BUILTIN2) {
                t_builtin2 funcptr = builtin2_ptr(op);
                value *pos_args = interp_collect_list(cdr(expr), 2);
                return funcptr(i->alloc, pos_args[0], pos_args[1]);
            }
            else {
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
