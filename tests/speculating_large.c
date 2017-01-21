#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include "noomr.h"

#define NUM_ROUNDS 50

// Random number generation based off of http://www.azillionmonkeys.com/qed/random.html
#define RS_SCALE (1.0 / (1.0 + RAND_MAX))

volatile bool spec = false;

void record_allocation(void * p, size_t s) {

}

bool speculating() {
  return spec;
}

int getuniqueid() {
  return (int) getpgid(getpid());
}

int main() {
  int * sequential = noomr_malloc(sizeof(int));
  int * ptrs[NUM_ROUNDS] = {NULL};
  ptrs[0] = sequential;
  beginspec();
  spec = true;


  int rnd, check;
  size_t alloc_size;

  srand(0);

  for (rnd = 1; rnd < NUM_ROUNDS; rnd++) {
    alloc_size = MAX_SIZE + sizeof(block_t) + 1;
    int * payload = noomr_malloc(alloc_size);
    printf("rnd %d Allocated %p\n", rnd, payload);
    assert(noomr_usable_space(payload) >= alloc_size);
    ptrs[rnd] = payload;
    for (check = 0; check < rnd; check++) {
      if (ptrs[check] == payload) {
        fprintf(stderr, "Duplicate allocation found indexes %d %d\n", rnd, check);
        exit(-1);
      }
    }
    *payload = 0xdeadbeef;
  }
  endspec();
  printf("Small spec allocation test passed! No duplicate allocations detected\n");
  print_noomr_stats();
}
