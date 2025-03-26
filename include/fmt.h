#ifndef FMT_H
#define FMT_H

#include <stdarg.h>
#include <stdio.h>

#include "allocator.h"

static int fmt_vasprintf(
	Allocator *a, char **ret, const char *fmt, va_list args
) {
	int64_t count = vsnprintf(0, 0, fmt, args);
	if (count < 1) return (int)count;
	void *p = alloc_new(a, count+1);
	if (!p) return 0;
	count = vsnprintf(p, (size_t)count+1, fmt, args);
	*ret = p;
	return (int)count;
}

static int fmt_asprintf(Allocator *a, char **ret, const char *fmt, ...) {
	int result = 0;
	va_list args;
	va_start(args, fmt);
	result = fmt_vasprintf(a, ret, fmt, args);
	va_end(args);
	return result;
}

#endif
