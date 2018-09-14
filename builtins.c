#include "builtins.h"

#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "interp.h"
#include "ports.h"

value builtin_plus(struct allocator *alloc, value l) {
    int ret = 0;
    while (value_type(l) == TYPE_CONS) {
        ret += intval(car(l));
        l = cdr(l);    
    }

    return make_int(alloc, ret);
}

value builtin_minus(struct allocator *alloc, value l) {
    assert(value_type(l) == TYPE_CONS);
    int ret = 0;
    int count = 0;
    int first = 0;
    while (value_type(l) == TYPE_CONS) {
        ret -= intval(car(l));
        if (!count) {
            first = ret;
        }
        l = cdr(l);
        count++;
    }
    if (count > 1) {
        ret -= first*2;
    }
    return make_int(alloc, ret);
}

value builtin_mul(struct allocator *alloc, value l) {
    int ret = 1;
    while (value_type(l) == TYPE_CONS) {
        ret *= intval(car(l));
        l = cdr(l);    
    }

    return make_int(alloc, ret);
}

value builtin_div(struct allocator *alloc, value l) {
    // XXX arity-one case doesn't work with this logic, but we don't have
    // rationals anyway...
    assert(value_type(l) == TYPE_CONS);
    int ret = intval(car(l));
    l = cdr(l);    
    while (value_type(l) == TYPE_CONS) {
        ret /= intval(car(l));
        l = cdr(l);    
    }

    return make_int(alloc, ret);
}

value builtin_numeric_equals(struct allocator *alloc, value a, value b) {
    assert(value_type(a) == TYPE_INT);
    assert(value_type(b) == TYPE_INT);

    return intval(a) == intval(b) ? VALUE_TRUE : VALUE_FALSE;
}

value builtin_lt(struct allocator *alloc, value a, value b) {
    assert(value_type(a) == TYPE_INT);
    assert(value_type(b) == TYPE_INT);

    return intval(a) < intval(b) ? VALUE_TRUE : VALUE_FALSE;
}

value builtin_gt(struct allocator *alloc, value a, value b) {
    assert(value_type(a) == TYPE_INT);
    assert(value_type(b) == TYPE_INT);

    return intval(a) > intval(b) ? VALUE_TRUE : VALUE_FALSE;
}

value builtin_le(struct allocator *alloc, value a, value b) {
    assert(value_type(a) == TYPE_INT);
    assert(value_type(b) == TYPE_INT);

    return intval(a) <= intval(b) ? VALUE_TRUE : VALUE_FALSE;
}

value builtin_ge(struct allocator *alloc, value a, value b) {
    assert(value_type(a) == TYPE_INT);
    assert(value_type(b) == TYPE_INT);

    return intval(a) >= intval(b) ? VALUE_TRUE : VALUE_FALSE;
}

value builtin_expt(struct allocator *alloc, value a, value b) {
    assert(value_type(a) == TYPE_INT);
    assert(value_type(b) == TYPE_INT);

    int result = 1;
    int base = intval(a);
    int exp = intval(b);
    while (exp) {
        if (exp & 1) {
            result *= base;
        }
        exp >>= 1;
        base *= base;
    }

    return make_int(alloc, result);
}

value builtin_car(struct allocator *alloc, value v) {
    assert(value_type(v) == TYPE_CONS);

    return car(v);
}

value builtin_cdr(struct allocator *alloc, value v) {
    assert(value_type(v) == TYPE_CONS);

    return cdr(v);
}

value builtin_pair(struct allocator *alloc, value v) {
    return (value_type(v) == TYPE_CONS)
        ? VALUE_TRUE
        : VALUE_FALSE;
}

value builtin_eq(struct allocator *alloc, value a, value b) {
    if (a == b) {
        return VALUE_TRUE;
    }
    if (value_is_symbol(a) && value_is_symbol(b)) {
        if (strcmp(value_to_symbol(&a), value_to_symbol(&b)) == 0) {
            return VALUE_TRUE;
        }
    }
    return VALUE_FALSE;
}

value builtin_eqv(struct allocator *alloc, value a, value b) {
    if (builtin_eq(alloc, a, b) == VALUE_TRUE) {
        return VALUE_TRUE;
    }
    if (value_is_string(a) && value_is_string(b)) {
        if (strcmp(value_to_string(&a), value_to_string(&b)) == 0) {
            return VALUE_TRUE;
        }
    }
    return VALUE_FALSE;
}

