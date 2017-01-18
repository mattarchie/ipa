#ifndef _H_NOOMR_SYNC_H
#define _H_NOOMR_SYNC_H
void beginspec();
void endspec();
void synch_lists();
void promote_list();
void free_delayed(void);

static inline void set_large_perm();

extern void * inc_heap(size_t);
extern void noomr_init(void);


#endif
