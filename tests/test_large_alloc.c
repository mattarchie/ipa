#include <stdlib.h>
#include <stdio.h>
#include "bomalloc.h"
#include "dummy.h"
#include "teardown.h"

int main() {
  int * payload = bomalloc(MAX_SIZE + 1);
  printf("bomalloc payload %p\n", payload);
  *payload = 42;
  printf("Large allocation passed\n");
  bofree(payload);
  printf("Successfully freed large block\n");
  print_bomalloc_stats();
  return 0;
}
