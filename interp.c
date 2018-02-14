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

struct interp_lambda {
    int arity;
    value *arg_names;
    value body;
    struct interp_env *env;
};

value env_lookup(struct interp_env *env, value symbol) {
    while (env) {
        assert(value_type(env->name) == TYPE_SYMBOL);
        if (strcmp(value_to_symbol(env->name), value_to_symbol(symbol)) == 0) {
            return env->value;
        }
        env = env->outer;
    }
    fprintf(stderr, "Could not find symbol '%s' in environment\n", value_to_symbol(symbol));
    return VALUE_NIL;
}

struct interp_env* env_bind(struct allocator *alloc, struct interp_env *env, value symbol, value value) {
    assert(value_type(symbol) == TYPE_SYMBOL);
    struct interp_env *ret = allocator_alloc(alloc, (sizeof(struct interp_env)));
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

value builtin_minus(struct allocator *alloc, value a, value b) {
    assert(value_type(a) == TYPE_INT);
    assert(value_type(b) == TYPE_INT);

    return make_int(alloc, intval(a) - intval(b));
}

value builtin_mul(struct allocator *alloc, value a, value b) {
    assert(value_type(a) == TYPE_INT);
    assert(value_type(b) == TYPE_INT);

    return make_int(alloc, intval(a) * intval(b));
}

value builtin_div(struct allocator *alloc, value a, value b) {
    assert(value_type(a) == TYPE_INT);
    assert(value_type(b) == TYPE_INT);

    return make_int(alloc, intval(a) / intval(b));
}

value builtin_equals(struct allocator *alloc, value a, value b) {
    assert(value_type(a) == TYPE_INT);
    assert(value_type(b) == TYPE_INT);

    return intval(a) == intval(b) ? VALUE_TRUE : VALUE_FALSE;
}

struct interp_ctx* interp_new(struct allocator *alloc) {
    struct interp_ctx *ret = malloc(sizeof(struct interp_ctx));
    ret->alloc = alloc;
    ret->env = NULL;

    // create basic environment
    ret->env = env_bind(alloc, ret->env, make_symbol(ret->alloc, "+"), make_builtin2(ret->alloc, &builtin_plus));
    ret->env = env_bind(alloc, ret->env, make_symbol(ret->alloc, "-"), make_builtin2(ret->alloc, &builtin_minus));
    ret->env = env_bind(alloc, ret->env, make_symbol(ret->alloc, "*"), make_builtin2(ret->alloc, &builtin_mul));
    ret->env = env_bind(alloc, ret->env, make_symbol(ret->alloc, "/"), make_builtin2(ret->alloc, &builtin_div));
    ret->env = env_bind(alloc, ret->env, make_symbol(ret->alloc, "="), make_builtin2(ret->alloc, &builtin_equals));

    ret->env = env_bind(alloc, ret->env, make_symbol(ret->alloc, "if"), VALUE_SP_IF);
    ret->env = env_bind(alloc, ret->env, make_symbol(ret->alloc, "define"), VALUE_SP_DEFINE);
    ret->env = env_bind(alloc, ret->env, make_symbol(ret->alloc, "lambda"), VALUE_SP_LAMBDA);
    ret->env = env_bind(alloc, ret->env, make_symbol(ret->alloc, "begin"), VALUE_SP_BEGIN);
    ret->env = env_bind(alloc, ret->env, make_symbol(ret->alloc, "quote"), VALUE_SP_QUOTE);
    ret->env = env_bind(alloc, ret->env, make_symbol(ret->alloc, "let"), VALUE_SP_LET);

