#ifndef ___NOOMR_STACK_H
#define ___NOOMR_STACK_H
#include <stdlib.h>
#include <stdint.h>

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

#endif
