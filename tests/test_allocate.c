#include <stdlib.h>
#include <stdio.h>
#include "noomr.h"
#include "dummy.h"

int main() {
  int * payload = noomr_malloc(sizeof(int));
  printf("Noomr payload %p\n", payload);
  *payload = 42;
  printf("Basic allocation test passed\n");
  noomr_free(payload);
  print_noomr_stats();
  return 0;
}
