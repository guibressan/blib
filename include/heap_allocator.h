#ifndef HEAP_ALLOCATOR_H
#define HEAP_ALLOCATOR_H

#include <stdlib.h> // malloc, free, realloc
#include "slice.h"
#include "arena_allocator.h"

typedef struct HeapAllocation {
	void *ptr;
	size_t size;
	unsigned char freed;
	const char *file;
	int line;
} HeapAllocation;

typedef struct HeapAllocator {
#ifdef DEBUG
	size_t alloc_tot;
	size_t free_tot;
	Allocator arena;
	Slice allocs;
#endif
} HeapAllocator;

static int heap_allocation_cmp_ptr(void *ctx, void *test) {
	return (int)(((size_t)ctx) == (size_t)((HeapAllocation *)test)->ptr);
}

static void *heap_alloc_fn(Allocator *a, AllocatorOP op) {
	HeapAllocator *h = a->state;
	void *p = 0;
#ifdef DEBUG
	HeapAllocation ha = {0};
	HeapAllocation *haptr = 0;
#endif
	switch (op.opcode) {
	case ALLOC_ALLOC:
		p = malloc(op.data.alloc.size);
		if (!p) return 0;
#ifdef DEBUG
		h->alloc_tot += op.data.alloc.size;
		ha = (HeapAllocation){
			.ptr = p,
			.size = op.data.alloc.size,
			.file = op.data.alloc.file,
			.line = op.data.alloc.line,
		};
		if (
			slice_find_ptr(
				&h->allocs,
				(void*)op.data.free.ptr,
				heap_allocation_cmp_ptr,
				(void *)&haptr
			)
		) {
			*haptr = ha;
		}
		assert(!slice_append(&h->allocs, &ha));
#endif
		return p;
	case ALLOC_FREE:
#ifdef DEBUG
		if (
			!slice_find_ptr(
				&h->allocs,
				(void*)op.data.free.ptr,
				heap_allocation_cmp_ptr,
				(void *)&haptr
			)
		) {
			printf(
				"%p %s:%d freeing a pointer that is not allocated\n",
				op.data.free.ptr,
				op.data.free.file,
				op.data.free.line
			);
			fflush(stdout);
			assert(0);
		}
		if (haptr->freed) {
			printf(
				"%p %s:%d double freeing a pointer allocated in %s:%d\n",
				op.data.free.ptr,
				op.data.free.file,
				op.data.free.line,
				haptr->file,
				haptr->line
			);
			fflush(stdout);
			assert(0);
		}
#endif
		free(op.data.free.ptr);
#ifdef DEBUG
		haptr->freed = 1;
		h->alloc_tot -=  haptr->size;
#endif
		return 0;
	case ALLOC_FREE_ALL:
		return 0;
	case ALLOC_REALLOC:
		p = realloc(op.data.realloc.old, op.data.realloc.newsz);
		if (!p) return 0;
		return p;
	default:
		return 0;
	}
	return 0;
}

static int heap_allocator_init(Allocator *a) {
	HeapAllocator *h = malloc(sizeof(HeapAllocator));
	if (!h) return 1;
	memset(h, 0, sizeof(HeapAllocator));
#ifdef DEBUG
	assert(!arena_init(&h->arena));
	Slice s = {0};
	slice_init(&s, &h->arena, sizeof(HeapAllocation));
	h->allocs = s;
#endif
	*a = (Allocator){.alloc_fn = &heap_alloc_fn, .state = h};
	return 0;
}

static void heap_allocator_destroy(Allocator *a) {
	if (!a->state || a->alloc_fn != &heap_alloc_fn) return;
	HeapAllocator *h = a->state;
#ifdef DEBUG
	arena_destroy(&h->arena);
#endif
	free(a->state);
	Allocator b = {0};
	*a = b;
}

static void heap_allocator_print_stats(Allocator *a) {
#ifdef DEBUG
	if (!a->state || a->alloc_fn != &heap_alloc_fn) return;
	HeapAllocator *h = a->state;
	HeapAllocation ha = {0};
	size_t alloc_tot = 0, free_tot = 0;
	for (size_t i = 0; i < slice_len(&h->allocs); i++) {
		assert(!slice_get(&h->allocs, i, &ha));
		alloc_tot += ha.size;
		free_tot += ha.freed ? ha.size : 0;
		if (!ha.freed) 
			printf("%p LEAKED %s:%d\n", ha.ptr, ha.file, ha.line);
	}
	printf(
		"TOTAL ALLOC %zu | TOTAL FREE %zu | TOTAL LEAK %zu\n",
		alloc_tot, free_tot, alloc_tot-free_tot
	);
#endif
}

#endif // HEAP_ALLOCATOR_H
