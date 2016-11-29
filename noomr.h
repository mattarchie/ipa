#ifndef __HAVE_ALLOC_H
#define __HAVE_ALLOC_H
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <assert.h>
#include "stack.h"


#if __WORDSIZE == 64
#define ALIGNMENT (8) // I think?
#define MAX_CLASSES (8*sizeof(size_t) - (8 * SIZE_OFFSET))
#elif __WORDSIZE == 32
#define ALIGNMENT (4)
#define MAX_CLASSES 16
#else
#error "Need 32 or 64 b wordsize"
#endif

// Header size macros
#define NUM_CLASSES (MAX_CLASSES)
#define SIZE_OFFSET (5)
#define CLASS_TO_SIZE(x) (1 << ((x) + SIZE_OFFSET)) // to actually be determined later
#define LOG2(x) ((size_t) (8*sizeof (typeof(x)) - __builtin_clzll((x)) - 1))
#define SIZE_TO_CLASS(x) (class_for_size(x))
#define MAX_SIZE CLASS_TO_SIZE(((NUM_CLASSES) - 1))
#define MAX_REQUEST (ALIGN(MAX_SIZE - sizeof(block_t)))


#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))
#define PAGE_ALIGN(size) (((size) + (PAGE_SIZE-1)) & ~(PAGE_SIZE-1))


#define PAGE_SIZE 4096 //default Linux


static size_t llog2(size_t x) {
  return LOG2(x);
}

typedef union {
#ifdef NOOMR_ALIGN_HEADERS
  //for header alignment to cache line bounds
  void * ____padding[64/sizeof(void*)];
#endif
  struct {
    void * payload; // This be set once and not made globally visable until set
    size_t size; // This be set once and not made globally visable until set
    volatile node_t seq_next;
    volatile node_t spec_next;
  };
} header_t;

/*
  Header pages contain a list of (possibly unallocated) headers
  Each header correspondes to one block (contig. memory for prog data).
  Additionally, the header page struct keeps track of the next unallocated
  header index within this page.



  A header page is defined to be no more than PAGE_SIZE in length,
  and to fit the maximum number of headers as possible.
*/

#define HEADERS_PER_PAGE ((PAGE_SIZE - \
                          (sizeof(volatile struct header_page_t *) + sizeof(size_t)))  \
                          / sizeof(header_t))

//TODO need to include the ## (in path) for the next h pg
// when looking up a header need to verify that the target is mapped in memory
// this will use the same corrdination mechanisms as large allocations to ensure no conflicts
typedef struct {
  volatile struct header_page_t * next;
  // unsigned number; // to be used used to help minimize the number of header pages?
  size_t next_free;
  header_t headers[HEADERS_PER_PAGE];
} header_page_t;

typedef union {
  header_t * header;
  struct {
    size_t huge_block_sz; //Note: this includes the block_t space
    volatile void * next_block;
    int file_name;
  };
} block_t;


// Used to collect statics with minimal cache impact
typedef unsigned line_int_t __attribute__ ((aligned (64))); //64B aligned int

// Allocated at the start of the program in shared mem.
typedef struct {
#ifdef COLLECT_STATS
  volatile line_int_t allocations;
  volatile line_int_t frees;
  volatile line_int_t sbrks;
  volatile unsigned allocs_per_class[NUM_CLASSES];
  volatile line_int_t huge_allocations;
  volatile line_int_t header_pages;
  volatile size_t total_alloc; // total space allocated from the system (heap + mmap)
#endif
  volatile noomr_stack_t seq_free[NUM_CLASSES]; // sequential free list
  volatile noomr_stack_t spec_free[NUM_CLASSES]; // speculative free list
  volatile size_t base; //where segment begins (cache)
  volatile size_t spec_growth; // grow (B) done by spec group
  volatile unsigned header_num; // next header file to use
  volatile unsigned large_num; // next file for large allocation
  volatile header_page_t * header_pg; // the address of the first header mmap page
  volatile block_t * large_block; // pointer into the list of large blocks
  volatile size_t number_mmap; // how many pages where mmaped (headers & large)
  volatile int dummy; // used in unit tests
} shared_data_t;

// Function prototypes

// If compiled with the approiate flags, print the stats collected
// during run time
void print_noomr_stats(void);
bool speculating(void);
void noomr_init(void);

// Utility functions
static block_t * getblock(void * user_payload) {
  return (block_t *) (((char*) user_payload) - sizeof(block_t));
}

static void * getpayload(volatile block_t * block) {
  return (void *) (((char*) block) + sizeof(block_t));
}


#define SPEC_ALLOC_B (1<<0)
#define SEQ_ALLOC_B (1<<1)

static inline void * payload(volatile header_t * header) {
  return (void *) (((size_t) header->payload) & ~(SPEC_ALLOC_B | SEQ_ALLOC_B));
}

static inline void record_mode_alloc(volatile header_t * header) {
  if (speculating()) {
    __sync_fetch_and_or((int *) &header->payload, SPEC_ALLOC_B);
  } else {
    __sync_fetch_and_or((int *) &header->payload, SEQ_ALLOC_B);
  }
}

static inline bool seq_alloced(volatile header_t * header) {
  return (((size_t) header->payload) & SEQ_ALLOC_B) != 0;
}

static inline bool spec_alloced(volatile header_t * header) {
  return (((size_t) header->payload) & SPEC_ALLOC_B) != 0;
}

static inline void record_mode_free(volatile header_t * header) {
  if (speculating()) {
    __sync_fetch_and_and((int *) &header->payload, ~SPEC_ALLOC_B);
  } else {
    __sync_fetch_and_and((int *) &header->payload, ~SEQ_ALLOC_B);
  }
}

// Functions to convert between node links to the header
#define container_of(ptr, type, member) ({ \
                const typeof( ((type *)0)->member ) *__mptr = (ptr); \
                (type *)( (char *)__mptr - __builtin_offsetof(type,member) );})

static inline volatile header_t * spec_node_to_header(volatile node_t * node) {
  return container_of(node, volatile header_t, spec_next);
}

static inline volatile header_t * seq_node_to_header(volatile node_t * node) {
  return container_of(node, volatile header_t, seq_next);
}

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

void * noomr_malloc(size_t);
void * noomr_calloc(size_t, size_t);
void * noomr_realloc(void *, size_t);
void noomr_free(void *);
size_t noomr_usable_space(void *);

void beginspec(void);
void endspec(void);

void record_allocation(void *, size_t);

void noomr_teardown(void);
#endif
