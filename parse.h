#ifndef PARSE_H
#define PARSE_H

#include <stdbool.h>

#include "heap.h"
#include "types.h"

struct parser;

struct parser* parser_new(struct allocator *alloc, 
        void (*callback)(value exp, void *arg), void *cb_arg);
void parser_set_cb_arg(struct parser *p, void *cb_arg);
void parser_free(struct parser *p);
int parser_consume(struct parser *p, char *data);
void parser_eof(struct parser *p);
void parser_gc(struct parser *p);

#endif /* PARSE_H */
