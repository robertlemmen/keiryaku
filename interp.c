#include "interp.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "builtins.h"
#include "global.h"
#include "heap.h"

// XXX currently we are limited to that many arguments for most calls, which is
// silly. but the call frame should be of limited size. so we need some sort of
// overflow mechanism. or we always use a list...
#define NUM_LOCALS  8

#define MAX_INLINE_ENV_ENTRIES  3

struct interp_env_entry {
    uint8_t sub_type;
    value name;
    value value;
    value next_entry_v;     // always a interp_env_entry
};

struct interp_env_entry_inline {
    value name;
    value value;
};

struct interp_env {
    uint8_t sub_type;
    uint8_t num_inline_entries;
    value outer_v;          // always a interp_env
    value first_entry_v;    // always a interp_env_entry
    struct interp_env_entry_inline inline_entries[];
};

struct interp {
    struct allocator *alloc;
    value top_env_v;        // always a interp_env
    struct call_frame *current_frame;
    value current_dynamic_frame_v; // always a dynamic_frame
};

struct interp_lambda {
    uint_fast32_t arity : 31;
    uint_fast32_t variadic : 1;
    value body;
    value env_v;            // always a interp_env
    value arg_names[];      // inline array, struct needs to be allocated with 
                            // correct size 
};

struct call_frame {
    value expr;                     // expr to be evaluated in this frame
    value env_v;                    // the env in which to evaluate the expression
    value extra_env_v;              // scratch env used during eval
    value locals[NUM_LOCALS];       // scratch space required to evaluate
    struct call_frame *outer;       // caller
};

struct dynamic_frame {
    uint8_t sub_type;
    value param;
    value val;
    value outer_v;  // always a dynamic_frame
};

struct interp_env* env_new(struct allocator *alloc, struct interp_env *outer) {
    struct interp_env *ret = allocator_alloc(alloc,
              (sizeof(struct interp_env))
            + sizeof(struct interp_env_entry_inline) * MAX_INLINE_ENV_ENTRIES);
    ret->sub_type = SUBTYPE_ENV;
    ret->num_inline_entries = 0;
    if (outer) {
        ret->outer_v = make_environment(alloc, outer);
    }
    else {
        ret->outer_v = VALUE_NIL;
    }
    ret->first_entry_v = VALUE_NIL;
    return ret;
}

value env_lookup(struct interp_env *env, value symbol, value *lookup_vector) {
    assert(value_is_symbol(symbol));
    int envs = 0;
    while (env) {
        int entries = 0;    // XXX rename, also in vector lookup, so that it is clear that this  is the number of entries, not the field in the struct

        // lookup in the inlines
        for (int i = 0; i < env->num_inline_entries; i++) {
            struct interp_env_entry_inline *ee = &env->inline_entries[i];
            if (       (ee->name == symbol) // short symbol case
                    || (strcmp(value_to_symbol(&ee->name), value_to_symbol(&symbol)) == 0)) {
                if (lookup_vector) {
                    *lookup_vector = make_lookup_vector(NULL, envs, entries);
                }
                return ee->value;
            }
            entries++;
        }

        // lookup in the overflow list
        value ee_v = env->first_entry_v;
        while (ee_v != VALUE_NIL) {
            struct interp_env_entry *ee = value_to_env_entry(ee_v);
            assert(value_is_symbol(ee->name));
            if (       (ee->name == symbol) // short symbol case
                    || (strcmp(value_to_symbol(&ee->name), value_to_symbol(&symbol)) == 0)) {
                if (lookup_vector) {
                    *lookup_vector = make_lookup_vector(NULL, envs, entries);
                }
                return ee->value;
            }
            ee_v = ee->next_entry_v;
            entries++;
        }
        if (env->outer_v == VALUE_NIL) {
            env = NULL;
        }
        else {
            env = value_to_environment(env->outer_v);
        }
        envs++;
    }
    fprintf(stderr, "Could not find symbol '%s' in environment\n", value_to_symbol(&symbol));
    return VALUE_NIL;
}

value env_lookup_vector(struct interp_env *env, value vector) {
    int envs = lookup_vector_envs(vector);
    int entries = lookup_vector_entries(vector);
    while (envs) {
        envs--;
        env = value_to_environment(env->outer_v);
    }

    // is it one of the inlines?
    if (entries < env->num_inline_entries) {
        return env->inline_entries[entries].value;
    }
    else {
        entries -= env->num_inline_entries;
    }

    // go through the overflow list then
    value ee_v = env->first_entry_v;
    while (entries) {
        entries--;
        ee_v = value_to_env_entry(ee_v)->next_entry_v;
    }
    return value_to_env_entry(ee_v)->value;
}

