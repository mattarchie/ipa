#include "bomalloc.h"

#ifdef BOMALLOC_SYSTEM_ALLOC
#if defined(__GNUC__) && !defined(NO_HOOK)
#include <malloc.h>

static void * bomalloc_malloc_hook (size_t size, const void * c){
  return bomalloc_malloc(size);
}

static void bomalloc_free_hook (void* payload, const void * c){
  bomalloc_free(payload);
}

static void * bomalloc_realloc_hook(void * payload, size_t size, const void * c) {
  return bomalloc_realloc(payload, size);
}

static void * bomalloc_calloc_hook(size_t a, size_t b, const void * c) {
  return bomalloc_calloc(a, b);
}

static void bomalloc_dehook() {
  __malloc_hook = NULL;
  __free_hook = NULL;
  __realloc_hook = NULL;
#ifdef __calloc_hook
  __calloc_hook = NULL;
#endif

}

void __attribute__((constructor)) bomalloc_hook() {
  atexit(bomalloc_dehook);
  __malloc_hook = bomalloc_malloc_hook;
  __free_hook = bomalloc_free_hook;
  __realloc_hook = bomalloc_realloc_hook;
#ifdef __calloc_hook
  __calloc_hook = bomalloc_calloc_hook;
#endif
}

#else

void * malloc(size_t t) {
  return bomalloc_malloc(t);
}
void free(void * p) {
  bomalloc_free(p);
}
size_t malloc_usable_size(void * p) {
  return bomalloc_usable_space(p);
}
void * realloc(void * p, size_t size) {
  return bomalloc_realloc(p, size);
}
void * calloc(size_t a, size_t b) {
  return bomalloc_calloc(a, b);
}
#endif
#endif
