#include <stdlib.h>
#include <stdio.h>
#include "noomr.h"

extern void * noomr_malloc(size_t);

bool speculating() {
  return false;
}

int main() {
  int * payload = noomr_malloc(sizeof(int));
  printf("Noomr payload %p\n", payload);
  *payload = 42;
  printf("Basic allocation test passed\n");
  return 0;
}
