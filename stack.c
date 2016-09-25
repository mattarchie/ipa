#include <stdlib.h>
#include <stdbool.h>
#include "stack.h"

stack_t * new_stack() {
  return calloc(1, sizeof(stack_t));
}

node_t * pop(volatile stack_t * stack) {
  while (true) {
    stack_t expected = *stack;
    if (expected.head == NULL) {
      return NULL;
    } else {
      stack_t new_stack;
      new_stack.age = expected.age + 1;
      new_stack.head = (node_t *) expected.head->next;
      if (__sync_bool_compare_and_swap(&stack->combined,
                                        expected.combined,
                                        new_stack.combined)) {
        return expected.head;
      }
    }
  }
}

void push(volatile stack_t * stack, node_t * item) {
  stack_t new_stack, expected;
  do {
    expected = *stack;
    new_stack.age = expected.age + 1;
    new_stack.head = item;
    item->next = (struct node_t *) expected.head;
  } while(!__sync_bool_compare_and_swap(&stack->combined,
                                         expected.combined,
                                         new_stack.combined));
}

bool empty(volatile stack_t * stack) {
  return stack->head == NULL;
}
