#define DEBUG

#include "testing.h"
#include "arena_allocator.h"
#include "heap_allocator.h"
#include "malloc_allocator.h"
#include "testing.h"
#include "bytes.h"
#include "slice.h"

static void test_arena(testing_t *t) {
	Allocator arena = {0};
	testing_expect(t, !arena_init(&arena, t->heap));
	size_t sz = ((size_t)10)<<20;
	// the first allocation determines the size of the first arena memory block
	char *p = alloc_new(&arena, sz);
	testing_expect(t, p);
	memset(p, 0, sz);
	testing_expect(t, bytes_is((void *)p, 0, sz));
	// subsequent allocations will create new blocks if remaining block space is
	// insufficient
	char *p2 = alloc_new(&arena, sz);
	memset(p2, 1, sz);
	testing_expect(t, p2);
	testing_expect(t, bytes_is((void *)p2, 1, sz));
	// arena realloc will create a new allocation and copy the bytes from the
	// old allocation
	char *p3 = alloc_realloc(&arena, p2, sz, sz*2);
	testing_expect(t, p3);
	memset(p3+sz, 2, sz);
	testing_expect(t, bytes_is((void *)p3, 1, sz));
	testing_expect(t, bytes_is((void *)p3+sz, 2, sz));
	// arena free all will free all blocks, with the exception of the first
	// block, which will have the length set to 0, allowing deterministic reusage
	// without syscalls
	alloc_free_all(&arena);
	char *p4 = alloc_new(&arena, sz);
	testing_expect(t, p4 == p);
	// note, we are not initializing the memory, since is known that this is the
	// first arena memory block that was initialized before
	testing_expect(t, bytes_is((void *)p4, 0, sz));
	// arena destroy will free all the arena resources, including the first
	// memory block
	arena_destroy(&arena);
}

static void test_heap_allocator(testing_t *t) {
	Allocator ha = {0};
	// for now, heap allocator is a wrapper over the backing allocator, in this
	// case, malloc, the differece is that when 'DEBUG' is defined the program
	// will crash when you double free or attempt to realloc with a unknown 
	// pointer.
	// 
	// also, when 'DEBUG' is enabled, you can get reports about the allocations,
	// turning possible to track memory leaks
	testing_expect(t, !heap_allocator_init(&ha, t->heap));
	int *ptr = alloc_new(&ha, sizeof(int));
	testing_expect(t, ptr);
	int *ptr2 = alloc_new(&ha, sizeof(int));
	testing_expect(t, ptr2);
	int *ptr3 = alloc_new(&ha, sizeof(int));
	testing_expect(t, ptr3);
	alloc_free(&ha, ptr2);
	// oops, forgot to free some of the pointers
	HeapAllocatorReport r = {0};
	testing_expect(t, !heap_allocator_get_report(&ha, &r));
	testing_expect(t, r.alloc_bytes == 12);
	testing_expect(t, r.n_allocs == 3);
	testing_expect(t, r.leak_bytes == 8);
	testing_expect(t, r.n_leaks == 2);
	alloc_free(&ha, ptr);
	alloc_free(&ha, ptr3);
	// uncomment to print the heap allocator report
	// heap_allocator_report_print(&r);
	// uncomment the double free and the program will crash with a useful message
	//alloc_free(&ha, ptr2);
	heap_allocator_destroy(&ha);
}

static int char_cmp(void *ctx, void *item) {
	if (*((char *)ctx) == *((char *)item)) {
		return 1;
	}
	return 0;
}

static void test_slice(testing_t *t) {
	Allocator a = {0};
	testing_expect(t, !heap_allocator_init(&a, t->heap));
	Slice slice = {0};
	slice_init(&slice, &a, sizeof(char));
	for (int i = 0; i < 4; i++) {
		testing_expect(t, !slice_append(&slice, "1"));
	}
	testing_expect(t, slice_len(&slice) == 4);
	testing_expect(t, slice_cap(&slice) == 4);
	slice_reset(&slice);
	testing_expect(t, slice_len(&slice) == 0);
	testing_expect(t, slice_cap(&slice) == 4);
	char r = 0;
	testing_expect(t, !slice_append(&slice, "1"));
	testing_expect(t, !slice_set(&slice, 0, "2"));
	testing_expect(t, !slice_get(&slice, 0, &r));
	testing_expect(t, r == '2');
	slice_reset(&slice);
	char *str = "1234";
	testing_expect(t, !slice_append_multi(&slice, str, 4));
	testing_expect(t, !slice_uremove(&slice, 0));
	testing_expect(t, !slice_get(&slice, 0, &r));
	testing_expect(t, r == '4');
	testing_expect(t, !slice_oremove(&slice, 0));
	testing_expect(t, !slice_get(&slice, 0, &r));
	testing_expect(t, r == '2');
	size_t ptr = 0;
	testing_expect(t, !slice_get_ptr(&slice, 0, &ptr));
	testing_expect(t, bytes_eq((void *)ptr, (void *)"23", 2));
	char cmp = '2';
	size_t addr = 0;
	testing_expect(t, slice_find_ptr(&slice, &cmp, &char_cmp, &addr));
	testing_expect(t, cmp == *((char *)addr));
	// in this case, destroy is not necessary, as arena allocator is being used
	slice_destroy(&slice); 
	HeapAllocatorReport report= {0};
	testing_expect(t, !heap_allocator_get_report(&a, &report));
	testing_expect(t, report.n_leaks == 0);
	heap_allocator_destroy(&a);
}

void test_leak_detection(testing_t *t) {
	void *a, *b, *c, *d, *e;
	testing_expect(t, (a = alloc_new(t->arena, 1)));
	testing_expect(t, (b = alloc_new(t->arena, 2)));
	testing_expect(t, (c = alloc_new(t->arena, 4)));
	testing_expect(t, (d = alloc_new(t->heap, 4)));
	testing_expect(t, (e = alloc_new(t->heap, 4)));
	// comment the lines below to see a helpful message about the memleak
	alloc_free(t->heap, d);
	alloc_free(t->heap, e);
}

int main(void) {
	TestRunner tr = {0};
	testing_init(&tr);
	//
	testing_add(&tr, &test_arena);
	testing_add(&tr, &test_heap_allocator);
	testing_add(&tr, &test_slice);
	testing_add(&tr, &test_leak_detection);
	//
	testing_run(&tr);
	return 0;
}
