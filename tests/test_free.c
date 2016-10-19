#include <stdlib.h>
#include <stdio.h>
#include "noomr.h"

extern void * noomr_malloc(size_t);
extern void noomr_free(void*);

bool speculating() {
  return false;
}

int main() {
  int * payload = noomr_malloc(sizeof(int));
  printf("Noomr payload %p\n", payload);
  *payload = 42;
  noomr_free(payload);
  printf("Basic free test passed\n");
  return 0;
}
