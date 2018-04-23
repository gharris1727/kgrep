
#include <sys/types.h>
#include <sys/param.h>
#include <sys/file.h>
#include <sys/syscallsubr.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/proc.h>
#include <sys/fcntl.h>
#include <sys/kernel.h>
#include <sys/malloc.h>

#include "match.h"
#include "grep/src/grep.h"

#define BUFFER_SIZE 32768

#define GLOBAL_SHIM 1

MALLOC_DECLARE(MATCH_MEM);

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
    execute_fp_t execute;
    void *compiled_pattern;
    struct file_req *head, *tail;
};

static void match_add_file(struct grep_ctx *ctx, struct file_req *loc, int at_fd, char *name);
static void match_rem_file(struct grep_ctx *ctx, struct file_req *del);

struct grep_ctx *match_create_ctx(struct thread *td) {
    struct grep_ctx *ctx = malloc(sizeof(struct grep_ctx), MATCH_MEM, M_WAITOK|M_ZERO);

    ctx->td = td;
    ctx->out = slbuf_create(BUFFER_SIZE);
    // Allocate a dummmy head and tail node to eliminate edge cases.
    ctx->head = malloc(sizeof(struct file_req), MATCH_MEM, M_WAITOK|M_ZERO);
    ctx->tail = malloc(sizeof(struct file_req), MATCH_MEM, M_WAITOK|M_ZERO);
    ctx->head->at_fd = ctx->head->cur_fd = ctx->tail->at_fd = ctx->tail->cur_fd = -1;
    ctx->head->next = ctx->tail;
    ctx->tail->prev = ctx->head;

#if GLOBAL_SHIM
    match_icase = false;
    eolbyte = '\n';
#endif

    return ctx;
}

void match_destroy_ctx(struct grep_ctx *ctx) {
    while (ctx->head->next != ctx->tail) {
        match_rem_file(ctx, ctx->head->next);
    }
    free(ctx->head, MATCH_MEM);
    free(ctx->tail, MATCH_MEM);
    slbuf_destroy(ctx->out);
    free(ctx, MATCH_MEM);
}

int match_set_matcher(struct grep_ctx *ctx, char *matcher) {
    for (int i = 0; i < sizeof(matchers) / sizeof(*matchers); i++) {
        if (!strcmp(matcher, matchers[i].name)) {
            ctx->matcher = i;
            return 0;
        }
    }
    return 1;
}

int match_set_pattern(struct grep_ctx *ctx, char *pattern, size_t size) {
    ctx->execute = matchers[ctx->matcher].execute;
    ctx->compiled_pattern = matchers[ctx->matcher].compile(pattern, size, matchers[ctx->matcher].syntax);

#if GLOBAL_SHIM
    extern execute_fp_t execute;
    execute = ctx->execute;
    extern void *compiled_pattern;
    compiled_pattern = ctx->compiled_pattern;
#endif
    
    return !ctx->execute || !ctx->compiled_pattern;
}

int match_set_opt(struct grep_ctx *ctx, enum option opt, int value) {
    switch (opt) {
        case NULL_BOUND:
            eolbyte = '\0';
            return 0;
            break;
        case MATCH_LINE:
            match_lines = true;
            return 0;
            break;
        case MATCH_WORD:
            match_words = true;
            return 0;
            break;
    }
    return 1;
}

static void match_add_file(struct grep_ctx *ctx, struct file_req *loc, int at_fd, char *name) {
    struct file_req *n = malloc(sizeof(struct file_req), MATCH_MEM, M_WAITOK|M_ZERO);

    // Fill out the fields of the struct.
    n->name = malloc(strlen(name) + 1, MATCH_MEM, M_WAITOK);
    strcpy(n->name, name);
    n->cur_fd = kern_openat(ctx->td, n->at_fd, n->name, UIO_SYSSPACE, O_RDONLY, 0444) ? -1 : ctx->td->td_retval[0];

    // Insert into the list after the current element.
    n->next = loc->next;
    n->prev = loc;
    n->prev->next = n;
    n->next->prev = n;
}

static void match_rem_file(struct grep_ctx *ctx, struct file_req *del) {
    // Cut this element out of the list
    del->next->prev = del->prev;
    del->prev->next = del->next;

    // Clean up the contents.
    if (del->name) {
        free(del->name, MATCH_MEM);
    }
    if (del->cur_fd != -1 && ctx->td->td_proc->p_fd) {
        kern_close(ctx->td, del->cur_fd);
    }
    free(del, MATCH_MEM);
}

int match_input(struct grep_ctx *ctx, char *filename) {
    match_add_file(ctx, ctx->tail->prev, AT_FDCWD, filename);

    return ctx->tail->prev->cur_fd == -1;
}

int match_output(struct grep_ctx *ctx, struct uio *uio) {
    int error = 0;
    if ((error = slbuf_read(ctx->out, uio))) {
        return error;
    }
    while (uio->uio_resid > 0 && ctx->head->next != ctx->tail) {
#if GLOBAL_SHIM
        extern char const *filename;
        filename = ctx->head->next->name;
#endif
        error = grepdesc(ctx->td, ctx->out, ctx->head->next->cur_fd, false);
        match_rem_file(ctx, ctx->head->next);
    }
    return error;
}

MALLOC_DEFINE(MATCH_MEM, "matcher-mem", "Memory for the matcher state.");
