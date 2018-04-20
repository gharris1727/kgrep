
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/malloc.h>

#include <stdlib.h>

MALLOC_DEFINE(SHIM_MALLOC_TYPE, "kagrep-shim", "Buffers for malloc calls inside of kagrep.");

char	*_PathLocale;
