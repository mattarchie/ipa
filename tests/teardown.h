#ifndef __TEST_TEARDOWN
#define __TEST_TEARDOWN
#include "bomalloc.h"

static void __attribute__((destructor)) test_teardown() {
  bomalloc_teardown();
}

#endif
