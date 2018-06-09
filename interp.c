#include "interp.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "builtins.h"

// XXX could be more efficient structure rather than linked list
struct interp_env_entry {
    value name;
    value value;
    struct interp_env_entry *next;
};

struct interp_env {
    struct interp_env *outer;
    struct interp_env_entry *entries;
};

struct interp {
    struct allocator *alloc;
    struct interp_env *top_env;
    struct call_frame *current_frame;
};

struct interp_lambda {
    uint_fast32_t arity : 31;
    uint_fast32_t variadic : 1;
    value *arg_names;
    value body;
    struct interp_env *env;
};

struct interp_env* env_new(struct allocator *alloc, struct interp_env *outer) {
    struct interp_env *ret = allocator_alloc(alloc, (sizeof(struct interp_env)));
    ret->outer = outer;
    ret->entries = NULL;
    return ret;
}

value env_lookup(struct interp_env *env, value symbol) {
    assert(value_is_symbol(symbol));
    while (env) {
        struct interp_env_entry *ee = env->entries;
        while (ee) {
            if (!value_is_symbol(ee->name)) {
                assert(value_is_symbol(ee->name));
            }
            if (strcmp(value_to_symbol(&ee->name), value_to_symbol(&symbol)) == 0) {
                return ee->value;
            }
            ee = ee->next;
        }
        env = env->outer;
    }
    fprintf(stderr, "Could not find symbol '%s' in environment\n", value_to_symbol(&symbol));
    return VALUE_NIL;
}

void env_bind(struct allocator *alloc, struct interp_env *env, value symbol, value value) {
    assert(value_is_symbol(symbol));
    struct interp_env_entry *ee = env->entries;
    struct interp_env_entry *prev = NULL;

    while (ee) {
        assert(value_is_symbol(ee->name));
        if (strcmp(value_to_symbol(&ee->name), value_to_symbol(&symbol)) == 0) {
            ee->value = value;
            return;
        }
        prev = ee;
        ee = ee->next;
    }
    struct interp_env_entry *nee = allocator_alloc(alloc, (sizeof(struct interp_env_entry)));
    nee->next = NULL;
    nee->name = symbol;
    nee->value = value;
    if (prev) {
        prev->next = nee;
    }
    else {
        env->entries = nee;
    }
}

bool env_set(struct allocator *alloc, struct interp_env *env, value symbol, value value) {
    assert(value_is_symbol(symbol));

    while (env) {
        struct interp_env_entry *ee = env->entries;
        while (ee) {
            assert(value_is_symbol(ee->name));
            if (strcmp(value_to_symbol(&ee->name), value_to_symbol(&symbol)) == 0) {
                ee->value = value;
                return true;
            }
            ee = ee->next;
        }
        env = env->outer;
    }
    return false;
}

struct interp* interp_new(struct allocator *alloc) {
    struct interp *ret = malloc(sizeof(struct interp));
    ret->alloc = alloc;
    ret->top_env = env_new(ret->alloc, NULL);

    bind_builtins(alloc, ret->top_env);

    env_bind(alloc, ret->top_env, make_symbol(ret->alloc, "if"), VALUE_SP_IF);
    env_bind(alloc, ret->top_env, make_symbol(ret->alloc, "define"), VALUE_SP_DEFINE);
    env_bind(alloc, ret->top_env, make_symbol(ret->alloc, "lambda"), VALUE_SP_LAMBDA);
    env_bind(alloc, ret->top_env, make_symbol(ret->alloc, "begin"), VALUE_SP_BEGIN);
    env_bind(alloc, ret->top_env, make_symbol(ret->alloc, "quote"), VALUE_SP_QUOTE);
    env_bind(alloc, ret->top_env, make_symbol(ret->alloc, "let"), VALUE_SP_LET);
    env_bind(alloc, ret->top_env, make_symbol(ret->alloc, "apply"), VALUE_SP_APPLY);
    env_bind(alloc, ret->top_env, make_symbol(ret->alloc, "set!"), VALUE_SP_SET);
    env_bind(alloc, ret->top_env, make_symbol(ret->alloc, "eval"), VALUE_SP_EVAL);

    return ret;
}

void interp_free(struct interp *i) {
    assert(i != NULL);
    free(i);
}

