#ifndef ARENA_ALLOCATOR_H
#define ARENA_ALLOCATOR_H
#include "allocator.h"

typedef struct ArenaBlock {
	struct ArenaBlock *prev;
	void *base;
	size_t cap;
	size_t len;
} ArenaBlock;

typedef struct ArenaAllocator {
	ArenaBlock *last_block;
} ArenaAllocator;

static int arena_grow(ArenaAllocator **arena, size_t sz) {
	if (!arena) return -1;
	ArenaAllocator *a = *arena;
	size_t arenasz = sizeof(ArenaAllocator);
	size_t blocksz = sizeof(ArenaBlock);
	size_t datasz = MAX(sz, MIN_ALLOC_BLOCK);
	size_t allocsz = 0;
	ArenaBlock *oldblk = 0;
	unsigned char *allocp = 0;
	// initialize the arena
	if (!a) {
		allocsz = arenasz + blocksz + datasz;
		size_t zrsz = arenasz + blocksz; // do not zero data
		if (!(allocp = malloc(allocsz))) return -1;
		memset(allocp, 0, zrsz);
		a = (ArenaAllocator*)allocp;
		allocp += arenasz;
		a->last_block = (ArenaBlock *)allocp;
		allocp += blocksz;
		a->last_block->base = (void *)allocp;
		a->last_block->cap = datasz;
		*arena = a;
		return 0;
	}
	// grow the arena
	oldblk = a->last_block;
	allocsz = blocksz + datasz;
	if (!(allocp = malloc(allocsz))) return -1;
	memset(allocp, 0, blocksz);
	a->last_block = (ArenaBlock *)allocp;
	allocp += blocksz;
	a->last_block->base = allocp;
	a->last_block->cap = datasz;
	a->last_block->prev = oldblk;
	return 0;
}

static void *arena_alloc(ArenaAllocator *a, size_t sz) {
	if (a->last_block->cap - a->last_block->len < sz)
		if(arena_grow(&a, sz)) return 0;
	void *r = (void*) (
		(a->last_block->len) + (size_t) a->last_block->base
	);
	a->last_block->len += sz;
	return r;
}

static void *arena_alloc_fn(Allocator *a, AllocatorOP op) {
	ArenaAllocator *arena = (ArenaAllocator *)a->state;
	switch (op.opcode) {
	case ALLOC_ALLOC: {
		size_t needed = op.data.alloc.size;
		if (!a->state) {
			ArenaAllocator *arena1 = 0;
			if (arena_grow(&arena1, needed)) return 0;
			a->state = arena1;
			arena = arena1;
		}
		return arena_alloc(arena, needed);
	}
	case ALLOC_FREE:
		// free is not implemented for arena, but it is safe to call
		return 0;
	case ALLOC_FREE_ALL:
		if (!arena) {
			return 0;
		}
		for (ArenaBlock *b = 0; (b = arena->last_block->prev);) {
			free(arena->last_block);
			arena->last_block = b;
		}
		arena->last_block->len = 0;
		return 0;
	case ALLOC_REALLOC:
		if (!arena) return 0;
		void *newp = 0;
		if (!(newp = arena_alloc(arena, op.data.realloc.newsz))) return 0;
		memcpy(newp, op.data.realloc.old, op.data.realloc.oldsz);
		return newp;
	default:
		return 0;
	}
}

int arena_init(Allocator *a) {
	Allocator alloc = {0};
	alloc.alloc_fn = &arena_alloc_fn;
	*a = alloc;
	return 0;
}

int arena_destroy(Allocator *a) {
	if (!a->state) return 0;
	ArenaAllocator *arena = (ArenaAllocator *)a->state;
	int i = 0;
	for (ArenaBlock *b = 0; (b = arena->last_block->prev) ; i++) {
		free(arena->last_block);
		arena->last_block = b;
	}
	free(arena);
	a->state = 0;
	return 0;
}

#endif // ARENA_ALLOCATOR_H
