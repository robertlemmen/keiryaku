#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

#include "heap.h"

/* Value Representation
 *
 * values are represented as 64-bit, using tagging in the low order bits. This is
 * targeting a 64bit pointer size, and the heap objects are aligned to 16bytes, so
 * the lower 4 bits are used for tagging. a value can either hold a "immediate" value,
 * or hold a pointer to a heap-allocated object. the lowest order bit is used to
 * distinguish between immmediate and non-immediate values.
 *   0 - non-immediate
 *   1 - immediate
 *
 * in the case of immediate values, the next 3 bits are used to determine the type
 * of the value: 
 *   000 - integer
 *   001 - float 
 *   010 - enumerated. 
 *   011 - short symbol
 *
 * in the case of floats and integers, the top 32 bits contain the value, in the 
 * case of an enumerated value, the next bits are used to specify the exact value:
 *   0000 - Nil (internal, not usable in the scheme layer)
 *   0001 - True
 *   0010 - False
 *   0011 - Empty List
 *
 * 'Specials' all have the fourth bit of that set, to make testing for them
 * easier:
 *   1000 'If' Special
 *   1001 'Define' Special
 *   1010 'Lambda' Special
 *   1011 'Begin' Special
 *   1100 'Quote' Special
 *   1101 'Let' Special
 *   1110 'Apply' Special
 *
 * this means that enumerated "special" values can be compared for equality 
 * directly, by comparing the value bitwise.
 *
 * in the case of a non-immediate value, the next three bits are used to store 
 * type information, and the remaining bits contain the pointer (when the lower 
 * three bits are masked off) of the heap cell. We have a set of non-immediate 
 * types:
 *   000 - symbol
 *   001 - cons
 *   010 - builtin1 (stuff callable from scheme but written in C, arity 1
 *                   the pointer part is the address of the function to call)
 *   011 - builtin2
 *   100 - interpreter lambda
 *   101 - vector
 *
 * XXX will need boxes
 *  */

#if __SIZEOF_POINTER__ != 8
#error "This code targets 64-bit systems only"
#endif

typedef uint64_t value;

#define value_is_immediate(x) (((x) & 1))
#define value_type(x) ((x) & 15)
#define value_to_cell(x) (void*)((x) & ~15) // XXX should be called block?

#define TYPE_INT              0b0001
#define TYPE_FLOAT            0b0011
#define TYPE_ENUM             0b0101
#define TYPE_SHORT_SYMBOL     0b0111

#define TYPE_SYMBOL           0b0000
#define TYPE_CONS             0b0010
#define TYPE_BUILTIN1         0b0100
#define TYPE_BUILTIN2         0b0110
#define TYPE_BUILTIN3         0b0111
#define TYPE_INTERP_LAMBDA    0b1000
#define TYPE_VECTOR           0b1010

#define VALUE_NIL         0b00000101
#define VALUE_TRUE        0b00010101
#define VALUE_FALSE       0b00100101
#define VALUE_EMPTY_LIST  0b00110101

#define VALUE_SP_IF       0b10000101
#define VALUE_SP_DEFINE   0b10010101
#define VALUE_SP_LAMBDA   0b10100101
#define VALUE_SP_BEGIN    0b10110101
#define VALUE_SP_QUOTE    0b11000101
#define VALUE_SP_LET      0b11010101
#define VALUE_SP_APPLY    0b11100101

#define value_is_special(x) (((x) & 0b10001111) == 0b10000101)

/* symbols can be immediate/short or non-immediate */
#define value_is_symbol(x) ((value_type(x) == TYPE_SHORT_SYMBOL) || (value_type(x) == TYPE_SYMBOL))

/* values are considered true if they are not #f or an empty list */
#define value_is_true(x) (((x) & 0b11101111) != 0b00100101)

#define intval(x) ((int32_t)((x) >> 32))
#define make_int(a, x) (((uint64_t)(x) << 32) | TYPE_INT)
// XXX float are broken, need reinterpret_cast style casting
#define floatval(x) ((float)((x) >> 32))
#define make_float(a, x) (((uint64_t)(x) << 32) | TYPE_FLOAT)

/* Cons cells
 *
 * in our implementation, cons cells are simply two values. this means it is
 * the most simple heap cell imaginable.
 * */

struct cons {
    value car;
    value cdr;
};

value make_cons(struct allocator *a, value car, value cdr);

#define car(x) (((struct cons*)value_to_cell(x))->car)
#define cdr(x) (((struct cons*)value_to_cell(x))->cdr)

#define set_car(x, y) (((struct cons*)value_to_cell(x))->car = y)
#define set_cdr(x, y) (((struct cons*)value_to_cell(x))->cdr = y)

/* Symbols
 *
 * Symbols are represented on the heap as a null-terminated string
 * */
value make_symbol(struct allocator *a, char *s);
/* weirdly we need to pass this by address due to the short string optimization.
 * also means that the caller has to be very careful as the value returned from
 * this does not outlive the value passed in! */
char* value_to_symbol(value *s);

/* Builtins
 * */

typedef value (*t_builtin1)(struct allocator*, value);
typedef value (*t_builtin2)(struct allocator*, value, value);
typedef value (*t_builtin3)(struct allocator*, value, value, value);

value make_builtin1(struct allocator *a, t_builtin1 funcptr);
t_builtin1 builtin1_ptr(value);

value make_builtin2(struct allocator *a, t_builtin2 funcptr);
t_builtin2 builtin2_ptr(value);

value make_builtin3(struct allocator *a, t_builtin3 funcptr);
t_builtin3 builtin3_ptr(value);

/* Lambdas
 * */
#define make_interp_lambda(x) ((uint64_t)(x) | TYPE_INTERP_LAMBDA)
#define value_to_interp_lambda(x) ((struct interp_lambda*)value_to_cell(x))

/* Vectors
 * */
value make_vector(struct allocator *a, int length, value fill);
int vector_length(value v);
value vector_ref(value v, int pos);
void vector_set(value v, int pos, value i);
void traverse_vector(struct allocator_gc_ctx *gc, value v);

/* Input and Output
 *
 * these routines allow reading and printing of values. They are aimed
 * at development, debugging and bootstrapping, at program runtime equivalent
 * functions from the scheme layer should be used
 * */

void dump_value(value v);

#endif /* TYPES_H */
