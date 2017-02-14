#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include "bomalloc.h"
#include "bomalloc_utils.h"

#define NUM_ROUNDS 10
#define NUM_CHILDREN 2
#define PER_EACH (NUM_ROUNDS / NUM_CHILDREN)


// Random number generation based off of http://www.azillionmonkeys.com/qed/random.html
#define RS_SCALE (1.0 / (1.0 + RAND_MAX))


_Static_assert(NUM_ROUNDS % NUM_CHILDREN == 0, "Rounds must be divisible by children");

volatile bool spec = false;

void record_allocation(void * p, size_t s) {

}

bool speculating() {
  return spec;
}

int getuniqueid() {
  return (int) getpgid(getpid());
}

void __attribute__((noreturn)) child(int id)  {
  spec = true;
  size_t alloc_size = MAX_SIZE + sizeof(block_t) + 1;
  while (shared->dummy == 0) {
    ;
  }
  for (int rnd = PER_EACH * id; rnd < PER_EACH * (id + 1); rnd++) {
    int * payload = bomalloc_malloc(alloc_size);
    printf("rnd %d Allocated %p\n", rnd, payload);
    assert(bomalloc_usable_space(payload) >= alloc_size);
    *payload = 0xdeadbeef;
  }
  printf("Child %d exits\n", id);
  exit(0);
}

pid_t parent;

int main() {
  srand(0);
  spec = true;
  pid_t children[NUM_CHILDREN];
  parent = getpid();
  beginspec();
  for (int i = 0; i < NUM_CHILDREN; i++) {
    pid_t pid = fork();
    if (pid == 0) {
      child(i);
    } else if (pid == -1) {
      perror("Unable to fork process");
      exit(-1);
    }
    children[i] = pid;
  }
  shared->dummy = 1;
  int status;
  for (int i = 0; i < NUM_CHILDREN; i++) {
    if (waitpid(children[i], &status, 0) == -1) {
      perror("Unable to wait for child");
      exit(-1);
    }
    assert(WIFEXITED(status));
    assert(WEXITSTATUS(status) == 0);
  }
  endspec(true);
  spec = false;
  printf("Large spec allocation test passed!\n");
  print_bomalloc_stats();
}
