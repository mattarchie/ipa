#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <assert.h>
#include "noomr.h"
#include "memmap.h"
#include "stack.h"


#define max(a, b) ((a) > (b) ? (a) : (b))

shared_data_t * shared;
size_t my_growth;

extern bool speculating(void);

bool out_of_range(void * payload) {
  return payload < (void*) shared->base || payload >= sbrk(0);
}

static header_t * lookup_header(void * user_payload) {
  return out_of_range(user_payload) ? NULL : getblock(user_payload)->header;
}

static inline stack_t * get_stack_index(size_t index) {
  assert(index >= 0 && index < NUM_CLASSES);
  return speculating() ? &shared->spec_free[index] : &shared->seq_free[index];
}

void synch_lists() {
  size_t i;
  volatile header_page_t * page;
  for (page = shared->header_pg; page != NULL; page = (header_page_t *) page->next) {
    for (i = 0; i < page->next_free; i++) {
      page->headers[i].spec_next = page->headers[i].seq_next;
    }
  }
}

// This step can be eliminated by sticking the two items in an array and swaping the index
// mapping to spec / seq free lists
// pre-spec is still needed OR 3x wide CAS operations -- 2 for the pointers
// (which need to be adjacent to each other) and another for the counter
static void promote_list() {
  size_t i;
  volatile header_page_t * page;
  for (page = shared->header_pg; page != NULL; page = (header_page_t *) page->next) {
    for (i = 0; i < page->next_free; i++) {
      page->headers[i].seq_next = page->headers[i].spec_next;
    }
  }
}

static header_page_t * payload_to_header_page(void * payload) {
  if (out_of_range(payload)) {
    return NULL;
  } else {
    block_t * block = getblock(payload);
    return  (header_page_t *) (((intptr_t) block) & ~(PAGE_SIZE - 1));
  }
}

size_t noomr_usable_space(void * payload) {
  if (out_of_range(payload)) {
    return getblock(payload)->huge_block_sz - sizeof(block_t);
  } else {
    return getblock(payload)->header->size - sizeof(block_t);
  }
}


/**
 NOTE: when in spec the headers pointers will not be
 into all process's private address space
 To solve this, promise all spec-growth regions
*/
static void map_headers(char * begin, size_t index, size_t num_blocks) {
  size_t i, header_index = HEADERS_PER_PAGE + 1;
  volatile header_page_t * page;
  size_t block_size = CLASS_TO_SIZE(index);
  stack_t * spec_stack = &shared->spec_free[index];
  stack_t * seq_stack = &shared->seq_free[index];
  assert(block_size == ALIGN(block_size));

  for (i = 0; i < num_blocks - 1; i++) {
    while(header_index >= (HEADERS_PER_PAGE - 1)) {
      for (page = shared->header_pg; page != NULL; page = (volatile header_page_t *) page->next) {
        if (page->next_free >= HEADERS_PER_PAGE) {
          continue;
        }
        header_index = __sync_add_and_fetch(&page->next_free, 1);
        if (header_index < HEADERS_PER_PAGE) {
          break;
        } else {
          __sync_add_and_fetch(&page->next_free, -1);
        }
      }
      if (page == NULL) {
        allocate_header_page();
      }

    }
    block_t * block = (block_t *) (&begin[i * block_size]);

    page->headers[header_index].size = block_size;
    // mmaped pages are padded with zeros, set NULL anyways
    page->headers[header_index].spec_next.next = NULL;
    page->headers[header_index].seq_next.next = NULL;
    block->header = (header_t * ) &page->headers[header_index];
    page->headers[header_index].payload = getpayload(block);
    __sync_synchronize(); // mem fence
    push(seq_stack, (node_t * ) &page->headers[header_index].seq_next);
    push(spec_stack, (node_t * ) &page->headers[header_index].spec_next);
    // get the i-block
  }
}

static inline void set_large_perm(int flags) {
  block_t * block;
  for (block = (block_t *) shared->large_block; block != NULL; block = block->next_block) {
    int fd = create_large_pg(block->file_name);
    fsync(fd);
    if (!mmap(block, block->huge_block_sz, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0)) {
      perror("Unable to reconfigure permissions.");
    }
  }
}

void beginspec() {
  my_growth = 0;
  synch_lists();
  set_large_perm(MAP_PRIVATE);
}

void endspec() {
  promote_list();
  set_large_perm(MAP_SHARED);
}

