#include <stdlib.h>
#include <stdio.h>
#include "bomalloc.h"
#include "dummy.h"

int main() {
  int * payload = calloc(sizeof(int), 1);
  printf("bomalloc payload %p\n", payload);
  *payload = 42;
  printf("Override allocation worked\n");
  free(payload);
  printf("Allocation freed\n");
  print_bomalloc_stats();
  return 0;
}
