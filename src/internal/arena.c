#include "arena.h"

#include <stdint.h>
#include <stdlib.h>

#define ARENA_DEFAULT_BLOCK_SIZE (16 * 1024)

void cco_arena_init(cco_arena_t* a, size_t max_limit)
{
    if (a)
    {
        a->head = NULL;
        a->curr = NULL;
        a->total_allocated = 0;
        a->max_allocation_limit = max_limit;
    }
}

void cco_arena_reset(cco_arena_t* a)
{
    if (a)
    {
        cco_arena_block_t* b = a->head;
        while (b)
        {
            b->used = 0;
            b = b->next;
        }
        a->curr = a->head;
        a->total_allocated = 0;
    }
}

static cco_arena_block_t* cco_arena_alloc_block(size_t min_capacity)
{
    size_t capacity = ARENA_DEFAULT_BLOCK_SIZE;
    if (min_capacity > capacity)
    {
        capacity = min_capacity;
    }

    /* Check for overflow in struct size + capacity */
    if (SIZE_MAX - sizeof(cco_arena_block_t) < capacity)
    {
        return NULL;
    }

    cco_arena_block_t* block =
        (cco_arena_block_t*)malloc(sizeof(cco_arena_block_t) + capacity);
    if (block)
    {
        block->next = NULL;
        block->capacity = capacity;
        block->used = 0;
    }
    return block;
}

void* cco_arena_alloc(cco_arena_t* restrict a, size_t n)
{
    if (!a || n == 0)
    {
        return NULL;
    }

    /* Alignment padding (8 bytes alignment) */
    size_t padding = (8 - (n % 8)) % 8;
    if (SIZE_MAX - padding < n)
    {
        return NULL;
    }
    size_t aligned_n = n + padding;

    /* Check total limit */
    if (a->max_allocation_limit > 0 &&
        SIZE_MAX - a->total_allocated < aligned_n)
    {
        return NULL;
    }
    if (a->max_allocation_limit > 0 &&
        a->total_allocated + aligned_n > a->max_allocation_limit)
    {
        return NULL; /* Limit exceeded */
    }

    if (!a->curr || a->curr->used + aligned_n > a->curr->capacity)
    {
        /* Try next block if available and large enough */
        if (a->curr && a->curr->next && a->curr->next->capacity >= aligned_n) {
            a->curr = a->curr->next;
        } else {
            /* Allocate new block */
            cco_arena_block_t* new_block = cco_arena_alloc_block(aligned_n);
            if (!new_block)
            {
                return NULL;
            }
            if (a->curr) {
                new_block->next = a->curr->next;
                a->curr->next = new_block;
                a->curr = new_block;
            } else {
                a->head = new_block;
                a->curr = new_block;
            }
        }
    }

    void* ptr = a->curr->data + a->curr->used;
    a->curr->used += aligned_n;
    a->total_allocated += aligned_n;

    return ptr;
}

void cco_arena_destroy(cco_arena_t* a)
{
    if (a)
    {
        cco_arena_block_t* curr = a->head;
        while (curr)
        {
            cco_arena_block_t* next = curr->next;
            free(curr);
            curr = next;
        }
        a->head = NULL;
        a->total_allocated = 0;
    }
}
