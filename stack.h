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


static inline bool empty(volatile stack_t * stack) {
  return stack->head == NULL;
}

static inline void init_stack(stack_t * stack) {
  stack->head = NULL;
  stack->age = 0;
}

static inline stack_t atomic_stack_load(volatile stack_t * stack) {
  stack_t loaded;
  do {
    loaded = *stack;
  } while(!__sync_bool_compare_and_swap(&stack->combined,
                                        loaded.combined,
                                        loaded.combined));
  return loaded;
}
// The function below can be used as a more light-weight 'semi-atomic' load without spinning
//Loading the variable used to prevent the ABA problem first is suffient -- read barrier to prevent proc. reordering
static inline stack_t naba_load(volatile stack_t * stack) {
  stack_t load;
  load.age = stack->age;
  __sync_synchronize(); // Need a barrier -- really only read but no good GCC binding
  load.head = stack->head;
  return load;

}

static inline void push(volatile stack_t * stack, node_t * item) {
  stack_t new_stack, expected;
  do {
    expected = naba_load(stack);
    new_stack.age = expected.age + 1;
    new_stack.head = item;
    item->next = (struct node_t *) expected.head;
  } while(!__sync_bool_compare_and_swap(&stack->combined,
                                         expected.combined,
                                         new_stack.combined));
}

static inline node_t * pop(volatile stack_t * stack) {
  while (true) {
    stack_t expected = *stack;
    if (expected.head == NULL) {
      return NULL;
    } else {
      stack_t new_stack;
      new_stack.age = expected.age + 1;
      __sync_synchronize(); //More fine-grain than the naba load -- let the 1 happen 'whenever'
      new_stack.head = (node_t *) expected.head->next;
      if (__sync_bool_compare_and_swap(&stack->combined,
                                        expected.combined,
                                        new_stack.combined)) {
        return expected.head;
      }
    }
  }
}

static inline void push_ageless(stack_t * stack, node_t * node) {
  node->next = (struct node_t *) stack->head;
  stack->head = node;
}

static inline node_t * pop_ageless(stack_t * stack) {
  if (stack->head == NULL) {
    return NULL;
  } else {
    node_t * pop = stack->head;
    stack->head = (node_t *) pop->next;
    stack->age++;
    return pop;
  }
}

static inline stack_t * new_stack() {
  return calloc(1, sizeof(stack_t));
}

#endif
