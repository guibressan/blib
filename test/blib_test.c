#include <unistd.h> // sleep
#include <assert.h> // assert

#include "arena_allocator.h"
#include "heap_allocator.h"
#include "malloc_allocator.h"
#include "testing.h"
#include "bytes.h"
#include "slice.h"

void test_arena() {
	Allocator backing = {0};
	assert(!malloc_allocator_init(&backing));
	Allocator arena = {0};
	// arena init initializes the allocator with the arena function pointer
	assert(!arena_init(&arena, &backing));
	size_t sz = ((size_t)10)<<20;
	// the first allocation determines the size of the first arena memory block
	char *p = alloc_new(&arena, sz);
	assert(p);
	memset(p, 0, sz);
	assert(testing_byteslice_eq(p, sz, 0));
	// subsequent allocations will create new blocks if remaining block space is
	// insufficient
	char *p2 = alloc_new(&arena, sz);
	memset(p2, 1, sz);
	assert(p2);
	assert(testing_byteslice_eq(p2, sz, 1));
	// arena realloc will create a new allocation and copy the bytes from the
	// old allocation
	char *p3 = alloc_realloc(&arena, p2, sz, sz*2);
	assert(p3);
	memset(p3+sz, 2, sz);
	assert(testing_byteslice_eq(p3, sz, 1));
	assert(testing_byteslice_eq(p3+sz, sz, 2));
	// arena free all will free all blocks, with the exception of the first
	// block, which will have the length set to 0, allowing deterministic reusage
	// without syscalls
	alloc_free_all(&arena);
	char *p4 = alloc_new(&arena, sz);
	assert(p4 == p);
	// note, we are not initializing the memory, since is known that this is the
	// first arena memory block that was initialized before
	assert(testing_byteslice_eq(p4, sz, 0));
	// arena destroy will free all the arena resources, including the first
	// memory block
	arena_destroy(&arena);
}

void test_heap_allocator() {
	Allocator backing = {0};
	assert(!malloc_allocator_init(&backing));
	Allocator ha = {0};
	assert(!heap_allocator_init(&ha, &backing));
	int *ptr = alloc_new(&ha, sizeof(int));
	assert(ptr);
	int *ptr2 = alloc_new(&ha, sizeof(int));
	assert(ptr2);
	int *ptr3 = alloc_new(&ha, sizeof(int));
	assert(ptr3);
	alloc_free(&ha, ptr2);
	// oops, forgot to free some of the pointers
	HeapAllocatorReport r = {0};
	assert(!heap_allocator_get_report(&ha, &r));
	assert(r.alloc_bytes == 12);
	assert(r.n_allocs == 3);
	assert(r.leak_bytes == 8);
	assert(r.n_leaks == 2);
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

void test_slice() {
	Allocator backing = {0};
	assert(!malloc_allocator_init(&backing));
	Allocator a = {0};
	assert(!arena_init(&a, &backing));
	Slice slice = {0};
	slice_init(&slice, &a, sizeof(char));
	for (int i = 0; i < 4; i++) {
		assert(!slice_append(&slice, "1"));
	}
	assert(slice_len(&slice) == 4);
	assert(slice_cap(&slice) == 4);
	slice_reset(&slice);
	assert(slice_len(&slice) == 0);
	assert(slice_cap(&slice) == 4);
	char r = 0;
	assert(!slice_append(&slice, "1"));
	assert(!slice_set(&slice, 0, "2"));
	assert(!slice_get(&slice, 0, &r));
	assert(r == '2');
	slice_reset(&slice);
	for (int i = 0; i < 4; i++) {
		char r = '1'+i;
		assert(!slice_append(&slice, &r));
	}
	assert(!slice_uremove(&slice, 0));
	assert(!slice_get(&slice, 0, &r));
	assert(r == '4');
	assert(!slice_oremove(&slice, 0));
	assert(!slice_get(&slice, 0, &r));
	assert(r == '2');
	size_t ptr = 0;
	assert(!slice_get_ptr(&slice, 0, &ptr));
	assert(bytes_eq((void *)ptr, (void *)"23", 2));
	char cmp = '2';
	size_t addr = 0;
	assert(slice_find_ptr(&slice, &cmp, &char_cmp, &addr));
	assert(cmp == *((char *)addr));
	// in this case, destroy is not necessary, as arena allocator is being used
	slice_destroy(&slice); 
	arena_destroy(&a);
}

int main(void) {
	test_arena();
	test_heap_allocator();
	test_slice();
}
