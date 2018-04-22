#include <sys/types.h>
#include <sys/systm.h>
#include <sys/param.h>
#include <sys/conf.h>
#include <sys/uio.h>
#include <sys/kernel.h>

#include "control.h"
#include "match.h"

#define BUF_SIZE 1024

static struct cdev *control_dev;

/* Forward declarations. */
static d_open_t control_open;
static d_close_t control_close;
static d_read_t control_read;
static d_write_t control_write;

static struct cdevsw control_cdevsw = {
    .d_version = D_VERSION,
    .d_open    = control_open,
    .d_close   = control_close,
    .d_read    = control_read,
    .d_write   = control_write,
    .d_name    = "kagrepcontrol"
};

typedef enum {
    MATCHER, FLAGS, KEYCC, KEYS, FILES, ERROR
} input_state;

typedef struct kagrep {
    struct thread *td;
    char in_buf[BUF_SIZE];
    size_t in_buf_offset, in_buf_size;
    input_state in_state;
    char *err_str; // statically allocated.
    struct grep_ctx *ctx;
    char *pattern;
    ssize_t pattern_length, pattern_contents;
} kagrep_t;

static void print_kagrep_state(char *id, struct kagrep *instance) {
#if 0
    uprintf("kagrep: %s\n", id);
    uprintf("in_buf: %ld, %ld\n", instance->in_buf_offset, instance->in_buf_size);
    uprintf("in_state: %d %s\n", instance->in_state, instance->err_str);
    uprintf("pattern: %ld, %ld\n", instance->pattern_length, instance->pattern_contents);
#endif
}

    static int
control_open(struct cdev *dev, int oflags, int devtype, struct thread *td)
{
    kagrep_t *instance = malloc(sizeof(kagrep_t), CONTROL_STATE, M_WAITOK|M_ZERO);

    instance->td = td;
    instance->ctx = match_create_ctx(td);
    dev->si_drv1 = instance;
    return (0);
}

    static int
control_close(struct cdev *dev, int fflag, int devtype, struct thread *td)
{
    kagrep_t *instance = dev->si_drv1;
    print_kagrep_state("control_close_enter", instance);

    free(instance->pattern, CONTROL_STATE);

    free(instance, CONTROL_STATE);
    return (0);
}

    static int
