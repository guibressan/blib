#ifndef BYTES_H
#define BYTES_H

#include "io.h"

typedef struct Buffer {
	size_t off;
	Slice slice;
} Buffer;

static int buffer_init(Buffer *b, Allocator *backing, size_t element_size) {
	*b = (Buffer){ 0 };
	slice_init(&b->slice, backing, element_size);
	return 0;
}

static void buffer_destroy(Buffer *b) {
	slice_destroy(&b->slice);
	*b = (Buffer){0};
}

static int64_t buffer_read(Buffer *b, Slice *dest) {
	size_t i_len = slice_len(&b->slice);
	if (!i_len) return 0;
	if (i_len <= b->off) {
		slice_reset(&b->slice);
		b->off = 0;
		return 0;
	}
	size_t d_len = slice_len(dest);
	if (!d_len) return -1;
	void *first = 0;
	if (slice_get_ptr(&b->slice, b->off, (size_t *)&first)) return -2; 
	size_t nread = MIN(d_len, i_len-b->off);
	b->off += nread;
	slice_reset(dest);
	if (slice_append_multi(dest, first, nread)) return -3;
	return d_len;
}

static int64_t buffer_write(Buffer *b, Slice *src) {
	size_t i_len = slice_len(src);
	if (!i_len) return 0;
	void *first = 0;
	if (slice_get_ptr(src, 0, (size_t *)&first)) return -1;
	if (slice_append_multi(&b->slice, first, i_len)) return -2;
	return i_len;
}

static int64_t buffer_write_to(Buffer *b, Writer *w) {
	size_t slen = slice_len(&b->slice);
	size_t remaining = slen;
	int64_t n = 0;
	Slice reslice = {0};
	for (;remaining;) {
		slice_reslice(&b->slice, &reslice, slen-remaining, slen);
		n = writer_write(w, &reslice);
		remaining -= n;
		slice_destroy(&reslice);
	}
	return slen;
}

static int64_t buffer_read_from(Buffer *b, Reader *r) {
	int64_t n = 1, tot = 0;
	size_t maxread = 512;
	size_t len = slice_len(&b->slice);
	Slice reslice = {0};
	for (;n > 0;) {
		if (slice_grow_len_at(&b->slice, (len += maxread))) return -1;
		assert(!slice_reslice(&b->slice, &reslice, len-maxread, len));
		n = reader_read(r, &reslice);
		tot += n;
		if (n > 0 && n != maxread) 
			assert(!slice_set_len(&b->slice, (len -= (maxread - n))));
		if (n <= 0) 
			assert(!slice_set_len(&b->slice, (len -= maxread)));
		slice_destroy(&reslice);
	}
	return n == 0 ? tot : -2;
}

static int buffer_get_slice(Buffer *b, Slice *s) {
	*s = b->slice;
	return 0;
}

static int64_t buffer_reader_read(Reader *r, Slice *dest) {
	Buffer *b = (Buffer *)r->ctx;
	return buffer_read(b, dest);
}

static int64_t buffer_writer_write(Writer *r, Slice *src) {
	Buffer *b = (Buffer *)r->ctx;
	return buffer_write(b, src);
}

static Reader *buffer_as_reader(Buffer *b, Reader *reader) {
	*reader = (Reader){ .ctx = b, .read_proc = &buffer_reader_read };
	return reader;
}

static Writer *buffer_as_writer(Buffer *b, Writer *writer) {
	*writer = (Writer){ .ctx = b, .write_proc = &buffer_writer_write };
	return writer;
}

static int bytes_eq(unsigned char *a, unsigned char *b, size_t sz) {
	if (!a && !b) return 1;
	if (!a && b) return 0;
	if (a && !b) return 0;
	for (size_t i = 0; i < sz; i++) {
		if (*(a++) != *(b++)) return 0;
	}
	return 1;
}

static int bytes_is(unsigned char *a, unsigned char cmp, size_t sz) {
	for (size_t i = 0; i < sz; i++) {
		if (*(a++) != cmp) return 0;
	}
	return 1;
}

#endif
