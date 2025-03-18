#include <unistd.h>
#include "../include/allocator.h"

int main(void) {
	Allocator arena;
	assert(!arena_init(&arena));
	size_t sz = ((size_t)10)<<20;
	char *p = alloc_new(&arena, sz);
	memset(p, 0, sz);
	char *p2 = alloc_new(&arena, sz);
	memset(p2, 0, sz);
	sleep(15);
	arena_destroy(&arena);
	sleep(60);
}