control_write(struct cdev *dev, struct uio *uio, int ioflag)
{
    int error = 0;
    kagrep_t *instance = dev->si_drv1;
    print_kagrep_state("control_write_enter", instance);

#define EXTRACT_STRING(var) do { \
            var = instance->in_buf + instance->in_buf_offset; \
            char *end = memchr(var, '\n', instance->in_buf_size); \
            if (end) { \
                *end = '\0'; \
                size_t length = end + 1 - var; \
                instance->in_buf_offset += length; \
                instance->in_buf_size -= length; \
            } else { \
                var = NULL; \
            }\
        } while (0)

    do {

        size_t size = MIN(uio->uio_resid, (BUF_SIZE - 1 - instance->in_buf_size));
        error = uiomove(instance->in_buf + instance->in_buf_size, size, uio);

        if (error) {
            return (error);
        }

        instance->in_buf_size += size;

        switch (instance->in_state) {
            case MATCHER:
                {
                    char *matcher;
                    EXTRACT_STRING(matcher);
                    if (matcher) {
                        if (match_set_matcher(instance->ctx, matcher)) {
                            // The matcher specified is invalid, bail out.
                            instance->in_state = ERROR;
                            instance->err_str = "invalid matcher specified";
                        }
                    }
                }
                if (instance->in_state != FLAGS) {
                    break;
                }
            case FLAGS:
                {
                    char *flags;
                    EXTRACT_STRING(flags);
                    if (flags) {
                        instance->in_state = KEYCC;
                        int err = false;
                        for (char *flag = flags; *flag; flag++) {
                            switch (*flag) {
                                case 'z':
                                    err |= match_set_opt(instance->ctx, NULL_BOUND, true);
                                    break;
                                case 'x':
                                    err |= match_set_opt(instance->ctx, MATCH_LINE, true);
                                    break;
                                case 'w':
                                    err |= match_set_opt(instance->ctx, MATCH_WORD, true);
                                    break;
                                default:
                                    err = true;
                                    break;
                            }
                        }
                        if (err) {
                            instance->in_state = ERROR;
                            instance->err_str = "invalid flag specified";
                        }
                    }
                }
                if (instance->in_state != KEYCC) {
                    break;
                }
            case KEYCC:
                {
                    char *count;
                    EXTRACT_STRING(count);
                    if (count) {
                        // Extract the long from the buffer.
                        instance->pattern_length = strtol(count, NULL, 10);

                        if (instance->pattern_length >= 0) {
                            instance->pattern = malloc(instance->pattern_length, CONTROL_STATE, M_WAITOK);
                            instance->in_state = KEYS;
                        } else {
                            instance->pattern_length = 0;
                            instance->in_state = ERROR;
                            instance->err_str = "invalid key length specified";
                        }
                    }
                }
                if (instance->in_state != KEYS) {
                    break;
                }
            case KEYS:
                if (instance->in_buf_size) {
                    size_t length = MIN(instance->in_buf_size, (instance->pattern_length - instance->pattern_contents));

                    memcpy(instance->pattern + instance->pattern_contents, instance->in_buf + instance->in_buf_offset, length);
                    instance->pattern_contents += length;

                    // check if the transfer is finished.
                    if (instance->pattern_length == instance->pattern_contents) {
                        // increase the length to consume the newline without copying.
                        length += 1; 
                        instance->in_state = FILES;

                        if (match_set_pattern(instance->ctx, instance->pattern, instance->pattern_length)) {
                            instance->in_state = ERROR;
                            instance->err_str = "unable to compile pattern";
                        }
                    }

                    instance->in_buf_offset += length;
                    instance->in_buf_size -= length;
                }

                if (instance->in_state != FILES) {
                    break;
                }
            case FILES:
                {
                    char *filename;
                    EXTRACT_STRING(filename);
                    if (filename) {
                        if (match_input(instance->ctx, filename)) {
                            instance->in_state = ERROR;
                            instance->err_str = "unable to access input file";
                        }
                    }
                }
                break;
            case ERROR:
                break;
        }

        if (instance->in_buf_offset + instance->in_buf_size == BUF_SIZE) {

            if (instance->in_buf_offset == 0) {
                // It's already compacted, meaning that the buffer is completely full and cant be emptied.
                instance->in_state = ERROR;
                instance->err_str = "line length too long";
            } else {
                // The input buffer is out of space, compact and try again.
                memmove(instance->in_buf, instance->in_buf + instance->in_buf_offset, instance->in_buf_size);
                instance->in_buf_offset = 0;
            }
        }
        
    } while (uio->uio_resid > 0 && instance->in_state != ERROR);

    if (instance->in_state == ERROR) {
        error = EIO;
    }

    print_kagrep_state("control_write_exit", instance);
    return (error);
}

    static int
control_read(struct cdev *dev, struct uio *uio, int ioflag)
{
    kagrep_t *instance = dev->si_drv1;
    print_kagrep_state("control_read_enter", instance);

    int error = 0;

    do {
        error = match_output(instance->ctx, uio);
    } while (uio->uio_resid > 0 && !error);

    print_kagrep_state("control_read_exit", instance);
    return (error);
}

int control_load()
{
    control_dev = make_dev(&control_cdevsw, 0, UID_ROOT, GID_WHEEL, 0666, "kagrep_control");
    return control_dev == 0;
}

void control_unload()
{
    if (control_dev) {
        destroy_dev(control_dev);
        control_dev = NULL;
    }
}

MALLOC_DEFINE(CONTROL_STATE, "kagrep_control_state", "state for the kagrep control system.");

