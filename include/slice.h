#ifndef SLICE_H
#define SLICE_H

#include "allocator.h"

typedef struct { 
	char *base; 
	size_t isz;
	size_t len; 
	size_t cap; 
	Allocator *a; 
} Slice ; 

static void slice_init(Slice *s, Allocator *a, size_t item_size) {
	*s = (Slice){0};
	s->isz = item_size;
	s->a = a;
}
 
static int slice_append(Slice *s, void *value) { 
	if (!s->a || !s->isz) { 
		return 1; 
	} 
	if (s->cap > s->len) { 
		memcpy(s->base+(s->len*s->isz), value, s->isz);
		s->len++; 
		return 0; 
	}  
	size_t new_cap = MAX(s->cap*2, 1); 
	char *p = 0; 
	if (!s->cap) p = alloc_new(s->a, s->isz*new_cap); 
	else p = alloc_realloc( 
		s->a, (void *)s->base, s->isz*s->cap, s->isz*new_cap 
	); 
	if (!p) return 1; 
	s->base = p; 
	s->cap = new_cap; 
	memcpy(s->base+(s->len*s->isz), value, s->isz);
	s->len++; 
	return 0; 
} 

static int slice_get(Slice *s, size_t index, void *dest) { 
	if (s->len-1 < index) return 1;  
	memcpy(dest, s->base+(index*s->isz), s->isz);
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

static size_t slice_len(Slice *s) { 
	return s->base ? s->len: 0; 
} 

static size_t slice_cap(Slice *s) { 
	return s->base ? s->cap : 0; 
} 

static void slice_reset(Slice *s) { 
	if (s->base) s->len = 0; 
} 

static void slice_destroy(Slice *s) { 
	if (s->base) alloc_free(s->a, s->base); 
	*s = (Slice){0};
}

// #ifndef CAT
// #define CAT(a,b) a ## b
// #endif
// 
// #ifndef CAT3
// #define CAT3(a,b,c) a ## b ## c
// #endif
// 
// #ifndef CATX
// #define CATX(a, b) CAT(a, b)
// #endif
// 
// #ifndef CATX3
// #define CATX3(a, b, c) CAT3(a, b, c)
// #endif
// 
// #ifndef TYPE
// #error you must define TYPE before including this header
// #endif
// 
// typedef struct { 
// 	TYPE *base; 
// 	size_t len; 
// 	size_t cap; 
// 	Allocator *a; 
// } CATX(slice_,TYPE); 
// 
// static void CATX3(slice_,TYPE,_init)(CATX(slice_,TYPE) *s, Allocator *a) {
// 	CATX(slice_,TYPE) s1 = {0};
// 	s1.a = a; 
// 	*s = s1; 
// }
//  
// static int CATX3(slice_,TYPE,_append)(CATX(slice_,TYPE) *s, TYPE value) { 
// 	if (!s->a) { 
// 		return 1; 
// 	} 
// 	if (s->cap > s->len) { 
// 		*(s->base+s->len) = value; 
// 		s->len++; 
// 		return 0; 
// 	}  
// 	size_t new_cap = MAX(s->cap*2, 1); 
// 	TYPE *p = 0; 
// 	if (!s->cap) p = alloc_new(s->a, sizeof(TYPE)*new_cap); 
// 	else p = alloc_realloc( 
// 		s->a, (void *)s->base, sizeof(TYPE)*s->cap, sizeof(TYPE)*new_cap 
// 	); 
// 	if (!p) return 1; 
// 	s->base = p; 
// 	s->cap = new_cap; 
// 	*(s->base+s->len) = value; 
// 	s->len++; 
// 	return 0; 
// } 
// 
// static int CATX3(slice_,TYPE,_get)(CATX(slice_,TYPE) *s, size_t index, TYPE *dest) { 
// 	if (s->len-1 < index) return 1;  
// 	*dest = *(s->base+index); 
// 	return 0; 
// } 
// 
// static int CATX3(slice_,TYPE,_set)(CATX(slice_,TYPE) *s, size_t index, TYPE value) { 
// 	if (s->len-1 < index) return 1;  
// 	*(s->base+index) = value; 
// 	return 0; 
// } 
// 
// static int CATX3(slice_,TYPE,_uremove)(CATX(slice_,TYPE) *s, size_t index) { 
// 	if (s->len-1 < index) return 1;  
// 	*(s->base+index) = *(s->base+s->len-1); 
// 	s->len--; 
// 	return 0; 
// } 
// 
// static int CATX3(slice_,TYPE,_oremove)(CATX(slice_,TYPE) *s, size_t index) { 
// 	if (s->len-1 < index) return 1;  
// 	for (size_t i = index; i < s->len-1; i++) { 
// 		*(s->base+i) = *(s->base+i+1); 
// 	} 
// 	s->len--; 
// 	return 0; 
// } 
// 
// static size_t CATX3(slice_,TYPE,_len)(CATX(slice_,TYPE) *s) { 
// 	return s->base ? s->len: 0; 
// } 
// 
// static size_t CATX3(slice_,TYPE,_cap)(CATX(slice_,TYPE) *s) { 
// 	return s->base ? s->cap : 0; 
// } 
// 
// static void CATX3(slice_,TYPE,_reset)(CATX(slice_,TYPE) *s) { 
// 	if (s->base) s->len = 0; 
// } 
// 
// static void CATX3(slice_,TYPE,_destroy)(CATX(slice_,TYPE) *s) { 
// 	if (s->base) alloc_free(s->a, s->base); 
// }

#endif // SLICE_H

