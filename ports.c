#include "ports.h"

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <pwd.h>
#include <unistd.h>
#include <sys/types.h>

#include "linenoise/linenoise.h"

#include "heap.h"
#include "parse.h"

// XXX this is to make struct port a nice size, but there needs to be a better
// way to achieve this
#define BUFSIZE 456

struct result_list_entry {
    value result;
    struct result_list_entry *younger;
};

// XXX should perhaps be in struct?
char *history_file = NULL;

struct port {
    uint8_t sub_type;
    bool in:1;
    bool out:1;
    bool text:1;
    bool binary:1;
    bool tty:1;
    int buf_pos;
    FILE *file;
    struct parser *p;
    struct allocator *a;
    /* we need to store the results from buffers pushed into the parser. most of
     * the time that is 0 or 1 expression, but it can be multiple. we do not
     * want to use a list all the time as it means allocations, so we have a
     * single result value and a list for overflows */
    value result;
    struct result_list_entry *result_overflow_oldest;
    struct result_list_entry *result_overflow_youngest;
    char buffer[BUFSIZE];
};

static void parser_callback(value expr, void *arg) {
    // XXX hmm, how does the pointer in the parser struct know the port hs
    // moved??
    struct port *ps = (struct port*)arg;
    //fprintf(stderr, "parser_callback %p\n", ps);
    if (ps->result == VALUE_NIL) {
        ps->result = expr;
        write_barrier(ps->a, (uint64_t)ps | TYPE_BOXED, &ps->result);
    }
    else {
        struct result_list_entry *rle = malloc(sizeof(struct result_list_entry));
        rle->result = expr;
        write_barrier(ps->a, (uint64_t)ps | TYPE_BOXED, &rle->result);
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
    ps->sub_type = SUBTYPE_PORT;
    ps->tty = false;
    ps->in = in;
    ps->out = out;
    ps->text = text;
    ps->binary = binary;
    ps->buf_pos = 0;
    ps->file = file;
    ps->p = parser_new(a, &parser_callback, ps);
    ps->a = a;
    ps->result = VALUE_NIL;
    ps->result_overflow_youngest = NULL;
    ps->result_overflow_oldest = NULL;

    return (uint64_t)ps | TYPE_BOXED;
}

value port_new_tty(struct allocator *a) {
    struct port *ps = allocator_alloc(a, sizeof(struct port));
    ps->sub_type = SUBTYPE_PORT;
    ps->tty = true;
    ps->in = true;
    ps->text = true;
    ps->out = false;
    ps->binary = false;
    ps->buf_pos = 0;
    ps->file = NULL;
    ps->p = parser_new(a, &parser_callback, ps);
    ps->a = a;
    ps->result = VALUE_NIL;
    ps->result_overflow_youngest = NULL;
    ps->result_overflow_oldest = NULL;

    linenoiseSetMultiLine(1);
    linenoiseHistorySetMaxLen(1024);
    const char *homedir;
    if ((homedir = getenv("HOME")) == NULL) {
        homedir = getpwuid(getuid())->pw_dir;
    }
    history_file = malloc(strlen(homedir) + strlen("/.keiryaku_history") + 1);
    sprintf(history_file, "%s/.keiryaku_history", homedir);
    linenoiseHistoryLoad(history_file);
    linenoiseSetCompletionCallback(NULL);

    return (uint64_t)ps | TYPE_BOXED;
}

bool port_in(value p) {
    if (!value_is_port(p)) {
        return VALUE_FALSE;
    }
    struct port *ps = (struct port*)value_to_cell(p);
    return ps->in;
}

bool port_out(value p) {
    if (!value_is_port(p)) {
        return VALUE_FALSE;
    }
    struct port *ps = (struct port*)value_to_cell(p);
    return ps->out;
}

bool port_text(value p) {
    if (!value_is_port(p)) {
        return VALUE_FALSE;
    }
    struct port *ps = (struct port*)value_to_cell(p);
    return ps->text;
}

bool port_binary(value p) {
    if (!value_is_port(p)) {
        return VALUE_FALSE;
    }
    struct port *ps = (struct port*)value_to_cell(p);
    return ps->binary;
}

void port_newline(value p) {
    assert(value_is_port(p));
    struct port *ps = (struct port*)value_to_cell(p);
    if (!ps->out) {
        fprintf(stderr, "Trying to write to non output port\n");
        // XXX need way to raise error
        exit(1);
    }
    fprintf(ps->file, "\n");
}

void port_write(value p, value o) {
    assert(value_is_port(p));
    struct port *ps = (struct port*)value_to_cell(p);
    if (!ps->out) {
        fprintf(stderr, "Trying to write to non output port\n");
        // XXX need way to raise error
        exit(1);
    }
    dump_value(o, ps->file);
}

void port_write_string(value p, value o) {
    assert(value_is_port(p));
    struct port *ps = (struct port*)value_to_cell(p);
    if (!ps->out) {
        fprintf(stderr, "Trying to write to non output port\n");
        // XXX need way to raise error
        exit(1);
    }
    dump_string_value(o, ps->file);
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
    return port_new(a, fs, true, false, true, false);
}

void port_close(value p) {
    assert(value_is_port(p));
    struct port *ps = (struct port*)value_to_cell(p);
    if (fclose(ps->file)) {
        fprintf(stderr, "Could not close: %s", strerror(errno));
    }
    ps->file = NULL;
}

value port_read(value p) {
    assert(value_is_port(p));
    struct port *ps = (struct port*)value_to_cell(p);
    while (ps->result == VALUE_NIL) {
        if (ps->tty) {
            char *input;
            input = linenoise("> ");
            if (input) {
                strncpy(&ps->buffer[ps->buf_pos], input, BUFSIZE - ps->buf_pos - 1);
                linenoiseHistoryAdd(input);
                // XXX this is ugly and unsafe...
                ps->buffer[ps->buf_pos + strlen(input)] = '\n';
                ps->buffer[ps->buf_pos + strlen(input) + 1 ] = '\0';
                linenoiseHistorySave(history_file);
                free(input);
            }
            else {
                parser_eof(ps->p);
                return VALUE_EOF;
            }
        }
        else {
            char *ret = fgets(&ps->buffer[ps->buf_pos], BUFSIZE - 1 - ps->buf_pos, ps->file);
            if (!ret) {
                if (feof(ps->file)) {
                    parser_eof(ps->p);
                    return VALUE_EOF;
                }
                else {
                    // XXX need way to raise
                    fprintf(stderr, "NYI fgets error\n");
                }
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

void traverse_port(struct allocator_gc_ctx *gc, value p) {
    assert(value_is_port(p));
    struct port *ps = (struct port*)value_to_cell(p);
    //fprintf(stderr, "traversing port %p\n", ps);
    allocator_gc_add_root_fp(gc, &ps->result);
    struct result_list_entry *rle = ps->result_overflow_oldest;
    while (rle) {
        allocator_gc_add_root_fp(gc, &rle->result);
        rle = rle->younger;
    }
    // this is a bit interesting because this is not traversal following a link,
    // but against the link: the parser needs to be updated when we move
    // the port
    parser_set_cb_arg(ps->p, ps); 
}
