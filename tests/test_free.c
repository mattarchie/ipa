#include <stdlib.h>
#include <stdio.h>
#include "bomalloc.h"
#include "dummy.h"
#include "teardown.h"

int main() {
  int * payload = bomalloc(sizeof(int));
  printf("bomalloc payload %p\n", payload);
  *payload = 42;
  bofree(payload);
  printf("Basic free test passed\n");
  return 0;
}
