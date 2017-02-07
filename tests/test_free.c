#include <stdlib.h>
#include <stdio.h>
#include "noomr.h"
#include "dummy.h"

int main() {
  int * payload = noomr_malloc(sizeof(int));
  printf("Noomr payload %p\n", payload);
  *payload = 42;
  noomr_free(payload);
  printf("Basic free test passed\n");
  return 0;
}
