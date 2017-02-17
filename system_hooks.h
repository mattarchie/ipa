#ifndef __BOMALLOC_SYSTEM
#define __BOMALLOC_SYSTEM
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>


bool speculating(void);
void record_allocation(void * p, size_t t);
int getuniqueid(void);

#endif
