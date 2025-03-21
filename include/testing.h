#ifndef TESTING_H
#define TESTING_H

#include "allocator.h"
#include "malloc_allocator.h"
#include "heap_allocator.h"
#include "arena_allocator.h"
#include "slice.h"
#include "assert.h"

typedef struct {
	Allocator *heap;
	Allocator *arena;
	int failed;
	char *message;
} testing_t ;

typedef void (*TestProcedure)(testing_t *t);

typedef struct {
	Allocator malloc;
	Allocator heap;
	Allocator arena;
	Slice test_procedures;
} TestRunner;

static void testing_init(TestRunner *tr) {
	*tr = (TestRunner){0};
	//
	assert(
		!malloc_allocator_init(&tr->malloc) &&
		!heap_allocator_init(&tr->heap, &tr->malloc) &&
		!arena_init(&tr->arena, &tr->malloc)
	);
	slice_init(&tr->test_procedures, &tr->heap, sizeof(TestProcedure));
}

static void testing_add(TestRunner *tr, TestProcedure t) {
	assert(!slice_append(&tr->test_procedures, &t));
}

#define testing_expect(t, b) _testing_expect((t), (b) != 0, __FILE__, __LINE__)

static void _testing_expect(
	testing_t *t, int test, const char *file, int line
) {
	if (test) return;
	size_t len = 0;
	char *ptr = 0;
	//
	len = snprintf(0, 0, "%s:%d assertion failed", file, line);
	assert((ptr = alloc_new(t->arena, len+1)));
	snprintf(ptr, len+1, "%s:%d assertion failed", file, line);
	t->message = ptr;
}

static void testing_run(TestRunner *tr) {
	testing_t t = {0};
	TestProcedure proc = {0};
	Allocator heap = {0};
	HeapAllocatorReport report = {0};
	//
	for (size_t i = 0; i < slice_len(&tr->test_procedures); i++) {
		t = (testing_t){0};
		report = (HeapAllocatorReport){0};
		assert(!heap_allocator_init(&heap, &tr->arena));
		t = (testing_t){.heap = &heap, .arena = &tr->arena};
		slice_get(&tr->test_procedures, i, &proc);
		// run the test
		proc(&t);
		//
		if (t.message) {
			printf("test %lu FAIL: %s\n", i+1, t.message);
			goto finalizer;
		} 
		if (t.failed) {
			printf("test %lu FAIL\n", i+1);
			goto finalizer;
		} 
		if (heap_allocator_get_report(&heap, &report)) {
			printf("test %lu PASS\n", i+1);
			goto finalizer;
		}
		if (report.n_leaks) {
			printf("test %lu PASS but has memory leaks\n", i+1);
			heap_allocator_report_print(&report);
			goto finalizer;
		}
		printf("test %lu PASS\n", i+1);
finalizer:
		alloc_free_all(&tr->arena);
	}
}

#endif // TESTING_H
