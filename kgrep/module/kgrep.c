#include <sys/types.h>
#include <sys/module.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/uio.h>
#include <sys/filedesc.h>
#include <sys/file.h>
#include <sys/syscallsubr.h>
#include <sys/fcntl.h>
#include <sys/sysproto.h>

#include "sregex/src/sregex/sregex.h"

#define COMMAND_SIZE 512
#define BLOCK_SIZE 4096
#define POOL_SIZE 4096

#define DPRINT(...) //uprintf(__VA_ARGS__)

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
    .d_name    = "kgrepcontrol"
};

typedef struct kgrep {
    char command[COMMAND_SIZE], inbuf[BLOCK_SIZE], outbuf[BLOCK_SIZE];
    char *pattern, *in_filename, *limit;
    struct thread *td;
    int run_phase, command_length,  match_limit, in_fd, eof, match;
    sre_pool_t *program_pool, *context_pool;
    sre_program_t *program;
    sre_vm_pike_ctx_t *ctx;
    long block_offset, in_offset, in_length, out_offset, out_length, ncaps, ovecsize, ctx_offset, nmatches;
    long *ovector;
} kgrep_t;

static void print_kgrep_state(char *id, struct kgrep *instance) {
#if 0
    DPRINT("INSTANCE %p @ %s\n", instance, id);
    if (instance) {
        DPRINT("thread: %p\n", instance->td);
        if (instance->td) {
            DPRINT("\tprocess: %p\n", instance->td->td_proc);
            if (instance->td->td_proc) {
                DPRINT("\t\tfiletable: %p\n", instance->td->td_proc->p_fd);
            }
        }
    }
#endif
}

static struct cdev *control_dev;

    static int 
open_file(struct thread *td, char *path, int flags) 
{
    DPRINT("kern_openat(%p, %d, %s, %d, %d, %d)\n", td, AT_FDCWD, path, UIO_SYSSPACE, flags, 0644);

    int fd = kern_openat(td, AT_FDCWD, path, UIO_SYSSPACE, flags, 0644);

    DPRINT("%d <- kern_openat\n", fd);

    fd = td->td_retval[0];

    DPRINT("Opened file descriptor %d\n", fd);

    return fd;
}

static int
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
    kgrep_t *instance = malloc(sizeof(kgrep_t), M_TEMP, M_WAITOK | M_ZERO);

    if (!instance) {
        uprintf("Unable to allocate memory!\n");
        return (1);
    }

    instance->in_fd = -1;
    instance->td = td;
    dev->si_drv1 = instance;

    print_kgrep_state("control_open_exit", instance);

    DPRINT("Opened control channel.\n");
    return (0);
}

    static int
control_close(struct cdev *dev, int fflag, int devtype, struct thread *td)
{
    kgrep_t *instance = dev->si_drv1;

    print_kgrep_state("control_close_enter", instance);

    close_file(instance->td, instance->in_fd);

    if (instance->ovector) {
        free(instance->ovector, M_TEMP);
    }

    if (instance->program_pool) {
        sre_destroy_pool(instance->program_pool);
    }

    if (instance->context_pool) {
        sre_destroy_pool(instance->context_pool);
    }

    free(instance, M_TEMP);

    DPRINT("Closed control channel.\n");
    return (0);
}

    static char *
strtok_r (char *s, const char *delim, char **save_ptr)
{
    char *end;

    if (s == NULL)
        s = *save_ptr;

    if (*s == '\0')
    {
        *save_ptr = s;
        return NULL;
    }

    /* Scan leading delimiters.  */
    s += strspn (s, delim);
    if (*s == '\0')
    {
        *save_ptr = s;
        return NULL;
    }

    /* Find the end of the token.  */
    end = s + strcspn (s, delim);
    if (*end == '\0')
    {
        *save_ptr = end;
        return s;
    }

    /* Terminate the token and make *SAVE_PTR point past it.  */
    *end = '\0';
    *save_ptr = end + 1;
    return s;
}

    static int
control_write(struct cdev *dev, struct uio *uio, int ioflag)
{
    kgrep_t *instance = dev->si_drv1;

    print_kgrep_state("control_write_enter", instance);

    int error = 0;

    if (instance->run_phase == 0) {
        int delta = MIN(uio->uio_iov->iov_len, COMMAND_SIZE - instance->command_length - 1);
        error = uiomove(instance->command + instance->command_length, delta, uio);
        if (error != 0) {
            return (error);
        }
        instance->command_length += delta;
        *(instance->command + instance->command_length) = 0;

        int commas = 0;
        for (int i = 0; i < instance->command_length; i++) { 
            commas += (instance->command[i] == ',');
        }
        if (commas != 2) {
            return (error);
        }

        char *tok_state = NULL;

        instance->pattern = strtok_r(instance->command, ",", &tok_state);
        instance->in_filename = strtok_r(NULL, ",", &tok_state);
        instance->limit = strtok_r(NULL, ",", &tok_state);

        instance->match_limit = strtol(instance->limit, NULL, 10);

        print_kgrep_state("pre_open_file", instance);
        instance->in_fd = open_file(instance->td, instance->in_filename, O_RDONLY);
        print_kgrep_state("post_open_file", instance);

        instance->program_pool = sre_create_pool(POOL_SIZE);
        instance->context_pool = sre_create_pool(POOL_SIZE);

        if (instance->in_fd == -1) {
            error = EIO;
            return (error);
        }

        DPRINT("Searching for %s, Reading from %s, Maximum %d matches\n", 
                instance->pattern, instance->in_filename, instance->match_limit);

        long error_pos;

        DPRINT("sre_regex_parse(%p, %s, %p, %d, %p)\n", 
                instance->program_pool, (unsigned char *) instance->pattern, &(instance->ncaps), 0, &error_pos);
        sre_regex_t *regex = sre_regex_parse(
                instance->program_pool, (unsigned char *) instance->pattern, &(instance->ncaps), 0, &error_pos);
        DPRINT("%p <- sre_regex_parse\n", regex);
        if (!regex) {
            uprintf("Error at %ld in regex string\n", error_pos);
            error = EIO;
            return (error);
        }

        DPRINT("sre_regex_compile(%p, %p)\n", instance->program_pool, regex);
        instance->program = sre_regex_compile(instance->program_pool, regex);
        DPRINT("%p <- sre_regex_compile\n", instance->program);
        if (!instance->program) {
            error = EIO;
            return (error);
        }

        long ovecsize = 2 * (instance->ncaps + 1) * sizeof(sre_int_t);
        instance->ovector = malloc(ovecsize, M_TEMP, M_WAITOK | M_ZERO);

        instance->run_phase = 1;
    }

    print_kgrep_state("control_write_exit", instance);

    return (error);
}

    static int
