#ifndef BCD_PQ_H
#define BCD_PQ_H

#include <stdbool.h>

/**
 * Minimal binary min-heap priority queue over (cell_index, f_cost) pairs.
 * Used by A* in bcd_pathfinding.c.
 *
 * Capacity grows automatically. The caller must call bcd_pq_free() when done.
 */

typedef struct
{
    int cell_index;
    float f_cost;
} bcd_pq_entry_t;

typedef struct
{
    bcd_pq_entry_t *data;
    int size;
    int capacity;
} bcd_pq_t;

void bcd_pq_init(bcd_pq_t *pq);
void bcd_pq_free(bcd_pq_t *pq);
bool bcd_pq_empty(const bcd_pq_t *pq);
void bcd_pq_push(bcd_pq_t *pq, int cell_index, float f_cost);

/**
 * Pops and returns the entry with the lowest f_cost.
 * Undefined behaviour if the queue is empty — check bcd_pq_empty() first.
 */
bcd_pq_entry_t bcd_pq_pop(bcd_pq_t *pq);

/**
 * Decrease-key: if cell_index is already in the heap with a higher f_cost,
 * update it. Otherwise behaves like push.
 * Linear scan — acceptable because BCD cell counts are small (< 1 000).
 */
void bcd_pq_decrease_key(bcd_pq_t *pq, int cell_index, float f_cost);

#endif // BCD_PQ_H
