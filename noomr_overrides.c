#include "noomr.h"

#ifdef NOOMR_SYSTEM_ALLOC
#if defined(__GNUC__) && !defined(NO_HOOK)
#include <malloc.h>

static void * noomr_malloc_hook (size_t size, const void * c){
  return noomr_malloc(size);
}

static void noomr_free_hook (void* payload, const void * c){
  noomr_free(payload);
}

static void * noomr_realloc_hook(void * payload, size_t size, const void * c) {
  return noomr_realloc(payload, size);
}

static void * noomr_calloc_hook(size_t a, size_t b, const void * c) {
  return noomr_calloc(a, b);
}

static void noomr_dehook() {
  __malloc_hook = NULL;
  __free_hook = NULL;
  __realloc_hook = NULL;
#ifdef __calloc_hook
  __calloc_hook = NULL;
#endif

}

void __attribute__((constructor)) noomr_hook() {
  atexit(noomr_dehook);
  __malloc_hook = noomr_malloc_hook;
  __free_hook = noomr_free_hook;
  __realloc_hook = noomr_realloc_hook;
#ifdef __calloc_hook
  __calloc_hook = noomr_calloc_hook;
#endif
}

#else

void * malloc(size_t t) {
  return noomr_malloc(t);
}
void free(void * p) {
  noomr_free(p);
}
size_t malloc_usable_size(void * p) {
  return noomr_usable_space(p);
}
void * realloc(void * p, size_t size) {
  return noomr_realloc(p, size);
}
void * calloc(size_t a, size_t b) {
  return noomr_calloc(a, b);
}
#endif
#endif
