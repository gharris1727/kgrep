
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "sregex/src/sregex/sregex.h"

#define BLOCK_SIZE 4096

int main(int argc, char **argv) {
    
    // argv[1] is pattern string
    // argv[2] is the input file
    // argv[3] is the output file
    // argv[4] is the number of matches to find

    if (argc != 4 && argc != 5) {
        fprintf(stderr, "Usage: <pattern> <input-file> <output-file> [n-matches]\n");
        exit(1);
    }

    FILE *in = strcmp(argv[2], "-") ? fopen(argv[2], "r") : stdin;
    FILE *out = strcmp(argv[3], "-") ? fopen(argv[3], "w") : stdout;

    if (!in) {
        fprintf(stderr, "Unable to open input file.\n");
        exit(2);
    }

    if (!out) {
        fprintf(stderr, "Unable to open output file.\n");
        exit(2);
    }

    int matchlimit = -1;
    if (argc >= 5) {
        matchlimit = atoi(argv[4]);
    }

    sre_pool_t *pool = sre_create_pool(4096);

    if (!pool) {
        fprintf(stderr, "Unable to allocate memory pool.\n");
        exit(3);
    }

    size_t ncaps;
    long err_offset;

    sre_regex_t *regex = sre_regex_parse(pool, (unsigned char *) argv[1], &ncaps, 0, &err_offset);

    if (!regex) {
        fprintf(stderr, "Unable to parse regex. Error near: %s\n", argv[1] + err_offset);
        exit(4);
    }

    sre_program_t *program = sre_regex_compile(pool, regex);

    if (!program) {
        fprintf(stderr, "Unable to compile regex.\n");
        exit(5);
    }

    size_t ovecsize = 2 * (ncaps + 1) * sizeof(sre_int_t);
    long *ovector = malloc(ovecsize);

    if (!ovector) {
        fprintf(stderr, "Unable to allocate capture memory.\n");
        exit(6);
    }

    sre_pool_t *ctx_pool= sre_create_pool(4096);

    int eof, match;
    sre_vm_pike_ctx_t *ctx = NULL;
    ssize_t block_offset = 0;
    ssize_t ctx_offset = 0;
    ssize_t nmatches = 0;
    do {

        unsigned char buffer[BLOCK_SIZE];
        ssize_t length = fread(buffer, 1, BLOCK_SIZE, in);
        eof = !length;
        ssize_t intrablock_offset = 0;

        if (length < 0) {
            fprintf(stderr, "Unable to read file!\n");
            exit(7);
        }

        do {
            if (!ctx && (!(ctx = sre_vm_pike_create_ctx(ctx_pool, program, ovector, ovecsize)))) {
                fprintf(stderr, "Unable to allocate VM context.\n");
                exit(8);
            }

            match = sre_vm_pike_exec(ctx, buffer + intrablock_offset, length - intrablock_offset, eof, NULL);

            if (match == SRE_ERROR) {
                fprintf(stderr, "Error during matching!\n");
                exit(9);
            }

            if (match >= 0) {
                fprintf(out, "%ld", nmatches);
                for (size_t i = 0; i <= ncaps; i++) {
                    fprintf(out, " [%ld,%ld]\n", ctx_offset + ovector[2*i], ctx_offset + ovector[2*i+1]);
                }

                ctx = NULL;
                intrablock_offset = ctx_offset + ovector[1] + 1 - block_offset;
                ctx_offset = block_offset + intrablock_offset;
                sre_reset_pool(ctx_pool);

                nmatches++;
                if (matchlimit > 0 && nmatches >= matchlimit) {
                    break;
                }
            }

        } while (match != SRE_AGAIN && match != SRE_DECLINED);

        block_offset += length;

    } while (!eof && !(matchlimit > 0 && nmatches >= matchlimit));

    sre_destroy_pool(ctx_pool);
    sre_destroy_pool(pool);

    fclose(in);
    fclose(out);

    exit(0);
}