void env_bind(struct allocator *alloc, struct interp_env *env, value symbol, value val) {
    assert(value_is_symbol(symbol));
    struct interp_env_entry *prev = NULL;

    // check for same name in lines
    for (int i = 0; i < env->num_inline_entries; i++) {
        struct interp_env_entry_inline *ee = &env->inline_entries[i];
        if (       (ee->name == symbol) // short symbol case
                || (strcmp(value_to_symbol(&ee->name), value_to_symbol(&symbol)) == 0)) {
            ee->value = val;
            write_barrier(alloc, make_environment(alloc, env), &env->first_entry_v);
            return;
        }
    }

    // check for same name in overflow list
    value ee_v = env->first_entry_v;
    while (ee_v != VALUE_NIL) {
        struct interp_env_entry *ee = value_to_env_entry(ee_v);
        assert(value_is_symbol(ee->name));
        if (       (ee->name == symbol) // short symbol case
                || (strcmp(value_to_symbol(&ee->name), value_to_symbol(&symbol)) == 0)) {
            ee->value = val;
            write_barrier(alloc, ee_v, &ee->value);
            return;
        }
        prev = ee;
        ee_v = ee->next_entry_v;
    }

    // add to inlines?
    if (env->num_inline_entries < MAX_INLINE_ENV_ENTRIES) {
        env->inline_entries[env->num_inline_entries].name = symbol;
        env->inline_entries[env->num_inline_entries].value = val;
        env->num_inline_entries++;
        write_barrier(alloc, make_environment(alloc, env), &env->first_entry_v);
        return;
    }

    // add to overflow_list
    struct interp_env_entry *nee = allocator_alloc(alloc, (sizeof(struct interp_env_entry)));
    nee->sub_type = SUBTYPE_ENV_ENTRY;
    nee->next_entry_v = VALUE_NIL;
    nee->name = symbol;
    nee->value = val;
    if (prev) {
        prev->next_entry_v = make_env_entry(alloc, nee);
        write_barrier(alloc, make_env_entry(alloc, prev), &prev->next_entry_v);
    }
    else {
        env->first_entry_v = make_env_entry(alloc, nee);
        write_barrier(alloc, make_environment(alloc, env), &env->first_entry_v);
    }
}

bool env_set(struct allocator *alloc, struct interp_env *env, value symbol, value val) {
    assert(value_is_symbol(symbol));

    while (env) {
        // set in the inline array
        for (int i = 0; i < env->num_inline_entries; i++) {
            struct interp_env_entry_inline *ee = &env->inline_entries[i];
            if (       (ee->name == symbol) // short symbol case
                    || (strcmp(value_to_symbol(&ee->name), value_to_symbol(&symbol)) == 0)) {
                ee->value = val;
                write_barrier(alloc, make_environment(alloc, env), &env->first_entry_v);
                return true;
            }
        }

        // set in the overflow list
        value ee_v = env->first_entry_v;
        while (ee_v != VALUE_NIL) {
            struct interp_env_entry *ee = value_to_env_entry(ee_v);
            assert(value_is_symbol(ee->name));
            if (       (ee->name == symbol) // short symbol case
                    || (strcmp(value_to_symbol(&ee->name), value_to_symbol(&symbol)) == 0)) {
                ee->value = val;
                write_barrier(alloc, ee_v, &ee->value);
                return true;
            }
            ee_v = ee->next_entry_v;
        }
        env = value_to_environment(env->outer_v);
    }
    return false;
}

struct interp* interp_new(struct allocator *alloc) {
    struct interp *ret = malloc(sizeof(struct interp));
    struct interp_env *top_env = env_new(alloc, NULL);
    ret->alloc = alloc;
    ret->top_env_v = make_environment(alloc, top_env);
    ret->current_frame = NULL;
    ret->current_dynamic_frame_v = VALUE_NIL;

    bind_builtins(alloc, top_env);

