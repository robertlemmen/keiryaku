#include "heap.h"

#include <malloc.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "types.h"
#include "interp.h"
#include "global.h"

#define ARENA_SIZE 0x100000
#define cell_to_arena(x) (arena)((uint64_t)x & 0xFFFFFFFFFFF00000)
#define cell_index(x) (((uint64_t)x & 0x00000000000FFFFF) >> 4)
#define cell_from_cell_index(a, i) (void*)((((uint64_t)a) & 0xFFFFFFFFFFF00000) \
                                            | ((((uint64_t)i) << 4) & 0x00000000000FFFFF))

#define meta_get_block(a, i)   (((uint8_t*)a)[(i) >> 3] &   (1 << (7 - ((i) & 0x07))))
#define meta_set_block(a, i)   (((uint8_t*)a)[(i) >> 3] |=  (1 << (7 - ((i) & 0x07))))
#define meta_clear_block(a, i) (((uint8_t*)a)[(i) >> 3] &= ~(1 << (7 - ((i) & 0x07))))
#define meta_get_mark(a, i)   (((uint8_t*)a)[8192 + ((i) >> 3)] &   (1 << (7 - ((i) & 0x07))))
#define meta_set_mark(a, i)   (((uint8_t*)a)[8192 + ((i) >> 3)] |=  (1 << (7 - ((i) & 0x07))))
#define meta_clear_mark(a, i) (((uint8_t*)a)[8192 + ((i) >> 3)] &= ~(1 << (7 - ((i) & 0x07))))

#define BITMAP_SIZE 8192

#define GC_LIST_SIZE 1022

struct arena_header {
    arena next;
    uint16_t scan_cache;
};

void dump_arena_meta(arena a) {
    printf("Arena at 0x%016lX:\n", (uint64_t)a);
    struct arena_header *ah = a;
    printf("scan_cache: %i\n", ah->scan_cache);
    uint8_t *cp = a;
    printf("block bitmap:");
    for (int i = 128; i < 128+32; i++) {
        if ((i % 16) == 0) {
            printf("\n  ");
        }
        else if ((i % 8) == 0) {
            printf(" ");
        }
        printf("%02X ", cp[i]);
    }
    printf("\nmark bitmap:");
    for (int i = 8192+128; i < 8192+128+32; i++) {
        if ((i % 16) == 0) {
            printf("\n  ");
        }
        else if ((i % 8) == 0) {
            printf(" ");
        }
        printf("%02X ", cp[i]);
    }
    printf("\n");
}

arena alloc_arena(void) {
    arena ret;
    posix_memalign(&ret, ARENA_SIZE, ARENA_SIZE);
    assert(ret != NULL);
    assert(cell_index(ret) == 0);


    memset(ret, 0, BITMAP_SIZE);
    memset(ret + BITMAP_SIZE, 0xff, BITMAP_SIZE);
    memset(ret + BITMAP_SIZE, 0, 128);

    struct arena_header *ah = ret;
    ah->scan_cache = 1024; // first actual cell

    return ret;
}

void free_arena(arena a) {
    assert(a != NULL);
    // XXX assert it is empty?
    free(a);
}

extern int call_count;

block alloc_block(arena a, int s) {
    assert(a != NULL);
    block ret = NULL;

    struct arena_header *ah = a;
    uint_fast16_t cell_idx = ah->scan_cache;

//    printf("alloc of %i bytes, %i cells, starting at idx %lu\n", s, cc, cell_idx);

    if (cell_idx == 0) {
//        fprintf(stderr, "arena is entirely full after %i calls!\n", call_count);
        // this arena is entirely full
        return NULL;
    }

    uint_fast16_t cc = (s + 15) & ~15; // number of cells required

    assert(!meta_get_block(a, cell_idx));
    assert(meta_get_mark(a, cell_idx));
    if (cc == 1) {
        meta_set_block(a, cell_idx);
        meta_clear_mark(a, cell_idx);
        ret = a + cell_idx * 16;
//        printf("  single-cell alloc in idx %lu\n", cell_idx);
        ah->scan_cache++;
    }
    else {
        for (int i = 1; i < cc; i++) {
            // XXX we also take extents, which actually makes this easier
            if (meta_get_block(a, cell_idx + i) || ! meta_get_mark(a, cell_idx + i)) {
                // the free block is not long enough
                // XXX handle properly
                return NULL;
            }
        }
        meta_set_block(a, cell_idx);
        meta_clear_mark(a, cell_idx);
        ah->scan_cache++;
        for (int i = 1; i < cc; i++) {
            meta_clear_block(a, cell_idx + i); // XXX one of these two is already cleared...
            meta_clear_mark(a, cell_idx + i);
            ah->scan_cache++;
        }
//        printf("  multi-cell alloc in idx %lu\n", cell_idx);
        ret = a + cell_idx * 16;
    }
    // scan to next free cell
    cell_idx = ah->scan_cache;
    while ((ah->scan_cache != 0) && meta_get_block(a, cell_idx) && !meta_get_mark(a, cell_idx)) {
        ah->scan_cache++;
        cell_idx = ah->scan_cache;
    }
//    printf("  scanned to next free cell in idx %lu\n", cell_idx);
//
//    dump_arena_meta(a);

    return ret;
}

