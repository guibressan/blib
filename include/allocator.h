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
	ALLOC_ALLOC,
	ALLOC_FREE,
	ALLOC_FREE_ALL,
	ALLOC_REALLOC,
} AllocatorOPCode;

typedef struct AllocatorAlloc {
	 size_t size;
#ifdef BLIB_DEBUG
	 const char *file;
	 int line;
#endif
} AllocatorAlloc;

typedef struct AllocatorFree {
	 void *ptr;
#ifdef BLIB_DEBUG
	 const char *file;
	 int line;
#endif
} AllocatorFree;

typedef struct AllocatorRealloc {
	 void *old;
	 size_t oldsz;
	 size_t newsz;
#ifdef BLIB_DEBUG
	 const char *file;
	 int line;
#endif
} AllocatorRealloc;

typedef struct AllocatorOP {
	 AllocatorOPCode opcode;
	 union {
		 AllocatorAlloc alloc;
		 AllocatorFree free;
		 AllocatorRealloc realloc;
	 } data;
} AllocatorOP;

typedef struct Allocator {
	void *state;
	void *(*alloc_fn)(struct Allocator *allocator, AllocatorOP op);
} Allocator;


#ifdef BLIB_DEBUG

#define alloc_new(a, s)\
_alloc_new((a), (s), __FILE__, __LINE__)
#define alloc_free(a, p) _alloc_free((a), (p), __FILE__, __LINE__)
#define alloc_realloc(a, p, o, n)\
_alloc_realloc((a),(p),(o),(n), __FILE__, __LINE__)

static void *_alloc_new(Allocator *a, size_t size, const char *file, int line);
static void _alloc_free(Allocator *a, void *ptr, const char *file, int line);
static void alloc_free_all(Allocator *a);
static void *_alloc_realloc(
	Allocator *a,
	void *ptr,
	size_t oldsz,
	size_t newsz,
	const char *file,
	int line
);

#else

static void *alloc_new(Allocator *a, size_t size);
static void alloc_free(Allocator *a, void *ptr);
static void alloc_free_all(Allocator *a);
static void *alloc_realloc(Allocator *a, void *ptr, size_t oldsz, size_t newsz);

#endif //BLIB_DEBUG


#ifdef BLIB_DEBUG
static void *_alloc_new(Allocator *a, size_t size, const char *file, int line) {
	AllocatorOP op;
	op.opcode = ALLOC_ALLOC;
	AllocatorAlloc data = { .size = size, .file = file, .line = line };
	op.data.alloc = data;
	return a->alloc_fn(a, op);
}

static void _alloc_free(Allocator *a, void *ptr, const char *file, int line) {
	AllocatorOP op;
	op.opcode = ALLOC_FREE;
	AllocatorFree data = { .ptr = ptr, .file = file, .line = line };
	op.data.free = data;
	a->alloc_fn(a, op);
	return;
}

static void alloc_free_all(Allocator *a) {
	AllocatorOP op = {0};
	op.opcode = ALLOC_FREE_ALL;
	a->alloc_fn(a, op);
	return;
}

static void *_alloc_realloc(
	Allocator *a,
	void *old,
	size_t oldsz,
	size_t newsz,
	const char *file,
	int line
) {
	AllocatorOP op = {};
	AllocatorRealloc data = {.old = old, .oldsz = oldsz, .newsz = newsz};
	op.opcode = ALLOC_REALLOC;
	op.data.realloc = data;
	return a->alloc_fn(a, op);
}

#else

static void *alloc_new(Allocator *a, size_t size) {
	AllocatorOP op = {0};
	op.opcode = ALLOC_ALLOC;
	AllocatorAlloc data = { .size = size };
	op.data.alloc = data;
	return a->alloc_fn(a, op);
}

static void alloc_free(Allocator *a, void *ptr) {
	AllocatorOP op = {0};
	op.opcode = ALLOC_FREE;
	AllocatorFree data = { .ptr = ptr };
	op.data.free = data;
	a->alloc_fn(a, op);
	return;
}

static void alloc_free_all(Allocator *a) {
	AllocatorOP op = {0};
	op.opcode = ALLOC_FREE_ALL;
	a->alloc_fn(a, op);
	return;
}

static void *alloc_realloc(Allocator *a, void *old, size_t oldsz, size_t newsz) {
	AllocatorOP op = {0};
	AllocatorRealloc data = {.old = old, .oldsz = oldsz, .newsz = newsz};
	op.opcode = ALLOC_REALLOC;
	op.data.realloc = data;
	return a->alloc_fn(a, op);
}

#endif //BLIB_DEBUG

#endif // ALLOCATOR_H
