#ifndef PARSE_H
#define PARSE_H

#include <stdbool.h>

#include "heap.h"

struct parser;
struct interp;

struct parser* parser_new(struct allocator *alloc, struct interp *i);
void parser_free(struct parser *p);
int parser_consume(struct parser *p, char *data, bool interactive);
void parser_eof(struct parser *p);
void parser_gc(struct parser *p);

#endif /* PARSE_H */
