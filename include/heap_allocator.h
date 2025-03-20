#ifndef HEAP_ALLOCATOR_H
#define HEAP_ALLOCATOR_H

#include <stdlib.h> // malloc, free, realloc

typedef struct HeapAllocation {
	void *ptr;
	size_t size;
	const char *file_call;
	int line_call;
} HeapAllocation;

typedef struct HeapAllocator {
#ifdef DEBUG
	HeapAllocation *allocs;
	size_t allocs_len;
	size_t allocs_cap;
	size_t total_alloc;
	size_t total_free;
#endif
} HeapAllocator;

void *heap_alloc_fn(Allocator *a, AllocatorOP op) {
	HeapAllocator *h = a->state;
	void *p = 0;
	switch (op.opcode) {
	case ALLOC_ALLOC:
		p = malloc(op.data.alloc.size);
		if (!p) return 0;
		return p;
	case ALLOC_FREE:
		free(op.data.free.ptr);
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

int heap_allocator_init(Allocator *a) {
	HeapAllocator *h = malloc(sizeof(HeapAllocator));
	if (!h) return 1;
	memset(h, 0, sizeof(HeapAllocator));
	Allocator b = {0};
	b.alloc_fn = &heap_alloc_fn;
	b.state = h;
	*a = b;
	return 0;
}

void heap_allocator_destroy(Allocator *a) {
	if (!a || !a->alloc_fn || !a->state) return;
	free(a->state);
	Allocator b = {0};
	*a = b;
}

#endif // HEAP_ALLOCATOR_H
