#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200112
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <error.h>
#include <unistd.h>
#include <errno.h>
#include "noomr.h"

void noomr_strerr(char * buffer, size_t len) {
  // For now, no error handling
  assert(strerror_r(errno, buffer, len) == 0);
}

// Get the
void noomr_perror(const char * msg) {
  char error_buffer[512]; // holds strerror
  char master_buffer[2048]; // final thing to write out
  // Get the error string
  noomr_strerr(&error_buffer[0], sizeof(error_buffer));
  size_t tsize = snprintf(&master_buffer[0], sizeof(master_buffer), "%s: %s\n", msg, &error_buffer[0]);
  // Get

  if (write(STDERR_FILENO, &master_buffer[0], tsize) == -1) {
    // TODO log the error somewhere where dynamically allocation won't break all of the things
  }
}

#include <locale.h>
#include <math.h>

void print_noomr_stats() {
#ifdef COLLECT_STATS
  shared_data_t snapshot = *shared;
  if (!isatty(fileno(stderr))) {
    return; // Do not print stats if output is redirected
  }
  int index;
  setlocale(LC_ALL,"");
  printf("NOOMR stats\n");
  printf("allocations: %u\n", snapshot.allocations);
  printf("frees: %u\n", snapshot.frees);
  printf("sbrks: %u\n", snapshot.sbrks);
  printf("huge allocations: %u\n", snapshot.huge_allocations);
  printf("header pages: %u\n", snapshot.header_pages);
  for (index = 0; index < NUM_CLASSES; index++) {
    printf("class %2d (%'10lu B) allocations: %u\n", index, CLASS_TO_SIZE(index), snapshot.allocs_per_class[index]);
  }
  printf("Total managed memory:\n\t%'lu B\n\t%'.0lf pages\n\t%'.2lf GB\n\t%'.2lf TB\n",
          (size_t) snapshot.total_alloc,
          ceil(((double) snapshot.total_alloc) / PAGE_SIZE),
          ((double) snapshot.total_alloc) / (1024L * 1024 * 1024),
#if __WORDSIZE == 64
          ((double) snapshot.total_alloc) / (1024L * 1024 * 1024 * 1024)
#else
          (double) 0
#endif
        );
#else
  printf("NOOMR not configured to collect statistics\n");
#endif
}
