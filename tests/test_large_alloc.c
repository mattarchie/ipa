#include <stdlib.h>
#include <stdio.h>
#include "bomalloc.h"
#include "dummy.h"

int main() {
  int * payload = bomalloc_malloc(MAX_SIZE + 1);
  printf("bomalloc payload %p\n", payload);
  *payload = 42;
  printf("Large allocation passed\n");
  bomalloc_free(payload);
  printf("Successfully freed large block\n");
  print_bomalloc_stats();
  return 0;
}
