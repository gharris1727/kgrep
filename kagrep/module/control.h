
#include <sys/malloc.h>

#ifndef CONTROL_H
#define CONTROL_H

int control_load (void);
void control_unload (void);

MALLOC_DECLARE(CONTROL_STATE);

#endif
