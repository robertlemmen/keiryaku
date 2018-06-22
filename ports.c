#include "ports.h"

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "heap.h"
#include "parse.h"

// XXX this is to make struct port a nice size, but there needs to be a better
// way to achieve this
#define BUFSIZE 464

struct result_list_entry {
    value result;
    struct result_list_entry *younger;
};

struct port {
    FILE *file;
    bool in:1;
    bool out:1;
    bool text:1;
    bool binary:1;
    int buf_pos;
    // XXX we only need a buffer and parser when we use (read ...) on this port,
    // not for e.g. text files. so this could be lazily allocated when needed
    char buffer[BUFSIZE];
    struct parser *p;
    /* we need to store the results from buffers pushed into the parser. most of
     * the time that is 0 or 1 expression, but it can be multiple. we do not
     * want to use a list all the time as it means allocations, so we have a
     * single result value and a list for overflows */
    value result;
    struct result_list_entry *result_overflow_oldest;
    struct result_list_entry *result_overflow_youngest;
};

static void parser_callback(value expr, void *arg) {
    struct port *ps = (struct port*)arg;
    if (ps->result == VALUE_NIL) {
        ps->result = expr;
    }
    else {
        struct result_list_entry *rle = malloc(sizeof(struct result_list_entry));
        rle->result = expr;
        rle->younger = NULL;
        if (ps->result_overflow_youngest) {
            ps->result_overflow_youngest->younger = rle;
        }
        else {
            ps->result_overflow_oldest = rle;
        }
        ps->result_overflow_youngest = rle;
    }
}

value port_new(struct allocator *a, FILE *file, bool in, bool out, bool text, bool binary) {
    struct port *ps = allocator_alloc(a, sizeof(struct port));
    ps->file = file;
    ps->in = in;
    ps->out = out;
    ps->text = text;
    ps->binary = binary;
    ps->buf_pos = 0;
    ps->p = parser_new(a, &parser_callback, ps);
    ps->result = VALUE_NIL;
    ps->result_overflow_youngest = NULL;
    ps->result_overflow_oldest = NULL;
    return (uint64_t)ps | TYPE_PORT;
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
    dump_value(o, ps->file);
}

value port_open_input_file(struct allocator *a, value filename) {
    assert(value_is_string(filename));
    // XXX should of course support other modes as well
    FILE *fs = fopen(value_to_string(&filename), "r");
    if (!fs) {
        fprintf(stderr, "Could not open file '%s': %s", value_to_string(&filename), strerror(errno));
        // XXX need way to raise error
        exit(1);
    }
    return port_new(a, fs, false, true, true, false);
}

void port_close(value p) {
    assert(value_type(p) == TYPE_PORT);
    struct port *ps = (struct port*)value_to_cell(p);
    if (fclose(ps->file)) {
        fprintf(stderr, "Could not close: %s", strerror(errno));
    }
    ps->file = NULL;
}

value port_read(value p) {
    assert(value_type(p) == TYPE_PORT);
    struct port *ps = (struct port*)value_to_cell(p);
    while (ps->result == VALUE_NIL) {
        char *ret = fgets(&ps->buffer[ps->buf_pos], BUFSIZE - 1 - ps->buf_pos, ps->file);
        if (!ret) {
            if (feof(ps->file)) {
                return VALUE_EOF;
            }
            else {
                // XXX need way to raise
                fprintf(stderr, "NYI fgets error\n");
            }
        }
        ps->buf_pos = parser_consume(ps->p, ps->buffer);
    }
    value temp = ps->result;
    if (ps->result_overflow_oldest == NULL) {
        ps->result = VALUE_NIL;
    }
    else {
        struct result_list_entry *rle = ps->result_overflow_oldest;
        ps->result_overflow_oldest = rle->younger;
        if (ps->result_overflow_oldest == NULL) {
            ps->result_overflow_youngest = NULL;
        }
        ps->result = rle->result;
        free(rle);
    }
    return temp;
}
