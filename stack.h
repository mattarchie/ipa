#ifndef ___NOOMR_STACK_H
#define ___NOOMR_STACK_H
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#if __SIZEOF_POINTER__ == 8
// 64b pointers
#define combined_stack_t unsigned __int128
#define stack_age_t uint64_t
#else
#define combined_stack_t uint64_t
#define stack_age_t uint32_t
#endif

typedef struct {
  struct node_t * next;
} node_t;

typedef union {
  struct {
    node_t * head;
    stack_age_t age;
  };
  combined_stack_t combined;
} stack_t;


stack_t * new_stack(void);

node_t * pop(volatile stack_t *);

void push(volatile stack_t *, node_t *);

bool empty(volatile stack_t *);

inline stack_t atomic_stack_load(volatile stack_t * stack) {
  stack_t loaded;
  do {
    loaded.age = stack->age;
    loaded.head = stack->head;
  } while(!__sync_bool_compare_and_swap(&stack->combined,
                                        loaded.combined,
                                        loaded.combined));
  return loaded;
}

#endif
