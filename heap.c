#include "heap.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>
// XXX for mprotect
#include <sys/mman.h>

#include "types.h"
#include "interp.h"
#include "global.h"
#include "ports.h"

#define ARENA_SIZE                  0x100000
#define BITMAP_SIZE                 8192
#define GC_LIST_SIZE                1022

#define cell_to_arena(x)            (arena)((uint64_t)x & 0xFFFFFFFFFFF00000)
#define cell_index(x)               (((uint64_t)x & 0x00000000000FFFFF) >> 4)
#define cell_from_cell_index(a, i)  (void*)((((uint64_t)a) & 0xFFFFFFFFFFF00000) \
                                        | ((((uint64_t)i) << 4) & 0x00000000000FFFFF))
#define meta_get_block(a, i)        (((uint8_t*)a)[(i) >> 3] \
                                        &   (1 << (7 - ((i) & 0x07))))
#define meta_set_block(a, i)        (((uint8_t*)a)[(i) >> 3] \
                                        |=  (1 << (7 - ((i) & 0x07))))
#define meta_clear_block(a, i)      (((uint8_t*)a)[(i) >> 3] \
                                        &= ~(1 << (7 - ((i) & 0x07))))
#define meta_get_mark(a, i)         (((uint8_t*)a)[BITMAP_SIZE + ((i) >> 3)] \
                                        &   (1 << (7 - ((i) & 0x07))))
#define meta_set_mark(a, i)         (((uint8_t*)a)[BITMAP_SIZE + ((i) >> 3)] \
                                        |=  (1 << (7 - ((i) & 0x07))))
#define meta_clear_mark(a, i)       (((uint8_t*)a)[BITMAP_SIZE + ((i) >> 3)] \
                                        &= ~(1 << (7 - ((i) & 0x07))))

#define ARENA_TYPE_NURSERY          0
#define ARENA_TYPE_NEW_SURVIVOR     1
#define ARENA_TYPE_OLD_SURVIVOR     2
#define ARENA_TYPE_TENURED          3

// XXX it's more a stack really, so why call it a list?
struct allocator_gc_list {
    union {
        value* valueps[GC_LIST_SIZE];   // used for the GC roots list
        value values[GC_LIST_SIZE];     // used for the remembered set
    };                                  // from the write barriers
    int count;
    struct allocator_gc_list *prev;
};

struct allocator_gc_ctx {
    struct allocator *a;
    struct allocator_gc_list *list;
    bool major_gc;
};

struct allocator {
    arena first_nursery;
    arena first_survivor;
    arena first_tenured;
    arena arenas_free_list;
    int pressure;
    int gc_count;
    bool gc_requested;
    bool gc_requested_full;
    struct allocator_gc_list *remset_once;
    struct allocator_gc_list *remset_twice;
    struct allocator_gc_list *remset_next;
};

struct arena_header {
    arena next;
    uint8_t arena_type;
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

// XXX sometimes we use new_... and sometimes .._new!
struct allocator_gc_list* new_gc_list(struct allocator_gc_list *prev) {
    struct allocator_gc_list *ret =  malloc(sizeof(struct allocator_gc_list));
    memset(ret, 0x13, sizeof(struct allocator_gc_list));
    ret->count = 0;
    ret->prev = prev;
    return ret;
}

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

    return ret;
}

void free_arena(struct allocator *a, arena r) {
    assert(r != NULL);
    struct arena_header *ah = r;
    memset(r, 0x20 + ah->arena_type, ARENA_SIZE);
    free(r);
}

void recycle_arena(struct allocator *a, arena r) {
    assert(r != NULL);
    struct arena_header *ah = r;
    memset(r, 0x30 + ah->arena_type, ARENA_SIZE);
    if (arg_debug) {
        assert(mprotect(r, ARENA_SIZE, PROT_NONE) == 0);
    }
    else {
        ah->next = a->arenas_free_list;
        a->arenas_free_list = r;
    }
}

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
            // block bit is already cleared
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

