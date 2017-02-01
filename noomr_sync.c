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
  assert(speculating()); // TODO -- probably special case this function
  noomr_init();
  my_growth = 0;
  map_missing_pages();
  synch_lists();
  // set_large_perm(MAP_PRIVATE);
}

void endspec() {
  assert(speculating()); // TODO -- probably special case this function
  if (my_growth < shared->spec_growth) {
    inc_heap(shared->spec_growth - my_growth);
  }
  map_missing_pages();
  free_delayed();
  promote_list();
  // set_large_perm(MAP_PRIVATE);
}

static inline void set_large_perm(int flags) {
  volatile huge_block_t * block;
  huge_block_t data = {0};
  for (block = (volatile huge_block_t *) shared->large_block; block != NULL; block = (volatile huge_block_t *)  block->next_block) {
    if (block->is_shared) {
      int fd = mmap_existing_fd(block->file_name);
      data = *block;
      fsync(fd);
      if (!mmap((void *) block, data.huge_block_sz, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0)) {
        noomr_perror("Unable to reconfigure permissions.");
      }
      close(fd);
      block->is_shared = false;
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
  for (page = shared->header_pg; page != NULL; page = (header_page_t *) page->next_header) {
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
  volatile header_page_t * page;
  map_missing_pages();
  // Set the heads of the stacks to the corresponding elements
  for (i = 0; i < NUM_CLASSES; i++) {
    if(shared->spec_free[i].head != NULL) {
      shared->seq_free[i].head = (node_t *) &spec_node_to_header(shared->spec_free[i].head)->seq_next;
    } else {
      shared->seq_free[i].head = NULL;
    }
  }
  // Set the stack elements
  for (page = shared->header_pg; page != NULL; page = (header_page_t *) page->next_header) {
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
