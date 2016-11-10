#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200112
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <error.h>
#include <unistd.h>
#include <errno.h>
#include "noomr.h"
// Require placing data in user-supplied buffer

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

  write(STDOUT_FILENO, &master_buffer[0], tsize);
}
