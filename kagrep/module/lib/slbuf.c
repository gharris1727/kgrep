
#include <sys/types.h>
#include <sys/systm.h>
#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/kernel.h>

#include "slbuf.h"

MALLOC_DECLARE(SLBUF_BUFFER);

#define FIRST_OFF(slbuf) (slbuf->data_offset)
#define FIRST_SZ(slbuf) (MIN(slbuf->buf_size - FIRST_OFF(slbuf), slbuf->data_size))
#define FIRST_GAP_OFF(slbuf) (FIRST_OFF(slbuf) + FIRST_SZ(slbuf))
#define FIRST_GAP_SZ(slbuf) (slbuf->buf_size - FIRST_GAP_OFF(slbuf))

#define SECOND_OFF(slbuf) (0)
#define SECOND_SZ(slbuf) (slbuf->data_size - FIRST_SZ(slbuf))
#define SECOND_GAP_OFF(slbuf) (SECOND_OFF(slbuf) + SECOND_SZ(slbuf))
#define SECOND_GAP_SZ(slbuf) (slbuf->data_offset - SECOND_GAP_OFF(slbuf))

struct slbuf {
    struct uio *out;
    char *buf;
    size_t buf_size, data_offset, data_size;
};

static int slbuf_resize(struct slbuf *slbuf, size_t len);

struct slbuf *slbuf_create(size_t bufsize) {
    struct slbuf *slbuf = malloc(sizeof(struct slbuf), SLBUF_BUFFER, M_ZERO|M_WAITOK);

    slbuf_resize(slbuf, bufsize);

    return slbuf;
}

void slbuf_destroy(struct slbuf *slbuf) {
    free(slbuf->buf, SLBUF_BUFFER);

    free(slbuf, SLBUF_BUFFER);
}

int slbuf_full(struct slbuf *slbuf) {
    return !slbuf->out || slbuf->out->uio_resid == 0;
}

static int slbuf_copyout(struct slbuf *slbuf, void *data, size_t len) {
    size_t first = MIN(len, FIRST_SZ(slbuf));
    size_t second = MIN(len - first, SECOND_SZ(slbuf));
    if (first + second != len) {
        return 1;
    }
    if (first > 0) {
        memcpy(data, slbuf->buf + FIRST_OFF(slbuf), first);
    }
    if (second > 0) {
        memcpy(((char *)data) + first, slbuf->buf + SECOND_OFF(slbuf), second);
    }
    slbuf->data_size -= len;
    slbuf->data_offset += len;
    slbuf->data_offset %= slbuf->buf_size;
    return 0;
}

static int slbuf_copyin(struct slbuf *slbuf, const void *data, size_t len) {
    size_t first = MIN(len, FIRST_GAP_SZ(slbuf));
    size_t second = MIN(len - first, SECOND_GAP_SZ(slbuf));
    if (first + second != len) {
        return 1;
    }
    if (first > 0) {
        memcpy(slbuf->buf + FIRST_GAP_OFF(slbuf), data, first);
    }
    if (second > 0) {
        memcpy(slbuf->buf + SECOND_GAP_OFF(slbuf), ((const char *)data) + first, second);
    }
    slbuf->data_size += len;
    return 0;
}

static int slbuf_resize(struct slbuf *slbuf, size_t len) {
    if (len != slbuf->buf_size) {
        char *new = malloc(len, SLBUF_BUFFER, M_WAITOK);
        size_t data_size = slbuf->data_size;
        if (data_size > 0 && slbuf_copyout(slbuf, new, data_size)) {
            return 1;
        }
        free(slbuf->buf, SLBUF_BUFFER);
        slbuf->buf = new;
        slbuf->buf_size = len;
        slbuf->data_offset = 0;
        slbuf->data_size = data_size;
    }
    return 0;
}


int slbuf_write(struct slbuf *slbuf, const void *data, size_t len) {
    int rc = 0;
    size_t direct = slbuf->out ? MIN(slbuf->out->uio_resid, len) : 0;
    size_t slack = len - direct;
    ssize_t overrun = MAX(slack + slbuf->data_size - slbuf->buf_size, 0);
    if (slack > 0 && overrun > 0) {
        rc = slbuf_resize(slbuf, MAX(slbuf->buf_size * 2, overrun + slbuf->buf_size));
    }
    if (!rc && slbuf->out && direct > 0) {
        rc = uiomove((void*) (size_t) data, direct, slbuf->out);
    }
    if (!rc && slack > 0) {
        rc = slbuf_copyin(slbuf, (((const char*)data) + direct), slack);
    }
    if (!rc && slack > 0) {
        rc = 1;
    }
    return rc;
}

int slbuf_read(struct slbuf *slbuf, struct uio *out) {
    slbuf->out = out;
    int rc = 0;
    if (slbuf->out) {
        if (slbuf->out->uio_resid > 0 && slbuf->data_size > 0) {
            size_t first = MIN(slbuf->out->uio_resid, FIRST_SZ(slbuf));
            size_t second = MIN(slbuf->out->uio_resid - first, SECOND_SZ(slbuf));
            if (first > 0) {
                rc = uiomove(slbuf->buf + FIRST_OFF(slbuf), first, slbuf->out);
            }
            if (!rc && second > 0) {
                rc = uiomove(slbuf->buf + SECOND_OFF(slbuf), second, slbuf->out);
            }
            if (!rc) {
                slbuf->data_size -= (first + second);
                slbuf->data_offset += (first + second);
                slbuf->data_offset %= slbuf->buf_size;
            }
        } 
    }
    return rc;
}

MALLOC_DEFINE(SLBUF_BUFFER, "slbuf", "Slack Buffers");
