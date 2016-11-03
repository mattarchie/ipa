#include <stdlib.h>
#include <stdio.h>
#include "noomr.h"

extern void * noomr_malloc(size_t);
extern size_t noomr_usable_space(void*);

bool speculating() {
  return false;
}
#define NUM_ROUNDS 50000

// Random number generation based off of http://www.azillionmonkeys.com/qed/random.html
#define RS_SCALE (1.0 / (1.0 + RAND_MAX))

size_t random_class () {
    double d;
    do {
       d = (((rand () * RS_SCALE) + rand ()) * RS_SCALE + rand ()) * RS_SCALE;
    } while (d >= 1); /* Round off */
    return d * NUM_CLASSES;
}

size_t uniform_size_class() {
  return ALIGN(CLASS_TO_SIZE(random_class()) - sizeof(block_t));
}

int main() {
  int rnd, check;
  size_t alloc_size;

  srand(0);

  int * p1 = noomr_malloc(sizeof(int));
  int * p2 = noomr_malloc(sizeof(int));
  assert(p1 != p2);
  int * ptrs[NUM_ROUNDS] = {NULL};

  for (rnd = 0; rnd < NUM_ROUNDS; rnd++) {
    alloc_size = uniform_size_class();
    int * payload = noomr_malloc(alloc_size);
    assert(noomr_usable_space(payload) >= alloc_size);
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
