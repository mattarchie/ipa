#include <stdlib.h>
#include <stdio.h>
#include "bomalloc.h"
#include "dummy.h"
#include "teardown.h"

int main() {
  int * payload = bomalloc(sizeof(int));
  printf("bomalloc payload %p\n", payload);
  *payload = 42;
  printf("Basic allocation test passed\n");
  bofree(payload);
  print_bomalloc_stats();
  return 0;
}
