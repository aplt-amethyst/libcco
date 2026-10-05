#ifndef CNT_CCO_INTERNAL_ARENA_H
#define CNT_CCO_INTERNAL_ARENA_H

#include <stddef.h>

typedef struct cco_arena_block_s cco_arena_block_t;

struct cco_arena_block_s
{
    cco_arena_block_t* next;
    size_t capacity;
    size_t used;
    unsigned char data[];
};

typedef struct cco_arena_s
{
    cco_arena_block_t* head;
    cco_arena_block_t* curr;
    size_t total_allocated;
    size_t max_allocation_limit;
} cco_arena_t;

/* Initializes the arena. max_limit is the maximum total bytes allowed. */
void cco_arena_init(cco_arena_t* a, size_t max_limit);

/* Resets the arena for reuse without freeing underlying memory. */
void cco_arena_reset(cco_arena_t* a);

/* Allocates n bytes from the arena. Returns NULL on OOM or limit exceeded. */
void* cco_arena_alloc(cco_arena_t* restrict a, size_t n);

/* Destroys the arena, freeing all blocks. */
void cco_arena_destroy(cco_arena_t* a);

#endif /* CNT_CCO_INTERNAL_ARENA_H */
