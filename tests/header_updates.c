#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

#include "noomr.h"
#include "dummy.h"


void parent(int * payload1, pid_t child, void* value) {
  while (getblock(payload1)->header->spec_next.next != value) {
    ; // do nothing
  }
  assert(getblock(payload1)->header->spec_next.next == value);
  kill(child, SIGINT);
  printf("Header update test succeded\n");
  exit(0);
}

int main() {
  int * payload1 = noomr_malloc(sizeof(int));
  void * value = main;
  pid_t child = fork();
  if (child == 0) {
    // in the child process -- update something and then quit
    block_t * block = getblock(payload1);
    block->header->spec_next.next = value; // some field in the header -- shared memory
    // while(1) {
    //   ; // do nothing
    // }
    printf("Child exits\n");
  } else {
    parent(payload1, child, value);
  }
}
