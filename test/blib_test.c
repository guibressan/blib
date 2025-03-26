#include "testing.h"
#include "arena_allocator.h"
#include "heap_allocator.h"
#include "malloc_allocator.h"
#include "testing.h"
#include "bytes.h"
#include "slice.h"
#include "error.h"
#include "io.h"
#include "fmt.h"

static void test_arena(testing_t *t) {
	Allocator arena = {0};
	testing_expect(t, !arena_init(&arena, t->heap));
	size_t sz = ((size_t)10)<<20;
	// the first allocation determines the size of the first arena memory block
	char *p = alloc_new(&arena, sz);
	testing_expect(t, p);
	memset(p, 0, sz);
	testing_expect(t, bytes_is((void *)p, 0, sz));
	// subsequent allocations will create new blocks if remaining block space is
	// insufficient
	char *p2 = alloc_new(&arena, sz);
	memset(p2, 1, sz);
	testing_expect(t, p2);
	testing_expect(t, bytes_is((void *)p2, 1, sz));
	// arena realloc will create a new allocation and copy the bytes from the
	// old allocation
	char *p3 = alloc_realloc(&arena, p2, sz, sz*2);
	testing_expect(t, p3);
	memset(p3+sz, 2, sz);
	testing_expect(t, bytes_is((void *)p3, 1, sz));
	testing_expect(t, bytes_is((void *)p3+sz, 2, sz));
	// arena free all will free all blocks, with the exception of the first
	// block, which will have the length set to 0, allowing deterministic reusage
	// without syscalls
	alloc_free_all(&arena);
	char *p4 = alloc_new(&arena, sz);
	testing_expect(t, p4 == p);
	// note, we are not initializing the memory, since is known that this is the
	// first arena memory block that was initialized before
	testing_expect(t, bytes_is((void *)p4, 0, sz));
	// arena destroy will free all the arena resources, including the first
	// memory block
	arena_destroy(&arena);
}

static void test_heap_allocator(testing_t *t) {
	Allocator ha = {0};
	// for now, heap allocator is a wrapper over the backing allocator, in this
	// case, malloc, the differece is that when 'BLIB_DEBUG' is defined the program
	// will crash when you double free or attempt to realloc with a unknown 
	// pointer.
	// 
	// also, when 'BLIB_DEBUG' is enabled, you can get reports about the allocations,
	// turning possible to track memory leaks
	testing_expect(t, !heap_allocator_init(&ha, t->heap));
	int *ptr = alloc_new(&ha, sizeof(int));
	testing_expect(t, ptr);
	int *ptr2 = alloc_new(&ha, sizeof(int));
	testing_expect(t, ptr2);
	int *ptr3 = alloc_new(&ha, sizeof(int));
	testing_expect(t, ptr3);
	alloc_free(&ha, ptr2);
	// oops, forgot to free some of the pointers
	// heap allocator just tracks allocations when 'BLIB_DEBUG' is defined
#ifdef BLIB_DEBUG
	HeapAllocatorReport r = {0};
	testing_expect(t, !heap_allocator_get_report(&ha, &r));
	testing_expect(t, r.alloc_bytes == 12);
	testing_expect(t, r.n_allocs == 3);
	testing_expect(t, r.leak_bytes == 8);
	testing_expect(t, r.n_leaks == 2);
#endif
	alloc_free(&ha, ptr);
	alloc_free(&ha, ptr3);
	// uncomment to print the heap allocator report
	// heap_allocator_report_print(&r);
	// uncomment the double free below and the program will crash with a useful
	// message if 'BLIB_DEBUG' is defined
	// alloc_free(&ha, ptr2);
	heap_allocator_destroy(&ha);
}

static int char_cmp(void *ctx, void *item) {
	if (*((char *)ctx) == *((char *)item)) {
		return 1;
	}
	return 0;
}

