
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/malloc.h>

#include <stdlib.h>
#include <unistd.h>

MALLOC_DEFINE(SHIM_MALLOC_TYPE, "kagrep-shim", "Buffers for malloc calls inside of kagrep.");

    char *
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

char * getenv(const char * key) {
    return NULL;
}

int issetugid(void) {
    return 0;
}
