#include <sys/malloc.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/systm.h>

#ifndef STDLIB_H
#define STDLIB_H

MALLOC_DECLARE(SHIM_MALLOC_TYPE);

#define free(x) free((x), SHIM_MALLOC_TYPE)
#define malloc(x) malloc((x), SHIM_MALLOC_TYPE, M_WAITOK)
#define calloc(x,y) memset(malloc((x)*(y)), 0, x)
#define realloc(x, y) realloc((x), (y), SHIM_MALLOC_TYPE, M_WAITOK)
#define strdup(x) strdup(x, SHIM_MALLOC_TYPE)
#define fprintf(...)

#define exit(x) panic("MANUAL EXIT: %d\n", x)
#define abort() panic("MANUAL ABORT")

#define MB_CUR_MAX 16
#define EXIT_FAILURE 1

char * strtok_r (char *s, const char *delim, char **save_ptr);
char * getenv(const char *);

#endif
