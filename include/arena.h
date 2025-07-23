#ifndef ARENA2_H
#define ARENA2_H

/****************************************/
#include "util.h"

/******************************************************************************/
/*				Arena					      */
/******************************************************************************/

/****************************************/
struct arena {
	usize cap;
	usize len;
	u8 *base;
};

struct arena_block_header {
	usize cap;
	struct arena_block_header *parent;
};

#ifndef ARENA_MIN_ALLOC_SIZE
#define ARENA_MIN_ALLOC_SIZE ((usize)16 << (usize)10)
#endif

/****************************************/
static i32 arena_init(struct arena *a, usize opt_cap)
{
	usize alloc_size;
	struct arena_block_header *bhead;

	blib_memset(a, 0, sizeof(*a));

	if (opt_cap == 0)
		return 0;
	alloc_size = align_size(
		MAX(ARENA_MIN_ALLOC_SIZE, opt_cap + sizeof(*bhead)),
		ARENA_MIN_ALLOC_SIZE
	);
	bhead = blib_malloc(alloc_size);
	if (!bhead)
		return -1;
	blib_memset(bhead, 0, sizeof(*bhead));
	bhead->cap = alloc_size - sizeof(*bhead);
	a->cap = bhead->cap;
	a->base = (u8 *)(bhead + 1);
	return 0;
}

/****************************************/
static i32 _arena_grow(struct arena *a, usize needed)
{
	struct arena_block_header *bhead, *parent;
	usize size;

	if (!a->base)
		parent = NULL;
	else
		parent = ((struct arena_block_header *)a->base)-1;
	size = align_size(
		MAX(ARENA_MIN_ALLOC_SIZE, needed + sizeof(*bhead)),
		ARENA_MIN_ALLOC_SIZE
	);
	bhead = blib_malloc(size);
	if (!bhead)
		return -1;
	blib_memset(bhead, 0, sizeof(*bhead));
	bhead->parent = parent;
	bhead->cap = size - sizeof(*bhead);
	a->base = ((u8 *)bhead) + sizeof(*bhead);
	a->cap = bhead->cap;
	a->len = 0;
	return 0;
}

/****************************************/
static void *arena_alloc(struct arena *a, usize size)
{
	i32 err;
	usize needed;
	u8 *ptr;

	needed = size + sizeof(size);

	if (!a->base || a->cap - a->len < size) {
		err = _arena_grow(a, needed);
		if (err)
			return NULL;
	}
	ptr = a->base + a->len;
	*((usize *)ptr) = size;
	ptr += sizeof(size);
	a->len += needed;
	return (void *)ptr;
}

/****************************************/
static void *arena_realloc(struct arena *a, void *old, usize new_size)
{
	usize old_size;
	u8 *ptr;

	old_size = *(((usize *)old)-1);

	if (old_size >= new_size)
		return old;
	ptr = arena_alloc(a, new_size);
	if (!ptr)
		return NULL;
	blib_memcpy(ptr, old, old_size);
	return (void *)ptr;
}

/****************************************/
static void arena_free_all(struct arena *a)
{
	usize high_water;
	struct arena_block_header *bhead;

	high_water = 0;

	if (!a->base)
		return;

	if (!(bhead = ((struct arena_block_header *)a->base)-1)->parent) {
		a->len = 0;
		return;
	}

	for (;bhead->parent;) {
		a->base = ((u8 *)(bhead->parent+1));
		high_water += bhead->cap;
		blib_free(bhead);
		bhead = ((struct arena_block_header *)a->base)-1;
	}
	
	high_water += bhead->cap;
	blib_free(bhead);
	ASSERT(!arena_init(a, high_water));
}

/****************************************/
static void arena_deinit(struct arena *a)
{
	arena_free_all(a);
	if (a->base)
		blib_free(((struct arena_block_header *)a->base)-1);
	blib_memset(a, 0, sizeof(*a));
}

#endif /* ARENA2_H */
