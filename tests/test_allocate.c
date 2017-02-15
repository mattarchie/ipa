#include <stdlib.h>
#include <stdio.h>
#include "bomalloc.h"
#include "dummy.h"
#include "teardown.h"

int main() {
  int * payload = bomalloc_malloc(sizeof(int));
  printf("bomalloc payload %p\n", payload);
  *payload = 42;
  printf("Basic allocation test passed\n");
  bomalloc_free(payload);
  print_bomalloc_stats();
  return 0;
}
