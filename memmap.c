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

int create_header_pg(int file_no) {
  if (!speculating()) {
    return -1;
  }
  char path[2048]; // 2 MB of path -- more than enough
  int written = snprintf(&path[0], sizeof(path), "%s%d%s%d", "/tmp/bop/", getpgid(getpid()), "/headers/", file_no);
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

void allocate_header_page() {
  header_page_t * headers = NULL;
  // Reserve the resources in shared for this allocation
  int file_no, file_descriptor;
  int mmap_page_no = __sync_add_and_fetch(&shared->number_mmap, 1); // Allocating one page
  char * destination = ((char*) MIN((void *) shared->firstpg, shared->first_huge)) + PAGE_SIZE * mmap_page_no;
  if (speculating()) {
    file_no = __sync_add_and_fetch(&shared->header_num, 1);
  } else {
    file_no = -1;
  }
  file_descriptor = create_header_pg(file_no);
  headers = mmap(destination, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, file_descriptor, 0);
  if ( (speculating() && headers != (header_page_t *) destination) || headers == (header_page_t *) -1) {
    perror("Unable to set up header page");
  }
  // Add into the headers linked list
  header_page_t * last_page = lastheaderpg();
  do {
    while (last_page->next != NULL) {
      last_page = (header_page_t *) last_page->next;
    }
  } while (__sync_bool_compare_and_swap(&last_page->next, NULL, headers));
}

void* allocate_large(size_t size) {
  //TODO implement
  return NULL;
}
