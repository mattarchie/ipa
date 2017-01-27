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
#include "noomr.h"
#include "memmap.h"
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

/**
 * Open a file descriptor NAMED file_no for later use by mmap
 * @param  file_no [description]
 * @return         [description]
 */
int mmap_fd(unsigned file_no) {
  if (!speculating()) {
    return -1;
  }
  char path[2048]; // 2 kB of path -- more than enough
  int written;
  // ensure the directory is present
  written = snprintf(&path[0], sizeof(path), "%s%d/", "/tmp/bop/", getuniqueid());
  if (written > sizeof(path) || written < 0) {
    noomr_perror("Unable to write directory name");
  }

  if (rmkdir(&path[0]) != 0 && errno != EEXIST) {
    noomr_perror("Unable to make the directory");
  }
  // now create the file
  written = snprintf(&path[0], sizeof(path), "%s%d/%u", "/tmp/bop/", getuniqueid(), file_no);
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

bool is_mapped(void * ptr) {
  void * aligned = (void *) (((intptr_t) ptr) & ~(PAGE_SIZE  - 1));
  return !(msync(aligned, 1, MS_ASYNC) == -1 && errno == ENOMEM);
}

static inline size_t get_size_fd(int fd) {
  struct stat st;
  if (fstat(fd, &st) == -1) {
    noomr_perror("Unable to get size of file");
  }
  return st.st_size;
}

static void map_now (noomr_page_t * last_page) {
  int fd = mmap_fd(last_page->next_pg_name);
  if (fd == -1) {
    // TODO handle error
  } else {
    size_t size = get_size_fd(fd);
    if (mmap((void *) last_page->next_page, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, fd, 0) == MAP_FAILED) {
      // TODO handle error
      abort();
    }
    close(fd); // think can do here
    assert(is_mapped((void *) last_page->next_page));
  }
}

// Map all missing pages
// Returns the last non-null page (eg. the one with the next page filed set to null)
noomr_page_t * map_missing_pages() {
  noomr_page_t * last_page = &shared->next_page;
  volatile int rounds = 0, maps = 0;
  if (shared->next_page.next_page != NULL) {
    if (!is_mapped((void *) &shared->next_page)) {
      map_now(&shared->next_page);
      maps++;
      assert(is_mapped(shared->next_page.next_page));
    }
    for (last_page = &shared->next_page;
         last_page->next_page != NULL;
         last_page = (noomr_page_t *) last_page->next_page) {
      rounds++;
      if (!is_mapped((void *) last_page->next_page)) {
        map_now(last_page);
        maps++;
      }
    }
  }
  return last_page;
}

/**
 * MMap a page according to @type
 *
 * @method allocate_noomr_page
 *
 * @param  minsize             the minimal size of the allocation, including any headers
 * @param  flags               baseline flags to pass to mmap, MAP_FIXED always added,
 *                             	MAP_ANONYMOUS while not speculating
 */
static inline void * allocate_noomr_page(int file_no, size_t minsize, int flags) {
  void * allocation = NULL;
  noomr_page_t alloc, expected = {0};
  // Reserve the resources in shared for this allocation
  int file_descriptor;
  size_t allocation_size = MAX(minsize, PAGE_SIZE);
  assert(allocation_size % PAGE_SIZE == 0);
  assert(shared != NULL);
#ifdef COLLECT_STATS
  __sync_add_and_fetch(&shared->total_alloc, allocation_size);
#endif
  if (!speculating()) {
    flags |= MAP_ANONYMOUS;
  } else {
    flags &= ~MAP_PRIVATE;
    flags |= MAP_SHARED;
  }
  file_descriptor = mmap_fd(file_no);
  noomr_page_t * last_page;
  while (true) {
    last_page = map_missing_pages();
    /**
     * Let the kernel decide where to put the new page(s)
     * Tasks communicate by requiring the CAS to succeed. If it fails
     * then some other task allocated its page
     */
    allocation = mmap(NULL, allocation_size, PROT_READ | PROT_WRITE, flags, file_descriptor, 0);
    if (allocation == (void *) -1) {
      noomr_perror("Unable to set up mmap page");
      continue;
    }
    alloc.next_page = allocation;
    alloc.next_pg_name = file_no;
    if (__sync_bool_compare_and_swap(&last_page->combined, expected.combined, alloc.combined)) {
      goto checks;
    } else {
      munmap(allocation, allocation_size);
    }
  }
  checks:
  if (file_descriptor != -1) {
    if (close(file_descriptor)) {
      noomr_perror("Unable to close file descriptor");
    }
  }
  return allocation;
}

static header_page_next_t free_pg_next = {
  .next = NULL,
  .next_file_num = 0
};

void allocate_header_page() {
  header_page_next_t update;
  const int file_no = !speculating() ? -1 : __sync_add_and_fetch(&shared->next_name, 1);
  header_page_t * headers = allocate_noomr_page(file_no, PAGE_SIZE, MAP_SHARED);
  if (headers == (header_page_t *) -1) {
    exit(-1);
  }
  bzero(headers, PAGE_SIZE);
  headers->next.next_file_num = 0;
  headers->next_free = 0;
  // Add increate_header_pgto the headers linked list
  if (shared->header_pg == NULL) {
    shared->header_pg = headers;
    assert(shared->header_pg != (volatile header_page_t *) shared->header_pg->next.next);
  } else {
    update.next = (volatile struct header_page_t *) headers;
    update.next_file_num = file_no;

    volatile header_page_t * last_page = shared->header_pg;
    do {

      while (last_page->next.next != NULL) {
        assert(last_page != (volatile header_page_t *) last_page->next.next);
        last_page = (header_page_t *) last_page->next.next;
      }
      // Load the relevant data into a new struct
    } while (!__sync_bool_compare_and_swap(&last_page->next.combined,
                                           free_pg_next.combined,
                                           update.combined));

    assert(headers != (volatile header_page_t *) headers->next.next);
    assert((volatile header_page_t *) last_page->next.next != last_page);
  }
#ifdef COLLECT_STATS
  __sync_add_and_fetch(&shared->header_pages, 1);
#endif
}

void * allocate_large(size_t size) {
  int file_no = !speculating() ? -1 : __sync_add_and_fetch(&shared->next_name, 1);
  // Align to a page size
  size_t alloc_size = PAGE_ALIGN((size + sizeof(huge_block_t)));
  assert(alloc_size > size);
  assert(alloc_size % PAGE_SIZE == 0);
  huge_block_t * block = allocate_noomr_page(file_no, alloc_size, MAP_PRIVATE);
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
