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
 *
 * in the case of floats and integers, the top 32 bits contain the value, in the 
 * case of an enumerated value, the next bits are used to specify the exact value:
 *   00 - Nil (internal, not usable in the scheme layer)
 *   01 - True
 *   10 - False
 *   11 - Empty List
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
 *   010 - builtin2 (stuff callable from scheme but written in C, arity 2
 *                   the pointer part is the address of the function to call)
 *
 * XXX will need boxes
 *
 * XXX immediate short string optimisation
 *  */

#if __SIZEOF_POINTER__ != 8
#error "This code targets 64-bit systems only"
#endif

typedef uint64_t value;

#define value_is_immediate(x) ((x) & 1)
#define value_type(x) ((x) & 15)
#define value_to_cell(x) ((x) & ~15)

#define TYPE_INT              0b0001
#define TYPE_FLOAT            0b0011
#define TYPE_ENUM             0b0101

#define TYPE_SYMBOL           0b0000
#define TYPE_CONS             0b0010
#define TYPE_BUILTIN2         0b0100

#define VALUE_NIL           0b000101
#define VALUE_TRUE          0b010101
#define VALUE_FALSE         0b100101
#define VALUE_EMPTY_LIST    0b110101

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

#define symbol(x) ((char*)value_to_cell(x))

/* Builtins
 *
 * */

typedef value (*t_builtin2)(struct allocator*, value, value);

value make_builtin2(struct allocator *a, t_builtin2 funcptr);
t_builtin2 builtin2_ptr(value);

/* Input and Output
 *
 * these routines allow reading and printing of values. They are aimed
 * at development, debugging and bootstrapping, at program runtime equivalent
 * functions from the scheme layer should be used
 * */

void dump_value(value v);

#endif /* TYPES_H */
