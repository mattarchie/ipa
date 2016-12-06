#ifndef ___HAVE_NOOMR_MMAP_H
#define ___HAVE_NOOMR_MMAP_H
#include <stdbool.h>
#include <stdlib.h>

void allocate_header_page(void);
void* allocate_large(size_t);
int create_large_pg(int);
int getuniqueid(void);
void * claim_region(size_t s);

#endif