int interp_collect_list(value expr, int count, value *collector) {
    assert(collector != NULL);
    assert(count >= 0);
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

int interp_collect_nonlist(value expr, int count, value *collector) {
    assert(collector != NULL);
    assert(count >= 0);
    int pos = 0;
    value ca = expr;
    while (value_type(ca) == TYPE_CONS) {
        if (pos < count) {
            collector[pos] = car(ca);
        }
        pos++;
        ca = cdr(ca);
    }
    if ((ca != VALUE_NIL) && (ca != VALUE_EMPTY_LIST)) {
        collector[pos] = ca;
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

// XXX we could use onlu the _nonlist variants
// also counts non-list trailing args so (a b . c) => 3
int interp_count_nonlist(value expr, bool *well_formed) {
    int ret = 0;
    while (value_type(expr) == TYPE_CONS) {
        ret++;
        expr = cdr(expr);
    }
    if ((expr != VALUE_NIL) && (expr != VALUE_EMPTY_LIST)) {
        ret++;
        *well_formed = false;
    }
    else {
        *well_formed = true;
    }
    return ret;
}

#define NUM_LOCALS  5

struct call_frame {
    value expr;                     // expr to be evaluated in this frame
    struct interp_env *env;         // the env in which to evaluate the expression
    value locals[NUM_LOCALS];       // scratch space required to evaluate 
    struct interp_env *extra_env;   // scratch env used during eval
    struct call_frame *outer;       // caller
};

inline __attribute__((always_inline)) 
struct call_frame* call_frame_new(struct allocator *alloc, struct call_frame *outer, 
        value expr, struct interp_env *env) {
    struct call_frame *ret = alloca(sizeof(struct call_frame));
    ret->expr = expr;
    ret->env = env;
    for (int i = 0; i < NUM_LOCALS; i++) {
        ret->locals[i] = VALUE_NIL;
    }
    ret->extra_env = NULL;
    ret->outer = outer;
    return ret;
}

value interp_eval_env_int(struct interp *i, struct call_frame *f);

// XXX can we put a struct in the args rather than a pointer to it?
value interp_eval_env(struct interp *i, struct call_frame *caller_frame, value expr, struct interp_env *env) {
    i->current_frame = call_frame_new(i->alloc, caller_frame, expr, env);
    // XXX may be cheaper to have bogus outer frame than to check all the
    // time...
    // XXX i->current_frame is always == second argument... ??
    value ret = interp_eval_env_int(i, i->current_frame);
    if (i->current_frame) {
        i->current_frame = i->current_frame->outer;
    }
    return ret;
}

inline __attribute__((always_inline))
value interp_eval_env_int(struct interp *i, struct call_frame *f) {
    assert(i != NULL);

tailcall_label:

    if (allocator_needs_gc(i->alloc)) {
        interp_gc(i);
    }

    switch (value_type(f->expr)) {
        case TYPE_INT:
        case TYPE_FLOAT:
        case TYPE_ENUM:
        case TYPE_STRING:
        case TYPE_SHORT_STRING:
            return f->expr;
            break;

        case TYPE_SHORT_SYMBOL:
        case TYPE_SYMBOL:
            return env_lookup(f->env, f->expr);
            break;

        case TYPE_CONS:;
            value op = f->locals[0] = interp_eval_env(i, f, car(f->expr), f->env); 
            if (op == VALUE_NIL) {
                // XXX is this right? what if the cdr is set?
                return VALUE_EMPTY_LIST;
            }
            else if (value_is_special(op)) {
                value special = op;
                value args = cdr(f->expr);
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
                        if (value_is_true(interp_eval_env(i, f, pos_args[0], f->env))) {
                            f->expr = pos_args[1];
                            goto tailcall_label;
                        }
                        else {
                            f->expr = pos_args[2];
                            goto tailcall_label;
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
                        if (!value_is_symbol(pos_args[0])) {
                            fprintf(stderr, "Type error in application of special 'define': expected a symbol args but got %li\n",
                                // XXX we should have a textual type, just for error
                                // repoting and logging
                                value_type(pos_args[0]));
                            return VALUE_NIL;
                        }
            /* XXX this breaks the tail-call idea of passing teh env around. perhaps it's
            * not needed anyway? 
                        if (i->current_env) {
                            i->current_env = env_bind(i->alloc, i->current_env, pos_args[0], VALUE_NIL);
                            i->current_env->value = interp_eval(i, pos_args[1]);
                        }
                        else {*/
                            // XXX perhaps this should bind in env rather then in top_env?
                            env_bind(i->alloc, i->top_env, pos_args[0], 
                                interp_eval_env(i, f, pos_args[1], f->env));
            //            }
                        // XXX or what does it return?
                        return VALUE_NIL;
                        break;
                    case VALUE_SP_LAMBDA:;
                        struct interp_lambda *lambda = allocator_alloc(i->alloc, sizeof(struct interp_lambda));
                        lambda->env = f->env;
                        arg_count = interp_collect_list(args, 2, pos_args);
                        if (arg_count != 2) {
                            fprintf(stderr, "Arity error in application of special 'lambda': expected 2 args but got %i\n",
                                arg_count);
                            return VALUE_NIL;
                        }
                        if (value_type(car(args)) != TYPE_CONS) {
                            lambda->variadic = 1;
                            lambda->arity = 1; 
                            lambda->arg_names = allocator_alloc(i->alloc, sizeof(value) * lambda->arity);
                            lambda->arg_names[0] = car(args);
                        }
                        else {
                            bool well_formed;
                            lambda->arity = interp_count_nonlist(pos_args[0], &well_formed);
                            lambda->variadic = well_formed ? 0 : 1;
                            lambda->arg_names = allocator_alloc(i->alloc, sizeof(value) * lambda->arity);
                            interp_collect_nonlist(pos_args[0], lambda->arity, lambda->arg_names);
                        }
                        lambda->body = pos_args[1];
                        return make_interp_lambda(lambda);
                        break;
                    case VALUE_SP_BEGIN:;
                        value cret = VALUE_NIL;
                        while (value_type(args ) == TYPE_CONS) {
                            value next_args = cdr(args);
                            if (next_args == VALUE_EMPTY_LIST) {
                                f->expr = car(args);
                                goto tailcall_label;
                            }
                            else {
                                cret = interp_eval_env(i, f, car(args), f->env);
                            }
                            args = next_args;
                        }
                        if (args != VALUE_EMPTY_LIST) {
                            fprintf(stderr, "arguments to BEGIN are not well-formed list\n");
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
                        value current_arg = pos_args[0];
                        while (value_type(current_arg) == TYPE_CONS) {
                            value arg_pair = car(current_arg);
                            if (value_type(arg_pair) != TYPE_CONS) {
                                fprintf(stderr, "arg binding to let is not a pair\n");
                                return VALUE_NIL;
                            }
                            // XXX is there not a way this can be evaled?
                            value arg_name = car(arg_pair);
                            if (!value_is_symbol(arg_name)) {
                                fprintf(stderr, "first part of arg binding to let is not a symbol\n");
                                return VALUE_NIL;
                            }
                            value arg_value = interp_eval_env(i, f, car(cdr(arg_pair)), f->env);
                            env_bind(i->alloc, f->env, arg_name, arg_value);
                            current_arg = cdr(current_arg);
                        }
                        f->expr = pos_args[1];
                        goto tailcall_label;
                        break;
                    case VALUE_SP_APPLY:
                        arg_count = interp_collect_list(args, 2, pos_args);
                        if (arg_count != 2) {
                            fprintf(stderr, "Arity error in application of special 'apply': expected 2 args but got %i\n",
                                arg_count);
                            return VALUE_NIL;
                        }
                        f->expr = make_cons(i->alloc, pos_args[0], 
                            interp_eval_env(i, f, pos_args[1], f->env));
                        goto tailcall_label;
                        break;
                    case VALUE_SP_SET:
                        // XXX perhaps this should not be allowed to redefined
                        // entries in the top_env
                        arg_count = interp_collect_list(args, 2, pos_args);
                        if (arg_count != 2) {
                            fprintf(stderr, "Arity error in application of special 'set!': expected 2 args but got %i\n",
                                arg_count);
                            return VALUE_NIL;
                        }
                        // XXX do we need to eval it first?
                        if (!value_is_symbol(pos_args[0])) {
                            fprintf(stderr, "Type error in application of special 'set!': expected a symbol args but got %li\n",
                                // XXX we should have a textual type, just for error
                                // repoting and logging
                                value_type(pos_args[0]));
                            return VALUE_NIL;
                        }
                        if (!env_set(i->alloc, f->env, pos_args[0], interp_eval_env(i, f, pos_args[1], f->env))) {
                            fprintf(stderr, "Error in application of special 'set!': binding for symbol '%s' not found\n", value_to_symbol(&pos_args[0]));
                        }
                        return VALUE_NIL;
                        break;
                    case VALUE_SP_EVAL:
                        arg_count = interp_collect_list(args, 1, pos_args);
                        if (arg_count != 1) {
                            fprintf(stderr, "Arity error in application of special 'eval': expected 1 args but got %i\n",
                                arg_count);
                            return VALUE_NIL;
                        }
                        f->expr = interp_eval_env(i, f, pos_args[0], f->env);
                        goto tailcall_label;
                        break;
                    default:
                        fprintf(stderr, "Unknown special 0x%lX\n", special);
                        return VALUE_NIL;
                }
            }
            else if (value_type(op) == TYPE_BUILTIN) {
                if (builtin_arity(op) == 0) {
                    value current_arg = cdr(f->expr);
                    f->locals[1] = VALUE_EMPTY_LIST;
                    value out_ca = VALUE_NIL;
                    while (value_type(current_arg) == TYPE_CONS) {
                        value temp = make_cons(i->alloc,
                                               interp_eval_env(i, f, car(current_arg), f->env),
                                               VALUE_EMPTY_LIST);
                        if (f->locals[1] == VALUE_EMPTY_LIST) {
                            f->locals[1] = temp;
                        }
                        else {
                            set_cdr(out_ca, temp);
                        }
                        out_ca = temp;
                        current_arg = cdr(current_arg);
                    }
                    t_builtinv funcptr = builtinv_ptr(op);
                    return funcptr(i->alloc, f->locals[1]);
                }
                else if (builtin_arity(op) == 1) {
                    t_builtin1 funcptr = builtin1_ptr(op);
                    value pos_args[1];
                    int arg_count = interp_collect_list(cdr(f->expr), 1, pos_args);
                    if (arg_count != 1) {
                        fprintf(stderr, "Arity error in application of builtin: expected 1 args but got %i\n",
                            arg_count);
                        return VALUE_NIL;
                    }
                    return funcptr(i->alloc, interp_eval_env(i, f, pos_args[0], f->env));
                }
                else if (builtin_arity(op) == 2) {
                    t_builtin2 funcptr = builtin2_ptr(op);
                    value pos_args[2];
                    int arg_count = interp_collect_list(cdr(f->expr), 2, pos_args);
                    if (arg_count != 2) {
                        fprintf(stderr, "Arity error in application of builtin: expected 2 args but got %i\n",
                            arg_count);
                        return VALUE_NIL;
                    }
                    f->locals[2] = interp_eval_env(i, f, pos_args[0], f->env);
                    f->locals[3] = interp_eval_env(i, f, pos_args[1], f->env);
                    return funcptr(i->alloc, f->locals[2], f->locals[3]);
                }
                else if (builtin_arity(op) == 3) {
                    t_builtin3 funcptr = builtin3_ptr(op);
                    value pos_args[3];
                    int arg_count = interp_collect_list(cdr(f->expr), 3, pos_args);
                    if (arg_count != 3) {
                        fprintf(stderr, "Arity error in application of builtin: expected 3 args but got %i\n",
                            arg_count);
                        return VALUE_NIL;
                    }
                    f->locals[2] = interp_eval_env(i, f, pos_args[0], f->env);
                    f->locals[3] = interp_eval_env(i, f, pos_args[1], f->env);
                    f->locals[4] = interp_eval_env(i, f, pos_args[2], f->env);
                    return funcptr(i->alloc, f->locals[2], f->locals[3], f->locals[4]);
                }
                else {
                    fprintf(stderr, "Unsupported builtin arity %d\n", builtin_arity(op));
                    return VALUE_NIL;
                }
            }
            else if (value_type(op) == TYPE_INTERP_LAMBDA) {
                struct interp_lambda *lambda = value_to_interp_lambda(op);
                int application_arity = interp_count_list(cdr(f->expr));
                // XXX feels as if we can just reuse the current environment if
                // ->outer is lambda->env, btu somehow that doesn't work...
                f->extra_env = env_new(i->alloc, lambda->env);
                if (lambda->variadic) {
                    int ax;
                    // XXX this does not need to be a local
                    f->locals[2] = cdr(f->expr);
                    for (ax = 0; ax < lambda->arity - 1; ax++) {
                        value t = interp_eval_env(i, f, car(f->locals[2]), f->env);
                        env_bind(i->alloc, f->extra_env, lambda->arg_names[ax], t);
                        f->locals[2] = cdr(f->locals[2]);
                    }
                    f->locals[1] = VALUE_EMPTY_LIST;
                    value out_ca = VALUE_NIL;
                    value current_arg = f->locals[2];
                    // XXX some of this list-building is nicer in the variadic
                    // builtin case below
                    for (int idx = ax; idx < application_arity; idx++) {
                        f->locals[2] = make_cons(i->alloc, interp_eval_env(i, f, car(current_arg), f->env), VALUE_EMPTY_LIST);
                        current_arg = cdr(current_arg);
                        if (f->locals[1] == VALUE_EMPTY_LIST) {
                            f->locals[1] = f->locals[2];
                            out_ca = f->locals[2];
                        }
                        else {
                            set_cdr(out_ca, f->locals[2]);
                            out_ca = f->locals[2];
                        }
                    }
                    // XXX hmm, if we could bind arg_list first and set! in the
                    // env, we could save one local...
                    env_bind(i->alloc, f->extra_env, lambda->arg_names[ax], f->locals[1]);
                    f->locals[1] = VALUE_NIL;
                }
                else {
                    if (lambda->arity != application_arity) {
                        fprintf(stderr, "Arity error in application of lambda: expected %i args but got %i\n",
                            lambda->arity, application_arity);
                        return VALUE_NIL;
                    }
                    // XXX this does not need to be a local
                    f->locals[2] = cdr(f->expr);
                    for (int idx = 0; idx < application_arity; idx++) {
                        value t = interp_eval_env(i, f, car(f->locals[2]), f->env);
                        env_bind(i->alloc, f->extra_env, lambda->arg_names[idx], t);
                        f->locals[2] = cdr(f->locals[2]);
                    }
                    f->locals[2] = VALUE_NIL;
                }
                // XXX is there a way to not allocate an env in all
                // cases? perhaps we can reuse the one we have in recursions?
                f->expr = lambda->body;
                f->env = f->extra_env;
                f->extra_env = NULL;
                goto tailcall_label;
            }
            else {
                fprintf(stderr, "No idea how to apply operator of type 0x%lX\n", value_type(op));
                return VALUE_NIL;
            }
            break;
        default:
            fprintf(stderr, "Unexpected value type 0x%lX in interp_eval\n", value_type(f->expr));
            return VALUE_NIL;
    }

    // XXX can we ever get here? 
    return f->expr;
}

void interp_add_gc_root_env(struct allocator_gc_ctx *gc, struct interp_env *env) {
    while (env) {
        allocator_gc_add_nonval_root(gc, env);
        struct interp_env_entry *ee = env->entries;
        while (ee) {
            allocator_gc_add_nonval_root(gc, ee);
            allocator_gc_add_root(gc, ee->name);
            allocator_gc_add_root(gc, ee->value);
            ee = ee->next;
        }
        env = env->outer;
    }
}

void interp_add_gc_root_frame(struct allocator_gc_ctx *gc, struct call_frame *f) {
    while (f) {
        allocator_gc_add_root(gc, f->expr);
        interp_add_gc_root_env(gc, f->env);
        if (f->extra_env) {
            interp_add_gc_root_env(gc, f->extra_env);
        }
        for (int i = 0; i < NUM_LOCALS; i++) {
            allocator_gc_add_root(gc, f->locals[i]);
        }
        f = f->outer;
    }
}

value interp_eval(struct interp *i, value expr) {
    return interp_eval_env(i, NULL, expr, i->top_env);
}

void interp_gc(struct interp *i) {
    struct allocator_gc_ctx *gc = allocator_gc_new(i->alloc);
    interp_add_gc_root_env(gc, i->top_env);
    interp_add_gc_root_frame(gc, i->current_frame);
    allocator_gc_perform(gc);
}

void interp_traverse_lambda(struct allocator_gc_ctx *gc, struct interp_lambda *l) {
    allocator_gc_add_root(gc, l->body);
    allocator_gc_add_nonval_root(gc, l->arg_names);
    for (int i = 0; i < l->arity; i++) {
        allocator_gc_add_root(gc, l->arg_names[i]);
    }
    struct interp_env *ce = l->env;
    while (ce) {
        allocator_gc_add_nonval_root(gc, ce);
        struct interp_env_entry *ee = ce->entries;
        while (ee) {
            allocator_gc_add_nonval_root(gc, ee);
            allocator_gc_add_root(gc, ee->name);
            allocator_gc_add_root(gc, ee->value);
            ee = ee->next;
        }
        ce = ce->outer;
    }
}
