#ifndef BUILTINS_H
#define BUILTINS_H

struct allocator;
struct interp_env;

void bind_builtins(struct allocator *alloc, struct interp_env *env);

#endif /* BUILTINS_H */
