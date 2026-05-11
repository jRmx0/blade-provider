#include "bcd_pathfinding.h"

#include <math.h>
#include <stdlib.h>
#include <stdbool.h>

#include "../../../../../../dependencies/cvector/cvector.h"
#include "bcd_pq.h"
#include "bcd_geometry.h"

/* ---------- helpers ---------------------------------------------------- */

static float cell_centroid_x(const bcd_cell_t *cell)
{
    return (cell->c_begin.x + cell->c_end.x) / 2.0f;
}

static float cell_centroid_y(const bcd_cell_t *cell)
{
    /* Midpoint between ceiling midpoint and floor midpoint */
    float top_y = (cell->c_begin.y + cell->c_end.y) / 2.0f;
    float bot_y = (cell->f_begin.y + cell->f_end.y) / 2.0f;
    return (top_y + bot_y) / 2.0f;
}

static float euclidean(const bcd_cell_t *a, const bcd_cell_t *b)
{
    float dx = cell_centroid_x(a) - cell_centroid_x(b);
    float dy = cell_centroid_y(a) - cell_centroid_y(b);
    return sqrtf(dx * dx + dy * dy);
}

/* ---------- bcd_astar -------------------------------------------------- */

cvector_vector_type(int) bcd_astar(int cell_index_from,
                                   int cell_index_to,
                                   const cvector_vector_type(bcd_cell_t) * cell_list)
{
    cvector_vector_type(int) path = NULL;

    if (cell_list == NULL || *cell_list == NULL)
        return path;

    int cell_count = (int)cvector_size(*cell_list);
    if (cell_index_from < 0 || cell_index_from >= cell_count ||
        cell_index_to < 0 || cell_index_to >= cell_count)
        return path;

    if (cell_index_from == cell_index_to)
    {
        cvector_push_back(path, cell_index_from);
        return path;
    }

    /* g_cost[i] = cheapest known cost from start to cell i */
    float *g_cost = (float *)malloc((size_t)cell_count * sizeof(float));
    /* parent[i]  = predecessor cell index on the best known path */
    int *parent = (int *)malloc((size_t)cell_count * sizeof(int));
    bool *closed = (bool *)malloc((size_t)cell_count * sizeof(bool));

    if (g_cost == NULL || parent == NULL || closed == NULL)
    {
        free(g_cost);
        free(parent);
        free(closed);
        return path;
    }

    for (int i = 0; i < cell_count; ++i)
    {
        g_cost[i] = 1e38f;
        parent[i] = -1;
        closed[i] = false;
    }

    g_cost[cell_index_from] = 0.0f;

    bcd_pq_t open;
    bcd_pq_init(&open);

    float h0 = euclidean(&(*cell_list)[cell_index_from], &(*cell_list)[cell_index_to]);
    bcd_pq_push(&open, cell_index_from, h0);

    bool found = false;

    while (!bcd_pq_empty(&open))
    {
        bcd_pq_entry_t entry = bcd_pq_pop(&open);
        int current = entry.cell_index;

        if (current < 0 || current >= cell_count)
            continue;
        if (closed[current])
            continue;
        closed[current] = true;

        if (current == cell_index_to)
        {
            found = true;
            break;
        }

        /* Walk adjacency list */
        bcd_neighbor_node_t *node = (*cell_list)[current].neighbor_list.head;
        while (node != NULL)
        {
            int nb = node->cell_index;
            if (nb < 0 || nb >= cell_count || closed[nb])
            {
                node = node->next;
                continue;
            }

            float tentative_g = g_cost[current] +
                                euclidean(&(*cell_list)[current], &(*cell_list)[nb]);

            if (tentative_g < g_cost[nb])
            {
                g_cost[nb] = tentative_g;
                parent[nb] = current;

                float f = tentative_g +
                          euclidean(&(*cell_list)[nb], &(*cell_list)[cell_index_to]);
                bcd_pq_decrease_key(&open, nb, f);
            }

            node = node->next;
        }
    }

    /* Reconstruct path */
    if (found)
    {
        cvector_vector_type(int) rev = NULL;
        int cur = cell_index_to;
        while (cur != -1)
        {
            cvector_push_back(rev, cur);
            cur = parent[cur];
        }

        /* Reverse */
        int len = (int)cvector_size(rev);
        for (int i = len - 1; i >= 0; --i)
            cvector_push_back(path, rev[i]);

        cvector_free(rev);
    }

    bcd_pq_free(&open);
    free(g_cost);
    free(parent);
    free(closed);

    return path;
}

/* ---------- bcd_find_cell ---------------------------------------------- */

int bcd_find_cell(const cvector_vector_type(bcd_cell_t) * cell_list, point_t p)
{
    if (cell_list == NULL || *cell_list == NULL)
        return -1;

    int cell_count = (int)cvector_size(*cell_list);
    for (int i = 0; i < cell_count; ++i)
    {
        if (bcd_cell_contains_point(&(*cell_list)[i], p))
            return i;
    }
    return -1;
}
