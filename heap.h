#ifndef HEAP_H
#define HEAP_H

#include <stdint.h>

/* Heap Organization
 *
 * data on the heap is stored as "cells", where each cell needs to hold the data in 
 * question, but also track the type and hold garbage collector state. cells are a 
 * multiple of 16bytes and are aligned to 16byte-boundaries as well. 
 *
 * we would like to keep typical cells (e.g. cons) small, simple and aligned, so we 
 * use larger blocks of memory to hold the cells and organise it so that the extra
 * byte of object metadata can be computed from the address: each block has a fixed 
 * size and is aligned so that the start address only has zeroes in the corresponding 
 * number of lower order bits. e.g a 1MB page could start at address 0xDEADBEEF12300000.
 * there can be up to 65536 cells of 16bytes in this, so we reserve the first 64k of 
 * the block for metadata. this way each cell has a byte of metadata, whose address
 * can be easily computed: 
 *   cell_addr & 0xFFFFFFFFFFF00000 is the start of the block
 *   (cell_addr & 0x00000000000FFFFF >> 8) is the index of the cell into the block
 *   adding these two gives the address of the metadata byte
 *
 * XXX what to store in metadata? depends on GC design, but we need size for
 * variable blobs...
 *
 * Note that the first 64kB of the block are not used by cells, consequently the first
 * 4096 metadata bytes are not used either, these can be used to store other 
 * information, e.g. allocator data.
 *
 * We should have buckets of blocks for allocations of common fixed sizes, e.g.
 * 16 bytes like cons cells. these could then just be a bump allocator + free
 * list!
 * */

#define BLOCK_SIZE 0x100000
#define cell_to_block(x) ((uint64_t)x & 0xFFFFFFFFFFF00000)
#define cell_index(x) (((uint64_t)x & 0x00000000000FFFFF) >> 8)
#define cell_to_cell_meta(x) (cell_to_block(x) + cell_index(x))

typedef void* block;
typedef void* cell;
typedef uint8_t cell_meta;

block alloc_block(void);
void free_block(block b);
cell alloc_cell(block b, int s); // s is size required
void free_cell(cell c);

struct allocator;

struct allocator* allocator_new(void);
void allocator_free(struct allocator *a);

cell allocator_alloc(struct allocator *a, int s);

#endif /* HEAP_H */
