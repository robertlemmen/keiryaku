#include "heap.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "types.h"
#include "interp.h"
#include "global.h"
#include "ports.h"

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

#define ARENA_TYPE_NURSERY          0
#define ARENA_TYPE_OLD_SURVIVOR     1
#define ARENA_TYPE_NEW_SURVIVOR     2
#define ARENA_TYPE_TENURED          3

struct allocator {
    arena first_nursery;
    arena first_survivor;
    arena first_tenured;
    arena arenas_free_list;
    int pressure;
    bool gc_requested;
};

struct arena_header {
    uint8_t arena_type;
    arena next;
    /* in a bump allocator, scan_cache simply is the first free cell that we
     * will use next. In a block allocator, we use this as a lower bound where
     * we never look at cells before this, to avoid scanning them again and
     * again */
    uint16_t scan_cache;
};

void hexdump(uint8_t *d, int l) {
    for (int i = 0; i < l; i++) {
        if ((i % 16) == 0) {
            printf("\n  ");
        }
        else if ((i % 8) == 0) {
            printf(" ");
        }
        printf("%02X ", d[i]);
    }
    printf("\n");
}

void dump_arena_meta(arena a) {
    printf("Arena at %p:\n", a);
    struct arena_header *ah = a;
    printf("scan_cache: %i\n", ah->scan_cache);
    uint8_t *cp = a;
    printf("block bitmap:");
    hexdump(cp+128, 32);
    printf("\nmark bitmap:");
    hexdump(cp+128+8192, 32);
}

// XXX with nursery we need some sort of arena freelist
arena alloc_arena(struct allocator *a, uint8_t type) {
    if (!a->arenas_free_list) {
        void *temp = NULL;
        posix_memalign(&temp, ARENA_SIZE, ARENA_SIZE);
        a->arenas_free_list = temp;
        struct arena_header *at = temp;
        at->next = NULL;
    }
    arena ret = a->arenas_free_list;
    struct arena_header *ah = ret;
    a->arenas_free_list = ah->next;
    assert(ret != NULL);
    assert(cell_index(ret) == 0);

    memset(ret, 0, BITMAP_SIZE);
    memset(ret + BITMAP_SIZE, 0xff, BITMAP_SIZE);
    memset(ret + BITMAP_SIZE, 0, 128);

    ah->arena_type = type;
    ah->scan_cache = 1024; // first actual cell

    //printf("new arena at %p\n", ret);
    return ret;
}

void free_arena(struct allocator *a, arena r) {
    assert(r != NULL);
    struct arena_header *ah = r;
    memset(r, 0x20 + ah->arena_type, ARENA_SIZE);
    // XXX assert it is empty?
    free(r);
}

void recycle_arena(struct allocator *a, arena r) {
    assert(r != NULL);
    struct arena_header *ah = r;
    memset(r, 0x30 + ah->arena_type, ARENA_SIZE);
    // XXX assert it is empty?
    ah->next = a->arenas_free_list;
    a->arenas_free_list = r;
}

extern int call_count;

block alloc_block_bump(arena a, int s) {
    assert(a != NULL);
    block ret = NULL;

    struct arena_header *ah = a;
    uint_fast16_t cell_idx = ah->scan_cache;

    uint_fast16_t cc = ((s + 15) & ~15) >> 4; // number of cells required

    assert(!meta_get_block(a, cell_idx));
    assert(meta_get_mark(a, cell_idx));

    if (cell_idx + cc > 65535) {
        return NULL;
    }
    else {
        ret = a + cell_idx * 16;
        meta_set_block(a, cell_idx);
        meta_clear_mark(a, cell_idx);
        for (int i = 1; i < cc; i++) {
            meta_clear_block(a, cell_idx + i); // XXX one of these two is already cleared??
            meta_clear_mark(a, cell_idx + i);
        }
        cell_idx += cc;
        assert(cell_idx > 1024);
        ah->scan_cache = cell_idx;
    }

    if (arg_debug) {
        memset(ret, 0x16, s);
    }

    return ret;
}

