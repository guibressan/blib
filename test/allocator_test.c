#include <unistd.h> // sleep
#include <assert.h> // assert

#include "arena_allocator.h"
#include "testing.h"

void test_arena() {
	Allocator arena;
	// arena init initializes the allocator with the arena function pointer
	assert(!arena_init(&arena));
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
	// block, which will have the length set to 0, allowing reusage of the block
	alloc_free_all(&arena);
	p = alloc_new(&arena, sz);
	assert(p);
	// note, we are not initializing the memory, but is known that this is the
	// first arena memory block that was initialized before
	assert(testing_byteslice_eq(p, sz, 0));
	// arena destroy will free all the arena resources, including the first
	// memory block
	arena_destroy(&arena);
}

int main(void) {
	test_arena();
}
