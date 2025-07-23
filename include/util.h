#ifndef UTIL_H
#define UTIL_H

/****************************************/
#include "primitives.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

/******************************************************************************/
/*				Base alloc				      */
/******************************************************************************/
/****************************************/
static void *blib_malloc(usize size)
{
	return malloc(size);
}

/****************************************/
static void *blib_realloc(void *old, usize new_size)
{
	return realloc(old, new_size);
}

/****************************************/
static void blib_free(void *ptr)
{
	free(ptr);
}

/******************************************************************************/
/*				Utils					      */
/******************************************************************************/
/****************************************/
#define MAX(a, b) ((a) > (b) ? (a) : (b))

/****************************************/
#define MIN(a, b) ((a) < (b) ? (a) : (b))

/****************************************/
#define ASSERT(cond) assert((cond) != 0, __FILE__, __LINE__)
static void assert(u8 cond, const i8 *file, i32 line)
{
	if (cond) return ;
	fprintf(stderr, "assertion failed at %s:%d\n", file, line);
	exit(1);
}

/****************************************/
#define ASSERTM(cond, msg) assertm((cond) != 0, (msg), __FILE__, __LINE__)
static void assertm(u8 cond, const char *msg, const i8 *file, i32 line)
{
	if (cond) return ;
	fprintf(stderr, "%s:%d %s\n", file, line, msg);
	exit(1);
}

/****************************************/
static f32 timespec_interval(struct timespec start,
			     struct timespec stop)
{
	return (f32)((stop.tv_sec - start.tv_sec) * 1000) +
	       (f32)(stop.tv_nsec - start.tv_nsec) / (f32)1000000;
}

/****************************************/
static void blib_memset(void *dest, i8 fill, usize count)
{
	memset(dest, fill, count);
}

/****************************************/
static void blib_memcpy(void *dest, void *src, usize count)
{
	memcpy(dest, src, count);
}

/****************************************/
static i32 blib_memcmp(void *a, void *b, usize count)
{
	return memcmp(a, b, count);
}

/****************************************/
static usize align_size(usize size, usize alignment)
{
	if (size == 0)
		return 0;
	return alignment * ((size / alignment) + (size % alignment ? 1 : 0));
}

#endif /* UTIL_H */
