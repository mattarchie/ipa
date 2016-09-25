#ifndef ___HAVE_NOOMR_MMAP_H
#define ___HAVE_NOOMR_MMAP_H
#include <stdbool.h>
#include <stdlib.h>

void allocate_header_page(void);
void* allocate_large(size_t);


#endif
