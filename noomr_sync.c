#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

#include "stack.h"
#include "noomr.h"
#include "noomr_sync.h"
#include "noomr_utils.h"
#include "memmap.h"

extern size_t my_growth;
extern shared_data_t * shared;

void beginspec() {
  noomr_init();
  my_growth = 0;
  synch_lists();
  set_large_perm(MAP_PRIVATE);
}

void endspec() {
  if (my_growth < shared->spec_growth) {
    inc_heap(shared->spec_growth - my_growth);
  }
  promote_list();
  set_large_perm(MAP_PRIVATE);
  free_delayed();
}

static inline void set_large_perm(int flags) {
  volatile huge_block_t * block;
  for (block = (huge_block_t *) shared->large_block; block != NULL; block = block->next_block) {
    int fd = create_large_pg(block->file_name);
    fsync(fd);
    if (!mmap((void *) block, block->huge_block_sz, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0)) {
      noomr_perror("Unable to reconfigure permissions.");
    }
  }
}

void synch_lists() {
  size_t i;
  volatile header_page_t * page;
  // Set the heads of the stacks to the corresponding elements
  for (i = 0; i < NUM_CLASSES; i++) {
    if(shared->seq_free[i].head != NULL) {
      shared->spec_free[i].head = (node_t *) &seq_node_to_header(shared->seq_free[i].head)->spec_next;
    } else {
      shared->spec_free[i].head = NULL;
    }
  }

  // Set the stack elements
  for (page = shared->header_pg; page != NULL; page = (header_page_t *) page->next.next) {
    for (i = 0; i < MIN(HEADERS_PER_PAGE, page->next_free); i++) {
      if (!seq_alloced(&page->headers[i]) && page->headers[i].seq_next.next != NULL) {
        volatile header_t * next_header = seq_node_to_header((node_t*) page->headers[i].seq_next.next);
        page->headers[i].spec_next.next = (volatile struct node_t *) &next_header->spec_next;
      } else {
        page->headers[i].spec_next.next = NULL;
      }
    }
  }
}

// This step can (may?) be eliminated by sticking the two items in an array and swaping the index
// mapping to spec / seq free lists
// pre-spec is still needed OR 3x wide CAS operations -- 2 for the pointers
// (which need to be adjacent to each other) and another for the counter
void promote_list() {
  size_t i;
  volatile header_page_t * page, * prev = NULL;
  // Set the heads of the stacks to the corresponding elements
  for (i = 0; i < NUM_CLASSES; i++) {
    if(shared->spec_free[i].head != NULL) {
      shared->seq_free[i].head = (node_t *) &spec_node_to_header(shared->spec_free[i].head)->seq_next;
    } else {
      shared->seq_free[i].head = NULL;
    }
  }
  // Set the stack elements
  for (page = shared->header_pg; page != NULL; prev = page,  page = (header_page_t *) page->next.next) {
    // Ensure that PAGE is mapped in
    if (msync((void *) page, PAGE_SIZE, 0) == -1 && errno == ENOMEM) {
      // needs to be mapped in
      char path[2048]; // 2 kB of path -- more than enough
      int written;
      // ensure the directory is present
      written = snprintf(&path[0], sizeof(path), "%s%d%s%d", "/tmp/bop/", getuniqueid(), "/headers/", prev->next.next_file_num);
      if (written > sizeof(path) || written < 0) {
        noomr_perror("Unable to write directory name");
      }
      int fd = open(path, O_RDWR | O_CREAT | O_SYNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
      if (fd == -1) {
        noomr_perror("Unable to create the file.");
      }
      mmap((void *) prev->next.next, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_FIXED, fd, 0);
    }
    for (i = 0; i < MIN(HEADERS_PER_PAGE, page->next_free); i++) {
      assert(payload(&page->headers[i]) != NULL);
      if (!spec_alloced(&page->headers[i]) && page->headers[i].seq_next.next != NULL) {
        volatile header_t * next_header = spec_node_to_header((node_t *) page->headers[i].spec_next.next);
        page->headers[i].seq_next.next = (volatile struct node_t *) &next_header->seq_next;
      } else {
        page->headers[i].seq_next.next = NULL;
      }
    }
  }
}
