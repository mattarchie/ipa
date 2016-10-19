#ifndef __HAVE_ALLOC_H
#define __HAVE_ALLOC_H
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include "stack.h"


// Header size macros
#define NUM_CLASSES (16)
#define SIZE_OFFSET (5)
#define CLASS_TO_SIZE(x) (1 << ((x) + SIZE_OFFSET)) // to actually be determined later
#define SIZE_TO_CLASS(x) ((size_t) x >> (SIZE_OFFSET + 1))
#define MAX_SIZE CLASS_TO_SIZE(((NUM_CLASSES) - 1))

#if __WORDSIZE == 64
#define ALIGNMENT (8) // I think?
#elif __WORDSIZE == 32
#define ALIGNMENT (4)
#else
#error "Need 32 or 64 b wordsize"
#endif

#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))



#define PAGE_SIZE 4096 //default Linux
#define HEADERS_PER_PAGE ((PAGE_SIZE - \
                          (sizeof(volatile struct header_page_t *) + sizeof(size_t)))  \
                          / sizeof(header_t))


typedef union {
#ifdef NOOMR_ALIGN_HEADERS
  //for header alignment to cache line bounds
  char ____padding[64];
#endif
  struct {
    node_t seq_next;
    node_t spec_next;
    void * payload;
    size_t size;
  };
  // do I need to set low-end bits for allocated in spec / seq ...?
  // yes -- look up header of freed payload to check when alloc in O(1) time
} header_t;


/*
  Header pages contain a list of (possibly unallocated) headers
  Each header correspondes to one block (contig. memory for prog data).
  Additionally, the header page struct keeps track of the next unallocated
  header index within this page.



  A header page is defined to be no more than PAGE_SIZE in length,
  and to fit the maximum number of headers as possible.
*/

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
  size_t huge_block_sz; //Note: this includes the block_t space
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
#endif
  stack_t seq_free[NUM_CLASSES]; // sequential free list
  stack_t spec_free[NUM_CLASSES]; // speculative free list
  size_t base; //where segment begins (cache)
  volatile size_t spec_growth; // grow (B) done by spec group
  volatile unsigned header_num; // next header file to use
  volatile unsigned large_num; // next file for large allocation
  volatile header_page_t * firstpg; // the address of the first header mmap page
  volatile void * first_huge; // where the next large page should be allocated
  volatile size_t number_mmap; // how many pages where mmaped (headers & large)
} shared_data_t;

// Function prototypes

// If compiled with the approiate flags, print the stats collected
// during run time
void print_noomr_stats(void);

// Utility functions
static block_t * getblock(void * user_payload) {
  return (block_t *) (((char*) user_payload) - sizeof(block_t));
}

static void * getpayload(block_t * block) {
  return (void *) (((char*) block) + sizeof(block_t));
}

static size_t align(size_t value, size_t alignment) {
  return (((value) + (alignment-1)) & ~(alignment-1));
}
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

#endif
