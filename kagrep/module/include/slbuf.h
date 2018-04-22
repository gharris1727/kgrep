
#include <sys/types.h>
#include <sys/uio.h>

#ifndef SLBUF_H
#define SLBUF_H

struct slbuf;

// Allocate a new slbuf that allocates in blocks of a set size.
struct slbuf *slbuf_create(size_t bufsize);

// Destroy a slbuf and release all of its memory.
void slbuf_destroy(struct slbuf *slbuf);

// Write into a slbuf from a kernel buffer.
// Prefers to write into the read buffer last used.
// returns EFAULT for errors, 1 for overruns suggesting a read before more writes.
int slbuf_write(struct slbuf *slbuf, const void *data, size_t len);

// Read from a slbuf into a uio vector.
// This reference is kept for subsequent slbuf_write calls for a zero-copy transfer.
// passing NULL clears this reference.
int slbuf_read(struct slbuf *slbuf, struct uio *out);

#endif
