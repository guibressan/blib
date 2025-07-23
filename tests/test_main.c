#include "../include/util.h"
#include "../include/arena.h"
#include "../include/testing.h"

/****************************************/
__attribute__((optnone))
static void test_arena(struct testing_t *t)
{
	struct arena a;
	void *ptr, *ptr2;
	void *base;
	usize cap;

	/* init without setting initial block size */
	ASSERT(!arena_init(&a, 0));
	ASSERT(!a.base);

	/* alloc some memory */
	ASSERT(ptr = arena_alloc(&a, (usize)1 << 20));
	blib_memset(ptr, 0xCC, (usize)1 << 20);
	ASSERT(a.len = ((usize)1 << 20) + sizeof(ptr));

	/* free all keep capacity but resets the length */
	cap = a.cap;
	arena_free_all(&a);
	ASSERT(a.cap = cap);
	ASSERT(a.len == 0);

	/* The arena can grow by allocating new blocks */
	ASSERT(ptr = arena_alloc(&a, (usize)5 << 20));
	/* Cap is the capacity of the last block */
	ASSERT(a.cap > cap);
	cap = a.cap;
	base = a.base;

	/* Free all will sum the high-water mark to set the capacity of the
	first block to be the sum of the capacity of all blocks, this way the
	allocator adjusts itself to avoid allocations in hot paths */
	arena_free_all(&a);
	ASSERT(a.cap > cap);
	ASSERT(a.base != base);
	base = a.base;
	cap = a.cap;
	ASSERT(ptr = arena_alloc(&a, (usize)5 << 20));
	ASSERT(a.cap == cap);
	ASSERT(a.base == base);

	/* deinit releases all the resources */
	arena_deinit(&a);
	ASSERT(!a.base);

	/* arena can be initialized with predefined min capacity */
	arena_init(&a, (usize)10 << 20);
	ASSERT(a.base);
	ASSERT(a.cap >= (usize)10 << 20);
	ASSERT(ptr = arena_alloc(&a, (usize)5 << 20));
	blib_memset(ptr, 0xCC, (usize)5 << 20);

	/* realloc can be used to copy the contents of a section and increase
	the length of the section */
	ASSERT(ptr2 = arena_realloc(&a, ptr, (usize)10 << 20));
	ASSERT(!blib_memcmp(ptr, ptr2, (usize)5 << 20));
	blib_memset(ptr2, 0xAA, (usize)10 << 20);
	/* Both pointers still valid, with different values */
	ASSERT(blib_memcmp(ptr, ptr2, (usize)5 << 20));

	arena_deinit(&a);
	ASSERT(!a.base);
}

/****************************************/
int main(void)
{
	struct arena a;
	struct test_runner tr;

	ASSERT(!arena_init(&a, 0));
	test_runner_init(&tr, &a);

  	test_runner_add(&tr, test_arena);

	test_runner_run(&tr);

	arena_deinit(&a);
	return 0;
}