// XXX rename to first-fit?
block alloc_block_tenured(arena a, int s) {
    assert(a != NULL);
    block ret = NULL;

    struct arena_header *ah = a;
    uint_fast16_t cc = ((s + 15) & ~15) >> 4; // number of cells required
    uint_fast16_t cell_idx = ah->scan_cache;

    // scan to first free cell
    while (    (cell_idx < 65536 - cc)
            && (meta_get_block(a, cell_idx) || !meta_get_mark(a, cell_idx))) {
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
        // check that we have enough free adjacent cells
        bool found = false;
        while ((cell_idx < 65536 - cc) && !found) {
            found = true;
            for (int i = 1; i < cc; i++) {
                // we can use free cells as well as extents
                if (meta_get_block(a, cell_idx + i)) {
                    found = false;
                    cell_idx += i;
                    break;
                }
            }
            if (!found) {
                // scan to the next candidate
                while (    (cell_idx < 65536 - cc)
                        && (meta_get_block(a, cell_idx) || !meta_get_mark(a, cell_idx))) {
                    cell_idx++;
                }
            }
        }
        if (!found) {
            return NULL;
        }

        meta_set_block(a, cell_idx);
        meta_clear_mark(a, cell_idx);
        for (int i = 1; i < cc; i++) {
            // block bit is already cleared
            meta_clear_mark(a, cell_idx + i);
        }
        ret = a + cell_idx * 16;

        // we also need to check of the next cell after this allocation is
        // an extent, and if so turn into a free cell
        if (!meta_get_block(a, cell_idx + cc) && !meta_get_mark(a, cell_idx + cc)) {
            meta_set_mark(a, cell_idx + cc);
        }
    }
    if (arg_debug) {
        memset(ret, 0x17, s);
    }

    return ret;
}