block alloc_block_tenured(arena a, int s) {
    assert(a != NULL);
    block ret = NULL;

    struct arena_header *ah = a;
    uint_fast16_t cc = ((s + 15) & ~15) >> 4; // number of cells required
    uint_fast16_t cell_idx = ah->scan_cache;

    // scan to first free cell
    while ((cell_idx < 65536 - cc) && (meta_get_block(a, cell_idx) || !meta_get_mark(a, cell_idx))) {
        cell_idx++;
    }
    if (cell_idx >= 65536 - cc) {
        // the arena is mostly full, the block will not fit
        return NULL;
    }
    ah->scan_cache = cell_idx;

    if (cc == 1) {
        meta_set_block(a, cell_idx);
        meta_clear_mark(a, cell_idx);
        ret = a + cell_idx * 16;
    }
    else {
        for (int i = 1; i < cc; i++) {
            if (meta_get_block(a, cell_idx + i) || ! meta_get_mark(a, cell_idx + i)) {
                // XXX handle properly, scan to next free block rather than just
                // pick a new arena
                return NULL;
            }
        }
        meta_set_block(a, cell_idx);
        meta_clear_mark(a, cell_idx);
        for (int i = 1; i < cc; i++) {
            meta_clear_block(a, cell_idx + i); // XXX one of these two is already cleared...
            meta_clear_mark(a, cell_idx + i);
        }
        ret = a + cell_idx * 16;
    }
    if (arg_debug) {
        memset(ret, 0x17, s);
    }

    return ret;
}

void free_cell(cell c) {
    // XXX are we not doing GC?
}

struct allocator* allocator_new(void) {
    struct allocator *ret = malloc(sizeof(struct allocator));
    ret->arenas_free_list = NULL;
    ret->first_nursery = alloc_arena(ret, ARENA_TYPE_NURSERY);
    ret->first_survivor = alloc_arena(ret, ARENA_TYPE_NEW_SURVIVOR);
    ret->first_tenured = alloc_arena(ret, ARENA_TYPE_TENURED);
    ret->pressure = 0;
    return ret;
}

void allocator_free(struct allocator *a) {
    assert(a != NULL);
    assert(a->first_nursery != NULL);
    assert(a->first_survivor != NULL);
    assert(a->first_tenured != NULL);
    while (a->first_nursery) {
        arena temp = a->first_nursery;
        struct arena_header *ah = temp;
        a->first_nursery = ah->next;
        free_arena(a, temp);
    }
    while (a->first_survivor) {
        arena temp = a->first_survivor;
        struct arena_header *ah = temp;
        a->first_survivor = ah->next;
        free_arena(a, temp);
    }
    while (a->first_tenured) {
        arena temp = a->first_tenured;
        struct arena_header *ah = temp;
        a->first_tenured = ah->next;
        free_arena(a, temp);
    }
    while (a->arenas_free_list) {
        arena temp = a->arenas_free_list;
        struct arena_header *ah = temp;
        a->arenas_free_list = ah->next;
        free_arena(a, temp);
    }
}

int alloc_count_nursery = 0;
int alloc_count_survivor = 0;
int alloc_count_tenured = 0;

void dump_clear_alloc_stats(void) {
    fprintf(stderr, "#   nursery allocations:  %i\n", alloc_count_nursery);
    fprintf(stderr, "#   survivor allocations: %i\n", alloc_count_survivor);
    fprintf(stderr, "#   tenured allocations:  %i\n", alloc_count_tenured);
    alloc_count_nursery = 0;
    alloc_count_survivor = 0;
    alloc_count_tenured = 0;
}

