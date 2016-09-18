#ifndef ___NOOMR_STACK_H
#include <stdint.h>
#include <stdlib.h>

typedef struct {
  struct node_t * next;
} node_t;

typedef struct {
  union {
    struct {
      uint64_t age;
      node_t * head;
    };
    __int128 combined;
  };
} stack_t;


stack_t * new_stack(void);

node_t * pop(volatile stack_t *);

void push(volatile stack_t *, node_t *);

#endif