    return ret;
}

void interp_free(struct interp_ctx *i) {
    assert(i != NULL);
    free(i);
}

int interp_collect_list(value expr, int count, value *collector) {
    assert(collector != NULL);
    assert(count > 0);
    int pos = 0;
    value ca = expr;
    while (value_type(ca) == TYPE_CONS) {
        if (pos < count) {
            collector[pos] = car(ca);
        }
        pos++;
        ca = cdr(ca);
    }
    return pos;
}

int interp_count_list(value expr) {
    int ret = 0;
    while (value_type(expr) == TYPE_CONS) {
        ret++;
        expr = cdr(expr);
    }
    return ret;
}

value interp_apply_special(struct interp_ctx *i, value special, value args) {
    value pos_args[3];
    int arg_count;
    switch (special) {
        case VALUE_SP_IF:
            arg_count = interp_collect_list(args, 3, pos_args);
            if (arg_count != 3) {
                fprintf(stderr, "Arity error in application of special 'if': expected 3 args but got %i\n",
                    arg_count);
                return VALUE_NIL;
            }
            if (value_is_true(interp_eval(i, pos_args[0]))) {
                return interp_eval(i, pos_args[1]);
            }
            else {
                return interp_eval(i, pos_args[2]);
            }
            break;
        case VALUE_SP_DEFINE:
            arg_count = interp_collect_list(args, 2, pos_args);
            if (arg_count != 2) {
                fprintf(stderr, "Arity error in application of special 'define': expected 2 args but got %i\n",
                    arg_count);
                return VALUE_NIL;
            }
            // XXX do we need to eval it first?
            if (value_type(pos_args[0]) != TYPE_SYMBOL) {
                fprintf(stderr, "Type error in application of special 'define': expected a symbol args but got %li\n",
                    // XXX we should have a textual type, just for error
                    // repoting and logging
                    value_type(pos_args[0]));
                return VALUE_NIL;
            }
            i->env = env_bind(i->alloc, i->env, pos_args[0], interp_eval(i, pos_args[1]));
            // XXX or what does it return
            return VALUE_NIL;
            break;
        case VALUE_SP_LAMBDA:
            arg_count = interp_collect_list(args, 2, pos_args);
            if (arg_count != 2) {
                fprintf(stderr, "Arity error in application of special 'lambda': expected 2 args but got %i\n",
                    arg_count);
                return VALUE_NIL;
            }
            struct interp_lambda *lambda = allocator_alloc(i->alloc, sizeof(struct interp_lambda));
            lambda->env = i->env;
            lambda->arity = interp_count_list(pos_args[0]);
            lambda->arg_names = malloc(sizeof(value) * lambda->arity);
            interp_collect_list(pos_args[0], lambda->arity, lambda->arg_names);
            lambda->body = pos_args[1];
            return make_interp_lambda(lambda);
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
            arg_count = interp_collect_list(args, 1, pos_args);
            if (arg_count != 1) {
                fprintf(stderr, "Arity error in application of special 'quote': expected 1 args but got %i\n",
                    arg_count);
                return VALUE_NIL;
            }
            return pos_args[0];
            break;
        case VALUE_SP_LET:
            arg_count = interp_collect_list(args, 2, pos_args);
            if (arg_count != 2) {
                fprintf(stderr, "Arity error in application of special 'let': expected 2 args but got %i\n",
                    arg_count);
                return VALUE_NIL;
            }
            struct interp_env *old_env = i->env;
            value current_arg = pos_args[0];
            while (value_type(current_arg) == TYPE_CONS) {
                value arg_pair = car(current_arg);
                if (value_type(arg_pair) != TYPE_CONS) {
                    fprintf(stderr, "arg binding to let is not a pair\n");
                    return VALUE_NIL;
                }
                // XXX is there not a way this can be evaled?
                value arg_name = car(arg_pair);
                if (value_type(arg_name) != TYPE_SYMBOL) {
                    fprintf(stderr, "first part of arg binding to let is not a symbol\n");
                    return VALUE_NIL;
                }
                value arg_value = interp_eval(i, car(cdr(arg_pair)));
                i->env = env_bind(i->alloc, i->env, arg_name, arg_value);
                current_arg = cdr(current_arg);
            }
            value result = interp_eval(i, pos_args[1]);
            i->env = old_env;
            return result;
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
                value pos_args[2];
                int arg_count = interp_collect_list(cdr(expr), 2, pos_args);
                if (arg_count != 2) {
                    if (arg_count != 1) {
                        fprintf(stderr, "Arity error in application of builtin: expected 2 args but got %i\n",
                            arg_count);
                        return VALUE_NIL;
                    }
                }
                return funcptr(i->alloc, interp_eval(i, pos_args[0]), interp_eval(i, pos_args[1]));
            }
            else if (value_type(op) == TYPE_INTERP_LAMBDA) {
                struct interp_lambda *lambda = value_to_interp_lambda(op);
                int application_arity = interp_count_list(cdr(expr));
                if (lambda->arity != application_arity) {
                    fprintf(stderr, "Arity error in application of lambda: expected %i args but got %i\n",
                        lambda->arity, application_arity);
                    return VALUE_NIL;
                }
                struct interp_env *application_env = lambda->env;
                value current_arg = cdr(expr);
                for (int idx = 0; idx < application_arity; idx++) {
                    application_env = env_bind(i->alloc, application_env, lambda->arg_names[idx], car(current_arg));
                    current_arg = cdr(current_arg);
                }
                struct interp_env *previous_env = i->env;
                i->env = application_env;
                value result = interp_eval(i, lambda->body);
                i->env = previous_env;
                return result;
                return VALUE_NIL;
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
