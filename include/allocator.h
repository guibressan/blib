#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stddef.h> // size_t
#include <stdlib.h> // malloc / free / realloc
#include <strings.h> // memset
#include <assert.h> // assert
#include <stdio.h> // printf for debugging

////////////////////////////////////////
// Allocator base

#ifndef MIN_ALLOC_BLOCK
#define MIN_ALLOC_BLOCK (((size_t)1) << 20)
#endif

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

typedef enum AllocatorOPCode {
	ALLOC_NEW,
	ALLOC_FREE,
	ALLOC_FREE_ALL,
	ALLOC_REALLOC,
} AllocatorOPCode;

typedef struct AllocatorOP {
	 AllocatorOPCode opcode;
	 union {
		 size_t size;
		 void *ptr;
	 } data;
} AllocatorOP;

typedef struct Allocator {
	void *state;
	void *(*alloc_fn)(struct Allocator *allocator, AllocatorOP op);
} Allocator;

void *alloc_new(Allocator *a, size_t size);
void alloc_free(Allocator *a, void *ptr);
void alloc_free_all(Allocator *a);
void *alloc_realloc(Allocator *a, void *ptr);

void *alloc_new(Allocator *a, size_t size) {
	AllocatorOP op;
	op.opcode = ALLOC_NEW;
	op.data.size = size;
	return a->alloc_fn(a, op);
}

void alloc_free(Allocator *a, void *ptr) {
	AllocatorOP op;
	op.opcode = ALLOC_FREE;
	op.data.ptr = ptr;
	a->alloc_fn(a, op);
	return;
}

void alloc_free_all(Allocator *a) {
	AllocatorOP op;
	op.opcode = ALLOC_FREE_ALL;
	a->alloc_fn(a, op);
	return;
}

void *alloc_realloc(Allocator *a, void *ptr) {
	AllocatorOP op;
	op.opcode = ALLOC_FREE_ALL;
	op.data.ptr = ptr;
	return a->alloc_fn(a, op);
}

////////////////////////////////////////
// Arena allocator

typedef struct ArenaBlock {
	struct ArenaBlock *prev;
	void *base;
	size_t cap;
	size_t len;
} ArenaBlock;

typedef struct ArenaAllocator {
	ArenaBlock *last_block;
} ArenaAllocator;

static int *arena_grow(ArenaAllocator **arena, size_t sz) {
	assert(arena);
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
		allocp = malloc(allocsz);
		assert(allocp);
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
	allocp = malloc(allocsz);
	assert(allocp);
	memset(allocp, 0, blocksz);
	a->last_block = (ArenaBlock *)allocp;
	allocp += blocksz;
	a->last_block->base = allocp;
	a->last_block->cap = datasz;
	a->last_block->prev = oldblk;
	return 0;
}

static void *arena_alloc_fn(Allocator *a, AllocatorOP op) {
	ArenaAllocator *arena = (ArenaAllocator *)a->state;
	switch (op.opcode) {
	case ALLOC_NEW: {
		size_t needed = op.data.size;
		if (!a->state) {
			ArenaAllocator *arena1 = 0;
			assert(!arena_grow(&arena1, needed));
			a->state = arena1;
			arena = arena1;
		}
		if (arena->last_block->cap - arena->last_block->len < needed) {
			assert(!arena_grow(&arena, needed));
		} 
		void *r = (void *)(
			((size_t)arena->last_block->base) + arena->last_block->len
		);
		arena->last_block->len += needed;
		return r;
	}
	case ALLOC_FREE:
		break;
	case ALLOC_FREE_ALL:
		break;
	case ALLOC_REALLOC:
		break;
	default:
		assert(0);
	}
	return 0;
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

#endif
