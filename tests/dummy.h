#ifndef __NOOMR_TEST_DUMMY
#define __NOOMR_TEST_DUMMY
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>


bool speculating() {
  return false;
}

void record_allocation(void * p, size_t t) {

}

int getuniqueid() {
  return (int) getpgid(getpid());
}

#endif