    env_bind(alloc, top_env, make_symbol(alloc, "if"),           VALUE_SP_IF);
    env_bind(alloc, top_env, make_symbol(alloc, "define"),       VALUE_SP_DEFINE);
    env_bind(alloc, top_env, make_symbol(alloc, "lambda"),       VALUE_SP_LAMBDA);
    env_bind(alloc, top_env, make_symbol(alloc, "begin"),        VALUE_SP_BEGIN);
    env_bind(alloc, top_env, make_symbol(alloc, "quote"),        VALUE_SP_QUOTE);
    env_bind(alloc, top_env, make_symbol(alloc, "let"),          VALUE_SP_LET);
    env_bind(alloc, top_env, make_symbol(alloc, "let*"),         VALUE_SP_LETS);
    env_bind(alloc, top_env, make_symbol(alloc, "letrec"),       VALUE_SP_LETREC);
    // our (letrec ...) already behaves like (letrec* ...)
    env_bind(alloc, top_env, make_symbol(alloc, "letrec*"),      VALUE_SP_LETREC);
    env_bind(alloc, top_env, make_symbol(alloc, "apply"),        VALUE_SP_APPLY);
    env_bind(alloc, top_env, make_symbol(alloc, "set!"),         VALUE_SP_SET);
    env_bind(alloc, top_env, make_symbol(alloc, "eval"),         VALUE_SP_EVAL);
    env_bind(alloc, top_env, make_symbol(alloc, "parameterize"), VALUE_SP_PARAM);
    env_bind(alloc, top_env, make_symbol(alloc, "_nil"),         VALUE_NIL);
    // required for (interaction-environment)
    env_bind(alloc, top_env, make_symbol(alloc, "_top_env"),     ret->top_env_v);

    return ret;
}