void noomr_init() {
  shared = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  bzero(shared, PAGE_SIZE);
  shared->number_mmap = 1;
  shared->base = (size_t) sbrk(0); // get the starting address
}

#define container_of(ptr, type, member) ({ \
                const typeof( ((type *)0)->member ) *__mptr = (ptr); \
                (type *)( (char *)__mptr - __builtin_offsetof(type,member) );})

static inline header_t * convert_head_mode_aware(node_t * node) {
  return speculating() ? container_of(node, header_t, spec_next) :
                         container_of(node, header_t, seq_next);
}

void * inc_heap(size_t s) {
#ifdef COLLECT_STATS
  __sync_add_and_fetch(&shared->sbrks, 1);
#endif
  return sbrk(s);
}

size_t stack_for_size(size_t min_size) {
  size_t klass;
  for (klass = 1; CLASS_TO_SIZE(klass) < min_size && klass < NUM_CLASSES; klass++) {
    ;
  }
  if (klass >= NUM_CLASSES) {
    return -1;
  }
  return CLASS_TO_SIZE(klass);
}

void * noomr_malloc(size_t size) {
  header_t * header;
  stack_t * stack;
  node_t * stack_node;
  size_t aligned = ALIGN(size + sizeof(block_t));
  if (shared == NULL) {
    noomr_init();
  }
#ifdef COLLECT_STATS
  __sync_add_and_fetch(&shared->allocations, 1);
#endif
  if (size > MAX_SIZE) {
    return (void*) allocate_large(aligned);
  } else {
    alloc: stack = get_stack_index(SIZE_TO_CLASS(aligned));
    stack_node = pop(stack);
    if (! stack_node) {
      // Need to grow the space
      size_t size = align(aligned, CLASS_TO_SIZE(0));
      assert(size > 0);
      size_t index = SIZE_TO_CLASS(size);
      size_t blocks = max(1024 / size, 5);

      size_t my_region_size = size * blocks;
      if (speculating()) {
        // first allocate the extra space that's needed
        inc_heap(__sync_add_and_fetch(&shared->spec_growth, my_region_size) - my_growth);
        __sync_add_and_fetch(&my_growth, size * blocks);
      }
      // Grow the heap by the space needed for my allocations
      char * base = inc_heap(my_region_size);
      // now map headers for my new (private) address region
      map_headers(base, index, blocks);
      goto alloc;
    }
    header = convert_head_mode_aware(stack_node);
    assert(header->payload != NULL);
    return header->payload;
  }
}

void noomr_free(void * payload) {
#ifdef COLLECT_STATS
  __sync_add_and_fetch(&shared->frees, 1);
#endif
  if (out_of_range(payload)) {
    // A huge block is unmapped directly to kernel
    // This can be done immediately -- there is no re-allocation conflicts
    block_t * block = getblock(payload);
    if (munmap(block, block->huge_block_sz) == -1) {
      perror("Unable to unmap block");
    }
  } else {
    // TODO do I need to delay the free while speculating?
    //  there is no overwrite issue
    header_t * header = getblock(payload)->header;
    // look up the index & push onto the stack
    stack_t * stack = get_stack_index(SIZE_TO_CLASS(header->size));
    push(stack, speculating() ? &header->spec_next : &header->seq_next);
  }
}

void * noomr_calloc(size_t nmemb, size_t size) {
  void * payload = noomr_malloc(nmemb * size);
  memset(payload, 0, noomr_usable_space(payload));
  return payload;
}

void print_noomr_stats() {
#ifdef COLLECT_STATS
  int index;
  printf("NOOMR stats\n");
  printf("allocations: %u\n", shared->allocations);
  printf("frees: %u\n", shared->frees);
  printf("sbrks: %u\n", shared->sbrks);
  printf("huge allocations: %u\n", shared->huge_allocations);
  for (index = 0; index < NUM_CLASSES; index++) {
    printf("class %d allocations: %u\n", index, shared->allocs_per_class[index]);
  }
#endif
}

void * noomr_realloc(void * p, size_t size) {
  size_t original_size = noomr_usable_space(p);
  if (original_size >= size) {
    return p;
  } else {
    void * new_payload = noomr_malloc(size);
    memcpy(new_payload, p, original_size);
    noomr_free(p);
    return new_payload;
  }
}

#ifdef NOOMR_SYSTEM_ALLOC
#ifdef __GNUC__
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
