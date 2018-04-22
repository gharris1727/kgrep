
#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/kernel.h>

#include "slbuf.h"

MALLOC_DECLARE(SLBUF_BUFFER);

struct slbuf {
    struct uio *out, *drain, *fill;
    size_t bufsize;
    size_t drain_offset, drain_last_size;
    int drain_index, drain_last_index;
    struct uio a;
    struct uio b;
};

struct slbuf *slbuf_create(size_t bufsize) {
    struct slbuf *slbuf = malloc(sizeof(struct slbuf), SLBUF_BUFFER, M_ZERO|M_WAITOK);
    slbuf->drain = &slbuf->a;
    slbuf->fill = &slbuf->b;

    slbuf->drain->uio_rw = UIO_WRITE;
    slbuf->fill->uio_rw = UIO_READ;
    slbuf->drain_index = -1;

    return slbuf;
}

void slbuf_destroy(struct slbuf *slbuf) {
    for (int i = 0; i < slbuf->a.uio_iovcnt; i++) {
        free(slbuf->a.uio_iov[i].iov_base, SLBUF_BUFFER);
    }
    free(slbuf->a.uio_iov, SLBUF_BUFFER);

    for (int i = 0; i < slbuf->b.uio_iovcnt; i++) {
        free(slbuf->b.uio_iov[i].iov_base, SLBUF_BUFFER);
    }
    free(slbuf->b.uio_iov, SLBUF_BUFFER);

    free(slbuf, SLBUF_BUFFER);
}

int slbuf_write(struct slbuf *slbuf, const void *data, size_t len) {
    int rc = 0;
    ssize_t direct = slbuf->out ? MIN(slbuf->out->uio_resid, len) : 0;
    ssize_t slack = len - direct;
    ssize_t overrun = slack - slbuf->fill->uio_resid;
    if (slack > 0 && overrun > 0) {
        struct uio *fill = slbuf->fill;
        // Round up to the next bufsize for the next block to allocate.
        ssize_t buf_len = slbuf->bufsize * ((overrun / slbuf->bufsize) + (overrun % slbuf->bufsize > 0));
        void *buf_ptr = malloc(buf_len, SLBUF_BUFFER, M_WAITOK);
        // Add a new entry to the scatter/gather list.
        fill->uio_iovcnt++;
        fill->uio_iov = realloc(slbuf->fill->uio_iov, slbuf->fill->uio_iovcnt * sizeof(struct iovec), SLBUF_BUFFER, M_WAITOK);
        fill->uio_iov[slbuf->fill->uio_iovcnt-1].iov_base = buf_ptr;
        fill->uio_iov[slbuf->fill->uio_iovcnt-1].iov_len = buf_len;
        fill->uio_resid += buf_len;
    }
    if (!rc && slbuf->out && direct > 0) {
        rc = uiomove((void*) (intptr_t) data, direct, slbuf->out);
    }
    if (!rc && slack > 0) {
        rc = uiomove((((char*)(intptr_t)data) + direct), slack, slbuf->fill);
    }
    if (!rc && slack > 0) {
        rc = 1;
    }
    return rc;
}

static int slbuf_drain(struct slbuf *slbuf) {
    struct iovec *drain_vec = slbuf->drain->uio_iov;
    while (slbuf->out->uio_resid > 0 && slbuf->drain_index != -1) {
        // Move a maximal sized block of data to the output buf.
        void *src = ((char*) drain_vec[slbuf->drain_index].iov_base) + slbuf->drain_offset;
        ssize_t amt =
            (slbuf->drain_index == slbuf->drain_last_index 
                ?  slbuf->drain_last_size 
                : drain_vec[slbuf->drain_index].iov_len) 
            - slbuf->drain_offset;
        amt = MIN(amt, slbuf->out->uio_resid);
        if (amt > 0) {
            int rc = uiomove(src, amt, slbuf->out);
            if (rc) {
                return rc;
            }
            slbuf->drain_offset += amt;
        }

        // advance to the next iovec entry if this one is finished.
        if (slbuf->drain_offset == drain_vec[slbuf->drain_index].iov_len) {
            slbuf->drain_offset = 0;
            slbuf->drain_index++;
        }
        if (slbuf->drain_index > slbuf->drain_last_index ||
                (slbuf->drain_index == slbuf->drain_last_index &&
                 slbuf->drain_offset == slbuf->drain_last_size)) {
            // We emptied this buffer completely, so reset it.
            slbuf->drain_index = -1;
            slbuf->drain_last_index = 0;
            slbuf->drain_offset = slbuf->drain_last_size = 0;
            slbuf->drain->uio_resid = 0;
        }
    } 
    return 0;
}

int slbuf_read(struct slbuf *slbuf, struct uio *out) {
    slbuf->out = out;
    int rc = 0;
    if (slbuf->out) {
        if ((rc = slbuf_drain(slbuf))) {
            return rc;
        }

        // Check if that drain exhausted the previous buffer
        if (slbuf->drain_index == -1) {

            // Swap to the other buffer
            struct uio *tmp = slbuf->drain;
            slbuf->drain = slbuf->fill;
            slbuf->fill = tmp;
            slbuf->drain->uio_rw = UIO_WRITE;
            slbuf->fill->uio_rw = UIO_READ;

            // Compute the size of this buffer overall.
            size_t total = 0;
            for (int i = 0; i < slbuf->drain->uio_iovcnt; i++) {
                total += slbuf->drain->uio_iov[i].iov_len;
            }

            // Compute the index and offset for the last buffer element.
            size_t size = total - slbuf->drain->uio_resid;
            struct iovec *drain_vec = slbuf->drain->uio_iov;
            while (size > drain_vec[slbuf->drain_last_index].iov_len) {
                size -= drain_vec[slbuf->drain_last_index].iov_len;
                slbuf->drain_last_index++;
            }
            slbuf->drain_last_size = size;

            // Try and drain this buffer of data.
            if ((rc = slbuf_drain(slbuf))) {
                return rc;
            }
        }
    }
    return rc;
}

MALLOC_DEFINE(SLBUF_BUFFER, "slbuf", "Slack Buffers");
