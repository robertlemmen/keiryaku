#include "check_heap.h"

#include <stdlib.h>
#include <stdio.h>

#include "heap.h"
#include "types.h"

// globals required for linking, but not by this test
bool arg_debug = false;
int arg_gc_threshold = 100000;

struct interp_lambda {
};

void interp_traverse_lambda(struct allocator_gc_ctx *gc, struct interp_lambda *l) {
}

void traverse_port(struct allocator_gc_ctx *gc, value p) {
}

START_TEST(test_heap_01) {
    printf("- test_heap_01: non-moving allocation should not move during GC\n");
    struct allocator *a = allocator_new();

    cell item = allocator_alloc_nonmoving(a, 20);
    *((uint32_t*)item) = 0x12345678;
    ck_assert(item != NULL);

    cell orig_item = item;
    
    struct allocator_gc_ctx *gc_ctx = allocator_gc_new(a);

    allocator_gc_add_nonval_root(gc_ctx, item);
    allocator_gc_perform(gc_ctx);

    ck_assert(item == orig_item);
    ck_assert(*((uint32_t*)item) == 0x12345678);

    allocator_free(a);
}
END_TEST

START_TEST(test_heap_02) {
    printf("- test_heap_02: normal allocation should move during GC\n");
    struct allocator *a = allocator_new();

    value item = make_cons(a, make_int(a, 0x123456), make_int(a, 0));

    value orig_item = item;
    
    struct allocator_gc_ctx *gc_ctx = allocator_gc_new(a);

    // we add the item multiple times, GC should be able to tolerate this
    allocator_gc_add_root(gc_ctx, &item);
    // add it twice, this is a bit nefarious for the GC
    allocator_gc_add_root(gc_ctx, &item);
    allocator_gc_perform(gc_ctx);

    // should have moved
    ck_assert(item != orig_item); 
//    ck_assert(item == item2); 
    // ...but should be the same value
    ck_assert(car(item) == make_int(a, 0x123456));

    // let's do another GC cycle, this should go from survivor to tenured
    orig_item = item;
    gc_ctx = allocator_gc_new(a);

    allocator_gc_add_root(gc_ctx, &item);
    allocator_gc_add_root(gc_ctx, &item);
    allocator_gc_perform(gc_ctx);

    ck_assert(item != orig_item); 
    ck_assert(car(item) == make_int(a, 0x123456));

    // this time the item is already in tenured, so should not move again
    orig_item = item;
    gc_ctx = allocator_gc_new(a);

    allocator_gc_add_root(gc_ctx, &item);
    allocator_gc_add_root(gc_ctx, &item);
    allocator_gc_perform(gc_ctx);

    ck_assert(item == orig_item); 
    ck_assert(car(item) == make_int(a, 0x123456));

    allocator_free(a);
}
END_TEST

START_TEST(test_heap_03) {
    printf("- test_heap_03: verify the first-fit behavior\n");
    struct allocator *a = allocator_new();

    cell item_a = allocator_alloc_nonmoving(a, 16);
    cell item_b = allocator_alloc_nonmoving(a, 32);
    cell item_c = allocator_alloc_nonmoving(a, 16);
    cell item_d = allocator_alloc_nonmoving(a, 48);
    cell item_e = allocator_alloc_nonmoving(a, 16);

    ck_assert(item_a + 16 == item_b);
    ck_assert(item_b + 32 == item_c);
    ck_assert(item_c + 16 == item_d);
    ck_assert(item_d + 48 == item_e);

    struct allocator_gc_ctx *gc_ctx = allocator_gc_new(a);
    allocator_gc_add_nonval_root(gc_ctx, item_a);
    allocator_gc_add_nonval_root(gc_ctx, item_c);
    allocator_gc_add_nonval_root(gc_ctx, item_e);
    allocator_gc_perform(gc_ctx);

    cell item_f = allocator_alloc_nonmoving(a, 16);
    cell item_g = allocator_alloc_nonmoving(a, 32);
    cell item_h = allocator_alloc_nonmoving(a, 16);
    cell item_i = allocator_alloc_nonmoving(a, 16);

    ck_assert(item_a + 16 == item_f);
    ck_assert(item_f + 16 == item_h);
    ck_assert(item_c + 16 == item_g);
    ck_assert(item_g + 32 == item_i);

    allocator_free(a);
}
END_TEST

TCase* make_heap_checks(void) {
    TCase *tc_types;

    tc_types = tcase_create("heap");
    tcase_add_test(tc_types, test_heap_01);
    tcase_add_test(tc_types, test_heap_02);
    tcase_add_test(tc_types, test_heap_03);

    return tc_types;
}

