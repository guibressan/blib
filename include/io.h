#ifndef IO_H
#define IO_H

typedef struct Reader {
	void *ctx;
	int64_t (*read_proc) (struct Reader *r, Slice *s); // slice is the output
} Reader;

typedef struct Writer {
	void *ctx;
	int64_t (*write_proc) (struct Writer *w, Slice *s); // slice is the input
} Writer;

static int64_t reader_read(Reader *r, Slice *s) {
	return r->read_proc(r, s);
}

static int64_t writer_write(Writer *w, Slice *s) {
	return w->write_proc(w, s);
}

#endif // IO_H
