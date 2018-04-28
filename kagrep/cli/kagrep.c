
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define BLOCK_SIZE 4096

int main (int argc, char **argv) {

#define PATTERN argv[1]
#define MATCHER argv[2]
#define OPTIONS argv[3]
#define INPUT   argv[4]
#define OUTPUT  (argc > 5 ? argv[5] : "-")
#define MATCHES (argc > 6 ? argv[6] : "-1")

    if (argc != 5 && argc != 6 && argc != 7) {
        fprintf(stderr, "Usage: <pattern> <matcher> <options> <input-file> [output-file [n-matches]]\n");
        exit(1);
    }

    FILE *out = strcmp(OUTPUT, "-") ? fopen(OUTPUT, "w") : stdout;

    if (!out) {
        fprintf(stderr, "Unable to open output file.\n");
        exit(2);
    }

    long matchlimit = strtol(MATCHES, NULL, 10);

    FILE *kgrep = fopen("/dev/kagrep_control","r+");

    if (!kgrep) {
        fprintf(stderr, "Could not open /dev/kgrep_control, is kgrep kernel module loaded?\n");
        return 1;
    }

    fprintf(kgrep, 
            "%s\n"              // matcher
            "%s\n"              // options
            "%ld\n"             // limit
            "%d\n"              // before
            "%d\n"              // after
            "text\n"            // binary
            "read\n"            // device
            "\n"                // colors
            "%lu\n"             // pattern length
            "%s\n"              // pattern
            "%s\n",             // filename
            MATCHER,            // matcher
            OPTIONS,            // options
            matchlimit,         // limit
            0,                  // before
            0,                  // after
            strlen(PATTERN),    // pattern length
            PATTERN,            // pattern
            INPUT);           // filename

    char buf[BLOCK_SIZE];
    size_t n = 0;
    do {
        n = fread(buf, 1, sizeof(buf), kgrep);
        fwrite(buf, 1, n, out);
    } while (n > 0);

    fclose(kgrep);
    return 0;
}