// XXX equal? needs to support cyclic structures
value builtin_equal(struct allocator *alloc, value a, value b) {
tailcall_label:
    if (builtin_eqv(alloc, a, b) == VALUE_TRUE) {
        return VALUE_TRUE;
    }
    if ((value_type(a) == TYPE_CONS) && (value_type(b) == TYPE_CONS)) {
        if (builtin_equal(alloc, car(a), car(b)) == VALUE_FALSE) {
            return VALUE_FALSE;
        }
        a = cdr(a);
        b = cdr(b);
        goto tailcall_label;
    }
    // XXX vectors
    return VALUE_FALSE;
}

value builtin_null(struct allocator *alloc, value v) {
    // XXX we would need a way to make sure every cons cell with nil+nil gets
    // reduced to VALUE_EMPTY_LIST
    return (v == VALUE_EMPTY_LIST)
        ? VALUE_TRUE
        : VALUE_FALSE;
}

value builtin_nil(struct allocator *alloc, value v) {
    return (v == VALUE_NIL)
        ? VALUE_TRUE
        : VALUE_FALSE;
}

value builtin_number(struct allocator *alloc, value v) {
    return (value_type(v) == TYPE_INT)
        ? VALUE_TRUE
        : VALUE_FALSE;
}

value builtin_integer(struct allocator *alloc, value v) {
    return (value_type(v) == TYPE_INT)
        ? VALUE_TRUE
        : VALUE_FALSE;
}

value builtin_string(struct allocator *alloc, value v) {
    return value_is_string(v)
        ? VALUE_TRUE
        : VALUE_FALSE;
}

value builtin_symbol(struct allocator *alloc, value v) {
    return value_is_symbol(v)
        ? VALUE_TRUE
        : VALUE_FALSE;
}

value builtin_string_length(struct allocator *alloc, value v) {
    assert(value_is_string(v));
    return make_int(alloc, strlen(value_to_string(&v)));
}

value builtin_string_eq(struct allocator *alloc, value a, value b) {
    assert(value_is_string(a));
    assert(value_is_string(b));
    if (a == b) {
        return VALUE_TRUE;
    }
    return (strcmp(value_to_string(&a), value_to_string(&b)) == 0)
        ? VALUE_TRUE
        : VALUE_FALSE;
}

value builtin_symbol_eq(struct allocator *alloc, value a, value b) {
    assert(value_is_symbol(a));
    assert(value_is_symbol(b));
    if (a == b) {
        return VALUE_TRUE;
    }
    return (strcmp(value_to_string(&a), value_to_string(&b)) == 0)
        ? VALUE_TRUE
        : VALUE_FALSE;
    // XXX we could possibly use the comparison to slowly make all equivalent
    // symbols the same actual heap address. but perhaps that is better done
    // when creating the symbol in the first place? It is also unclear whether
    // it would ever help, or whether we copy the value before comparing and
    // therefore loosing the original location
}

// XXX we need a compiler transfrom to support (make-vector 12) without "fill"
value builtin_make_vector(struct allocator *alloc, value l, value f) {
    assert(value_type(l) == TYPE_INT);
    return make_vector(alloc, intval(l), f);
}

value builtin_vector(struct allocator *alloc, value v) {
    return (value_type(v) == TYPE_VECTOR)
        ? VALUE_TRUE
        : VALUE_FALSE;
}

value builtin_vector_length(struct allocator *alloc, value v) {
    assert(value_type(v) == TYPE_VECTOR);
    return make_int(alloc, vector_length(v));
}

value builtin_vector_ref(struct allocator *alloc, value v, value p) {
    assert(value_type(v) == TYPE_VECTOR);
    assert(value_type(p) == TYPE_INT);
    return vector_ref(v, intval(p));
}

value builtin_vector_set(struct allocator *alloc, value v, value p, value i) {
    assert(value_type(v) == TYPE_VECTOR);
    assert(value_type(p) == TYPE_INT);
    vector_set(v, intval(p), i);
    return VALUE_NIL;
}

value builtin_boolean(struct allocator *alloc, value v) {
    return ((v == VALUE_TRUE) || (v == VALUE_FALSE))
        ? VALUE_TRUE
        : VALUE_FALSE;
}

value builtin_set_car(struct allocator *alloc, value p, value v) {
    assert(value_type(p) == TYPE_CONS);
    set_car(p, v);
    return VALUE_NIL;
}

value builtin_set_cdr(struct allocator *alloc, value p, value v) {
    assert(value_type(p) == TYPE_CONS);
    set_cdr(p, v);
    return VALUE_NIL;
}

