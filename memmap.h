#ifndef ___HAVE_BOMALLOC_MMAP_H
#define ___HAVE_BOMALLOC_MMAP_H
#include <stdbool.h>
#include <stdlib.h>

#include "bomalloc.h"

void allocate_header_page(void);
huge_block_t * allocate_large(size_t);
int getuniqueid(void);
int mmap_fd(unsigned, size_t);
int mmap_existing_fd(unsigned);
volatile bomalloc_page_t * map_missing_pages(void);
bool is_mapped(void *);
#endif
