#ifndef HEAP_ALLOCATOR_H
#define HEAP_ALLOCATOR_H

#include "assert.h"
#include <stdlib.h> // malloc, free, realloc
#include "slice.h"
#include "arena_allocator.h"
#include "heap_debug_allocator.h"
#include "bytes.h"

typedef struct HeapAllocator {
	Allocator *backing;
} HeapAllocator;

static void *heap_alloc_fn(Allocator *a, AllocatorOP op) {
	HeapAllocator *h = a->state;
	void *p = 0;
	switch (op.opcode) {
	case ALLOC_ALLOC:
		p = alloc_new(h->backing, op.data.alloc.size);
		if (!p) return 0;
		return p;
	case ALLOC_FREE:
		alloc_free(h->backing, op.data.free.ptr);
		return 0;
	case ALLOC_FREE_ALL:
		return 0;
	case ALLOC_REALLOC:
		p = alloc_realloc(
			h->backing,
			op.data.realloc.old,
			op.data.realloc.oldsz,
			op.data.realloc.newsz
		);
		if (!p) return 0;
		return p;
	default:
		return 0;
	}
}

static int heap_allocator_init(Allocator *a, Allocator *backing) {
#ifdef BLIB_DEBUG
	return heap_debug_allocator_init(a, backing);
#endif
	HeapAllocator *h = 0;
	if (!(h = alloc_new(backing, sizeof(HeapAllocator)))) return 1;
	*h = (HeapAllocator){0};
	h->backing = backing;
	*a = (Allocator){.alloc_fn = &heap_alloc_fn, .state = h};
	return 0;
}

static void heap_allocator_destroy(Allocator *a) {
#ifdef BLIB_DEBUG
		return heap_debug_allocator_destroy(a);
#endif
	if (!a->state || a->alloc_fn != &heap_alloc_fn) return;
	HeapAllocator *h = a->state;
	alloc_free(h->backing, a->state);
	Allocator b = {0};
	*a = b;
}

#endif // HEAP_ALLOCATOR_H
