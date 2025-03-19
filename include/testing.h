#ifndef TESTING_H
#define TESTING_H

int testing_byteslice_eq(char *value, size_t sz, char cmp) {
	for (size_t i = 0; i < sz; i++) {
		if (value[i] != cmp) {
			return 0;
		}
	}
	return 1;
}

#endif // TESTING_H
