#ifndef HEAP_H
#define HEAP_H

#include <stdint.h>
#include <stdbool.h>

/* Heap Organization
 *
 * This is largely a simplified version of http://wiki.luajit.org/New-Garbage-Collector
 *
 * Data on the heap is stored as "cells", with each cell holding 16bytes. One or more
 * consecutive cells make up an allocaoted memory block. Blocks are allocated
 * from arenas of 1MB size, which hold some metadata at the beginning of each
 * Arena. Arenas are aligned to their size, so the arena start and therefore the
 * metadata block can easily be computed by masking the cell address. The index
 * of a cell within an arena always fits in 16bits and can also be computed
 * easily by masking and shifting. 
 *
 * The metadata block is 16kB in size, which means that the first 1024 cell
 * indices are never used. It holds two bitmaps: "block" and "mark", each with
 * one bit per cell. Since the first 1024 cells indices are actually metadata,
 * the first 1024 bits of each bitmap are also not used for actual cells and are
 * used for other purposes (see below).
 *
 * The two bits for each cell have the following meaning:
 *
 * block  mark  
 *     0     0  block extent, for blocks larger than one cell
 *     0     1  free cell
 *     1     0  allocated, white block
 *     1     1  allocated, black block
 *
 * When allocating memory we simply scan the two bitmaps until we find a free
 * cell. we cache the first free cell in the metadata, and reset it on garbage
 * collection. Once we have found a freee cell, we need to check that we have
 * enough consecutive free cells for our allocation. Once we found a freee
 * block, it is marked as allocated, white and the extents are set accordingly.
 *
 * When doing garbage collection, we trace through root objects and mark all
 * reachable objects black. The list of objects that still need to be traced is
 * per arena and linked from the metadata. By traversing objects from the list
 * of the current arena first, we switch less between arenas and therefore cause
 * less cache pollution. 
 *
 * Once we have marked all reachable objects, we can go through the metadata of
 * all arenas and turn all white blocks into free ones, and then turn all black
 * blocks white again.
 * */

typedef void* arena;
typedef void* cell;
typedef void* block;

arena alloc_arena(void);
void free_arena(arena a);
block alloc_block(arena a, int s);
void free_block(block b);

struct allocator;

struct allocator* allocator_new(void);
void allocator_free(struct allocator *a);

cell allocator_alloc(struct allocator *a, int s);

bool allocator_needs_gc(struct allocator *a);

struct allocator_gc_ctx;

struct allocator_gc_ctx* allocator_gc_new(struct allocator *a);
// XXX ugly! why do we have a cycle between this and types.h? eprhaps gc needs
// to be in own file?
void allocator_gc_add_root(struct allocator_gc_ctx *gc, uint64_t v);
void allocator_gc_add_nonval_root(struct allocator_gc_ctx *gc, void *m);
void allocator_gc_perform(struct allocator_gc_ctx *gc);

#endif /* HEAP_H */
