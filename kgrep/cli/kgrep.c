
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define BLOCK_SIZE 4096

int main (int argc, char **argv) {

    // argv[1] is pattern string
    // argv[2] is the input file
    // argv[3] is the output file
    // argv[4] is the number of matches to find

    if (argc != 4 && argc != 5) {
        fprintf(stderr, "Usage: <pattern> <input-file> <output-file> [n-matches]\n");
        exit(1);
    }

    FILE *out = strcmp(argv[3], "-") ? fopen(argv[3], "w") : stdout;

    if (!out) {
        fprintf(stderr, "Unable to open output file.\n");
        exit(2);
    }

    int matchlimit = -1;
    if (argc >= 5) {
        matchlimit = atoi(argv[4]);
    }

    FILE *kgrep = fopen("/dev/kgrep_control","r+");

    if (!kgrep) {
        fprintf(stderr, "Could not open /dev/kgrep_control, is kgrep kernel module loaded?\n");
        return 1;
    }

    fprintf(kgrep, "%s,%s,%d", argv[1], argv[2], matchlimit);

    char buf[BLOCK_SIZE];
    size_t n = 0;
    fread(buf, 1, sizeof(buf), kgrep);
    do {
        n = fread(buf, 1, sizeof(buf), kgrep);
        fwrite(buf, 1, n, out);
    } while (n > 0);

    fclose(kgrep);
    return 0;
}
