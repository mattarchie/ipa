#ifndef ___HAVE_NOOMR_MMAP_H
#define ___HAVE_NOOMR_MMAP_H
#include <stdbool.h>
#include <stdlib.h>

#if __SIZEOF_POINTER__ == 8
// 64b pointers
#define combined_page_t unsigned __int128
#else
#define combined_page_t uint64_t
#endif

typedef struct {
  union {
    struct {
      volatile struct noomr_page_t * next_page;
      volatile int next_pg_name;
    };
    combined_page_t combined;
  };
} noomr_page_t;

void allocate_header_page(void);
void* allocate_large(size_t);
int getuniqueid(void);
int mmap_fd(unsigned name);
noomr_page_t * map_missing_pages(void);
bool is_mapped(void *);

#endif
