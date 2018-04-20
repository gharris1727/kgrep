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

#include "grep/src/search.h"

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
    .d_name    = "kagrepcontrol"
};

#define BUF_SIZE 1024

typedef void *(*compile_fp_t) (char *, size_t, reg_syntax_t);
typedef size_t (*execute_fp_t) (void *, char const *, size_t, size_t *,
                                char const *);

bool match_icase;
char eolbyte;
struct localeinfo localeinfo;
bool match_lines;
bool match_words;

/* See below */
struct FL_pair
  {
    char const *filename;
    size_t lineno;
  };

/* A list of lineno,filename pairs corresponding to -f FILENAME
   arguments. Since we store the concatenation of all patterns in
   a single array, KEYS, be they from the command line via "-e PAT"
   or read from one or more -f-specified FILENAMES.  Given this
   invocation, grep -f <(seq 5) -f <(seq 2) -f <(seq 3) FILE, there
   will be three entries in LF_PAIR: {1, x} {6, y} {8, z}, where
   x, y and z are just place-holders for shell-generated names.  */
static struct FL_pair *fl_pair;
static size_t n_fl_pair_slots;

/* Count not only -f-specified files, but also individual -e operands
   and any command-line argument that serves as a regular expression.  */
static size_t n_pattern_files;

/* The number of patterns seen so far.
   It is advanced by fl_add and, when needed, used in pattern_file_name
   to derive a file-relative line number.  */
static size_t n_patterns;

/* Return the number of newline bytes in BUF with size SIZE.  */
static size_t _GL_ATTRIBUTE_PURE
count_nl_bytes (char const *buf, size_t size)
{
  char const *p = buf;
  char const *end_p = buf + size;
  size_t n = 0;
  while ((p = memchr (p, '\n', end_p - p)))
    p++, n++;
  return n;
}

/* Append a FILENAME,line-number pair to FL_PAIR, and update
   pattern-related counts from the contents of BUF with SIZE bytes.  */
static void
__attribute__((unused))
fl_add (char const *buf, size_t size, char const *filename)
{
  if (n_fl_pair_slots <= n_pattern_files)
    fl_pair = x2nrealloc (fl_pair, &n_fl_pair_slots, sizeof *fl_pair);

  fl_pair[n_pattern_files].lineno = n_patterns + 1;
  fl_pair[n_pattern_files].filename = filename;
  n_pattern_files++;
  n_patterns += count_nl_bytes (buf, size);
}

/* Map the line number, LINENO, of one of the input patterns to the
   name of the file from which it came.  If it was read from stdin
   or if it was specified on the command line, return "-".  */
char const * _GL_ATTRIBUTE_PURE
pattern_file_name (size_t lineno, size_t *new_lineno)
{
  size_t i;
  for (i = 1; i < n_pattern_files; i++)
    {
      if (lineno < fl_pair[i].lineno)
        break;
    }

  *new_lineno = lineno - fl_pair[i - 1].lineno + 1;
  return fl_pair[i - 1].filename;
}

/* Change the pattern *KEYS_P, of size *LEN_P, from fgrep to grep style.  */

void
fgrep_to_grep_pattern (char **keys_p, size_t *len_p)
{
  size_t len = *len_p;
  char *keys = *keys_p;
  mbstate_t mb_state = { {0} };
  char *new_keys = xnmalloc (len + 1, 2);
  char *p = new_keys;
  size_t n;

  for (; len; keys += n, len -= n)
    {
      n = mb_clen (keys, len, &mb_state);
      switch (n)
        {
        case (size_t) -2:
          n = len;
          FALLTHROUGH;
        default:
          memcpy (p, keys, n);
          p += n;
          break;

        case (size_t) -1:
          memset (&mb_state, 0, sizeof mb_state);
          n = 1;
          FALLTHROUGH;
        case 1:
          switch (*keys)
            {
            case '$': case '*': case '.': case '[': case '\\': case '^':
              *p++ = '\\'; break;
            }
          *p++ = *keys;
          break;
        }
    }

  free (*keys_p);
  *keys_p = new_keys;
  *len_p = p - new_keys;
}

static __attribute__((unused)) execute_fp_t execute;
static __attribute__((unused)) void *compiled_pattern;

static struct
{
  char name[12];
  int syntax; /* used if compile == GEAcompile */
  compile_fp_t compile;
  execute_fp_t execute;
} const matchers[] = {
  { "grep", RE_SYNTAX_GREP, GEAcompile, EGexecute },
  { "egrep", RE_SYNTAX_EGREP, GEAcompile, EGexecute },
  { "fgrep", 0, Fcompile, Fexecute, },
  { "awk", RE_SYNTAX_AWK, GEAcompile, EGexecute },
  { "gawk", RE_SYNTAX_GNU_AWK, GEAcompile, EGexecute },
  { "posixawk", RE_SYNTAX_POSIX_AWK, GEAcompile, EGexecute },
  { "perl", 0, Pcompile, Pexecute, },
};

typedef struct kagrep {
    struct thread *td;
    char in_buf[BUF_SIZE];
    size_t in_buf_size;
    char **command;
} kagrep_t;

static void print_kagrep_state(char *id, struct kagrep *instance) {
}

static struct cdev *control_dev;

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
    kagrep_t *instance = malloc(sizeof(kagrep_t));

    if (!instance) {
        uprintf("Unable to allocate memory!\n");
        return (1);
    }

    instance->td = td;
    dev->si_drv1 = instance;

    DPRINT("Opened control channel.\n");
    return (0);
}

    static int
control_close(struct cdev *dev, int fflag, int devtype, struct thread *td)
{
    kagrep_t *instance = dev->si_drv1;

    print_kagrep_state("control_close_enter", instance);

    free(instance);

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
    int error = 0;
    kagrep_t *instance = dev->si_drv1;

    print_kagrep_state("control_write_enter", instance);

    size_t size = MIN(uio->uio_resid, (BUF_SIZE - 1 - instance->in_buf_size));
    error = uiomove(instance->in_buf + instance->in_buf_size, size, uio);

    if (error) {
        return (error);
    }

    char *temp;
    char *first = strtok_r(instance->in_buf, "\n", &temp);

    strcmp(first, matchers[0].name);

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

    static int 
kagrep_loader(struct module *m, int what, void *arg) 
{
    int err = 0;

    switch (what) {
        case MOD_LOAD:
            control_dev = make_dev(&control_cdevsw, 0, UID_ROOT, GID_WHEEL, 0666, "kagrep_control");

            DPRINT("kagrep loaded.\n");
            break;
        case MOD_UNLOAD:
            destroy_dev(control_dev);

            DPRINT("kagrep unloaded.\n");
            break;
        default:
            err = EOPNOTSUPP;
            break;
    }
    return(err);
}

static moduledata_t kagrep_mod = {
    "kagrep",
    kagrep_loader,
    NULL
};

DECLARE_MODULE(kagrep, kagrep_mod, SI_SUB_KLD, SI_ORDER_ANY);