static void test_slice(testing_t *t) {
	Allocator a = {0};
	testing_expect(t, !heap_allocator_init(&a, t->heap));
	Slice slice = {0};
	slice_init(&slice, &a, sizeof(char));
	for (int i = 0; i < 4; i++) {
		testing_expect(t, !slice_append(&slice, "1"));
	}
	testing_expect(t, slice_len(&slice) == 4);
	testing_expect(t, slice_cap(&slice) == 4);
	slice_reset(&slice);
	testing_expect(t, slice_len(&slice) == 0);
	testing_expect(t, slice_cap(&slice) == 4);
	char r = 0;
	testing_expect(t, !slice_append(&slice, "1"));
	testing_expect(t, !slice_set(&slice, 0, "2"));
	testing_expect(t, !slice_get(&slice, 0, &r));
	testing_expect(t, r == '2');
	slice_reset(&slice);
	char *str = "1234";
	testing_expect(t, !slice_append_multi(&slice, str, 4));
	testing_expect(t, !slice_uremove(&slice, 0));
	testing_expect(t, !slice_get(&slice, 0, &r));
	testing_expect(t, r == '4');
	testing_expect(t, !slice_oremove(&slice, 0));
	testing_expect(t, !slice_get(&slice, 0, &r));
	testing_expect(t, r == '2');
	size_t ptr = 0;
	testing_expect(t, !slice_get_ptr(&slice, 0, &ptr));
	testing_expect(t, bytes_eq((void *)ptr, (void *)"23", 2));
	char cmp = '2';
	size_t addr = 0;
	testing_expect(t, slice_find_ptr(&slice, &cmp, &char_cmp, &addr));
	testing_expect(t, cmp == *((char *)addr));
	slice_grow_len_at(&slice, 4);
	testing_expect(t, !slice_set_multi(&slice, 0, (void *)"FFFF", 4));
	testing_expect(t, !slice_get_ptr(&slice, 0, &ptr));
	testing_expect(t, bytes_eq((void *)"FFFF", (void *)ptr, 4));
	testing_expect(t, !slice_set_len(&slice, 0));
	testing_expect(t, 0 == slice_len(&slice));
	size_t cap = slice_cap(&slice);
	testing_expect(t, !slice_grow_cap_at(&slice, cap+1));
	testing_expect(t, cap*2 == slice_cap(&slice));
	Slice reslice = {0};
	testing_expect(t, !slice_reslice(&slice, &reslice, 0, 0));
	// note that there's no leak
	// the reslice does not own the underlying bytes until it needs to grow
	for (size_t i = 0; i < slice_cap(&reslice); i++) {
		testing_expect(t, !slice_append(&reslice, (void *)"F"));
	}
	Slice reslice2 = {0};
	testing_expect(t, !slice_reslice(&slice, &reslice2, 0, 0));
	// in this case, we must destroy the reslice, since we are forcing it to grow
	for (size_t i = slice_cap(&reslice2)+1; i > 0 ; i--) {
		testing_expect(t, !slice_append(&reslice2, (void *)"F"));
	}
	// uncomment the line below and the test will fail due to the memory leak 
	slice_destroy(&reslice2);
	slice_destroy(&slice); 
#ifdef BLIB_DEBUG
	HeapAllocatorReport report= {0};
	testing_expect(t, !heap_allocator_get_report(&a, &report));
	testing_expect(t, report.n_leaks == 0);
#endif
	heap_allocator_destroy(&a);
}

void test_leak_detection(testing_t *t) {
	void *a, *b, *c, *d, *e;
	testing_expect(t, (a = alloc_new(t->arena, 1)));
	testing_expect(t, (b = alloc_new(t->arena, 2)));
	testing_expect(t, (c = alloc_new(t->arena, 4)));
	testing_expect(t, (d = alloc_new(t->heap, 4)));
	testing_expect(t, (e = alloc_new(t->heap, 4)));
	// comment the lines below to see a helpful message about the memleak
	alloc_free(t->heap, d);
	alloc_free(t->heap, e);
}

void test_errors(testing_t *t) {
	Errors err = {0};
	errors_init(&err, t->heap);
	const char *msg = "const char error";
	size_t count = snprintf(0, 0, "non const char error");
	char *msg2 = 0;
	testing_expect(t, (msg2 = alloc_new(t->arena, count+1)));
	snprintf(msg2, count+1, "non const char error");
	// append const messages to errors just saves the string pointer
	testing_expect(t, !errors_append_const(&err, msg));
	// append make a copy of the string
	testing_expect(t, !errors_append(&err, msg2, count));
	*(msg2+count-2) = '\0';
	testing_expect(t, !errors_append(&err, msg2, count));
	testing_expect(t, errors_len(&err) == 3);
	// check if a specific error happened
	testing_expect(t, errors_has(&err, msg));
	// it does not work with non const strings, since we can not compare the 
	// address
	//
	// but nothing prevents you to use const strings for matching and non-const
	// strings for context
	testing_expect(t, !errors_has(&err, msg2));
	// print the errors
	//errors_print(&err);
	// reset for reuse, the copies of non const strings will be freed
	errors_reset(&err);
	// testing_expect(t, errors_len(&err) == 0);
	//errors_print(&err);
	errors_destroy(&err);
}

