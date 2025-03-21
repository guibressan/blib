#ifndef MALLOC_ALLOCATOR_H
#define MALLOC_ALLOCATOR_H

#include "allocator.h"

static void *malloc_alloc_func(Allocator *_, AllocatorOP op) {
	switch (op.opcode) {
	case ALLOC_ALLOC:
		return malloc(op.data.alloc.size);
	case ALLOC_FREE:
		free(op.data.free.ptr);
		return (void *)0;
	case ALLOC_FREE_ALL:
		return (void *)0;
	case ALLOC_REALLOC:
		return realloc(op.data.realloc.old, op.data.realloc.newsz);
	}
}

static int malloc_allocator_init(Allocator *a) {
	*a = (Allocator){0};
	a->alloc_fn = &malloc_alloc_func;
	return 0;
}

#endif
