#ifndef PARSE_H
#define PARSE_H

#include "heap.h"

struct parser;

struct parser* parser_new(struct allocator *alloc);
void parser_free(struct parser *p);
int parser_consume(struct parser *p, char *data);

#endif /* PARSE_H */