void test_buffer(testing_t *t) {
	const char *str = "AAAAAAAAAABBBBBBBBBB";
	size_t slen = strlen(str);
	Slice s = {0}, s1 = {0};
	slice_init(&s, t->heap, sizeof(char));
	slice_init(&s1, t->heap, sizeof(char));
	Buffer buf = {0};
	testing_expect(t, !buffer_init(&buf, t->heap, sizeof(char)));
	Writer w = {0};
	Reader r = {0};
	slice_append_multi(&s, (void *)str, slen);
	// writing to the buffer will grow it as needed
	testing_expect(t, slen == writer_write(buffer_as_writer(&buf, &w), &s));
	void *p = 0;
	slice_append_multi(&s1, (void *)"0000000000", slen/2);
	slice_get_ptr(&s1, 0, (size_t *)&p);
	testing_expect(t, slen/2 == reader_read(buffer_as_reader(&buf, &r), &s1))
	testing_expect(t, bytes_eq((void *)str, p, slen/2));
	// reader can do partial reads, but it does not grow the output slice, this
	// way you can use the reader in a loop without doing memory allocations
	testing_expect(t, slen/2 == reader_read(buffer_as_reader(&buf, &r), &s1))
	testing_expect(t, bytes_eq((void *)(str+slen/2), p, slen/2));
	// when there's no bytes remaining in the buffer, reader_read returns 0
	testing_expect(t, 0 == reader_read(buffer_as_reader(&buf, &r), &s1));
	slice_destroy(&s);
	slice_destroy(&s1);
	buffer_destroy(&buf);
}

void test_buffer_write_to_read_from(testing_t *t) {
	Writer w = {0};
	Reader r = {0};
	size_t plen = ((size_t)10) << 20;
	Slice payload = {0};
	slice_init(&payload, t->heap, sizeof(char));
	slice_grow_len_at(&payload, plen);
	void *p = 0;
	slice_get_ptr(&payload, 0, (size_t *)&p);
	memset(p, 'F', plen);
	Buffer buf = {0};
	buffer_init(&buf, t->heap, sizeof(char));
	testing_expect(t, plen == buffer_write(&buf, &payload));
	Buffer buf2 = {0};
	buffer_init(&buf2, t->heap, sizeof(char));
	// you can write all the buffer bytes to any writer
	testing_expect(t, plen == buffer_write_to(&buf, buffer_as_writer(&buf2, &w)));
	slice_get_ptr(&buf2.slice, 0, (size_t *)&p);
	for (size_t i = 0; i < plen; i++) testing_expect(t, 'F' == *((char*)(p+i)));
	Buffer buf3 = {0};
	buffer_init(&buf3, t->heap, sizeof(char));
	int64_t n = 0;
	// you can read from any reader until EOF
	n = buffer_read_from(&buf3, buffer_as_reader(&buf2, &r));
	testing_expect(t, plen == n);
	Slice tmp = {0};
	buffer_get_slice(&buf3, &tmp);
	testing_expect(t, plen == slice_len(&tmp));
	slice_get_ptr(&tmp, 0, (size_t *)&p);
	for (size_t i = 0; i < plen; i++) testing_expect(t, 'F' == *((char*)(p+i)));
	buffer_destroy(&buf3);
	buffer_destroy(&buf2);
	buffer_destroy(&buf);
	slice_destroy(&payload);
}

void test_fmt_asprintf(testing_t *t) {
	char *str = 0;
	testing_expect(
		t,
		fmt_asprintf(
			t->heap, &str, "This will be allocated in the %s\n", "heap_allocator"
		)
	);
	// uncomment to print the message
	//printf("%s", str);
	alloc_free(t->heap, str);
}

int main(void) {
	TestRunner tr = {0};
	testing_init(&tr);
	//
	testing_add(&tr, test_arena);
	testing_add(&tr, test_heap_allocator);
	testing_add(&tr, test_slice);
	testing_add(&tr, test_leak_detection);
	testing_add(&tr, test_errors);
	testing_add(&tr, test_buffer);
	testing_add(&tr, test_buffer_write_to_read_from);
	testing_add(&tr, test_fmt_asprintf);
	//
	testing_run(&tr);
	return 0;
}