cell allocator_alloc_type(struct allocator *a, int s, uint8_t type) {
    // XXX a different way to assess pressure...
    a->pressure++;

    if (type == ARENA_TYPE_TENURED) {
        alloc_count_tenured++;
        arena current_arena = a->first_tenured;
        do {
            cell ret = alloc_block_tenured(current_arena, s);
            if (!ret) {
    //            printf("arena 0x%016lX is full, trying next\n", current_arena);
                // that arena is full, try the next one
                struct arena_header *ah = current_arena;
                if (!ah->next) {
                    arena new_arena = alloc_arena(a, ah->arena_type);
                    struct arena_header *nah = new_arena;
                    nah->next = a->first_tenured;
                    a->first_tenured = new_arena;
                    current_arena = new_arena;

                }
                else {
                    // XXX we could avoid some re-scanning by moving full arenas
                    // towards the end
                    current_arena = ah->next;
                }
            }
            else {
                assert(((uint64_t)ret & 15) == 0);
                assert(cell_to_arena(ret) == current_arena);
                return ret;
            }
        } while (1);
    }
    else {
        arena current_arena = NULL;
        if (type == ARENA_TYPE_NURSERY) {
            alloc_count_nursery++;
            current_arena = a->first_nursery;
        }
        else if (type == ARENA_TYPE_NEW_SURVIVOR) {
            alloc_count_survivor++;
            current_arena = a->first_survivor;
        }
        do {
            cell ret = alloc_block_bump(current_arena, s);
            if (!ret) {
                struct arena_header *ah = current_arena;
                arena new_arena = alloc_arena(a, ah->arena_type);
                struct arena_header *nah = new_arena;
                if (type == ARENA_TYPE_NURSERY) {
                    nah->next = a->first_nursery;
                    a->first_nursery = new_arena;
                }
                else if (type == ARENA_TYPE_NEW_SURVIVOR) {
                    nah->next = a->first_survivor;
                    a->first_survivor = new_arena;
                }
                current_arena = new_arena;
            }
            else {
                assert(((uint64_t)ret & 15) == 0);
                assert(cell_to_arena(ret) == current_arena);
            //    printf("# allocation in nursery at 0x%016X\n", ret);
                return ret;
            }
        } while (1);
    }
}

cell allocator_alloc(struct allocator *a, int s) {
    assert(a != NULL);
    return allocator_alloc_type(a, s, ARENA_TYPE_NURSERY);
}

cell allocator_alloc_nonmoving(struct allocator *a, int s) {
    assert(a != NULL);
    return allocator_alloc_type(a, s, ARENA_TYPE_TENURED);
}

bool allocator_needs_gc(struct allocator *a) {
    return (a->pressure > arg_gc_threshold) || a->gc_requested;
}

void allocator_request_gc(struct allocator *a) {
    a->gc_requested = true;
}

