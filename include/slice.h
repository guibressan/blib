#ifndef SLICE_H
#define SLICE_H

#include "allocator.h"

typedef struct { 
	char *base; 
	size_t isz;
	size_t len; 
	size_t cap; 
	Allocator *a; 
	int is_reslice;
} Slice ; 

static void slice_init(Slice *s, Allocator *a, size_t item_size) {
	*s = (Slice){0};
	s->isz = item_size;
	s->a = a;
}

static void slice_destroy(Slice *s) { 
	if (s->is_reslice) return; // reslices do not own memory
	if (s->base) alloc_free(s->a, s->base); 
	*s = (Slice){0};
}

static int slice_set_len(Slice *s, size_t len) {
	if (len < 0) return -1;
	if (len > s->len) return -2;
	s->len = len;
	return 0;
}

static int slice_reslice(
	Slice *s, Slice *reslice, size_t startlen, size_t endlen
) {
	if (startlen > s->len) return -1;
	if (endlen > s->len) return -2;
	*reslice = *s;
	reslice->is_reslice = 1;
	reslice->base += startlen;
	reslice->cap -= startlen;
	reslice->len -= startlen;
	return 0;
}

static int slice_append_multi(Slice *s, void *value, size_t count) {
	if (!s->a || !s->isz) return -1; 
	if (s->cap - s->len >= count) {
		memcpy(s->base+(s->len*s->isz), value, count*s->isz);
		s->len += count;
		return 0;
	} 
	size_t new_cap = MAX(s->cap*2, count); 
	char *p = 0;
	if (!s->base) p = alloc_new(s->a, s->isz*new_cap); 
	else if (!s->is_reslice) p = alloc_realloc( 
		s->a, (void *)s->base, s->isz*s->cap, s->isz*new_cap 
	); else {
		p = alloc_new(s->a, s->isz*new_cap);
	}
	if (!p) return 1; 
	s->is_reslice = 0;
	s->base = p; 
	s->cap = new_cap; 
	memcpy(s->base+(s->len*s->isz), value, count*s->isz);
	s->len += count;
	return 0; 
}
 
static int slice_append(Slice *s, void *value) { 
	return slice_append_multi(s, value, 1);
} 

static int slice_get(Slice *s, size_t index, void *dest) { 
	if (s->len-1 < index) return 1;  
	memcpy(dest, s->base+(index*s->isz), s->isz);
	return 0; 
} 

static int slice_find_ptr(
	Slice *s, void *ctx, int (*match)(void *ctx, void *item), size_t *dest
) { 
	for (size_t i = 0; i < s->len; i++) {
		if (!match(ctx, (void *)(s->base+(i*s->isz)))) continue;
		*dest = (size_t)s->base+(i*s->isz);
		return 1;
	}
	return 0; 
} 

static int slice_get_ptr(Slice *s, size_t index, size_t *dest) { 
	if (s->len-1 < index) return 1;  
	*dest = (size_t)s->base+(index*s->isz);
	return 0; 
} 

static int slice_set(Slice *s, size_t index, void *value) { 
	if (s->len-1 < index) return 1;  
	memcpy(s->base+(index*s->isz), value, s->isz);
	return 0; 
} 

static int slice_set_multi(Slice *s, size_t index, void *value, size_t count) { 
	if (s->len < index+count) return 1;  
	memcpy(s->base+(index*s->isz), value, s->isz*count);
	return 0; 
} 

static int slice_grow_cap_at(Slice *s, size_t cap) {
	if (s->cap >= cap) return 0;
	size_t new_cap = MAX(cap, s->cap*2);
	void *p = 0;
	if (s->is_reslice || !s->base) p = alloc_new(s->a, new_cap); 
	else p = alloc_realloc(s->a, s->base, s->cap, new_cap);
	if (!p) return -1;
	s->is_reslice = 0;
	s->base = p;
	s->cap = new_cap;
	return 0;
}

static int slice_grow_len_at(Slice *s, size_t len) {
	if (s->len >= len) return 0;
	if (s->cap >= len) s->len = len;
	if (slice_grow_cap_at(s, len)) return -1;
	s->len = len;
	return 0;
}

static int slice_uremove(Slice *s, size_t index) { 
	if (s->len-1 < index) return 1;  
	memcpy(s->base+(index*s->isz), s->base+((s->len-1)*s->isz), s->isz);
	s->len--; 
	return 0; 
} 

static int slice_oremove(Slice *s, size_t index) { 
	if (s->len-1 < index) return 1;  
	for (size_t i = index; i < s->len-1; i++) { 
		memcpy(s->base+(i*s->isz), s->base+((i+1)*s->isz), s->isz);
	} 
	s->len--; 
	return 0; 
} 

static int slice_peek(Slice *s, void *dest) {
	if (!s->len) {
		memset(dest, 0, s->isz);
		return -1;
	}
	memcpy(dest, s->base+((s->len-1)*s->isz), s->isz);
	return 0;
}

static int slice_pop(Slice *s, void *dest) {
	if (!s->len) {
		memset(dest, 0, s->isz);
		return -1;
	}
	memcpy(dest, s->base+((s->len-1)*s->isz), s->isz);
	s->len--;
	return 0;
}

static size_t slice_len(Slice *s) { 
	return s->base ? s->len: 0; 
} 

static size_t slice_cap(Slice *s) { 
	return s->base ? s->cap : 0; 
} 

static void slice_reset(Slice *s) { 
	if (s->base) s->len = 0; 
} 

#endif // SLICE_H