control_read(struct cdev *dev, struct uio *uio, int ioflag)
{
    kgrep_t *instance = dev->si_drv1;
    print_kgrep_state("control_read_enter", instance);

    int error = 0;

    if (instance->run_phase == 1) {

        do {

            if (instance->in_offset == instance->in_length) {
                instance->block_offset += instance->in_length;
                instance->in_offset = 0;

                struct uio auio;
                struct iovec aiov;

                aiov.iov_base = instance->inbuf;
                aiov.iov_len = BLOCK_SIZE;
                auio.uio_iov = &aiov;
                auio.uio_iovcnt = 1;
                auio.uio_resid = BLOCK_SIZE;
                auio.uio_segflg = UIO_SYSSPACE;

                error = kern_preadv(instance->td, instance->in_fd, &auio, instance->block_offset);

                if (error) {
                    return (error);
                }

                instance->in_length = BLOCK_SIZE - auio.uio_resid;

                instance->eof = !instance->in_length;
            }

            if (!instance->ctx) {
                instance->ctx = sre_vm_pike_create_ctx(
                                instance->context_pool, 
                                instance->program, 
                                instance->ovector, 
                                instance->ovecsize);

                DPRINT("Created new parsing context.\n");
                if (!instance->ctx) {
                    error = EIO;
                    return (error);
                }
            }

            if (!instance->out_length) {
                instance->match = sre_vm_pike_exec(
                        instance->ctx, 
                        instance->inbuf + instance->in_offset, 
                        instance->in_length - instance->in_offset, 
                        instance->eof, 
                        NULL);

                DPRINT("Executed parsing context.\n");

                if (instance->match == SRE_ERROR) {
                    error = EIO;
                    return (error);
                }

                if (instance->match >= 0) {

                    DPRINT("Found match.\n");
                    instance->out_length += snprintf(
                            instance->outbuf + instance->out_offset,
                            BLOCK_SIZE - instance->out_offset - instance->out_length,
                            "%ld", instance->nmatches);
                    for (size_t i = 0; i <= instance->ncaps; i++) {
                        instance->out_length += snprintf(
                                instance->outbuf + instance->out_offset,
                                BLOCK_SIZE - instance->out_offset - instance->out_length,
                                " [%ld,%ld]",
                                instance->ctx_offset + instance->ovector[2*i],
                                instance->ctx_offset + instance->ovector[2*i+1]);
                    }
                    instance->out_length += snprintf(
                            instance->outbuf + instance->out_offset,
                            BLOCK_SIZE - instance->out_offset - instance->out_length,
                            "\n");

                    instance->ctx = NULL;
                    instance->in_offset = instance->ctx_offset + instance->ovector[1] + 1 - instance->block_offset;
                    instance->ctx_offset = instance->block_offset + instance->in_offset;
                    sre_reset_pool(instance->context_pool);

                    instance->nmatches++;
                }

                if (instance->match == SRE_AGAIN) {
                    instance->in_offset = instance->in_length;
                }

                if (instance->match == SRE_DECLINED) {
                    instance->run_phase = 2;
                }

            }


            int amount = MIN(uio->uio_resid, MAX(instance->out_length, 0));

            if (amount > 0) {
                error = uiomove(instance->outbuf + instance->out_offset, amount, uio);
                if (error != 0) {
                    return(error);
                }
                instance->out_offset += amount;
                instance->out_length -= amount;
                if (instance->out_length == 0) {
                    instance->out_offset = 0;
                }
                DPRINT("Copied to output.\n");
            }

        } while (instance->match != SRE_DECLINED && 
                uio->uio_resid > 0 && 
                (instance->match_limit < 0 || instance->nmatches < instance->match_limit));
    }

    print_kgrep_state("control_read_exit", instance);

    return (error);
}

    static int 
kgrep_loader(struct module *m, int what, void *arg) 
{
    int err = 0;

    switch (what) {
        case MOD_LOAD:
            control_dev = make_dev(&control_cdevsw, 0, UID_ROOT, GID_WHEEL, 0666, "kgrep_control");

            DPRINT("KGrep loaded.\n");
            break;
        case MOD_UNLOAD:
            destroy_dev(control_dev);

            DPRINT("KGrep unloaded.\n");
            break;
        default:
            err = EOPNOTSUPP;
            break;
    }
    return(err);
}

static moduledata_t kgrep_mod = {
    "kgrep",
    kgrep_loader,
    NULL
};

DECLARE_MODULE(kgrep, kgrep_mod, SI_SUB_KLD, SI_ORDER_ANY);
