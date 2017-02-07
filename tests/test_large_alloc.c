#include <stdlib.h>
#include <stdio.h>
#include "noomr.h"
#include "dummy.h"

int main() {
  int * payload = noomr_malloc(MAX_SIZE + 1);
  printf("Noomr payload %p\n", payload);
  *payload = 42;
  printf("Large allocation passed\n");
  noomr_free(payload);
  printf("Successfully freed large block\n");
  print_noomr_stats();
  return 0;
}
