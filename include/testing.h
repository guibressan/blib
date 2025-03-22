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
	const char *name;
	TestProcedure proc;
} Test;

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
	slice_init(&tr->test_procedures, &tr->heap, sizeof(Test));
}

#define testing_add(tr, t) _testing_add((tr), &(t), #t)

static void _testing_add(TestRunner *tr, TestProcedure t, const char *name) {
	Test test = {0};
	test.name = name;
	test.proc = t;
	assert(!slice_append(&tr->test_procedures, &test));
}

#define testing_expect(t, b) \
if (!_testing_expect((t), (b) != 0, __FILE__, __LINE__)) return;

static int _testing_expect(
	testing_t *t, int test, const char *file, int line
) {
	if (test) return 1;
	size_t len = 0;
	char *ptr = 0;
	//
	len = snprintf(0, 0, "%s:%d assertion failed", file, line);
	assert((ptr = alloc_new(t->arena, len+1)));
	snprintf(ptr, len+1, "%s:%d assertion failed", file, line);
	t->message = ptr;
	return 0;
}

static void testing_run(TestRunner *tr) {
	testing_t t = {0};
	Test test = {0};
	Allocator heap = {0};
#ifdef BLIB_DEBUG
	HeapAllocatorReport report = {0};
#endif
	//
	for (size_t i = 0; i < slice_len(&tr->test_procedures); i++) {
		t = (testing_t){0};
#ifdef BLIB_DEBUG
		report = (HeapAllocatorReport){0};
#endif
		assert(!heap_allocator_init(&heap, &tr->arena));
		t = (testing_t){.heap = &heap, .arena = &tr->arena};
		slice_get(&tr->test_procedures, i, &test);
		// run the test
		test.proc(&t);
		//
		if (t.message) {
			printf("%s FAIL: %s\n", test.name, t.message);
			goto finalizer;
		} 
		if (t.failed) {
			printf("%s FAIL\n", test.name);
			goto finalizer;
		} 
#ifdef BLIB_DEBUG
		assert(!heap_allocator_get_report(&heap, &report));
		if (report.n_leaks) {
			printf("%s PASS but has memory leaks\n", test.name);
			heap_allocator_report_print(&report);
			goto finalizer;
		}
#endif
		printf("%s PASS\n", test.name);
finalizer:
		alloc_free_all(&tr->arena);
	}
}

#endif // TESTING_H
