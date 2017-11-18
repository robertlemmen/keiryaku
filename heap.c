#include "heap.h"

#include <malloc.h>
#include <assert.h>
#include <string.h> 
#include <stdlib.h>

block alloc_block(void) {
    block ret;
    posix_memalign(&ret, BLOCK_SIZE, BLOCK_SIZE);
    assert(ret != NULL);
    assert(cell_index(ret) == 0);
    memset(ret, 0, BLOCK_SIZE); // XXX for now

    // XXX init bump allocator
    uint16_t *cell_count_ptr = ret;
    *cell_count_ptr = 4096; // pretend we already have that many cells in use

    return ret;
}

void free_block(block b) {
    assert(b != NULL);
    assert(cell_index(b) == 0);
    // XXX assert it is empty?
    free(b);
}

cell alloc_cell(block b, int s) {
    // XXX for now we use a non-freeing bump allocator
    uint16_t *cell_count_ptr = b;
   
    assert(*cell_count_ptr >= 4096);

    s = (s + 15) / 16; // number of 16 byte mini-cells, round up

    cell temp = b + *cell_count_ptr * 16;

    *cell_count_ptr += s;
    assert(*cell_count_ptr < 65536);

    return temp;
}

void free_cell(cell c) {
    // XXX for now we use a non-freeing bump allocator, so nothing to do
}

struct allocator {
    block b; // XXX for now
};

struct allocator* allocator_new(void) {
    struct allocator *ret = malloc(sizeof(struct allocator));
    ret->b = alloc_block();
    return ret;
}

void allocator_free(struct allocator *a) {
    assert(a != NULL);
    assert(a->b != NULL);
    free_block(a->b);
    free(a);
}

cell allocator_alloc(struct allocator *a, int s) {
    assert(a != NULL);
    return alloc_cell(a->b, s);
}