struct interp_env* interp_top_env(struct interp *i) {
    return value_to_environment(i->top_env_v);
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

inline __attribute__((always_inline))
struct call_frame* call_frame_new(struct allocator *alloc, struct call_frame *outer,
        value expr, struct interp_env *env) {
    struct call_frame *ret = alloca(sizeof(struct call_frame));
    ret->expr = expr;
    ret->env_v = make_environment(alloc, env);
    for (int i = 0; i < NUM_LOCALS; i++) {
        ret->locals[i] = VALUE_NIL;
    }
    ret->extra_env_v = VALUE_NIL;
    ret->outer = outer;
    return ret;
}

value interp_eval_env_int(struct interp *i, struct call_frame *f, struct dynamic_frame *dyn_frame, value lookup_cons);

// XXX can we put a struct in the args rather than a pointer to it?
value interp_eval_env(struct interp *i, struct call_frame *caller_frame, struct dynamic_frame *dyn_frame, value expr, struct interp_env *env, value lookup_cons) {
    i->current_frame = call_frame_new(i->alloc, caller_frame, expr, env);
    // XXX may be cheaper to have bogus outer frame than to check all the
    // time...
    // XXX same problem with dynamic chain
    value ret = interp_eval_env_int(i, i->current_frame, dyn_frame, lookup_cons);
    if (i->current_frame) {
        i->current_frame = i->current_frame->outer;
    }
    return ret;
}

// XXX we do a lot of conversions between value types and struct interp_env,
// change the signatures of interp_eval and the env_bind/lookups to avoid this
// and mostly use values. gets less and less statically typed...

// XXX this does not seem to get inlined, at least I see the frames in gdb.
// perhaps only because of missing optimisation?
// XXX what if lookup_cons is poiting to something that gets moved away by GC?
inline __attribute__((always_inline))
value interp_eval_env_int(struct interp *i, struct call_frame *f, struct dynamic_frame *dyn_frame, value lookup_cons) {
    assert(i != NULL);

tailcall_label:

    if (allocator_needs_gc(i->alloc)) {
        interp_gc(i);
    }

    // XXX should really just use f->locals
    switch (value_type(f->expr)) {
        case TYPE_INT:
        case TYPE_FLOAT:
        case TYPE_ENUM:
        case TYPE_STRING:
        case TYPE_SHORT_STRING:
        case TYPE_BUILTIN:
        case TYPE_INTERP_LAMBDA:
            return f->expr;
            break;

        case TYPE_SHORT_SYMBOL:
        case TYPE_SYMBOL:;
            value lookup_vector;
            value ret = env_lookup(value_to_environment(f->env_v), f->expr, &lookup_vector);
            if (lookup_cons != VALUE_NIL) {
                set_car(i->alloc, lookup_cons, lookup_vector);
            }
            return ret;
            break;
        case TYPE_LOOKUP_VECTOR:
            return env_lookup_vector(value_to_environment(f->env_v), f->expr);
            break;
        case TYPE_CONS:;
            int arg_count;
            // XXX only pass in vector thingie if car(f->expr) is some sort of
            // symbol, otherwise ((atom-to-function ...) replaces the lookup
            // with the last result
            value op = f->locals[0] = interp_eval_env(i, f, dyn_frame, car(f->expr), value_to_environment(f->env_v), value_is_symbol(car(f->expr)) ? f->expr : VALUE_NIL);
            if (op == VALUE_NIL) {
                // XXX is this right? what if the cdr is set?
                return VALUE_EMPTY_LIST;
            }
            else if (value_is_special(op)) {
                value special = op;
                value args = cdr(f->expr);
                switch (special) {
                    case VALUE_SP_IF:
                        arg_count = interp_collect_list(args, 3, &f->locals[1]);
                        if (arg_count != 3) {
                            fprintf(stderr, "Arity error in application of special 'if': expected 3 args but got %i\n",
                                arg_count);
                            return VALUE_NIL;
                        }
                        if (interp_eval_env(i, f, dyn_frame, f->locals[1], value_to_environment(f->env_v), VALUE_NIL) != VALUE_FALSE) {
                            f->expr = f->locals[2];
                            goto tailcall_label;
                        }
                        else {
                            f->expr = f->locals[3];
                            goto tailcall_label;
                        }
                        break;
                    case VALUE_SP_DEFINE:
                        arg_count = interp_collect_list(args, 2, &f->locals[1]);
                        if (arg_count != 2) {
                            fprintf(stderr, "Arity error in application of special 'define': expected 2 args but got %i\n",
                                arg_count);
                            return VALUE_NIL;
                        }
                        // XXX do we need to eval it first?
                        if (!value_is_symbol(f->locals[1])) {
                            fprintf(stderr, "Type error in application of special 'define': expected a symbol args but got type %li\n",
                                // XXX we should have a textual type, just for error
                                // repoting and logging
                                value_type(f->locals[1]));
                            return VALUE_NIL;
                        }
                        env_bind(i->alloc, value_to_environment(f->env_v), f->locals[1],
                            interp_eval_env(i, f, dyn_frame, f->locals[2], value_to_environment(f->env_v), VALUE_NIL));
                        // XXX or what does it return?
                        return VALUE_NIL;
                        break;
                    case VALUE_SP_LAMBDA:;
                        arg_count = interp_collect_list(args, 2, &f->locals[1]);
                        struct interp_lambda *lambda;
                        if (arg_count != 2) {
                            fprintf(stderr, "Arity error in application of special 'lambda': expected 2 args but got %i\n",
                                arg_count);
                            return VALUE_NIL;
                        }
                        if ((value_type(car(args)) != TYPE_CONS) && (car(args) != VALUE_EMPTY_LIST)) {
                            lambda = allocator_alloc(i->alloc, sizeof(struct interp_lambda) + sizeof(value) * 1);
                            lambda->variadic = 1;
                            lambda->arity = 1;
                            lambda->env_v = f->env_v;
                            lambda->arg_names[0] = car(args);
                        }
                        else {
                            bool well_formed;
                            int arity = interp_count_nonlist(f->locals[1], &well_formed);

                            lambda = allocator_alloc(i->alloc, sizeof(struct interp_lambda) + sizeof(value) * arity);
                            lambda->arity = arity;
                            lambda->variadic = well_formed ? 0 : 1;
                            lambda->env_v = f->env_v;
                            interp_collect_nonlist(f->locals[1], lambda->arity, lambda->arg_names);
                        }
                        lambda->body = f->locals[2];
                        return make_interp_lambda(lambda);
                        break;
                    case VALUE_SP_BEGIN:;
                        f->locals[1] = VALUE_NIL;
                        f->locals[2] = args;
                        while (value_type(args) == TYPE_CONS) {
                            f->locals[3] = cdr(f->locals[2]);
                            if (f->locals[3] == VALUE_EMPTY_LIST) {
                                f->expr = car(f->locals[2]);
                                goto tailcall_label;
                            }
                            else {
                                f->locals[1] = interp_eval_env(i, f, dyn_frame, car(f->locals[2]), value_to_environment(f->env_v), VALUE_NIL);
                            }
                            f->locals[2] = f->locals[3];
                        }
                        if (f->locals[2] != VALUE_EMPTY_LIST) {
                            fprintf(stderr, "arguments to special 'begin' are not well-formed list\n");
                        }
                        return f->locals[1];
                    case VALUE_SP_QUOTE:
                        arg_count = interp_collect_list(args, 1, &f->locals[1]);
                        if (arg_count != 1) {
                            fprintf(stderr, "Arity error in application of special 'quote': expected 1 args but got %i\n",
                                arg_count);
                            return VALUE_NIL;
                        }
                        return f->locals[1];
                        break;
                    case VALUE_SP_LET:
                    case VALUE_SP_LETS:
                    case VALUE_SP_LETREC:
                        arg_count = interp_collect_list(args, 2, &f->locals[1]);
                        if (arg_count != 2) {
                            fprintf(stderr, "Arity error in application of special 'let/let*/letrec': expected 2 args but got %i\n",
                                arg_count);
                            return VALUE_NIL;
                        }
                        f->extra_env_v = make_environment(i->alloc, env_new(i->alloc, value_to_environment(f->env_v)));
                        f->locals[3] = f->locals[1];
                        if (special == VALUE_SP_LETREC) {
                            while (value_type(f->locals[3]) == TYPE_CONS) {
                                value arg_pair = car(f->locals[3]);
                                value arg_name = car(arg_pair);
                                env_bind(i->alloc, value_to_environment(f->extra_env_v), arg_name, VALUE_NIL);
                                f->locals[3] = cdr(f->locals[3]);
                            }
                        }
                        f->locals[3] = VALUE_NIL;
                        while (value_type(f->locals[1]) == TYPE_CONS) {
                            // XXX all three values in here are subject to GC
                            // moving stuff away while we still hold a ref to it
                            f->locals[4] = car(f->locals[1]);
                            if (value_type(f->locals[4]) != TYPE_CONS) {
                                fprintf(stderr, "arg binding to 'let/let*/letrec' is not a pair\n");
                                return VALUE_NIL;
                            }
                            // XXX is there not a way this can be evaled?
                            f->locals[6] = car(f->locals[4]);
                            if (!value_is_symbol(f->locals[6])) {
                                fprintf(stderr, "first part of arg binding to 'let/let*/letrec' is not a symbol\n");
                                return VALUE_NIL;
                            }
                            if (special == VALUE_SP_LET) {
                                f->locals[5] = interp_eval_env(i, f, dyn_frame, car(cdr(f->locals[4])), value_to_environment(f->env_v), VALUE_NIL);
                            }
                            else {
                                f->locals[5] = interp_eval_env(i, f, dyn_frame, car(cdr(f->locals[4])), value_to_environment(f->extra_env_v), VALUE_NIL);
                            }
                            env_bind(i->alloc, value_to_environment(f->extra_env_v), f->locals[6], f->locals[5]);
                            f->locals[1] = cdr(f->locals[1]);
                        }
// XXX clean up?                       f->locals[1] = VALUE_NIL;
                        f->expr = f->locals[2];
                        f->env_v = f->extra_env_v;
                        f->extra_env_v = VALUE_NIL;
                        goto tailcall_label;
                        break;
                    case VALUE_SP_APPLY:;
                        arg_count = 0;
                        // we use f->locals[NUM_LOCALS-1] as a temporary
                        f->locals[NUM_LOCALS-1] = args;
                        while (value_type(f->locals[NUM_LOCALS-1]) == TYPE_CONS) {
                            // XXX check overflows, take temporary into acocunt
                            f->locals[arg_count] = interp_eval_env(i, f, dyn_frame, car(f->locals[NUM_LOCALS-1]), value_to_environment(f->env_v), VALUE_NIL);
                            f->locals[NUM_LOCALS-1] = cdr(f->locals[NUM_LOCALS-1]);
                            arg_count++;
                        }
                        if (arg_count < 2) {
                            fprintf(stderr, "Arity error in application of special 'apply': expected 2 or more args but got %i\n",
                                arg_count);
                            return VALUE_NIL;
                        }
                        if (   (value_type(f->locals[arg_count-1]) != TYPE_CONS)
                            && (f->locals[arg_count-1] != VALUE_EMPTY_LIST) ) {
                            fprintf(stderr, "Error in application of special 'apply': last argument is not a list\n");
                            return VALUE_NIL;
                        }
                        f->locals[NUM_LOCALS-1] = f->locals[arg_count-1];
                        arg_count--;
                        while (value_type(f->locals[NUM_LOCALS-1]) == TYPE_CONS) {
                            // XXX check overflows
                            f->locals[arg_count] = car(f->locals[NUM_LOCALS-1]);
                            f->locals[NUM_LOCALS-1] = cdr(f->locals[NUM_LOCALS-1]);
                            arg_count++;
                        }
                        // so far we have treated the first arg as an arg, but
                        // in eval it is the op
                        arg_count--;
                        op = f->locals[0];
                        // XXX check that the operator is not a special
                        // arg_count and f->locals is set up correctly, just
                        // jump to the eval logic after its own evaluation of
                        // arguments
                        goto apply_eval_label;
                        break;
                    case VALUE_SP_SET:
                        // XXX perhaps this should not be allowed to redefined
                        // entries in the top_env
                        arg_count = interp_collect_list(args, 2, &f->locals[1]);
                        if (arg_count != 2) {
                            fprintf(stderr, "Arity error in application of special 'set!': expected 2 args but got %i\n",
                                arg_count);
                            return VALUE_NIL;
                        }
                        // XXX do we need to eval it first?
                        if (!value_is_symbol(f->locals[1])) {
                            fprintf(stderr, "Type error in application of special 'set!': expected a symbol args but got %li\n",
                                // XXX we should have a textual type, just for error
                                // reporting and logging
                                value_type(f->locals[1]));
                            return VALUE_NIL;
                        }
                        if (!env_set(i->alloc, value_to_environment(f->env_v), f->locals[1], interp_eval_env(i, f, dyn_frame, f->locals[2], value_to_environment(f->env_v), VALUE_NIL))) {
                            fprintf(stderr, "Error in application of special 'set!': binding for symbol '%s' not found\n", value_to_symbol(&f->locals[1]));
                        }
                        return VALUE_NIL;
                        break;
                    case VALUE_SP_EVAL:
                        arg_count = interp_collect_list(args, 2, &f->locals[1]);
                        f->locals[3] = f->env_v; // XXX this needs to be in a local to protect against GC
                        if (arg_count == 2) {
                            // XXX what if this does not return an environment?
                            f->locals[3] = interp_eval_env(i, f, dyn_frame, f->locals[2], value_to_environment(f->env_v), VALUE_NIL);
                        }
                        else if (arg_count != 1) {
                            fprintf(stderr, "Arity error in application of special 'eval': expected 1 or 2 args but got %i\n",
                                arg_count);
                            return VALUE_NIL;
                        }
                        f->expr = interp_eval_env(i, f, dyn_frame, f->locals[1], value_to_environment(f->env_v), VALUE_NIL);
                        f->env_v = f->locals[3];
                        goto tailcall_label;
                        break;
                    case VALUE_SP_PARAM:
                        arg_count = interp_collect_list(args, 2, &f->locals[1]);
                        if (arg_count != 2) {
                            fprintf(stderr, "Arity error in application of special 'parameterize': expected 2 args but got %i\n",
                                arg_count);
                            return VALUE_NIL;
                        }
                        f->locals[NUM_LOCALS-3] = i->current_dynamic_frame_v;
                        // this is an iterator over f->locals[1]
                        // XXX can this conflict with another use of the same
                        // local?
                        f->locals[NUM_LOCALS-1] = f->locals[1];
                        struct dynamic_frame *new_dyn_frame = NULL;
                        // XXX hmm, we should really check that it is either
                        // CONS or EMPTY, needs to be well-formed
                        while (value_type(f->locals[NUM_LOCALS-1]) == TYPE_CONS) {
                            f->locals[NUM_LOCALS-2] = car(f->locals[NUM_LOCALS-1]);
                            new_dyn_frame = allocator_alloc(i->alloc, sizeof(struct dynamic_frame));
                            new_dyn_frame->sub_type = SUBTYPE_DYN_FRAME;
                            new_dyn_frame->outer_v = i->current_dynamic_frame_v;
                            new_dyn_frame->param = VALUE_NIL;
                            new_dyn_frame->val = VALUE_NIL;
                            i->current_dynamic_frame_v = make_dyn_frame(i->alloc, new_dyn_frame);
                            // it is important to do the value of the pair
                            // first, as the new dynamic frame is already
                            // visible during the evaluation. a NIL param
                            // ensures that it is ignored
                            // XXX we should use spare locals for this!
                            new_dyn_frame->val = interp_eval_env(i, f, dyn_frame, car(cdr(f->locals[NUM_LOCALS-2])), value_to_environment(f->env_v), VALUE_NIL);
                            new_dyn_frame->param = interp_eval_env(i, f, dyn_frame, car(f->locals[NUM_LOCALS-2]), value_to_environment(f->env_v), VALUE_NIL);
                            if (!value_is_param(new_dyn_frame->param)) {
                                fprintf(stderr, "Binding to parameterize is not for parameter\n");
                                return VALUE_NIL;
                            }
                            struct param *param_def = value_to_parameter(new_dyn_frame->param);
                            // evaluate the converter
                            value convert_expr = make_cons(i->alloc, param_def->convert,
                                                                    make_cons(i->alloc, new_dyn_frame->val, VALUE_EMPTY_LIST));
                            // XXX I guess during this call new_dyn_frame could
                            // be collected because i->current_dynamic_frame
                            // gets reset by the call
                            new_dyn_frame->val = interp_eval_env(i, f, dyn_frame, convert_expr, value_to_environment(f->env_v), VALUE_NIL);
                            f->locals[NUM_LOCALS-1] = cdr(f->locals[NUM_LOCALS-1]);
                        }
                        // XXX new_dyn_frame should not be NULL, that woul mean
                        // no bindings done
                        value ret = interp_eval_env(i, f, new_dyn_frame, f->locals[2], value_to_environment(f->env_v), VALUE_NIL);
                        i->current_dynamic_frame_v = f->locals[NUM_LOCALS-3];
                        return ret;
                    default:
                        fprintf(stderr, "Unknown special 0x%lX\n", special);
                        return VALUE_NIL;
                }
            }
            else {
                // XXX do we need to set this again?
                // XXX we could have one block for both builtins and lambdas, where
                // at the very start we would evaluate all arguments. this way we
                // could goto from the apply special after it has set up the
                // arguments, avoiding code duplication
                arg_count = interp_collect_list(cdr(f->expr), NUM_LOCALS-1, &f->locals[1]);
                if (arg_count == NUM_LOCALS-1) {
                    // XXX implement overflow logic
                    fprintf(stderr, "Currently only %d args are supported\n",  NUM_LOCALS - 2);
                    return VALUE_NIL;
                }
                // evaluate all arguments
                for (int j = 0; j < arg_count; j++) {
                    f->locals[j+1] = interp_eval_env(i, f, dyn_frame, f->locals[j+1], value_to_environment(f->env_v), VALUE_NIL);
                }
                // all the interp_eval_env could have cause GC, which would have
                // wrecked our "op", so we need to reset it from the
                // f->locals[0] that are safe. perhaps we should just use
                // f->locals in general
                op = f->locals[0];
apply_eval_label:
                if (value_type(op) == TYPE_BUILTIN) {
                    if (builtin_arity(op) == BUILTIN_ARITY_VARIADIC) {
                        value arg_list = VALUE_EMPTY_LIST;
                        value current_arg = VALUE_NIL;
                        for (int j = 0; j < arg_count; j++) {
                            value temp = make_cons(i->alloc,
                                                f->locals[j+1],
                                                VALUE_EMPTY_LIST);
                            if (arg_list == VALUE_EMPTY_LIST) {
                                arg_list = temp;
                            }
                            else {
                                set_cdr(i->alloc, current_arg, temp);
                            }
                            current_arg = temp;
                        }
                        t_builtinv funcptr = builtinv_ptr(op);
                        return funcptr(i->alloc, arg_list);
                    }
                    else {
                        int op_arity = builtin_arity(op);
                        if (op_arity != arg_count) {
                            fprintf(stderr, "Arity error in application of builtin '%s': expected %d args but got %d\n",
                                builtin_name(op), op_arity, arg_count);
                            return VALUE_NIL;
                        }
                        switch (op_arity) {
                            case 0:;
                                t_builtin0 funcptr0 = builtin0_ptr(op);
                                return funcptr0(i->alloc);
                                break;
                            case 1:;
                                t_builtin1 funcptr1 = builtin1_ptr(op);
                                return funcptr1(i->alloc, f->locals[1]);
                                break;
                            case 2:;
                                t_builtin2 funcptr2 = builtin2_ptr(op);
                                return funcptr2(i->alloc, f->locals[1], f->locals[2]);
                                break;
                            case 3:;
                                t_builtin3 funcptr3 = builtin3_ptr(op);
                                return funcptr3(i->alloc,  f->locals[1], f->locals[2], f->locals[3]);
                                break;
                            default:
                                // XXX this is more a programmer error, assert
                                // and abort
                                fprintf(stderr, "Unsupported builtin arity %d\n", builtin_arity(op));
                                return VALUE_NIL;
                        }
                    }
                }
                else if (value_type(op) == TYPE_INTERP_LAMBDA) {
                    // XXX feels as if we can just reuse the current environment if
                    // ->outer is lambda->env, btu somehow that doesn't work...
                    struct interp_lambda *lambda = value_to_interp_lambda(op);
                    f->extra_env_v = make_environment(i->alloc, env_new(i->alloc, value_to_environment(lambda->env_v)));
                    if (lambda->variadic) {
                        int ax;
                        if (arg_count < lambda->arity - 1) {
                            fprintf(stderr, "Arity error in application of variadic lambda: expected a minimum of %i args but got %i\n",
                                lambda->arity, arg_count);
                            return VALUE_NIL;
                        }
                        for (ax = 0; ax < lambda->arity - 1; ax++) {
                            env_bind(i->alloc, value_to_environment(f->extra_env_v), lambda->arg_names[ax], f->locals[ax+1]);
                        }
                        value arg_list = VALUE_EMPTY_LIST;
                        value current_arg = VALUE_NIL;
                        for (int j = ax; j < arg_count; j++) {
                            value temp = make_cons(i->alloc,
                                                f->locals[j+1],
                                                VALUE_EMPTY_LIST);
                            if (arg_list == VALUE_EMPTY_LIST) {
                                arg_list = temp;
                            }
                            else {
                                set_cdr(i->alloc, current_arg, temp);
                            }
                            current_arg = temp;
                        }
                        env_bind(i->alloc, value_to_environment(f->extra_env_v), lambda->arg_names[ax], arg_list);
                    }
                    else {
                        if (lambda->arity != arg_count) {
                            fprintf(stderr, "Arity error in application of lambda: expected %i args but got %i\n",
                                lambda->arity, arg_count);
                            return VALUE_NIL;
                        }
                        for (int idx = 0; idx < arg_count; idx++) {
                            env_bind(i->alloc, value_to_environment(f->extra_env_v), lambda->arg_names[idx], f->locals[idx+1]);
                        }
                    }
                    f->expr = lambda->body;
                    f->env_v = f->extra_env_v;
                    f->extra_env_v = VALUE_NIL;
                    goto tailcall_label;
                }
                else if (value_is_param(op)) {
                    struct param *p = value_to_parameter(op);
                    struct dynamic_frame *dyn_frame_iter = dyn_frame;
                    while (dyn_frame_iter) {
                        if (dyn_frame_iter->param == op) {
                            return dyn_frame_iter->val;
                        }
                        dyn_frame_iter = value_to_dyn_frame(dyn_frame_iter->outer_v);
                    }
                    return p->init;
                }
                else {
                    fprintf(stderr, "No idea how to apply operator of type 0x%lX\n", value_type(op));
                    return VALUE_NIL;
                }
            }
            break;
        default:
            fprintf(stderr, "Unexpected value type 0x%lX in interp_eval\n", value_type(f->expr));
            dump_value(f->expr, stderr);
            fprintf(stderr, "\n");
            return VALUE_NIL;
    }

    // XXX can we ever get here?
    return f->expr;
}

value interp_eval(struct interp *i, value expr) {
    return interp_eval_env(i, NULL, NULL, expr, value_to_environment(i->top_env_v), VALUE_NIL);
}

void interp_add_gc_root_frame(struct allocator_gc_ctx *gc, struct call_frame *f) {
    while (f) {
        allocator_gc_add_root_fp(gc, &f->expr);
        allocator_gc_add_root_fp(gc, &f->env_v);
        allocator_gc_add_root_fp(gc, &f->extra_env_v);
        for (int i = 0; i < NUM_LOCALS; i++) {
            allocator_gc_add_root_fp(gc, &f->locals[i]);
        }
        f = f->outer;
    }
}

void interp_traverse_dynamic_frame(struct allocator_gc_ctx *gc, struct dynamic_frame *df) {
    allocator_gc_add_root_fp(gc, &df->param);
    allocator_gc_add_root_fp(gc, &df->val);
    allocator_gc_add_root_fp(gc, &df->outer_v);
}

void interp_traverse_lambda(struct allocator_gc_ctx *gc, struct interp_lambda *l) {
    allocator_gc_add_root_fp(gc, &l->body);
    allocator_gc_add_root_fp(gc, &l->env_v);
    for (int i = 0; i < l->arity; i++) {
        allocator_gc_add_root_fp(gc, &l->arg_names[i]);
    }
}

void interp_traverse_env_entry(struct allocator_gc_ctx *gc, struct interp_env_entry *ee) {
    allocator_gc_add_root_fp(gc, &ee->name);
    allocator_gc_add_root_fp(gc, &ee->value);
    allocator_gc_add_root_fp(gc, &ee->next_entry_v);
}

void interp_traverse_env(struct allocator_gc_ctx *gc, struct interp_env *env) {
    allocator_gc_add_root_fp(gc, &env->first_entry_v);
    allocator_gc_add_root_fp(gc, &env->outer_v);
    for (int i = 0; i < env->num_inline_entries; i++) {
        allocator_gc_add_root_fp(gc, &env->inline_entries[i].name);
        allocator_gc_add_root_fp(gc, &env->inline_entries[i].value);
    }
}

void interp_gc(struct interp *i) {
    struct allocator_gc_ctx *gc = allocator_gc_new(i->alloc);
    allocator_gc_add_root_fp(gc, &i->top_env_v);
    allocator_gc_add_root_fp(gc, &i->current_dynamic_frame_v);
    interp_add_gc_root_frame(gc, i->current_frame);

    allocator_gc_perform(gc);
}
