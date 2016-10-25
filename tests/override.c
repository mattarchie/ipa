#include <stdlib.h>
#include <stdio.h>
#include "noomr.h"

extern void * noomr_malloc(size_t);

bool speculating() {
  return false;
}

int main() {
  int * payload = calloc(sizeof(int), 1);
  printf("Noomr payload %p\n", payload);
  *payload = 42;
  printf("Override allocation worked\n");
  free(payload);
  return 0;
}
