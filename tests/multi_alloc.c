#include <stdlib.h>
#include <stdio.h>
#include "noomr.h"

extern void * noomr_malloc(size_t);
extern void * noomr_free(void *);

bool speculating() {
  return false;
}
#define NUM_ROUNDS 5000

size_t rand_num_ints(){
  int max;
  for(max = 1; sizeof(int) * max < MAX_SIZE - sizeof(block_t); max++) {
    ;
  }
  return rand() % (max + 1 - 0) + 0;
}

int main() {
  int rnd, check;

  srand(0);

  int * p1 = noomr_malloc(sizeof(int));
  int * p2 = noomr_malloc(sizeof(int));
  assert(p1 != p2);
  int * ptrs[NUM_ROUNDS] = {NULL};

  for (rnd = 0; rnd < NUM_ROUNDS; rnd++) {
    int * payload = noomr_malloc(rand_num_ints());
    ptrs[rnd] = payload;
    for (check = 0; check < rnd; check++) {
      if (ptrs[check] == payload) {
        fprintf(stderr, "Duplicate allocation found");
        exit(-1);
      }
    }
    *payload = 42;
  }
  printf("Basic allocation test passed\n");
  print_noomr_stats();
}