// XXX use posix_memalign for this, and assert it has the right size.
struct allocator_gc_list {
    value* values[GC_LIST_SIZE];
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

void allocator_gc_add_root(struct allocator_gc_ctx *gc, value *v) {
    uint_fast16_t cell_idx = cell_index(value_to_cell(*v));
    assert(cell_idx >= 1024);
    if (gc->list->count == GC_LIST_SIZE) {
        gc->list = new_gc_list(gc->list);
    }
    gc->list->values[gc->list->count] = v;
    gc->list->count++;
}

void allocator_gc_add_nonval_root(struct allocator_gc_ctx *gc, void *m) {
    // we mark these right away
    uint_fast16_t cell_idx = cell_index(m);
    arena a = cell_to_arena(m);
    struct arena_header *ah = a;
    assert(ah->arena_type == ARENA_TYPE_TENURED);
    meta_set_mark(a, cell_idx);
    // XXX it is very unfortunate that non-vals are currently always allocated
    // in tenured, this needs fixing
}

// for an area type that an item is coming from, figure out where to promote it
// to next
int next_arena_type(int at) {
    switch (at) {
        case ARENA_TYPE_NURSERY:
            return ARENA_TYPE_NEW_SURVIVOR;
        case ARENA_TYPE_OLD_SURVIVOR:
            return ARENA_TYPE_TENURED;
        default:
            // XXX fail somehow, assert?
            return 0;
    }
}

long currentmicros() {
    struct timespec tp;
    clock_gettime(CLOCK_MONOTONIC, &tp);
    return tp.tv_sec * 1000000 + tp.tv_nsec / 1000;
}

long total_gc_time_us = 0;

void allocator_gc_perform(struct allocator_gc_ctx *gc) {
    fprintf(stderr, "# Doing GC after %i allocations (threshold %i)\n", gc->a->pressure, arg_gc_threshold);
    dump_clear_alloc_stats();
    long mark_start = currentmicros();

    gc->a->pressure = 0;
    gc->a->gc_requested = false;

//    int roots = gc->list->count;
    int visited = 0;
    int reclaimed = 0;

    // XXX turn all existing survivor arenas into ARENA_TYPE_OLD_SURVIVOR
    arena a = gc->a->first_survivor;
    while (a) {
        struct arena_header *ah = a;
        ah->arena_type = ARENA_TYPE_OLD_SURVIVOR;
        a = ah->next;
    }
    
    // we do not want to mix new survivors with old ones
    arena old_survivor = gc->a->first_survivor;
    gc->a->first_survivor = alloc_arena(gc->a, ARENA_TYPE_NEW_SURVIVOR);


    // mark phase
    while (gc->list->count) {
        value *cvptr = gc->list->values[--gc->list->count];
        value cv = *cvptr;
//        printf("tracing item at 0x%016p: 0x%016p\n", cvptr, cv);
        if ((!gc->list->count) && (gc->list->prev)) {
            struct allocator_gc_list *temp = gc->list;
            gc->list = temp->prev;
            free(temp);
        }
        // XXX not sure where we ever get a 0 value from...
        // XXX and not sure this can ever be immediate as we already check when
        // putting on the list...
        if (cv != 0 && !value_is_immediate(cv)) {
            visited++;
            arena a = cell_to_arena(value_to_cell(cv));
            struct arena_header *ah = a;
            uint_fast16_t cell_idx = cell_index(value_to_cell(cv));
            assert(cell_idx >= 1024);
            assert(meta_get_block(a, cell_idx));
            bool traverse = false;
            if (ah->arena_type == ARENA_TYPE_TENURED) {
                if (!meta_get_mark(a, cell_idx)) {
                    meta_set_mark(a, cell_idx);
                    traverse = true;
//                    printf("  marking in tenured\n");
                }
                else {
//                    printf("  already marked in tenured\n");
                }
            }
            else {
                if (!meta_get_mark(a, cell_idx)) {
                    meta_set_mark(a, cell_idx);
                    // XXX we use the same in add_nonval_root above, refactor
                    int cc = 1;
                    for (int i = cell_idx + 1; cell_idx < 65536; i++) {
                        if ((! meta_get_mark(a, i)) && (! meta_get_block(a, i))) {
                            cc++;
                        }
                        else {
                            break;
                        }
                    }
                    // XXX this should just go to the next type of arena, so
                    // survivor if coming from nursery, but that blows up
                    // quickly. I wonder whether it is because we could follow
                    // the link to it twice, and then try to interpret it as a
                    // forwarding pointer?
                    cell newloc = allocator_alloc_type(gc->a, cc*16, next_arena_type(ah->arena_type));
                    cell oldloc = value_to_cell(cv);
                    memcpy(newloc, oldloc, cc*16);
/*                    printf("  moving heap item from 0x%016p to 0x%016p (arena type %i -> %i)\n", oldloc, 
                        newloc, ah->arena_type, next_arena_type(ah->arena_type));*/
                    if (arg_debug) {
                        memset(oldloc, 0x19, cc*16); 
                    }
                    memcpy(oldloc, &newloc, sizeof(void*));

                    traverse = true;
                    *cvptr = (uint64_t)newloc | value_type(cv);
                    cv = *cvptr;
                    // we can have the same item on the root list twice, so the
                    // second time around we find the already-moved version. so
                    // we need to mark the target too
                    arena new_a = cell_to_arena(value_to_cell(cv));
                    uint_fast16_t new_cell_idx = cell_index(value_to_cell(cv));
                    meta_set_mark(new_a, new_cell_idx);
                }
                else {
                    // we may see the same item twice, and then we do not want
                    // to promote it twice
                    if (ah->arena_type != ARENA_TYPE_NEW_SURVIVOR) {
                        cell oldloc = value_to_cell(cv);
                        cell newloc;
                        memcpy(&newloc, oldloc, sizeof(void*));
/*                        printf("  following forwarding pointer 0x%016p to 0x%016p where oldloc is arena_type %i\n", 
                            oldloc, newloc, ah->arena_type);*/
                        *cvptr = (uint64_t)newloc | value_type(cv);
                    }
                }
            }
            if (traverse) {
                switch (value_type(cv)) {
                    case TYPE_CONS:
                        allocator_gc_add_root_fp(gc, carptr(cv));
                        allocator_gc_add_root_fp(gc, cdrptr(cv));
                        break;
                    case TYPE_INTERP_LAMBDA:
                        interp_traverse_lambda(gc, value_to_cell(cv));
                        break;
                    case TYPE_VECTOR:
                        traverse_vector(gc, cv);
                        break;
                    case TYPE_PORT:
                        traverse_port(gc, cv);
                        break;
                    case TYPE_OTHER:
                        switch (value_subtype(cv)) {
                            case SUBTYPE_ENV:;
                            // this could be traversed, but the environment in
                            // question is always traversed from somewhere else
                            // XXX really?
                            break;
                            case SUBTYPE_PARAM:;
                                struct param *cp = value_to_parameter(cv);
                                allocator_gc_add_root_fp(gc, &(cp->init)); 
                                allocator_gc_add_root_fp(gc, &(cp->convert)); 
                                break;
                            default:;
                            // not traversable
                        }
                    default:;
                        // not traversable
                }
            }
        }
    }
    long mark_end = currentmicros();

    // sweep tenured
    a = gc->a->first_tenured;
    while (a) {
        // XXX of course we want to do this with word-sized manipulations, not
        // looping
        for (int i = 1024; i < 65535; i++) {
            if (meta_get_block(a, i) && !meta_get_mark(a, i)) {
                reclaimed++;
                meta_clear_block(a, i);
                meta_set_mark(a, i);
                // printf("reclaiming 0x%016lX\n", cell_from_cell_index(a, i));
                // reclaim extent
                int j = i+1;
                while ((j != 0) && (!meta_get_block(a, j) && !meta_get_mark(a, j))) {
                    meta_set_mark(a, j);
                }
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
        // XXX temp unmark
        for (int i = 1024; i < 65535; i++) {
            if (meta_get_block(a, i) && meta_get_mark(a, i)) {
                meta_clear_mark(a, i);
            }
        }
        struct arena_header *ah = a;
        // reset scan_cache, we can just set this as it is only a lower bound
        ah->scan_cache = 1024;
        a = ah->next;
        // XXX faster unmark: flip a bit in the allocator and check for equals that in
        // get_mark/set_mark. new arenas need to be created with that bit set
        // correctly!
    }

    // also unmark the survivor pages
    a = gc->a->first_survivor;
    while (a) {
        // XXX this is even easier to clear because we never allocate to this again
        for (int i = 1024; i < 65535; i++) {
            if (meta_get_block(a, i) && meta_get_mark(a, i)) {
                meta_clear_mark(a, i);
            }
        }
        struct arena_header *ah = a;
        a = ah->next;
    }

    // XXX some sort of free list
    while (gc->a->first_nursery) {
        arena temp = ((struct arena_header*)gc->a->first_nursery)->next;
        recycle_arena(gc->a, gc->a->first_nursery);
        gc->a->first_nursery = temp;
    }
    gc->a->first_nursery = alloc_arena(gc->a, ARENA_TYPE_NURSERY);

    while (old_survivor) {
        arena temp = ((struct arena_header*)old_survivor)->next;
        recycle_arena(gc->a, old_survivor);
        old_survivor = temp;
    }
//    printf("# %i roots, %i visited, %i reclaimed\n", roots, visited, reclaimed);
//    fflush(stdout);
    long sweep_end = currentmicros();

    fprintf(stderr, "# GC done with mark phase of %lius and sweep of %lius\n", mark_end - mark_start, sweep_end - mark_end);
    dump_clear_alloc_stats();
    fprintf(stderr, "\n");

    total_gc_time_us += sweep_end - mark_start;

    free(gc->list);
    free(gc);
}
