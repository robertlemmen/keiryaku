#ifndef INTERP_H
#define INTERP_H

#include "types.h"
#include "heap.h"

struct interp_ctx;

struct interp_ctx* interp_new(struct allocator *alloc);
void interp_free(struct interp_ctx *i);

value interp_eval(struct interp_ctx *i, value expr);

#endif /* INTERP_H */
