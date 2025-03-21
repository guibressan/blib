#ifndef BYTES_H
#define BYTES_H

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
