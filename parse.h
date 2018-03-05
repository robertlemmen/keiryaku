#ifndef PARSE_H
#define PARSE_H

#include <stdbool.h>

#include "heap.h"

struct parser;

struct parser* parser_new(struct allocator *alloc);
void parser_free(struct parser *p);
int parser_consume(struct parser *p, char *data, bool interactive);
void parser_eof(struct parser *p);

#endif /* PARSE_H */
