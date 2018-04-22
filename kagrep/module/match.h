
#include <sys/types.h>

#ifndef MATCH_H
#define MATCH_H

struct uio;
struct thread;

struct grep_ctx;

enum option {
    NULL_BOUND,
    MATCH_LINE,
    MATCH_WORD
};

struct grep_ctx *match_create_ctx(struct thread *td);
void match_destroy_ctx(struct grep_ctx *ctx);

int match_set_matcher(struct grep_ctx *ctx, char *matcher);
int match_set_pattern(struct grep_ctx *ctx, char *pattern, size_t size);
int match_set_opt(struct grep_ctx *ctx, enum option opt, int value);

int match_input(struct grep_ctx *ctx, char *filename);
int match_output(struct grep_ctx *ctx, struct uio *uio);

#endif
