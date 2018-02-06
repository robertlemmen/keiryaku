#ifndef EVAL_H
#define EVAL_H

#include <stdint.h>

/* Virtual Machine
 * 
 * Keiryaku uses a virtual machine very similar to many hardware architectures:
 * - a small set of registers (8)
 * - a stack 
 * - an instruction pointer, stack pointer and frame pointer
 * - the ability to reference items on the stack relative to the frame pointer
 * - items on the stack are 64bit, so can hold a value or a code pointer
 *
 * We allocate insanely large stacks and do not check for stack overflows, risk is a 
 * gift after all! This takes less actual memory than one migt fear, as teh OS does the
 * lazy paging for us.
 *
 * Opcodes are 8 bits wide, followed by a variable number of arguments, whose size
 * depends on the opcode
 * XXX let's ignore continuations and tail calls for a while, closures are more necessary but
 * we can probably play a bit without them...
 * */

typedef uint8_t opcode;

struct eval_ctx;

struct eval_ctx* eval_ctx_new(void);
void eval_ctx_free(struct eval_ctx *ex);

void eval_ctx_eval(struct eval_ctx *ex, opcode *code);

#endif /* EVAL_H */
