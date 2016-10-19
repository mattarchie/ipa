#include <stdlib.h>
#include <sys/mman.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "memmap.h"
#include "noomr.h"

extern shared_data_t * shared;
extern bool speculating(void);
extern header_page_t * lastheaderpg(void);


static inline int mmap_fd(int file_no, const char * subdir) {
  if (!speculating()) {
    return -1;
  }
  char path[2048]; // 2 MB of path -- more than enough
  int written = snprintf(&path[0], sizeof(path), "%s%d%s%d", "/tmp/bop/", getpgid(getpid()), subdir, file_no);
  if (written > sizeof(path) || written < 0) {
    perror("Unable to write the output path");
  }
  int fd = open(path, O_RDWR | O_CREAT | O_SYNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
  // need to truncate (grow -- poor naming) the file
  if (ftruncate(fd, PAGE_SIZE) < 0) {
    perror("Unable to truncate/grow file");
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
 * @param  size                the acual size of the allocation will be written
 *                             	back to this address. if allocation fails, then
 *                             	the size that would have been allocated is written
 */
static inline void * allocate_noomr_page(noomr_page_t type, int file_no,
                                         size_t minsize, int flags, size_t * size) {
  void * allocation = NULL;
  // Reserve the resources in shared for this allocation
  int file_descriptor;
  size_t allocation_size = align(minsize, PAGE_SIZE);
  int mmap_page_no = __sync_add_and_fetch(&shared->number_mmap, allocation_size / PAGE_SIZE);
  char * destination = ((char*) shared) + (PAGE_SIZE * mmap_page_no);
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
  if ( (speculating() && allocation != (void *) destination) || allocation == (void *) -1) {
    perror("Unable to set up header page");
  }
  if (size != NULL) {
    *size = allocation_size;
  }
  return allocation;
}

void allocate_header_page() {
  int file_no = !speculating() ? -1 : __sync_add_and_fetch(&shared->header_num, 1);
  header_page_t * headers = allocate_noomr_page(header_pg, file_no, PAGE_SIZE, MAP_SHARED, NULL);
  // Add increate_header_pgto the headers linked list
  header_page_t * last_page = lastheaderpg();
  if (last_page == NULL) {
    shared->firstpg = headers;
  } else {
    do {
      while (last_page->next != NULL) {
        last_page = (header_page_t *) last_page->next;
      }
    } while (__sync_bool_compare_and_swap(&last_page->next, NULL, headers));
  }
}

void * allocate_large(size_t size) {
  int file_no = !speculating() ? -1 : __sync_add_and_fetch(&shared->large_num, 1);
  size_t alloc_size;
  block_t * block = allocate_noomr_page(large_alloc, file_no, size, MAP_PRIVATE, &alloc_size);
  block->huge_block_sz = alloc_size;
  do {
    block->next_block = (block_t *) shared->large_block;
  } while(!__sync_bool_compare_and_swap(&shared->large_block, block->next_block, block));
  return getpayload(block);
}
