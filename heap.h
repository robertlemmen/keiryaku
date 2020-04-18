#ifndef HEAP_H
#define HEAP_H

#include <stdint.h>
#include <stdbool.h>

/* Heap Organization and Garbage Collection
 *
 * Data on the heap is stored as "cells", with each cell holding 16bytes. One 
 * or more consecutive cells make up an allocated memory block. Blocks are 
 * allocated from arenas of 1MB size, which hold some metadata at the beginning 
 * of each Arena. Arenas are aligned to their size, so the arena start and 
 * therefore the metadata block can easily be computed by masking the cell 
 * address. The index of a cell within an arena always fits in 16bits and can 
 * also be computed easily by masking and shifting. 
 *
 * The metadata block is 16kB in size, which means that the first 1024 cell
 * indices are never used. It holds two bitmaps: "block" and "mark", each with
 * one bit per cell. Since the first 1024 cells indices are actually metadata,
 * the first 1024 bits of each bitmap are also not used for actual cells and are
 * used for other purposes. The first one, at the very beginning of the arena 
 * is used as an 128 byte arena header, the other one is currently unused.
 *
 * Arenas can be of different types, which affects the way they are organised
 * and how they behave: All initial allocation happens in nursery arenas which
 * use a simple bump allocator to find space. If an item is found in a nursery 
 * arena during the mark phase, it is moved to a survivor arena and a forwarding
 * pointer is used to signal the new location. The mark bit is set to indicate
 * that the cell now holds a forwarding pointer rather than an object. 
 *
 * Suvivor arenas also use a bump allocator and forwarding pointers, but they
 * move objects to the a tenured arena.
 *
 * Tenured arenas are different in so far that they do not move objects, but
 * leave them in place from one GC cycle to the next if they are still used, and
 * therefore allocate into the free areas between the objects. Here both
 * bitmap are important, the two bits for each cell have the following meaning:
 *
 * block  mark  
 *     0     0  block extent, for blocks larger than one cell
 *     0     1  free cell
 *     1     0  allocated, white block
 *     1     1  allocated, black block
 *
 * When allocating memory we simply scan the two bitmaps until we find a free
 * cell. we cache the first free cell in the arena header, and reset it on 
 * garbage collection. Once we have found a freee cell, we need to check that 
 * we have enough consecutive free cells for our allocation. Once we found a freee
 * block, it is marked as allocated, white and the extents are set accordingly.
 *
 * When doing garbage collection, we trace through root objects and recursively
 * through the references they contain. This is done with lists of items that
 * still need to be traced. If an object is in a nursery or survivor
 * arena, we simply allocate it in a survivor or tenured arena (respectively),
 * if it is in a tenured arena, we mark it as black.
 * 
 * The list of objects that still need to be traced is per arena and linked from 
 * the metadata. By traversing objects from the list of the current arena first, 
 * we switch less between arenas and therefore cause less cache pollution. 
 *
 * Once we have traced, and therefore marked or moved all reachable objects, we 
 * can go through the metadata of all tenured arenas and turn all white blocks into 
 * free ones, and then turn all black blocks white again. All nursery and
 * survivor arenas are simply recycled as free arenas, since the still-reachable 
 * entries have been moved to the next generation already.
 * */

typedef void* arena;
typedef void* cell;
typedef void* block;

struct allocator;

arena alloc_arena(struct allocator *a, uint8_t type);
void free_arena(struct allocator *a, arena r);
block alloc_block(arena a, int s);
void free_block(block b);

struct allocator* allocator_new(void);
void allocator_free(struct allocator *a);

cell allocator_alloc(struct allocator *a, int s);
cell allocator_alloc_nonmoving(struct allocator *a, int s);

bool allocator_needs_gc(struct allocator *a);
void allocator_request_gc(struct allocator *a, bool full);

struct allocator_gc_ctx;

// XXX ugly! why do we have a cycle between this and types.h? eprhaps gc needs
// to be in own file?

struct allocator_gc_ctx* allocator_gc_new(struct allocator *a);
// this needs a pointer to a value as it needs to update the source if the item
// did get moved. also note that you need to ensure that the passed value is a
// non-immediate. you can use the _fp macro below for this
void allocator_gc_add_root(struct allocator_gc_ctx *gc, uint64_t *v);
// fast-path macro that avoids calling _add_root() for non-immediates
#define allocator_gc_add_root_fp(gc, v) if (!value_is_immediate(*v)) { allocator_gc_add_root(gc, v); }
void allocator_gc_add_nonval_root(struct allocator_gc_ctx *gc, void *m);
void allocator_gc_perform(struct allocator_gc_ctx *gc);

extern long total_gc_time_us;

#endif /* HEAP_H */
