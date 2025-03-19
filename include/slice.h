#ifndef SLICE_H
#define SLICE_H

#include "allocator.h"
#include "heap_allocator.h"

#define DEFINE_SLICE(TYPE) \
typedef struct { \
	TYPE *base; \
	size_t len; \
	size_t cap; \
	Allocator *a; \
} slice_##TYPE; \
\
void slice_##TYPE##_init(slice_##TYPE *s, Allocator *a) { \
	slice_##TYPE s1 = {0}; \
	s1.a = a; \
	*s = s1; \
} \
\
int slice_##TYPE##_append(slice_##TYPE *s, TYPE value) { \
	if (!s->a) { \
		return 1; \
	} \
	if (s->cap > s->len) { \
		*(s->base+s->len) = value; \
		s->len++; \
		return 0; \
	}  \
	size_t new_cap = MAX(s->cap*2, 1); \
	TYPE *p = 0; \
	if (!s->cap) p = alloc_new(s->a, sizeof(TYPE)*new_cap); \
	else p = alloc_realloc( \
		s->a, (void *)s->base, sizeof(TYPE)*s->cap, sizeof(TYPE)*new_cap \
	); \
	if (!p) return 1; \
	s->base = p; \
	s->cap = new_cap; \
	*(s->base+s->len) = value; \
	s->len++; \
	return 0; \
} \
\
int slice_##TYPE##_get(slice_##TYPE *s, size_t index, TYPE *dest) { \
	if (s->len-1 < index) return 1;  \
	*dest = *(s->base+index); \
	return 0; \
} \
\
int slice_##TYPE##_set(slice_##TYPE *s, size_t index, TYPE value) { \
	if (s->len-1 < index) return 1;  \
	*(s->base+index) = value; \
	return 0; \
} \
\
int slice_##TYPE##_uremove(slice_##TYPE *s, size_t index) { \
	if (s->len-1 < index) return 1;  \
	*(s->base+index) = *(s->base+s->len-1); \
	s->len--; \
	return 0; \
} \
\
int slice_##TYPE##_oremove(slice_##TYPE *s, size_t index) { \
	if (s->len-1 < index) return 1;  \
	for (size_t i = index; i < s->len-1; i++) { \
		*(s->base+i) = *(s->base+i+1); \
	} \
	s->len--; \
	return 0; \
} \
\
size_t slice_##TYPE##_len(slice_##TYPE *s) { \
	return s->base ? s->len: 0; \
} \
\
size_t slice_##TYPE##_cap(slice_##TYPE *s) { \
	return s->base ? s->cap : 0; \
} \
\
void slice_##TYPE##_reset(slice_##TYPE *s) { \
	if (s->base) s->len = 0; \
} \
\
void slice_##TYPE##_destroy(slice_##TYPE *s) { \
	if (s->base) alloc_free(s->a, s->base); \
} \

#endif // SLICE_H
