#include "ports.h"

#include <assert.h>

#include "heap.h"

struct port {
    FILE *file;
    bool in:1;
    bool out:1;
    bool text:1;
    bool binary:1;
};

value make_port(struct allocator *a, FILE *file, bool in, bool out, bool text, bool binary) {
    struct port *p = allocator_alloc(a, sizeof(struct port));
    p->file = file;
    p->in = in;
    p->out = out;
    p->text = text;
    p->binary = binary;
    return (uint64_t)p | TYPE_PORT;
}

bool port_in(value p) {
    if (value_type(p) != TYPE_PORT) {
        return VALUE_FALSE;
    }
    struct port *ps = (struct port*)value_to_cell(p);
    return ps->in;
}

bool port_out(value p) {
    if (value_type(p) != TYPE_PORT) {
        return VALUE_FALSE;
    }
    struct port *ps = (struct port*)value_to_cell(p);
    return ps->out;
}

bool port_text(value p) {
    if (value_type(p) != TYPE_PORT) {
        return VALUE_FALSE;
    }
    struct port *ps = (struct port*)value_to_cell(p);
    return ps->text;
}

bool port_binary(value p) {
    if (value_type(p) != TYPE_PORT) {
        return VALUE_FALSE;
    }
    struct port *ps = (struct port*)value_to_cell(p);
    return ps->binary;
}

void port_newline(value p) {
    assert(value_type(p) == TYPE_PORT);
    struct port *ps = (struct port*)value_to_cell(p);
    fprintf(ps->file, "\n");
}

void port_display(value p, value o) {
    assert(value_type(p) == TYPE_PORT);
    struct port *ps = (struct port*)value_to_cell(p);
    // XXX implement
    fprintf(ps->file, "woohoo");
}
