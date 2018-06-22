#ifndef PORTS_H
#define PORTS_H

#include <stdbool.h>
#include <stdio.h>

#include "types.h"

struct allocator;

// XXX we need some sort of destructor to close the fd when we garbage-collect
// the object
value port_new(struct allocator *a, FILE *file, bool in, bool out, bool text, bool binary);
bool port_in(value p);
bool port_out(value p);
bool port_text(value p);
bool port_binary(value p);

void port_newline(value p);
void port_display(value p, value o);

value port_open_input_file(struct allocator *a, value filename);
void port_close(value p);

value port_read(value p);

#endif /* PORTS_H */