struct allocator* allocator_new(void) {
    struct allocator *ret = malloc(sizeof(struct allocator));
    ret->arenas_free_list = NULL;
    ret->first_nursery = alloc_arena(ret, ARENA_TYPE_NURSERY);
    ret->first_survivor = alloc_arena(ret, ARENA_TYPE_NEW_SURVIVOR);
    ret->first_tenured = alloc_arena(ret, ARENA_TYPE_TENURED);
    ret->gc_count = 1;
    ret->pressure = 0;
    ret->gc_requested = false;
    ret->remset_once = new_gc_list(NULL);
    ret->remset_twice = new_gc_list(NULL);
    ret->remset_next = new_gc_list(NULL);
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

cell allocator_alloc_type(struct allocator *a, int s, uint8_t type) {
    a->pressure++;

    if (type == ARENA_TYPE_TENURED) {
        arena current_arena = a->first_tenured;
        do {
            cell ret = alloc_block_tenured(current_arena, s);
            if (!ret) {
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
            current_arena = a->first_nursery;
        }
        else if (type == ARENA_TYPE_NEW_SURVIVOR) {
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

void allocator_request_gc(struct allocator *a, bool full) {
    a->gc_requested = true;
    a->gc_requested_full = full;
}

struct allocator_gc_ctx* allocator_gc_new(struct allocator *a) {
    struct allocator_gc_ctx *ret = malloc(sizeof(struct allocator_gc_ctx));
    ret->a = a;
    ret->list = new_gc_list(NULL);
    ret->major_gc = ((a->gc_count % arg_major_gc_ratio) == 0) || (a->gc_requested_full);
    return ret;
}


void allocator_gc_add_root(struct allocator_gc_ctx *gc, value *v) {
    struct arena_header *ah = cell_to_arena(*v);
    if ((!gc->major_gc) && (ah->arena_type == ARENA_TYPE_TENURED)) {
        // do not take tenured/old generation roots or heap items on minor
        // collections
        return;
    }
    if (gc->list->count == GC_LIST_SIZE) {
        gc->list = new_gc_list(gc->list);
    }
    gc->list->valueps[gc->list->count] = v;
    gc->list->count++;
}

// XXX we could have a _fp macro that does the immediate check...
// XXX and now we do not need a value pointer anymore for the youger arg, clean
// up
void write_barrier(struct allocator *a, value c, value *n) {
    if (!value_is_immediate(c) && !value_is_immediate(*n)) {
        struct arena_header *cah = cell_to_arena(c);
        struct arena_header *nah = cell_to_arena(*n);
        if (       (cah->arena_type == ARENA_TYPE_TENURED)
                && (nah->arena_type != ARENA_TYPE_TENURED)) {
            // we are gaining a reference from the tenured C to the younger-gen N,
            // so we need to record N for the next minor GCs as this goes against
            // the grain...
            if (a->remset_once->count == GC_LIST_SIZE) {
                a->remset_once = new_gc_list(a->remset_once);
            }
            a->remset_once->values[a->remset_once->count] = c;
            a->remset_once->count++;
            // references that go over two generations need to be handled the
            // next *two* collections
            if (nah->arena_type == ARENA_TYPE_NURSERY) {
                if (a->remset_twice->count == GC_LIST_SIZE) {
                    a->remset_twice = new_gc_list(a->remset_twice);
                }
                a->remset_twice->values[a->remset_twice->count] = c;
                a->remset_twice->count++;
            }
        }
        if (       (cah->arena_type == ARENA_TYPE_NEW_SURVIVOR)
                && (nah->arena_type == ARENA_TYPE_NURSERY)) {
            if (a->remset_next->count == GC_LIST_SIZE) {
                a->remset_next = new_gc_list(a->remset_next);
            }
            a->remset_next->values[a->remset_next->count] = c;
            a->remset_next->count++;
        }
    }
}

// XXX can this be removed now? might come in handy some time...
void allocator_gc_add_nonval_root(struct allocator_gc_ctx *gc, void *m) {
    // we mark these right away
    uint_fast16_t cell_idx = cell_index(m);
    arena a = cell_to_arena(m);
    struct arena_header *ah = a;
    assert(ah->arena_type == ARENA_TYPE_TENURED);
    meta_set_mark(a, cell_idx);
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

void traverse_heap_item(struct allocator_gc_ctx *gc, value cv) {
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
        case TYPE_BOXED:
            switch (value_subtype(cv)) {
                case SUBTYPE_ENV:
                    interp_traverse_env(gc, value_to_cell(cv));
                    break;
                case SUBTYPE_ENV_ENTRY:
                    interp_traverse_env_entry(gc, value_to_cell(cv));
                    break;
                case SUBTYPE_DYN_FRAME:
                    interp_traverse_dynamic_frame(gc, value_to_cell(cv));
                    break;
                case SUBTYPE_PORT:
                    // XXX should probably take a real port struct, not a
                    // value as second arg
                    traverse_port(gc, cv);
                    break;
                case SUBTYPE_PARAM:;
                    struct param *cp = value_to_parameter(cv);
                    allocator_gc_add_root_fp(gc, &cp->init);
                    allocator_gc_add_root_fp(gc, &cp->convert);
                    break;
                default:;
                // not traversable
            }
        default:;
            // not traversable
    }
}

void allocator_gc_perform(struct allocator_gc_ctx *gc) {
    if (arg_runtime_stats) {
        fprintf(stderr, "# %s GC #%i after %i allocations (threshold %i)\n",
            gc->major_gc ? "Major" : "Minor", gc->a->gc_count,
            gc->a->pressure, arg_gc_threshold);
    }
    long mark_start = currentmicros();

    gc->a->gc_requested = false;
    gc->a->gc_requested_full = false;
    gc->a->gc_count++;

    int visited = 0;
    int reclaimed = 0;
    int promoted = 0;

    // turn all existing survivor arenas into ARENA_TYPE_OLD_SURVIVOR
    arena a = gc->a->first_survivor;
    while (a) {
        struct arena_header *ah = a;
        ah->arena_type = ARENA_TYPE_OLD_SURVIVOR;
   //     fprintf(stderr, "turning arena %p into OLD_SURVIVOR\n", a);
        a = ah->next;
    }

    // we do not want to mix new survivor arenas with old ones, since we
    // allocate into the new ones and move from the old ones to tenured
    // note that we traverse these before mark/sweep, so it is possible
    // that the container is actually not reachable anymore, currently we do not
    // handle this and it just means some objects will be alive longer than
    // strictly necessary.
    arena old_survivor = gc->a->first_survivor;
    gc->a->first_survivor = alloc_arena(gc->a, ARENA_TYPE_NEW_SURVIVOR);

    // XXX explain
    struct allocator_gc_list *remset_temp = new_gc_list(NULL);
    if (!gc->major_gc) {
        while (gc->a->remset_once->count) {
            value cv = gc->a->remset_once->values[--gc->a->remset_once->count];
            if ((!gc->a->remset_once->count) && (gc->a->remset_once->prev)) {
                struct allocator_gc_list *temp = gc->a->remset_once;
                gc->a->remset_once = temp->prev;
                free(temp);
            }
            traverse_heap_item(gc, cv);
        }
        while (gc->a->remset_twice->count) {
            value cv = gc->a->remset_twice->values[--gc->a->remset_twice->count];
            if ((!gc->a->remset_twice->count) && (gc->a->remset_twice->prev)) {
                struct allocator_gc_list *temp = gc->a->remset_twice;
                gc->a->remset_twice = temp->prev;
                free(temp);
            }
            traverse_heap_item(gc, cv);
            // but add to the next _once set
            // XXX obviously this could be done more efficently
            if (remset_temp->count == GC_LIST_SIZE) {
                remset_temp = new_gc_list(remset_temp);
            }
            remset_temp->values[remset_temp->count] = cv;
            remset_temp->count++;
        }
    }
    // remset_next is weird because the containers themselves might move
    // during this GC, or go away entirely which is equally bad. so we just
    // add them as roots, even if that makes the whole affair less precise
    struct allocator_gc_list *cl = gc->a->remset_next;
    while (cl) {
        for (int i = 0; i < cl->count; i++) {
       //     fprintf(stderr, "Adding %p from remset_next to GC roots\n",
       //         (void*)cl->values[i]);
            allocator_gc_add_root_fp(gc, &cl->values[i]);
        }
        cl = cl->prev;
    }


    // mark phase
    while (gc->list->count) {
        value *cvptr = gc->list->valueps[--gc->list->count];
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
                    cell newloc = allocator_alloc_type(gc->a, cc*16, next_arena_type(ah->arena_type));
                    cell oldloc = value_to_cell(cv);
                    memcpy(newloc, oldloc, cc*16);
                    promoted++;
                    value nv = (uint64_t)newloc | value_type(cv);
                    //fprintf(stderr, "  moving heap item %p -> %p (arena type %i -> %i)\n",
                    //    (void*)cv, (void*)nv, ah->arena_type, next_arena_type(ah->arena_type));
                    if (arg_debug) {
                        memset(oldloc, 0x19, cc*16);
                    }
                    // install the forwarding pointer
                    memcpy(oldloc, &newloc, sizeof(void*));

                    traverse = true;
                    cv = nv;
                    *cvptr = cv;
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
                traverse_heap_item(gc, cv);
            }
        }
    }
    long mark_end = currentmicros();

    // after marking we need to do some householding on the remembered sets
    if (gc->major_gc) {
        // just clear the _once set, it's not needed in a full GC
        while (gc->a->remset_once) {
            struct allocator_gc_list *temp = gc->a->remset_once;
            gc->a->remset_once = temp->prev;
            free(temp);
        }
        // replace with _twice, shuffle along for the next GC cycle
        gc->a->remset_once = gc->a->remset_twice;
        gc->a->remset_twice = new_gc_list(NULL);
        // splice _next onto _once.
        // XXX could be more efficient
        while (gc->a->remset_next->count) {
            value cv = gc->a->remset_next->values[--gc->a->remset_next->count];
            if ((!gc->a->remset_next->count) && (gc->a->remset_next->prev)) {
                struct allocator_gc_list *temp = gc->a->remset_next;
                gc->a->remset_next = temp->prev;
                free(temp);
            }
            if (gc->a->remset_once->count == GC_LIST_SIZE) {
                gc->a->remset_once = new_gc_list(gc->a->remset_once);
            }
            gc->a->remset_once->values[gc->a->remset_once->count] = cv;
            gc->a->remset_once->count++;
        }
        // go through _once and remove all items in tenured that are not
        // marked, these will be sweeped and we do not want to trace into them
        // during the next GC!
        struct allocator_gc_list *cl = gc->a->remset_once;
        while (cl) {
            for (int i = 0; i < cl->count; i++) {
                value *cvp = &cl->values[i];
                arena a = cell_to_arena(value_to_cell(*cvp));
                struct arena_header *ah = a;
                uint_fast16_t cell_idx = cell_index(value_to_cell(*cvp));
                if (ah->arena_type == ARENA_TYPE_TENURED)
                    assert(meta_get_block(a, cell_idx));
                    if (!meta_get_mark(a, cell_idx)) {
                        //fprintf(stderr, "Removing %p from remset_once\n", (void*)*cvp);
                        *cvp = VALUE_NIL;
                    }
            }
            cl = cl->prev;
        }
    }
    else {
        // _twice and _next have already been combined into _temp
        free(gc->a->remset_once);
        gc->a->remset_once = remset_temp;
        // XXX what all of this is missing is some way to make sure _next and
        // _twice do not trace into an item not copied from survivor to tenured
        // we could just add them to the root set...

        // XXX same as above, refactor
        // splice _next onto _once.
        // XXX could be more efficient
        while (gc->a->remset_next->count) {
            value cv = gc->a->remset_next->values[--gc->a->remset_next->count];
            if ((!gc->a->remset_next->count) && (gc->a->remset_next->prev)) {
                struct allocator_gc_list *temp = gc->a->remset_next;
                gc->a->remset_next = temp->prev;
                free(temp);
            }
            if (gc->a->remset_once->count == GC_LIST_SIZE) {
                gc->a->remset_once = new_gc_list(gc->a->remset_once);
            }
            gc->a->remset_once->values[gc->a->remset_once->count] = cv;
            gc->a->remset_once->count++;
        }
    }

    // sweep tenured
    a = gc->a->first_tenured;
    while (a) {
        // XXX should the if be outside the loop?
        if (gc->major_gc) {
            // XXX of course we want to do this with word-sized manipulations, not
            // looping
            for (int i = 1024; i < 65535; i++) {
                if (meta_get_block(a, i) && !meta_get_mark(a, i)) {
                    reclaimed++;
                    meta_clear_block(a, i);
                    meta_set_mark(a, i);
                    //fprintf(stderr, "reclaiming in tenured %p\n", cell_from_cell_index(a, i));
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
            // XXX temp unmark, same as minor so move outside if/else
            for (int i = 1024; i < 65535; i++) {
                if (meta_get_block(a, i) && meta_get_mark(a, i)) {
                    meta_clear_mark(a, i);
                }
            }
        }
        else {
            // minor GC, just unmark
            for (int i = 1024; i < 65535; i++) {
                if (meta_get_block(a, i) && meta_get_mark(a, i)) {
                    meta_clear_mark(a, i);
                }
            }
        }
        struct arena_header *ah = a;
        // reset scan_cache, we can just set this as it is only a lower bound
        a = ah->next;
        ah->scan_cache = 1024;
    }

    // also unmark the survivor pages
    a = gc->a->first_survivor;
    while (a) {
        for (int i = 1024; i < 65535; i++) {
            if (meta_get_block(a, i) && meta_get_mark(a, i)) {
                meta_clear_mark(a, i);
            }
        }
        struct arena_header *ah = a;
        a = ah->next;
    }

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
    long sweep_end = currentmicros();

    if (arg_runtime_stats) {
        fprintf(stderr, "#   %i heap items traced, %i promoted, %i reclaimed in tenured\n", 
            visited, promoted, reclaimed);
        fprintf(stderr, "# Finished %s GC with mark phase of %lius and sweep of %lius\n",
            gc->major_gc ? "major" : "minor",
            mark_end - mark_start, sweep_end - mark_end);
        fprintf(stderr, "\n");

        total_gc_time_us += sweep_end - mark_start;
    }

    gc->a->pressure = 0;
    free(gc->list);
    free(gc);
}

void mprot_arena_chain(arena a, int prot) {
    while (a) {
        assert(mprotect(a, ARENA_SIZE, prot) == 0);
        struct arena_header *ah = a;
        a = ah->next;
    }
}
