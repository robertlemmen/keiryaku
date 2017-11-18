#ifndef EVAL_H
#define EVAL_H

/* Virtual Machine
 * 
 * Kenbou uses a virtual machine very similar to many hardware architectures:
 * - a small set of registers
 * - a stack 
 * - an instruction pointer, stack pointer and frame pointer
 * - the ability to reference items on the stack relative to the frame pointer
 *
 * Opcodes are 8 bits wide, followed by a variable number of arguments, whose size
 * depends on the opcode
 * XXX let's ignore continuations and tail calls for a while, closures are more necessary but
 * we can probably play a bit without them...
 *
 *
 *
 * */
