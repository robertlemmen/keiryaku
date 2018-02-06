#include "eval.h"

#include <malloc.h>
#include <assert.h>
#include <stdlib.h>

#include "types.h"

#define STACK_SIZE 1*1024*1024*1024

union stack_element {
    value v;
};

struct eval_env {
    struct eval_env *outer;
    value name;
    value value;
};

struct eval_ctx {
    opcode *ip;
    union stack_element *sp;
    union stack_element *fp;
    struct eval_env *env;
};

struct eval_ctx* eval_ctx_new(void) {
    struct eval_ctx *ret = malloc(sizeof(struct eval_ctx));
    ret->ip = NULL;
    ret->sp = malloc(STACK_SIZE);
    ret->fp = ret->sp;
    ret->env = NULL;
    return ret;
}

void eval_ctx_free(struct eval_ctx *ex) {
    assert(ex != NULL);
    assert(ex->sp == ex->fp); // we do not store the base of the stack, this 
                              // is a poor way to detect that the stack has been unwound correctly...
    free(ex->sp);
    free(ex);
}

void eval_ctx_eval(struct eval_ctx *ex, opcode *code) {
    assert(ex != NULL);
    ex->ip = code;
    // XXX
}
