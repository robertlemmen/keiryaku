#include "builtins.h"

#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "interp.h"
#include "ports.h"

value builtin_plus(struct allocator *alloc, value l) {
    int ret = 0;
    while (value_type(l) == TYPE_CONS) {
        value num = car(l);
        assert(value_type(num) == TYPE_INT);
        ret += intval(num);
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
        value num = car(l);
        assert(value_type(num) == TYPE_INT);
        ret -= intval(num);
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
        value num = car(l);
        assert(value_type(num) == TYPE_INT);
        ret *= intval(num);
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
        value num = car(l);
        assert(value_type(num) == TYPE_INT);
        ret /= intval(num);
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
    if (builtin_eq(alloc, a, b) != VALUE_FALSE) {
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
    if (builtin_eqv(alloc, a, b) != VALUE_FALSE) {
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
    set_car(alloc, p, v);
    return VALUE_NIL;
}

value builtin_set_cdr(struct allocator *alloc, value p, value v) {
    assert(value_type(p) == TYPE_CONS);
    set_cdr(alloc, p, v);
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
    return value_is_port(v) ? VALUE_TRUE : VALUE_FALSE;
}

value builtin_input_port(struct allocator *alloc, value v) {
    return port_in(v) ? VALUE_TRUE : VALUE_FALSE;
}

value builtin_output_port(struct allocator *alloc, value v) {
    return port_out(v) ? VALUE_TRUE : VALUE_FALSE;
}

value builtin_text_port(struct allocator *alloc, value v) {
    return port_text(v) ? VALUE_TRUE : VALUE_FALSE;
}

value builtin_binary_port(struct allocator *alloc, value v) {
    return port_binary(v) ? VALUE_TRUE : VALUE_FALSE;
}

value builtin_write(struct allocator *alloc, value o, value p) {
    port_write(p, o);
    return VALUE_NIL;
}

value builtin_write_string(struct allocator *alloc, value o, value p) {
    port_write_string(p, o);
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
    return v == VALUE_EOF ? VALUE_TRUE : VALUE_FALSE;
}

value builtin_mk_end_of_file(struct allocator *alloc) {
    return VALUE_EOF;
}

value builtin_procedure(struct allocator *alloc, value v) {
    return ( (value_type(v) == TYPE_INTERP_LAMBDA)
             || (value_type(v) == TYPE_BUILTIN)
             || value_is_param(v) )
        ? VALUE_TRUE
        : VALUE_FALSE;
}

value builtin_compile_stub(struct allocator *alloc, value v) {
    // XXX warrants explanation
    return v;
}

value builtin_request_gc(struct allocator *alloc, value full) {
    allocator_request_gc(alloc, full != VALUE_FALSE);
    return VALUE_NIL;
}

value builtin_make_parameter(struct allocator *alloc, value i, value c) {
    // XXX perhaps check that c is a procedure
    return make_parameter(alloc, i, c);
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

void bind_builtin_helper(struct allocator *alloc, struct interp_env *env, value builtin) {
    env_bind(alloc, env, make_symbol(alloc, builtin_name(builtin)), builtin);
}

void bind_builtins(struct allocator *alloc, struct interp_env *env) {
    // create basic environment
    bind_builtin_helper(alloc, env, make_builtinv(alloc, &builtin_plus, "+"));
    bind_builtin_helper(alloc, env, make_builtinv(alloc, &builtin_minus, "-"));
    bind_builtin_helper(alloc, env, make_builtinv(alloc, &builtin_mul, "*"));
    bind_builtin_helper(alloc, env, make_builtinv(alloc, &builtin_div, "/"));
    bind_builtin_helper(alloc, env, make_builtin2(alloc, &builtin_numeric_equals, "="));
    bind_builtin_helper(alloc, env, make_builtin2(alloc, &builtin_lt, "<"));
    bind_builtin_helper(alloc, env, make_builtin2(alloc, &builtin_gt, ">"));
    bind_builtin_helper(alloc, env, make_builtin2(alloc, &builtin_le, "<="));
    bind_builtin_helper(alloc, env, make_builtin2(alloc, &builtin_ge, ">="));
    bind_builtin_helper(alloc, env, make_builtin2(alloc, &builtin_expt, "expt"));
    bind_builtin_helper(alloc, env, make_builtin1(alloc, &builtin_car, "car"));
    bind_builtin_helper(alloc, env, make_builtin1(alloc, &builtin_cdr, "cdr"));
    bind_builtin_helper(alloc, env, make_builtin2(alloc, &make_cons, "cons"));
    bind_builtin_helper(alloc, env, make_builtin1(alloc, &builtin_pair, "pair?"));
    bind_builtin_helper(alloc, env, make_builtin2(alloc, &builtin_eq, "eq?"));
    bind_builtin_helper(alloc, env, make_builtin2(alloc, &builtin_eqv, "eqv?"));
    bind_builtin_helper(alloc, env, make_builtin2(alloc, &builtin_equal, "equal?"));
    bind_builtin_helper(alloc, env, make_builtin1(alloc, &builtin_null, "null?"));
    bind_builtin_helper(alloc, env, make_builtin1(alloc, &builtin_nil, "_nil?"));
    bind_builtin_helper(alloc, env, make_builtin1(alloc, &builtin_number, "number?"));
    bind_builtin_helper(alloc, env, make_builtin1(alloc, &builtin_integer, "integer?"));
    bind_builtin_helper(alloc, env, make_builtin1(alloc, &builtin_string, "string?"));
    bind_builtin_helper(alloc, env, make_builtin1(alloc, &builtin_symbol, "symbol?"));
    bind_builtin_helper(alloc, env, make_builtin1(alloc, &builtin_string_length, "string-length"));
    bind_builtin_helper(alloc, env, make_builtin2(alloc, &builtin_string_eq, "string=?"));
    bind_builtin_helper(alloc, env, make_builtin2(alloc, &builtin_symbol_eq, "symbol=?"));
    bind_builtin_helper(alloc, env, make_builtin2(alloc, &builtin_make_vector, "make-vector"));
    bind_builtin_helper(alloc, env, make_builtin1(alloc, &builtin_vector, "vector?"));
    bind_builtin_helper(alloc, env, make_builtin1(alloc, &builtin_vector_length, "vector-length"));
    bind_builtin_helper(alloc, env, make_builtin2(alloc, &builtin_vector_ref, "vector-ref"));
    bind_builtin_helper(alloc, env, make_builtin3(alloc, &builtin_vector_set, "vector-set!"));
    bind_builtin_helper(alloc, env, make_builtin1(alloc, &builtin_boolean, "boolean?"));
    bind_builtin_helper(alloc, env, make_builtin2(alloc, &builtin_set_car, "set-car!"));
    bind_builtin_helper(alloc, env, make_builtin2(alloc, &builtin_set_cdr, "set-cdr!"));
    bind_builtin_helper(alloc, env, make_builtin1(alloc, &builtin_list, "list?"));
    bind_builtin_helper(alloc, env, make_builtin1(alloc, &builtin_port, "port?"));
    bind_builtin_helper(alloc, env, make_builtin1(alloc, &builtin_input_port, "input-port?"));
    bind_builtin_helper(alloc, env, make_builtin1(alloc, &builtin_output_port, "output-port?"));
    bind_builtin_helper(alloc, env, make_builtin1(alloc, &builtin_text_port, "text-port?"));
    bind_builtin_helper(alloc, env, make_builtin1(alloc, &builtin_binary_port, "binary-port?"));
    bind_builtin_helper(alloc, env, make_builtin2(alloc, &builtin_write, "write"));
    bind_builtin_helper(alloc, env, make_builtin2(alloc, &builtin_write_string, "write-string"));
    bind_builtin_helper(alloc, env, make_builtin1(alloc, &builtin_open_input_file, "open-input-file"));
    bind_builtin_helper(alloc, env, make_builtin1(alloc, &builtin_close_port, "close-port"));
    bind_builtin_helper(alloc, env, make_builtin1(alloc, &builtin_read, "read"));
    bind_builtin_helper(alloc, env, make_builtin1(alloc, &builtin_end_of_file, "eof-object?"));
    bind_builtin_helper(alloc, env, make_builtin0(alloc, &builtin_mk_end_of_file, "eof-object"));
    bind_builtin_helper(alloc, env, make_builtin1(alloc, &builtin_procedure, "procedure?"));
    bind_builtin_helper(alloc, env, make_builtin2(alloc, &builtin_make_parameter, "_make-parameter"));
    bind_builtin_helper(alloc, env, make_builtin1(alloc, &builtin_compile_stub, "_compile"));
    bind_builtin_helper(alloc, env, make_builtin1(alloc, &builtin_request_gc, "_request_gc"));
    // XXX probably should not be a built-in, but enables a good test case for
    // named let
    bind_builtin_helper(alloc, env, make_builtin2(alloc, &builtin_quotient, "truncate-quotient"));
    bind_builtin_helper(alloc, env, make_builtin2(alloc, &builtin_remainder, "truncate-remainder"));
}
