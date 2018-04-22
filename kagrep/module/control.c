#include <sys/types.h>
#include <sys/systm.h>
#include <sys/param.h>
#include <sys/conf.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/fcntl.h>
#include <sys/syscallsubr.h>

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
    int matcher;
    bool null_bound, line_match, word_match;
    char *pattern;
    size_t pattern_length, pattern_contents;
    size_t n_files;
    int *files;
    char out_buf[BUF_SIZE];
    size_t out_buf_offset, out_buf_size;
} kagrep_t;

static void print_kagrep_state(char *id, struct kagrep *instance) {
#if 0
    uprintf("kagrep: %s\n", id);
    uprintf("in_buf: %ld, %ld\n", instance->in_buf_offset, instance->in_buf_size);
    uprintf("in_state: %d %s\n", instance->in_state, instance->err_str);
    uprintf("matcher: %d z/%d x/%d w/%d\n", instance->matcher, instance->null_bound, instance->line_match, instance->word_match);
    uprintf("pattern: %ld, %ld\n", instance->pattern_length, instance->pattern_contents);
    uprintf("files: %ld\n", instance->n_files);
    uprintf("out_buf: %ld, %ld\n", instance->out_buf_offset, instance->out_buf_size);
#endif
}

    static int 
    __attribute__((unused)) 
open_file(struct thread *td, char *path, int flags) 
{
    return kern_openat(td, AT_FDCWD, path, UIO_SYSSPACE, flags, 0644) ? EIO : td->td_retval[0];
}

    static int
    __attribute__((unused)) 
close_file(struct thread *td, int fd) 
{
    if (fd != -1 && td->td_proc->p_fd) {
        return kern_close(td, fd);
    }
    return 0;
}

    static int
control_open(struct cdev *dev, int oflags, int devtype, struct thread *td)
{
    kagrep_t *instance = malloc(sizeof(kagrep_t), CONTROL_STATE, M_WAITOK|M_ZERO);

    instance->td = td;
    dev->si_drv1 = instance;
    return (0);
}

    static int
control_close(struct cdev *dev, int fflag, int devtype, struct thread *td)
{
    kagrep_t *instance = dev->si_drv1;
    print_kagrep_state("control_close_enter", instance);

    free(instance->pattern, CONTROL_STATE);

    for (int i = 0; i < instance->n_files; i++) {
        close_file(instance->td, instance->files[i]);
    }
    free(instance->files, CONTROL_STATE);

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
                        // If the following loop doesn't find a matcher, fail.
                        instance->in_state = ERROR;
                        instance->err_str = "invalid matcher specified";

                        for (int i = 0; i < sizeof(matchers) / sizeof(*matchers); i++) {
                            if (!strcmp(matcher, matchers[i].name)) {
                                instance->matcher = i;
                                instance->in_state = FLAGS;
                                instance->err_str = NULL;
                            }
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
                        for (char *flag = flags; *flag; flag++) {
                            switch (*flag) {
                                case 'z':
                                    instance->null_bound = true;
                                    break;
                                case 'x':
                                    instance->line_match = true;
                                    break;
                                case 'w':
                                    instance->word_match = true;
                                    break;
                                default:
                                    instance->in_state = ERROR;
                                    instance->err_str = "invalid flag specified";
                                    break;
                            }
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

                        if (instance->pattern_length > 0) {
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
                        int fd = open_file(instance->td, filename, O_RDONLY);
                        if (fd >= 0) {
                            instance->n_files++;
                            instance->files = realloc(instance->files, sizeof(int)*instance->n_files, CONTROL_STATE, M_WAITOK);
                            instance->files[instance->n_files-1] = fd;
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

