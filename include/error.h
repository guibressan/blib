#ifndef ERROR_H
#define ERROR_H

#include "arena_allocator.h"
#include "slice.h"

typedef struct {
	char *msg;
	size_t msgsz;
	const char *file;
	int line;
} Error;

typedef struct {
	Allocator arena;
	Slice errors;
} Errors;

static int errors_init(Errors *e, Allocator *backing) {
	Allocator arena = {0};
	if (arena_init(&arena, backing)) return -1;
	*e = (Errors){0};
	e->arena = arena;
	slice_init(&e->errors, &e->arena, sizeof(Error));
	return 0;
}

static void errors_destroy(Errors *e) {
	arena_destroy(&e->arena);
	*e = (Errors){0};
}

static void errors_reset(Errors *e) {
	{
		alloc_free_all(&e->arena);
		Allocator arena = e->arena;
		*e = (Errors){0};
		e->arena = arena;
	}
	slice_init(&e->errors, &e->arena, sizeof(Error));
}

#define errors_append_const(e, m)\
_errors_append((e), (char *)(m), strlen((m)), 1, __FILE__, __LINE__) 

#define errors_append(e, m, sz)\
_errors_append((e), (m), (sz), 0, __FILE__, __LINE__) 

static size_t errors_len(Errors *e) {
	return slice_len(&e->errors);
}  

static int errors_has(Errors *e, const char *msg) {
	Error err = {0};
	for (size_t i = 0; i < slice_len(&e->errors); i++) {
		slice_get(&e->errors, i, &err);
		// compare the pointer values
		if (err.msg == msg) {
			return 1;
		}
	}
	return 0;
}

static int _errors_append(
	Errors *e,
	char *msg,
	size_t msgsz,
	int is_const,
	const char *file,
	int line
) {
	Error err = {0};
	err.msg = msg;
	err.file = file;
	err.line = line;
	err.msgsz = msgsz;
	if (!is_const) {
		if (!(err.msg = alloc_new(&e->arena, msgsz))) return -1;
		memcpy(err.msg, msg, msgsz);
	}
	if (slice_append(&e->errors, &err)) return -1;
	return 0;
}

static void errors_print(Errors *e) {
	Error err = {0};
	printf("########## BEGIN ERRORS PRINT ##########\n");
	for (size_t i = 0; i < slice_len(&e->errors); i++) {
		slice_get(&e->errors, i, &err);
		printf("%s:%d %.*s\n", err.file, err.line, (int)err.msgsz, err.msg);
	}
	printf("########## END ERRORS PRINT ##########\n");
	fflush(stdout);
}

#endif
