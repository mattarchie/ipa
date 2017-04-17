#include "ipa.h"

#ifdef BOMALLOC_SYSTEM_ALLOC
#if defined(__GNUC__) && !defined(NO_HOOK)
#include <malloc.h>

static void * ipa_hook (size_t size, const void * c){
  return ipa_malloc(size);
}

static void bofree_hook (void* payload, const void * c){
  bofree(payload);
}

static void * borealloc_hook(void * payload, size_t size, const void * c) {
  return borealloc(payload, size);
}

static void * bocalloc_hook(size_t a, size_t b, const void * c) {
  return bocalloc(a, b);
}

static void ipa_dehook() {
  __malloc_hook = NULL;
  __free_hook = NULL;
  __realloc_hook = NULL;
#ifdef __calloc_hook
  __calloc_hook = NULL;
#endif

}

void __attribute__((constructor)) ipa_hook() {
  atexit(ipa_dehook);
  __malloc_hook = ipa_hook;
  __free_hook = bofree_hook;
  __realloc_hook = borealloc_hook;
#ifdef __calloc_hook
  __calloc_hook = bocalloc_hook;
#endif
}

#else

void * malloc(size_t t) {
  return ipa_malloc(t);
}
void free(void * p) {
  bofree(p);
}
size_t malloc_usable_size(void * p) {
  return ipa_usable_space(p);
}
void * realloc(void * p, size_t size) {
  return borealloc(p, size);
}
void * calloc(size_t a, size_t b) {
  return bocalloc(a, b);
}
#endif
#endif
