#ifndef TESTING_H
#define TESTING_H

/****************************************/
#include "util.h"
#include "arena.h"

/******************************************************************************/
/*				Test Runner				      */
/******************************************************************************/
/*			A simple way to run tests			      */
/******************************************************************************/

/****************************************/
struct testing_t {
	struct arena *arena;
};

/****************************************/
typedef void (*test_func)(struct testing_t *t);

/****************************************/
struct test {
	const i8* tname;
	test_func tf;
};

/****************************************/
struct test_runner {
	struct test *tests;
	usize t_cap;
	usize t_len;
	struct arena *arena;
};

/****************************************/
static void test_runner_init(struct test_runner *tr, struct arena *a)
{
	blib_memset(tr, 0, sizeof(*tr));
	tr->arena = a;
}

/****************************************/
#define test_runner_add(TR, TN) _test_runner_add((TR), #TN, &(TN))

static void _test_runner_add(
	struct test_runner *tr, const i8* tname, test_func tf
)
{
	void *ptr;
	usize cap;

	if (tr->t_cap - tr->t_len == 0) {
		cap = MAX(tr->t_cap * 2, 100);
		if (!tr->tests)
			ptr = arena_alloc(tr->arena, cap * sizeof(*tr->tests));
		else
			ptr = arena_realloc(
				tr->arena, tr->tests, cap * sizeof(*tr->tests)
			);
		ASSERTM(ptr, "FAIL TO REALLOC tests array");
		tr->t_cap = cap;
		tr->tests = ptr;
	}
	blib_memset(tr->tests+tr->t_len, 0, sizeof(*tr->tests));
	(tr->tests+tr->t_len)->tname = tname;
	(tr->tests+tr->t_len)->tf = tf;
	tr->t_len++;
}

/****************************************/
static i32 test_runner_run(struct test_runner *tr)
{
	u32 i;
	struct arena a;
	struct timespec start, end;
	f32 elapsed;
	f32 elapsed_tot;
	struct testing_t t;

	blib_memset(&a, 0, sizeof(a));
	arena_init(&a, (usize)512 << 20);
	blib_memset(a.base, 0, a.cap);
	elapsed_tot = 0;

	printf("###### STARTING TESTS ######\n");

	for (i = 0; i < tr->t_len; i++) {
	blib_memset(&t, 0, sizeof(t));
		t.arena = &a;

		printf("START | %s\n", (tr->tests+i)->tname);
		clock_gettime(CLOCK_MONOTONIC, &start);
		(tr->tests+i)->tf(&t);
		clock_gettime(CLOCK_MONOTONIC, &end);
		elapsed = timespec_interval(start, end);
		elapsed_tot += elapsed;
		printf("\rPASS | %s	| %.3fms	| mem: %ld\n",
		       (tr->tests+i)->tname, elapsed, a.len);
		arena_free_all(&a);
	}

	printf("%ld tests executed in %.3fms\n", tr->t_len, elapsed_tot);
	printf("############################\n");

	arena_deinit(&a);
	return 0;
}

#endif /* TESTING_H */

