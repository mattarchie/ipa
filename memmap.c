#include <stdlib.h>
#include <sys/mman.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <strings.h>
#include <assert.h>
#include <error.h>
#include <string.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <errno.h>
#include "memmap.h"
#include "noomr.h"
#include "noomr_utils.h"

extern shared_data_t * shared;
extern bool speculating(void);
extern void noomr_init(void);


static inline int mkdir_ne(char * path, int flags) {
  struct stat st = {0};
  if (stat(path, &st) == -1) {
    return mkdir(path, flags);
  }
  return 0;
}

// Recursively make directories
// Based on:
// http://nion.modprobe.de/blog/archives/357-Recursive-directory-creation.html
static int rmkdir(char *dir) {
  char tmp[256];
  char *p = NULL;
  size_t len;
  errno = 0;
  snprintf(tmp, sizeof(tmp),"%s",dir);
  len = strlen(tmp);
  if(tmp[len - 1] == '/')
          tmp[len - 1] = 0;
  for(p = tmp + 1; *p; p++) {
    if(*p == '/') {
        *p = 0;
        if (mkdir_ne(tmp, S_IRWXU)) {
          return -1;
        }
        *p = '/';
    }
  }
  return mkdir_ne(tmp, S_IRWXU);
}

void * claim_region(size_t s) {
  if (shared == NULL) {
    noomr_init();
  }
  int mmap_page_no = __sync_add_and_fetch(&shared->number_mmap, MAX(1, s / PAGE_SIZE));
  return (char *) (shared - (PAGE_SIZE * mmap_page_no));
}

/** NB: We cannot use perror -- it internally calls malloc*/
static int mmap_fd(int file_no, const char * subdir) {
  if (!speculating()) {
    return -1;
  }
  char path[2048]; // 2 kB of path -- more than enough
  int written;
  // ensure the directory is present
  written = snprintf(&path[0], sizeof(path), "%s%d%s", "/tmp/bop/", getuniqueid(), subdir);
  if (written > sizeof(path) || written < 0) {
    noomr_perror("Unable to write directory name");
  }

  if (rmkdir(&path[0]) != 0 && errno != EEXIST) {
    noomr_perror("Unable to make the directory");
  }
  // now create the file
  written = snprintf(&path[0], sizeof(path), "%s%d%s%d", "/tmp/bop/", getuniqueid(), subdir, file_no);
  if (written > sizeof(path) || written < 0) {
    noomr_perror("Unable to write the output path");
  }

  int fd = open(path, O_RDWR | O_CREAT | O_SYNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
  if (fd == -1) {
    noomr_perror("Unable to create the file.");
  }

  // need to truncate (grow -- poor naming) the file
  if (ftruncate(fd, PAGE_SIZE) < 0) {
    noomr_perror("NOOMR_MMAP: Unable to truncate/grow file");
  }
  return fd;
}
int create_header_pg(int file_no) {
  return mmap_fd(file_no, "/headers/");
}

int create_large_pg(int file_no) {
  return mmap_fd(file_no, "/large/");
}

typedef enum {
  large_alloc,
  header_pg
} noomr_page_t;

/**
 * MMap a page according to @type
 *
 * @method allocate_noomr_page
 *
 * @param  type                the type of page to allocate
 * @param  minsize             the minimal size of the allocation, including any headers
 * @param  flags               baseline flags to pass to mmap, MAP_FIXED always added,
 *                             	MAP_ANONYMOUS while not speculating
 */
static inline void * allocate_noomr_page(noomr_page_t type, int file_no,
                                         size_t minsize, int flags) {
  void * allocation = NULL;
  // Reserve the resources in shared for this allocation
  int file_descriptor;
  size_t allocation_size = MAX(minsize, PAGE_SIZE);
  assert(allocation_size % PAGE_SIZE == 0);
  assert(shared != NULL);
#ifdef COLLECT_STATS
  __sync_add_and_fetch(&shared->total_alloc, allocation_size);
#endif
  char * destination = claim_region(allocation_size);
  if (!speculating()) {
    flags |= MAP_ANONYMOUS;
  }
  switch(type) {
    case header_pg:
      file_descriptor = create_header_pg(file_no);
      break;
    case large_alloc:
      file_descriptor = create_large_pg(file_no);
      break;
    default:
      fprintf(stderr, "Unable to create file descriptor for %d\n", type);
      file_descriptor = -1;
      break;
  }
  allocation = mmap(destination, allocation_size, PROT_READ | PROT_WRITE, flags | MAP_FIXED, file_descriptor, 0);
  if (allocation == (void *) -1) {
    noomr_perror("Unable to set up mmap page");
  } else if (allocation != destination) {
    noomr_perror("Unable to mmap to the required location");
  }
  if (file_descriptor != -1) {
    if (close(file_descriptor)) {
      noomr_perror("Unable to close file descriptor");
    }
  }
  return allocation;
}

void allocate_header_page() {
  int file_no = !speculating() ? -1 : __sync_add_and_fetch(&shared->header_num, 1);
  header_page_t * headers = allocate_noomr_page(header_pg, file_no, PAGE_SIZE, MAP_SHARED);
  if (headers == (header_page_t *) -1) {
    exit(-1);
  }
  bzero(headers, PAGE_SIZE);
  headers->next_free = 0;
  // Add increate_header_pgto the headers linked list
  if (shared->header_pg == NULL) {
    shared->header_pg = headers;
    assert(shared->header_pg != (volatile header_page_t *) shared->header_pg->next);
  } else {
    volatile header_page_t * last_page = shared->header_pg;
    do {
      while (last_page->next != NULL) {
        assert(last_page != (volatile header_page_t *) last_page->next);
        last_page = (header_page_t *) last_page->next;
      }
    } while (!__sync_bool_compare_and_swap(&last_page->next, NULL, (volatile struct header_page_t *) headers));
    assert(headers != (volatile header_page_t *) headers->next);
    assert((volatile header_page_t *) last_page->next != last_page);
  }
#ifdef COLLECT_STATS
  __sync_add_and_fetch(&shared->header_pages, 1);
#endif
}

void * allocate_large(size_t size) {
  int file_no = !speculating() ? -1 : __sync_add_and_fetch(&shared->large_num, 1);
  // Align to a page size
  size_t alloc_size = PAGE_ALIGN((size + sizeof(huge_block_t)));
  assert(alloc_size > size);
  assert(alloc_size % PAGE_SIZE == 0);
  huge_block_t * block = allocate_noomr_page(large_alloc, file_no, alloc_size, MAP_PRIVATE);
  if (block == (huge_block_t *) -1) {
    return NULL;
  }
  block->huge_block_sz = alloc_size;
  block->file_name = file_no;
  do {
    block->next_block = (block_t *) shared->large_block;
  } while(!__sync_bool_compare_and_swap(&shared->large_block, block->next_block, block));
#ifdef COLLECT_STATS
  __sync_add_and_fetch(&shared->huge_allocations, 1);
#endif
  return gethugepayload(block);
}
