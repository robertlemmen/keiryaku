#ifndef INTERP_H
#define INTERP_H

#include "types.h"
#include "heap.h"

struct interp_ctx;
struct interp_lambda;
struct interp_env;

struct interp_ctx* interp_new(struct allocator *alloc);
void interp_free(struct interp_ctx *i);

value interp_eval(struct interp_ctx *i, value expr);

void interp_gc(struct interp_ctx *i);
void interp_traverse_lambda(struct allocator_gc_ctx *gc, struct interp_lambda *l);

void env_bind(struct allocator *alloc, struct interp_env *env, value symbol, value value);

#endif /* INTERP_H */
