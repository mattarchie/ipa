#include <stdlib.h>
#include <stdio.h>
#include "bomalloc.h"
#include "dummy.h"

int main() {
  int * payload = bomalloc_malloc(sizeof(int));
  printf("bomalloc payload %p\n", payload);
  *payload = 42;
  bomalloc_free(payload);
  printf("Basic free test passed\n");
  return 0;
}