value builtin_list(struct allocator *alloc, value l) {
tailcall_label:
    if (l == VALUE_EMPTY_LIST) {
        return VALUE_TRUE;
    }
    else if (value_type(l) == TYPE_CONS) {
        l = cdr(l);
        goto tailcall_label;
    }
    else {
        return VALUE_FALSE;
    }
}

value builtin_port(struct allocator *alloc, value v) {
    return (value_type(v) == TYPE_PORT)
        ? VALUE_TRUE
        : VALUE_FALSE;
}

value builtin_input_port(struct allocator *alloc, value v) {
    return port_in(v)
        ? VALUE_TRUE
        : VALUE_FALSE;
}

value builtin_output_port(struct allocator *alloc, value v) {
    return port_out(v)
        ? VALUE_TRUE
        : VALUE_FALSE;
}

value builtin_text_port(struct allocator *alloc, value v) {
    return port_text(v)
        ? VALUE_TRUE
        : VALUE_FALSE;
}

value builtin_binary_port(struct allocator *alloc, value v) {
    return port_binary(v)
        ? VALUE_TRUE
        : VALUE_FALSE;
}

value builtin_newline(struct allocator *alloc, value p) {
    port_newline(p);
    return VALUE_NIL;
}

value builtin_display(struct allocator *alloc, value o, value p) {
    port_display(p, o);
    return VALUE_NIL;
}

value builtin_open_input_file(struct allocator *alloc, value f) {
    return port_open_input_file(alloc, f);
}

value builtin_close_port(struct allocator *alloc, value p) {
    port_close(p);
    return VALUE_NIL;
}

value builtin_read(struct allocator *alloc, value p) {
    return port_read(p);
}

value builtin_end_of_file(struct allocator *alloc, value v) {
    return v == VALUE_EOF
        ? VALUE_TRUE
        : VALUE_FALSE;
}

value builtin_mk_end_of_file(struct allocator *alloc) {
    return VALUE_EOF;
}

value builtin_procedure(struct allocator *alloc, value v) {
    return ((value_type(v) == TYPE_INTERP_LAMBDA) || (value_type(v) == TYPE_BUILTIN))
        ? VALUE_TRUE
        : VALUE_FALSE;
}

value builtin_compile_stub(struct allocator *alloc, value v) {
    // XXX warrants explanation
    return v;
}

value builtin_quotient(struct allocator *alloc, value a, value b) {
    assert(value_type(a) == TYPE_INT);
    assert(value_type(b) == TYPE_INT);

    return make_int(alloc, intval(a) / intval(b));
}

value builtin_remainder(struct allocator *alloc, value a, value b) {
    assert(value_type(a) == TYPE_INT);
    assert(value_type(b) == TYPE_INT);

    return make_int(alloc, intval(a) % intval(b));
}

