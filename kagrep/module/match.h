
#include <sys/types.h>
#include "localeinfo.h"

#ifndef MATCH_H
#define MATCH_H

#define GLOBAL_SHIM 1

enum option {
    NONE,
    CASE_INSENSITIVE    = 'i',
    INVERTED            = 'v',
    MATCH_WORD          = 'w',
    MATCH_LINE          = 'x',
    COUNT               = 'c',
    FILES_WITHOUT_MATCH = 'L',
    FILES_WITH_MATCH    = 'l',
    ONLY_MATCH          = 'o',
    BYTE_OFFSET         = 'b',
    FILENAMES           = 'H',
    NO_FILENAMES        = 'h',
    LINE_NUMBERS        = 'n',
    TAB_ALIGN           = 'T',
    FILENAME_NULL       = 'Z',
    NULL_BOUND          = 'z',
    CONTEXT_BEFORE      = 'B',
    CONTEXT_AFTER       = 'A',
    MATCH_LIMIT         = 'm',
    BINARY_HANDLING     = 'U',
    DEVICE_HANDLING     = 'D',
    COLOR_ENABLE        = 'C',
    SGR_ALT             = 'S',
    SUPPRESS_ERRORS     = 's',
    LAST_OPTION         = '~',
};

enum binary_handling {
    BINARY_HANDLE_NONE,
    BINARY_HANDLE_TEXT,
    BINARY_HANDLE_BINARY,
    BINARY_HANDLE_NO_MATCH,
    BINARY_HANDLE_MAX,
    BINARY_HANDLE_DEFAULT = BINARY_HANDLE_BINARY,
};

enum device_handling {
    DEVICE_HANDLE_NONE,
    DEVICE_HANDLE_READ,
    DEVICE_HANDLE_SKIP,
    DEVICE_HANDLE_MAX,
    DEVICE_HANDLE_DEFAULT = DEVICE_HANDLE_READ,
};

struct uio;
struct thread;
struct grep_ctx;

typedef void *(*compile_fp_t) (struct grep_ctx *, char *, size_t, unsigned long int);
typedef size_t (*execute_fp_t) (struct grep_ctx *, void *, char const *, size_t, size_t *,
        char const *);

struct color_cap {
    const char *name;
    const char **var;
    void (*fct) (struct grep_ctx *);
};

struct file_req {
    struct file_req *prev, *next;
    int at_fd, cur_fd;
    char *name;
    size_t offset;
};

struct grep_ctx {
    struct thread *td;
    struct slbuf *out;
    int matcher;
    long options[LAST_OPTION];

    // locale globals
    struct localeinfo localeinfo;
    uintmax_t unibyte_mask;

    // Color information
    const char *selected_match_color;
    const char *context_match_color;
    const char *filename_color;
    const char *line_num_color;
    const char *byte_num_color;
    const char *sep_color;
    const char *selected_line_color;
    const char *context_line_color;
    struct color_cap color_dict[12];

    // formatting variables
    int offset_width;
    const char *group_separator;
    const char *filename;

    // error handling
    bool seek_failed;
    bool seek_data_failed;
    bool errseen;
    bool encoding_error_output;

    execute_fp_t execute;
    void *compiled_pattern;

    // List of files to process
    struct file_req *head, *tail;
};

struct grep_ctx *match_create_ctx(struct thread *td);
void match_destroy_ctx(struct grep_ctx *ctx);

int match_set_matcher(struct grep_ctx *ctx, char *matcher);
int match_set_pattern(struct grep_ctx *ctx, char *pattern, size_t size);
int match_set_colors(struct grep_ctx *ctx, const char *colors);
int match_set_opt(struct grep_ctx *ctx, enum option opt, long value);

int match_input(struct grep_ctx *ctx, char *filename);
int match_output(struct grep_ctx *ctx, struct uio *uio);

#endif
