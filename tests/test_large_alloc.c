#include <stdlib.h>
#include <stdio.h>
#include "noomr.h"

extern void * noomr_malloc(size_t);
extern void noomr_free(void*);

bool speculating() {
  return false;
}

int main() {
  int * payload = noomr_malloc(MAX_SIZE + 1);
  printf("Noomr payload %p\n", payload);
  *payload = 42;
  printf("Large allocation passed\n");
  noomr_free(payload);
  printf("Successfully freed large block\n");
  return 0;
}
