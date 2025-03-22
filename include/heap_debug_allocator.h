#ifdef BLIB_DEBUG
#ifndef HEAP_DEBUG_ALLOCATOR_H
#define HEAP_DEBUG_ALLOCATOR_H

#include "assert.h"
#include <stdlib.h> // malloc, free, realloc
#include "slice.h"
#include "arena_allocator.h"
#include "bytes.h"

typedef struct HeapAllocation {
	void *ptr;
	size_t size;
	const char *file;
	int line;
	unsigned char freed;
} HeapAllocation;

typedef struct {
	size_t alloc_bytes;
	size_t n_allocs;
	size_t leak_bytes;
	size_t n_leaks;
	Slice allocs; // HeapAllocation
} HeapAllocatorReport ;

typedef struct HeapDebugAllocator {
	Allocator *backing;
	size_t alloc_tot;
	size_t n_allocs;
	Allocator arena;
	Slice allocs;
} HeapDebugAllocator;

static int heap_allocation_cmp_ptr(void *ctx, void *test) {
	return (int)(((size_t)ctx) == (size_t)((HeapAllocation *)test)->ptr);
}

static void *heap_debug_alloc_fn(Allocator *a, AllocatorOP op) {
	HeapDebugAllocator *h = a->state;
	void *p = 0;
	HeapAllocation ha = {0};
	HeapAllocation *haptr = 0;
	switch (op.opcode) {
	case ALLOC_ALLOC:
		p = _alloc_new(
			h->backing, op.data.alloc.size, op.data.alloc.file, op.data.alloc.line
		);
		if (!p) return 0;
		h->alloc_tot += op.data.alloc.size;
		h->n_allocs++;
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
			return p;
		}
		assert(!slice_append(&h->allocs, &ha));
		return p;
	case ALLOC_FREE:
		if (
			!slice_find_ptr(
				&h->allocs,
				(void*)op.data.free.ptr,
				heap_allocation_cmp_ptr,
				(void *)&haptr
			)
		) {
			printf(
				"%s:%d freeing pointer %p that is not allocated\n",
				op.data.free.file,
				op.data.free.line,
				op.data.free.ptr
			);
			fflush(stdout);
			assert(0);
		}
		if (haptr->freed) {
			printf(
				"%s:%d double freeing pointer %p allocated in %s:%d\n",
				op.data.free.file,
				op.data.free.line,
				op.data.free.ptr,
				haptr->file,
				haptr->line
			);
			fflush(stdout);
			assert(0);
		}
		_alloc_free(
			h->backing, op.data.free.ptr, op.data.free.file, op.data.free.line
		);
		haptr->freed = 1;
		return 0;
	case ALLOC_FREE_ALL:
		return 0;
	case ALLOC_REALLOC:
		if (
			!slice_find_ptr(
				&h->allocs,
				(void*)op.data.realloc.old,
				heap_allocation_cmp_ptr,
				(void *)&haptr
			)
		) {
			printf(
				"%s:%d attempt to realloc pointer %p that is not allocated\n",
				op.data.realloc.file,
				op.data.realloc.line,
				op.data.realloc.old
			);
			fflush(stdout);
			assert(0);
		}
		if (haptr->size < op.data.realloc.oldsz) {
			printf(
				"%s:%d attempt to realloc pointer %p with bad old size: %lu > %lu\n",
				op.data.realloc.file,
				op.data.realloc.line,
				op.data.realloc.old,
				op.data.realloc.oldsz,
				haptr->size
			);
			fflush(stdout);
			assert(0);
		}
		p = _alloc_realloc(
			h->backing,
			op.data.realloc.old,
			op.data.realloc.oldsz,
			op.data.realloc.newsz,
			op.data.realloc.file,
			op.data.realloc.line
		);
		if (!p) return 0;
		h->n_allocs++;
		h->alloc_tot += (op.data.realloc.newsz-haptr->size);
		haptr->ptr = p;
		haptr->size = op.data.realloc.newsz;
		return p;
	default:
		return 0;
	}
}

static int heap_debug_allocator_init(Allocator *a, Allocator *backing) {
	HeapDebugAllocator *h = 0;
	if (!(h = alloc_new(backing, sizeof(HeapDebugAllocator)))) return 1;
	*h = (HeapDebugAllocator){0};
	h->backing = backing;
	assert(!arena_init(&h->arena, backing));
	Slice s = {0};
	slice_init(&s, &h->arena, sizeof(HeapAllocation));
	h->allocs = s;
	*a = (Allocator){.alloc_fn = &heap_debug_alloc_fn, .state = h};
	return 0;
}

static void heap_debug_allocator_destroy(Allocator *a) {
	if (!a->state || a->alloc_fn != &heap_debug_alloc_fn) return;
	HeapDebugAllocator *h = a->state;
	arena_destroy(&h->arena);
	alloc_free(h->backing, a->state);
	Allocator b = {0};
	*a = b;
}

static int heap_allocator_get_report(Allocator *a, HeapAllocatorReport *r) {
	*r = (HeapAllocatorReport){0};
	HeapDebugAllocator *h = a->state;
	HeapAllocation ha = {0};
	//
	r->alloc_bytes = h->alloc_tot;
	r->n_allocs = h->n_allocs;
	r->allocs = h->allocs;
	//
	for (size_t i = 0; i < slice_len(&h->allocs); i++) {
		slice_get(&h->allocs, i, &ha);
		if (!ha.freed) {
			r->leak_bytes += ha.size;
			r->n_leaks++; 
		}
	}
	return 0;
}

static void heap_allocator_report_print(HeapAllocatorReport *r) {
	HeapAllocation ha = {0};
	printf("########## HEAP ALLOCATOR REPORT BEGIN ##########\n");
	if (bytes_is((void *)r, 0, sizeof(HeapAllocatorReport))) 
		printf("empty report\n");
	for (size_t i = 0; i < slice_len(&r->allocs); i++) {
		slice_get(&r->allocs, i, &ha);
		if (!ha.freed)
			printf("LEAK: ptr %p allocated in %s:%d\n", ha.ptr, ha.file, ha.line);
	}
	printf("TOTAL BYTES ALLOCATED: %lu\n", r->alloc_bytes);
	printf("TOTAL ALLOCATIONS: %lu\n", r->n_allocs);
	printf("TOTAL BYTES LEAKED: %lu\n", r->leak_bytes);
	printf("TOTAL POINTERS LEAKED: %lu\n", r->n_leaks);
	printf("########## HEAP ALLOCATOR REPORT END ##########\n");
}

#endif // HEAP_DEBUG_ALLOCATOR_H
#endif // BLIB_DEBUG
