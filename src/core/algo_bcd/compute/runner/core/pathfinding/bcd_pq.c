#include "bcd_pq.h"

#include <stdlib.h>
#include <string.h>

#define BCD_PQ_INITIAL_CAPACITY 16

/* ---------- helpers ---------------------------------------------------- */

static void swap_entries(bcd_pq_entry_t *a, bcd_pq_entry_t *b)
{
    bcd_pq_entry_t tmp = *a;
    *a = *b;
    *b = tmp;
}

static void sift_up(bcd_pq_t *pq, int i)
{
    while (i > 0)
    {
        int parent = (i - 1) / 2;
        if (pq->data[parent].f_cost > pq->data[i].f_cost)
        {
            swap_entries(&pq->data[parent], &pq->data[i]);
            i = parent;
        }
        else
            break;
    }
}

static void sift_down(bcd_pq_t *pq, int i)
{
    while (1)
    {
        int smallest = i;
        int left = 2 * i + 1;
        int right = 2 * i + 2;

        if (left < pq->size && pq->data[left].f_cost < pq->data[smallest].f_cost)
            smallest = left;
        if (right < pq->size && pq->data[right].f_cost < pq->data[smallest].f_cost)
            smallest = right;

        if (smallest == i)
            break;

        swap_entries(&pq->data[i], &pq->data[smallest]);
        i = smallest;
    }
}

static void ensure_capacity(bcd_pq_t *pq)
{
    if (pq->size < pq->capacity)
        return;

    int new_cap = pq->capacity * 2;
    bcd_pq_entry_t *new_data =
        (bcd_pq_entry_t *)realloc(pq->data, (size_t)new_cap * sizeof(bcd_pq_entry_t));
    if (new_data == NULL)
        return; /* allocation failed — push will silently drop; callers guard against empty pops */
    pq->data = new_data;
    pq->capacity = new_cap;
}

/* ---------- public API -------------------------------------------------- */

void bcd_pq_init(bcd_pq_t *pq)
{
    pq->data = (bcd_pq_entry_t *)malloc(BCD_PQ_INITIAL_CAPACITY * sizeof(bcd_pq_entry_t));
    pq->size = 0;
    pq->capacity = (pq->data != NULL) ? BCD_PQ_INITIAL_CAPACITY : 0;
}

void bcd_pq_free(bcd_pq_t *pq)
{
    free(pq->data);
    pq->data = NULL;
    pq->size = 0;
    pq->capacity = 0;
}

bool bcd_pq_empty(const bcd_pq_t *pq)
{
    return pq->size == 0;
}

void bcd_pq_push(bcd_pq_t *pq, int cell_index, float f_cost)
{
    ensure_capacity(pq);
    if (pq->size >= pq->capacity)
        return; /* out of memory */

    pq->data[pq->size].cell_index = cell_index;
    pq->data[pq->size].f_cost = f_cost;
    sift_up(pq, pq->size);
    pq->size++;
}

bcd_pq_entry_t bcd_pq_pop(bcd_pq_t *pq)
{
    bcd_pq_entry_t top = pq->data[0];
    pq->size--;
    if (pq->size > 0)
    {
        pq->data[0] = pq->data[pq->size];
        sift_down(pq, 0);
    }
    return top;
}

void bcd_pq_decrease_key(bcd_pq_t *pq, int cell_index, float f_cost)
{
    /* Linear scan — BCD cell graphs are small, so this is cheap. */
    for (int i = 0; i < pq->size; ++i)
    {
        if (pq->data[i].cell_index == cell_index)
        {
            if (f_cost < pq->data[i].f_cost)
            {
                pq->data[i].f_cost = f_cost;
                sift_up(pq, i);
            }
            return;
        }
    }
    /* Not found — push as new entry. */
    bcd_pq_push(pq, cell_index, f_cost);
}
