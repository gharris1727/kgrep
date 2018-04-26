
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

#include <slbuf.h>

#include "grep/src/grep.h"
#include "match.h"

#define BUFFER_SIZE 32768

MALLOC_DECLARE(MATCH_MEM);

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

    init_globals(ctx);

    /* The color strings used for matched text.
       The user can overwrite them using the deprecated
       environment variable GREP_COLOR or the new GREP_COLORS.  */
    ctx->selected_match_color = "01;31";	/* bold red */
    ctx->context_match_color  = "01;31";	/* bold red */

    /* Other colors.  Defaults look damn good.  */
    ctx->filename_color = "35";	/* magenta */
    ctx->line_num_color = "32";	/* green */
    ctx->byte_num_color = "32";	/* green */
    ctx->sep_color      = "36";	/* cyan */
    ctx->selected_line_color = "";	/* default color pair */
    ctx->context_line_color  = "";	/* default color pair */

    ctx->color_dict[0].name = "mt";
    ctx->color_dict[0].var = &ctx->selected_match_color;
    ctx->color_dict[0].fct = color_cap_mt_fct;
    ctx->color_dict[1].name = "ms";
    ctx->color_dict[1].var = &ctx->selected_match_color;
    ctx->color_dict[1].fct = NULL;
    ctx->color_dict[2].name = "mc";
    ctx->color_dict[2].var = &ctx->context_match_color;
    ctx->color_dict[2].fct = NULL;
    ctx->color_dict[3].name = "fn";
    ctx->color_dict[3].var = &ctx->filename_color;
    ctx->color_dict[3].fct = NULL;
    ctx->color_dict[4].name = "ln";
    ctx->color_dict[4].var = &ctx->line_num_color;
    ctx->color_dict[4].fct = NULL;
    ctx->color_dict[5].name = "bn";
    ctx->color_dict[5].var = &ctx->byte_num_color;
    ctx->color_dict[5].fct = NULL;
    ctx->color_dict[6].name = "se";
    ctx->color_dict[6].var = &ctx->sep_color;
    ctx->color_dict[6].fct = NULL;
    ctx->color_dict[7].name = "sl";
    ctx->color_dict[7].var = &ctx->selected_line_color;
    ctx->color_dict[7].fct = NULL;
    ctx->color_dict[8].name = "cx";
    ctx->color_dict[8].var = &ctx->context_line_color;
    ctx->color_dict[8].fct = NULL;
    ctx->color_dict[9].name = "rv";
    ctx->color_dict[9].var = NULL;
    ctx->color_dict[9].fct = color_cap_rv_fct;
    ctx->color_dict[10].name = "ne";
    ctx->color_dict[10].var = NULL;
    ctx->color_dict[10].fct = color_cap_ne_fct;
    ctx->color_dict[11].name = NULL;
    ctx->color_dict[11].var = NULL;
    ctx->color_dict[11].fct = NULL;

    ctx->group_separator = "--";

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

    clean_globals(ctx);
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
    ctx->compiled_pattern = matchers[ctx->matcher].compile(ctx, pattern, size, matchers[ctx->matcher].syntax);
    
    return !ctx->execute || !ctx->compiled_pattern;
}

int match_set_colors(struct grep_ctx *ctx, const char *colors) {
    parse_grep_colors(ctx, colors);
    return 0;
}

int match_set_opt(struct grep_ctx *ctx, enum option opt, int value) {
    if (opt < LAST_OPTION) {
        ctx->options[opt] = value;
        return 0;
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
        error = grepdesc(ctx, ctx->head->next->cur_fd, false);
        match_rem_file(ctx, ctx->head->next);
    }
    return error;
}

MALLOC_DEFINE(MATCH_MEM, "matcher-mem", "Memory for the matcher state.");
