#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stddef.h> // size_t
#include <stdlib.h> // malloc / free / realloc
#include <strings.h> // memset / memcpy
#include <stdio.h> // printf for debugging

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

////////////////////////////////////////
// Allocator base

#ifndef MIN_ALLOC_BLOCK
#define MIN_ALLOC_BLOCK (((size_t)1) << 20)
#endif

typedef enum AllocatorOPCode {
	ALLOC_NEW,
	ALLOC_FREE,
	ALLOC_FREE_ALL,
	ALLOC_REALLOC,
} AllocatorOPCode;

typedef struct AllocatorRealloc {
	 void *old;
	 size_t oldsz;
	 size_t newsz;
} AllocatorRealloc;

typedef struct AllocatorOP {
	 AllocatorOPCode opcode;
	 union {
		 size_t size;
		 void *ptr;
		 AllocatorRealloc realloc;
	 } data;
} AllocatorOP;

typedef struct Allocator {
	void *state;
	void *(*alloc_fn)(struct Allocator *allocator, AllocatorOP op);
} Allocator;

void *alloc_new(Allocator *a, size_t size);
void alloc_free(Allocator *a, void *ptr);
void alloc_free_all(Allocator *a);
void *alloc_realloc(Allocator *a, void *ptr, size_t oldsz, size_t newsz);

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

void *alloc_realloc(Allocator *a, void *old, size_t oldsz, size_t newsz) {
	AllocatorOP op = {};
	AllocatorRealloc r = {.old = old, .oldsz = oldsz, .newsz = newsz};
	op.opcode = ALLOC_REALLOC;
	op.data.realloc = r;
	return a->alloc_fn(a, op);
}

#endif // ALLOCATOR_H
