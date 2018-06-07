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
 *   0 - immediate
 *   1 - non-immediate
 *
 * in the case of immediate values, the next 3 bits are used to determine the type
 * of the value: 
 *   000_ - integer
 *   001_ - float
 *   010_ - enumerated
 *   011_ - short symbol
 *   100_ - short string
 *
 * in the case of floats and integers, the top 32 bits contain the value, in the 
 * case of an enumerated value, the next bits are used to specify the exact value:
 *   0000____ - Nil (internal, not usable in the scheme layer)
 *   0001____ - True
 *   0010____ - False
 *   0011____ - Empty List
 *
 * 'Specials' all have the fourth bit of that set, to make testing for them
 * easier:
 * XXX would be nicer if this was the lowest-order bit
 *   1000____ - 'If' Special
 *   1001____ - 'Define' Special
 *   1010____ - 'Lambda' Special
 *   1011____ - 'Begin' Special
 *   1100____ - 'Quote' Special
 *   1101____ - 'Let' Special
 *   1110____ - 'Apply' Special
 *
 * this means that enumerated "special" values can be compared for equality 
 * directly, by comparing the value bitwise.
 *
 * in the case of a non-immediate value, the next three bits are used to store 
 * type information, and the remaining bits contain the pointer (when the lower 
 * three bits are masked off) of the heap cell. We have a set of non-immediate 
 * types:
 *   000_ - symbol
 *   001_ - string
 *   010_ - cons cell
 *   011_ - builtin (stuff callable from scheme but written in C, with arity)
 *   100_ - interpreter lambda
 *   101_ - vector
 *
 * XXX will need boxes ?
 *  */

// XXX we have to make sure that we never have CONS entries that are empty,
// otherwise we need to support EMPTY_LIST == CONS. this might mean we can get
// rid of NIL as well

// XXX clever restructuring of this could mean EMPTY_LIST = 0 

#if __SIZEOF_POINTER__ != 8
#error "This code targets 64-bit systems only"
#endif

typedef uint64_t value;

#define value_is_immediate(x) (!(((x) & 1)))
#define value_type(x) ((x) & 15)
#define value_to_cell(x) (void*)((x) & ~15) // XXX should be called block?

#define TYPE_ENUM             0b0000
#define TYPE_INT              0b0010
#define TYPE_FLOAT            0b0100
#define TYPE_SHORT_SYMBOL     0b0110
#define TYPE_SHORT_STRING     0b1000

#define TYPE_SYMBOL           0b0001
#define TYPE_STRING           0b0011
#define TYPE_CONS             0b0101
#define TYPE_BUILTIN          0b0111
#define TYPE_INTERP_LAMBDA    0b1001
#define TYPE_VECTOR           0b1011

// XXX we should not need a nil, but then we need to make sure there are no CONS
// that are empty, they should all be EMPTY_LIST. then EMPTY_LIST could be == 0
#define VALUE_NIL         0b00000000
#define VALUE_TRUE        0b00010000
#define VALUE_FALSE       0b00100000
#define VALUE_EMPTY_LIST  0b00110000

#define VALUE_SP_IF       0b10000000
#define VALUE_SP_DEFINE   0b10010000
#define VALUE_SP_LAMBDA   0b10100000
#define VALUE_SP_BEGIN    0b10110000
#define VALUE_SP_QUOTE    0b11000000
#define VALUE_SP_LET      0b11010000
#define VALUE_SP_APPLY    0b11100000

#define value_is_special(x) (((x) & 0b10001111) == 0b10000000)

/* values are considered true if they are not #f or an empty list */
#define value_is_true(x) (((x) & 0b11101111) != VALUE_FALSE)

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
#define value_is_symbol(x) ((value_type(x) == TYPE_SHORT_SYMBOL) || (value_type(x) == TYPE_SYMBOL))

value make_symbol(struct allocator *a, char *s);
/* weirdly we need to pass this by address due to the short string optimization.
 * also means that the caller has to be very careful as the value returned from
 * this does not outlive the value passed in! */
char* value_to_symbol(value *s);

/* Strings
 *
 * Strings work just like symbols, so the same conditions apply
 * */
#define value_is_string(x) ((value_type(x) == TYPE_SHORT_STRING) || (value_type(x) == TYPE_STRING))

value make_string(struct allocator *a, char *s);
/* weirdly we need to pass this by address due to the short string optimization.
 * also means that the caller has to be very careful as the value returned from
 * this does not outlive the value passed in! */
char* value_to_string(value *s);

/* Builtins
 * */

int builtin_arity(value);

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
