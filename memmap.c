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
int mmap_fd_bool(unsigned file_no, size_t size, bool no_fd) {
  if (no_fd) {
    return -1;
  }
  char path[2048]; // 2 kB of path -- more than enough
  int written;
  // ensure the directory is present
  written = snprintf(&path[0], sizeof(path), "%s%d/", "/tmp/bop/", getuniqueid());
  if (written > sizeof(path) || written < 0) {
    noomr_perror("Unable to write directory name");
    abort();
  }

  if (rmkdir(&path[0]) != 0 && errno != EEXIST) {
    noomr_perror("Unable to make the directory");
    abort();
  }
  // now create the file
  written = snprintf(&path[0], sizeof(path), "%s%d/%u", "/tmp/bop/", getuniqueid(), file_no);
  if (written > sizeof(path) || written < 0) {
    noomr_perror("Unable to write the output path");
    abort();
  }

  int fd = open(path, O_RDWR | O_CREAT | O_SYNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
  if (fd == -1) {
    noomr_perror("Unable to create the file.");
    abort();
  }

  // need to truncate (grow -- poor naming) the file
  if (ftruncate(fd, size) < 0) {
    noomr_perror("NOOMR_MMAP: Unable to truncate/grow file");
    abort();
  }
  return fd;
}

int mmap_fd(unsigned name, size_t size) {
  return mmap_fd_bool(name, size, !speculating());
}

#define _GNU_SOURCE
#include <signal.h>

static noomr_page_t * map_info;

static void map_now(noomr_page_t *);

void map_handler(int _) {
  map_now(map_info);
}

bool mapped_handler(noomr_page_t * prev) {
  map_info = prev;
  typeof(&map_handler) old = signal(SIGSEGV, map_handler);
  if (old == SIG_ERR) {
    noomr_perror("Unable to install signal handler");
    abort();
  }
  __sync_synchronize();
  volatile noomr_page_t data = *prev;
  __sync_synchronize();
  signal(SIGSEGV, old);
}

bool is_mapped(void * ptr) {
  noomr_page_t * aligned = (noomr_page_t *) (((intptr_t) ptr) & ~(PAGE_SIZE  - 1));
  bool b = !(msync((void *) aligned, 1, MS_ASYNC) == -1 && errno == ENOMEM);
  // errno = 0;
  return b;
}

static inline size_t get_size_fd(int fd) {
  struct stat st;
  if (fstat(fd, &st) == -1) {
    noomr_perror("Unable to get size of file");
  }
  return st.st_size;
}

static inline size_t get_size_name(unsigned name) {
  char path[2048]; // 2 kB of path -- more than enough
  int written;
  written = snprintf(&path[0], sizeof(path), "%s%d/%u", "/tmp/bop/", getuniqueid(), name);
  if (written > sizeof(path) || written < 0) {
    noomr_perror("Unable to write the output path");
    abort();
  }
  int fd = open(path, O_RDONLY);
  if (fd == -1) {
    noomr_perror("Unable to open existing file");
  }
  size_t size = get_size_fd(fd);
  if (close(fd)) {
    noomr_perror("Unable to close fd used for temp size pole");
  }
  return size;
}

int mmap_existing_fd(unsigned name) {
  return mmap_fd_bool(name, get_size_name(name), false);
}

static void map_now (noomr_page_t * last_page) {
  int fd = mmap_existing_fd(last_page->next_pg_name);
  if (fd == -1) {
    // TODO handle error
    abort();
  } else {
    size_t size = get_size_fd(fd);
    if (mmap((void *) last_page->next_page, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, fd, 0) == MAP_FAILED) {
      // TODO handle error
      abort();
    }
    close(fd); // think can do here
    assert(is_mapped((void *) last_page->next_page));
    if ((noomr_page_t *) last_page->next_page == last_page) {
      abort();
    }
  }
}

// Map all missing pages
// Returns the last non-null page (eg. the one with the next page filed set to null)
noomr_page_t * map_missing_pages() {
  static noomr_page_t * cache = -1;
  noomr_page_t * start = cache == -1 ? &shared->next_page : cache;
  noomr_page_t * last_page = start;
  // These are for debugging, volatile so they can't be removed by compiler
  volatile int rounds = 0, maps = 0;
  if (start->next_page != NULL) {
    if (!is_mapped((void *) &start->next_page)) {
      maps++;
      map_now(start);
      assert(is_mapped((void *)  start->next_page));
    }
    for (last_page = start; last_page->next_page != NULL; last_page = (noomr_page_t *) last_page->next_page) {
      rounds++;
      if (!is_mapped((void *) last_page->next_page) && !mapped_handler(last_page)) {
        maps++;
        map_now(last_page);
      }
      if ((noomr_page_t *) last_page->next_page == last_page) {
        // last_page->next_page = NULL;
        // last_page->next_pg_name = 0;
        // break;
        abort();
      }
    }
  }
  return cache = last_page;
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
static inline noomr_page_t * allocate_noomr_page(int file_no, size_t minsize, int flags) {
  noomr_page_t * allocation = NULL;
  noomr_page_t alloc = {0}, expected = {0};
  // Reserve the resources in shared for this allocation
  int file_descriptor;
  size_t allocation_size = MAX(minsize, PAGE_SIZE);
  assert(allocation_size % PAGE_SIZE == 0);
  assert(shared != NULL);
  if (!speculating()) {
    flags |= MAP_ANONYMOUS;
  } else {
    flags &= ~MAP_PRIVATE;
    flags |= MAP_SHARED;
  }
  file_descriptor = mmap_fd(file_no, allocation_size);
  noomr_page_t * last_page;
  while (true) {
    last_page = map_missing_pages();
    /**
     * Let the kernel decide where to put the new page(s)
     * Tasks communicate by requiring the CAS to succeed. If it fails
     * then some other task allocated its page
     */
    allocation = mmap(NULL, allocation_size, PROT_READ | PROT_WRITE, flags, file_descriptor, 0);
    if (allocation == MAP_FAILED) {
      noomr_perror("Unable to set up mmap page");
      continue;
    }
#ifdef MANUAL_ZERO
    memset(allocation, 0, allocation_size);
#endif
    allocation->next_page =  NULL;
    allocation->next_pg_name = 0;

    alloc.next_page = (struct noomr_page_t *) allocation;
    alloc.next_pg_name = file_no;
    if (last_page == (volatile noomr_page_t *) last_page->next_page) {
      abort();
    }
    if (__sync_bool_compare_and_swap(&last_page->combined, expected.combined, alloc.combined)) {
      break;
    } else {
      munmap(allocation, allocation_size);
    }
  }
#ifdef COLLECT_STATS
  __sync_add_and_fetch(&shared->total_alloc, allocation_size);
  __sync_add_and_fetch(&shared->number_mmap, 1);
#endif
  if (file_descriptor != -1) {
    if (close(file_descriptor)) {
      noomr_perror("Unable to close file descriptor");
    }
  }
  assert(last_page->next_page == allocation);
  assert(allocation->next_page != last_page);
  assert(allocation->next_page != allocation);
  return allocation;
}

void allocate_header_page() {
  const int file_no = !speculating() ? -1 : __sync_add_and_fetch(&shared->next_name, 1);
  header_page_t * headers = (header_page_t *) allocate_noomr_page(file_no, MAX(PAGE_SIZE, sizeof(header_page_t)), MAP_SHARED);
  _Static_assert(__builtin_offsetof(header_page_t, next_page) == 0, "Offset must be 0");
  if (headers == (header_page_t *) -1) {
    exit(-1);
  }
  bzero(headers, PAGE_SIZE);
  headers->next_free = 0;
  // Add increate_header_pgto the headers linked list
  if (shared->header_pg == NULL) {
    shared->header_pg = headers;
  } else {
    do {
      // Load the relevant data into a new struct
      headers->next_header = (volatile struct header_page_t *) shared->header_pg;
    } while (!__sync_bool_compare_and_swap(&shared->header_pg, headers->next_header, (struct header_page_t *) headers));
  }
  if (headers == headers->next_page.next_page) {
    abort();
  }
#ifdef COLLECT_STATS
  __sync_add_and_fetch(&shared->header_pages, 1);
#endif
}

huge_block_t * allocate_large(size_t size) {
  int file_no = !speculating() ? -1 : __sync_add_and_fetch(&shared->next_name, 1);
  // Align to a page size
  size_t alloc_size = PAGE_ALIGN((size + sizeof(huge_block_t)));
  assert(alloc_size > size);
  assert(alloc_size % PAGE_SIZE == 0);
  huge_block_t * block = (huge_block_t *) allocate_noomr_page(file_no, alloc_size, MAP_PRIVATE);
  if (block == (huge_block_t *) -1) {
    return NULL;
  }
  block->huge_block_sz = alloc_size;
  block->file_name = file_no;
  do {
    block->next_block = (volatile struct block_t *) shared->large_block;
  } while(!__sync_bool_compare_and_swap(&shared->large_block, block->next_block, block));
  if (block == (huge_block_t *) block->next_page.next_page) {
    abort();
  }
#ifdef COLLECT_STATS
  __sync_add_and_fetch(&shared->huge_allocations, 1);
#endif
  return block;
}
