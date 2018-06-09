#ifndef INTERP_H
#define INTERP_H

#include "types.h"
#include "heap.h"

struct interp;
struct interp_lambda;
struct interp_env;

struct interp* interp_new(struct allocator *alloc);
void interp_free(struct interp *i);

value interp_eval(struct interp *i, value expr);

void interp_gc(struct interp *i);
void interp_traverse_lambda(struct allocator_gc_ctx *gc, struct interp_lambda *l);

void env_bind(struct allocator *alloc, struct interp_env *env, value symbol, value value);

#endif /* INTERP_H */
