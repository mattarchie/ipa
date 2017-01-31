#ifndef ___HAVE_NOOMR_MMAP_H
#define ___HAVE_NOOMR_MMAP_H
#include <stdbool.h>
#include <stdlib.h>

#include "noomr.h"

void allocate_header_page(void);
huge_block_t * allocate_large(size_t);
int getuniqueid(void);
int mmap_fd(unsigned, size_t);
int mmap_existing_fd(unsigned);
noomr_page_t * map_missing_pages(void);
bool is_mapped(void *);
#endif