void bind_builtins(struct allocator *alloc, struct interp_env *env) {
    // create basic environment
    env_bind(alloc, env, make_symbol(alloc, "+"), make_builtinv(alloc, &builtin_plus));
    env_bind(alloc, env, make_symbol(alloc, "-"), make_builtinv(alloc, &builtin_minus));
    env_bind(alloc, env, make_symbol(alloc, "*"), make_builtinv(alloc, &builtin_mul));
    env_bind(alloc, env, make_symbol(alloc, "/"), make_builtinv(alloc, &builtin_div));
    env_bind(alloc, env, make_symbol(alloc, "="), make_builtin2(alloc, &builtin_numeric_equals));
    env_bind(alloc, env, make_symbol(alloc, "<"), make_builtin2(alloc, &builtin_lt));
    env_bind(alloc, env, make_symbol(alloc, ">"), make_builtin2(alloc, &builtin_gt));
    env_bind(alloc, env, make_symbol(alloc, "<="), make_builtin2(alloc, &builtin_le));
    env_bind(alloc, env, make_symbol(alloc, ">="), make_builtin2(alloc, &builtin_ge));
    env_bind(alloc, env, make_symbol(alloc, "expt"), make_builtin2(alloc, &builtin_expt));
    env_bind(alloc, env, make_symbol(alloc, "car"), make_builtin1(alloc, &builtin_car));
    env_bind(alloc, env, make_symbol(alloc, "cdr"), make_builtin1(alloc, &builtin_cdr));
    env_bind(alloc, env, make_symbol(alloc, "cons"), make_builtin2(alloc, &make_cons));
    env_bind(alloc, env, make_symbol(alloc, "pair?"), make_builtin1(alloc, &builtin_pair));
    env_bind(alloc, env, make_symbol(alloc, "eq?"), make_builtin2(alloc, &builtin_eq));
    env_bind(alloc, env, make_symbol(alloc, "eqv?"), make_builtin2(alloc, &builtin_eqv));
    env_bind(alloc, env, make_symbol(alloc, "equal?"), make_builtin2(alloc, &builtin_equal));
    env_bind(alloc, env, make_symbol(alloc, "null?"), make_builtin1(alloc, &builtin_null));
    env_bind(alloc, env, make_symbol(alloc, "_nil?"), make_builtin1(alloc, &builtin_nil));
    env_bind(alloc, env, make_symbol(alloc, "number?"), make_builtin1(alloc, &builtin_number));
    env_bind(alloc, env, make_symbol(alloc, "integer?"), make_builtin1(alloc, &builtin_integer));
    env_bind(alloc, env, make_symbol(alloc, "string?"), make_builtin1(alloc, &builtin_string));
    env_bind(alloc, env, make_symbol(alloc, "symbol?"), make_builtin1(alloc, &builtin_symbol));
    env_bind(alloc, env, make_symbol(alloc, "string-length"), make_builtin1(alloc, &builtin_string_length));
    env_bind(alloc, env, make_symbol(alloc, "string=?"), make_builtin2(alloc, &builtin_string_eq));
    env_bind(alloc, env, make_symbol(alloc, "symbol=?"), make_builtin2(alloc, &builtin_symbol_eq));
    env_bind(alloc, env, make_symbol(alloc, "make-vector"), make_builtin2(alloc, &builtin_make_vector));
    env_bind(alloc, env, make_symbol(alloc, "vector?"), make_builtin1(alloc, &builtin_vector));
    env_bind(alloc, env, make_symbol(alloc, "vector-length"), make_builtin1(alloc, &builtin_vector_length));
    env_bind(alloc, env, make_symbol(alloc, "vector-ref"), make_builtin2(alloc, &builtin_vector_ref));
    env_bind(alloc, env, make_symbol(alloc, "vector-set!"), make_builtin3(alloc, &builtin_vector_set));
    env_bind(alloc, env, make_symbol(alloc, "boolean?"), make_builtin1(alloc, &builtin_boolean));
    env_bind(alloc, env, make_symbol(alloc, "set-car!"), make_builtin2(alloc, &builtin_set_car));
    env_bind(alloc, env, make_symbol(alloc, "set-cdr!"), make_builtin2(alloc, &builtin_set_cdr));
    env_bind(alloc, env, make_symbol(alloc, "list?"), make_builtin1(alloc, &builtin_list));
    env_bind(alloc, env, make_symbol(alloc, "port?"), make_builtin1(alloc, &builtin_port));
    env_bind(alloc, env, make_symbol(alloc, "input-port?"), make_builtin1(alloc, &builtin_input_port));
    env_bind(alloc, env, make_symbol(alloc, "output-port?"), make_builtin1(alloc, &builtin_output_port));
    env_bind(alloc, env, make_symbol(alloc, "text-port?"), make_builtin1(alloc, &builtin_text_port));
    env_bind(alloc, env, make_symbol(alloc, "binary-port?"), make_builtin1(alloc, &builtin_binary_port));
    env_bind(alloc, env, make_symbol(alloc, "newline"), make_builtin1(alloc, &builtin_newline));
    env_bind(alloc, env, make_symbol(alloc, "display"), make_builtin2(alloc, &builtin_display));
    env_bind(alloc, env, make_symbol(alloc, "open-input-file"), make_builtin1(alloc, &builtin_open_input_file));
    env_bind(alloc, env, make_symbol(alloc, "close-port"), make_builtin1(alloc, &builtin_close_port));
    env_bind(alloc, env, make_symbol(alloc, "read"), make_builtin1(alloc, &builtin_read));
    env_bind(alloc, env, make_symbol(alloc, "eof-object?"), make_builtin1(alloc, &builtin_end_of_file));
    env_bind(alloc, env, make_symbol(alloc, "eof-object"), make_builtin0(alloc, &builtin_mk_end_of_file));
    env_bind(alloc, env, make_symbol(alloc, "procedure?"), make_builtin1(alloc, &builtin_procedure));
    env_bind(alloc, env, make_symbol(alloc, "_compile"), make_builtin1(alloc, &builtin_compile_stub));
    // XXX probably should not be a built-in, but enables a good test case for
    // named let
    env_bind(alloc, env, make_symbol(alloc, "truncate-quotient"), make_builtin2(alloc, &builtin_quotient));
    env_bind(alloc, env, make_symbol(alloc, "truncate-remainder"), make_builtin2(alloc, &builtin_remainder));
}