void free_cell(cell c) {
    // XXX are we not doing GC?
}

struct allocator {
    arena first_arena;
};

struct allocator* allocator_new(void) {
    struct allocator *ret = malloc(sizeof(struct allocator));
    ret->first_arena = alloc_arena();
    return ret;
}

void allocator_free(struct allocator *a) {
    assert(a != NULL);
    assert(a->first_arena != NULL);
    // XXX free them all!
    free_arena(a->first_arena);
    free(a);
}

cell allocator_alloc(struct allocator *a, int s) {
    assert(a != NULL);
    arena current_arena = a->first_arena;
    do { 
        cell ret = alloc_block(current_arena, s);
        if (!ret) {
            // that allocator is full, try the next one
            struct arena_header *ah = current_arena;
            if (!ah->next) {
//                printf("new arena!\n");
                arena new_arena = alloc_arena();
                struct arena_header *nah = new_arena;
                nah->next = a->first_arena;
                a->first_arena = new_arena;
                current_arena = new_arena;
            }
            else {
                // XXX we could avoid some re-scanning by moving full arenas
                // towards the end
                current_arena = ah->next;
            }
        }
        else {
            return ret;
        }
    } while (1);
}

// XXX use posix_memalign for this, and assert it has the right size.
struct allocator_gc_list {
    value values[GC_LIST_SIZE];
    int count;
    struct allocator_gc_list *prev;
};

struct allocator_gc_ctx {
    struct allocator *a;
    struct allocator_gc_list *list;
};

// XXX sometimes we use new_... and sometimes .._new!
struct allocator_gc_list* new_gc_list(struct allocator_gc_list *prev) {
    struct allocator_gc_list *ret =  malloc(sizeof(struct allocator_gc_list));
    ret->count = 0;
    ret->prev = prev;
    return ret;
}

struct allocator_gc_ctx* allocator_gc_new(struct allocator *a) {
    struct allocator_gc_ctx *ret = malloc(sizeof(struct allocator_gc_ctx));
    ret->a = a;
    ret->list = new_gc_list(NULL);
    return ret;
}

void allocator_gc_add_root(struct allocator_gc_ctx *gc, value v) {
    // XXX we could do this outside to safe function calls, macro?
    if (value_is_immediate(v)) {
        return;
    }
    if (gc->list->count == GC_LIST_SIZE) {
        gc->list = new_gc_list(gc->list);
    }
    gc->list->values[gc->list->count] = v;
    gc->list->count++;
}

void allocator_gc_add_nonval_root(struct allocator_gc_ctx *gc, void *m) {
    // we mark these right away
    uint_fast16_t cell_idx = cell_index(m);
    arena a = gc->a->first_arena;
    meta_set_mark(a, cell_idx);
}

void allocator_gc_perform(struct allocator_gc_ctx *gc) {
//    printf("# Doing GC!\n");

//    int roots = gc->list->count;
    int visited = 0;
    int reclaimed = 0;

    // mark phase
    while (gc->list->count) {
        value cv = gc->list->values[--gc->list->count];
        if ((!gc->list->count) && (gc->list->prev)) {
            struct allocator_gc_list *temp = gc->list;
            gc->list = temp->prev;
            free(temp);
        }
        // XXX not sure where we ever get a 0 value from...
        if (cv != 0 && !value_is_immediate(cv)) {
            visited++;
            arena a = cell_to_arena(value_to_cell(cv));
            uint_fast16_t cell_idx = cell_index(value_to_cell(cv));
    //        assert(meta_get_block(a, cell_idx));
            if (!meta_get_mark(a, cell_idx)) {
                // not marked yet
                meta_set_mark(a, cell_idx);
                switch (value_type(cv)) {
                    case TYPE_CONS:
                        allocator_gc_add_root(gc, car(cv));
                        allocator_gc_add_root(gc, cdr(cv));
                        break;
                    case TYPE_INTERP_LAMBDA:
                        interp_traverse_lambda(gc, value_to_cell(cv));
                        break;
                    default:;
                        // not traversable
                }
            }
        }
    }
    // sweep
    arena a = gc->a->first_arena;
    while (a) {
        // XXX of course we want to do this with word-sized manipulations, not
        // looping
        for (int i = 1024; i < 65535; i++) {
            if (meta_get_block(a, i) && !meta_get_mark(a, i)) {
                reclaimed++;
                meta_clear_block(a, i);
                meta_set_mark(a, i);
                // XXX deal with subsequent extents
                if (arg_debug) {
                    void *cell = cell_from_cell_index(a, i);
                    assert(cell_to_arena(cell) == a);
                    assert(cell_index(cell) == i);
                    memset(cell, 0x42, 16);
                    // XXX also wipe extents
                }
            }
        }
        struct arena_header *ah = a;
        a = ah->next;
    }

//    printf("# %i roots, %i visited, %i reclaimed\n", roots, visited, reclaimed);

    free(gc->list);
    free(gc);
}
