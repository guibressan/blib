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

void test_heap() {

}

//DEFINE_SLICE(char)

#define TYPE char
#include "slice.h"
#undef TYPE

void test_slice() {
	Allocator a = {0};
	assert(!arena_init(&a));
	slice_char slice = {0};
	slice_char_init(&slice, &a);
	for (int i = 0; i < 4; i++) {
		assert(!slice_char_append(&slice, '1'));
	}
	assert(slice_char_len(&slice) == 4);
	assert(slice_char_cap(&slice) == 4);
	slice_char_reset(&slice);
	assert(slice_char_len(&slice) == 0);
	assert(slice_char_cap(&slice) == 4);
	char r = 0;
	assert(!slice_char_append(&slice, '1'));
	assert(!slice_char_set(&slice, 0, '2'));
	assert(!slice_char_get(&slice, 0, &r));
	assert(r == '2');
	slice_char_reset(&slice);
	for (int i = 0; i < 4; i++) {
		assert(!slice_char_append(&slice, '1'+i));
	}
	assert(!slice_char_uremove(&slice, 0));
	assert(!slice_char_get(&slice, 0, &r));
	assert(r == '4');
	assert(!slice_char_oremove(&slice, 0));
	assert(!slice_char_get(&slice, 0, &r));
	assert(r == '2');
	slice_char_destroy(&slice);
	arena_destroy(&a);
}

int main(void) {
	test_arena();
	test_heap();
	test_slice();
}
