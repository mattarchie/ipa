#include <stdint.h>

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
