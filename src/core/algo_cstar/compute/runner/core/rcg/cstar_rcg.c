/*
 * cstar_rcg.c  --  Rapidly-exploring Coverage Graph (RCG) data structure
 *
 * The RCG G = (V, E) is the sparse undirected graph maintained across all
 * iterations of the C* algorithm (Section III, Definition III.1).
 *
 *   V  - set of frontier-sample nodes, each labelled Open (OP) or Closed (CL)
 *   E  - collision-free edges connecting nodes within reach w * sqrt(2)
 *
 * Node layout
 * -----------
 * Every node stores four directional neighbour slots:
 *   up / down    -- same-lap neighbours (further / closer along the lap axis)
 *   left / right -- cross-lap neighbours on the adjacent left / right lap
 *
 * Lap structure
 * -------------
 * Laps are parallel sweeps spaced w apart.  Nodes on a lap are ordered by
 * increasing distance along the lap axis; end nodes mark the lap's endpoints.
 *
 * References: Section III.A, Definitions III.1-III.7
 */

#include <stdlib.h>
#include <math.h>
#include "cstar_rcg.h"

#define CSTAR_EPSILON 1e-6f

static float cstar_dist(point_t a, point_t b)
{
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return sqrtf(dx * dx + dy * dy);
}

static bool cstar_ensure_node_capacity(cstar_rcg_t *rcg)
{
    if (rcg->node_count < rcg->node_capacity)
    {
        return true;
    }

    int new_capacity = (rcg->node_capacity <= 0) ? 16 : (rcg->node_capacity * 2);
    cstar_node_t *grown = (cstar_node_t *)realloc(rcg->nodes, (size_t)new_capacity * sizeof(cstar_node_t));
    if (grown == NULL)
    {
        return false;
    }

    rcg->nodes = grown;
    rcg->node_capacity = new_capacity;
    return true;
}

static bool cstar_ensure_edge_capacity(cstar_rcg_t *rcg)
{
    if (rcg->edge_count < rcg->edge_capacity)
    {
        return true;
    }

    int new_capacity = (rcg->edge_capacity <= 0) ? 32 : (rcg->edge_capacity * 2);
    cstar_edge_t *grown = (cstar_edge_t *)realloc(rcg->edges, (size_t)new_capacity * sizeof(cstar_edge_t));
    if (grown == NULL)
    {
        return false;
    }

    rcg->edges = grown;
    rcg->edge_capacity = new_capacity;
    return true;
}

static int cstar_orientation(point_t a, point_t b, point_t c)
{
    float cross = (b.y - a.y) * (c.x - b.x) - (b.x - a.x) * (c.y - b.y);
    if (fabsf(cross) < CSTAR_EPSILON)
    {
        return 0;
    }
    return (cross > 0.0f) ? 1 : 2;
}

static bool cstar_on_segment(point_t a, point_t b, point_t p)
{
    return p.x <= fmaxf(a.x, b.x) + CSTAR_EPSILON &&
           p.x + CSTAR_EPSILON >= fminf(a.x, b.x) &&
           p.y <= fmaxf(a.y, b.y) + CSTAR_EPSILON &&
           p.y + CSTAR_EPSILON >= fminf(a.y, b.y);
}

static bool cstar_segments_intersect(point_t p1, point_t q1, point_t p2, point_t q2)
{
    int o1 = cstar_orientation(p1, q1, p2);
    int o2 = cstar_orientation(p1, q1, q2);
    int o3 = cstar_orientation(p2, q2, p1);
    int o4 = cstar_orientation(p2, q2, q1);

    if (o1 != o2 && o3 != o4)
    {
        return true;
    }

    if (o1 == 0 && cstar_on_segment(p1, q1, p2))
        return true;
    if (o2 == 0 && cstar_on_segment(p1, q1, q2))
        return true;
    if (o3 == 0 && cstar_on_segment(p2, q2, p1))
        return true;
    if (o4 == 0 && cstar_on_segment(p2, q2, q1))
        return true;

    return false;
}

static bool cstar_point_in_polygon(point_t point, const polygon_t *polygon)
{
    if (polygon == NULL || polygon->vertices == NULL || polygon->vertex_count < 3u)
    {
        return false;
    }

    bool inside = false;
    uint32_t j = polygon->vertex_count - 1u;
    for (uint32_t i = 0; i < polygon->vertex_count; ++i)
    {
        const point_t vi = polygon->vertices[i];
        const point_t vj = polygon->vertices[j];

        bool intersects = ((vi.y > point.y) != (vj.y > point.y)) &&
                          (point.x < (vj.x - vi.x) * (point.y - vi.y) / ((vj.y - vi.y) + CSTAR_EPSILON) + vi.x);
        if (intersects)
        {
            inside = !inside;
        }

        j = i;
    }

    return inside;
}

static bool cstar_edge_exists(const cstar_rcg_t *rcg, int node_a, int node_b)
{
    for (int i = 0; i < rcg->edge_count; ++i)
    {
        const cstar_edge_t *edge = &rcg->edges[i];
        if ((edge->node_a == node_a && edge->node_b == node_b) ||
            (edge->node_a == node_b && edge->node_b == node_a))
        {
            return true;
        }
    }
    return false;
}

static void cstar_clear_neighbor_reference(cstar_node_t *node, int target)
{
    if (node->neighbor_up == target)
        node->neighbor_up = CSTAR_NO_NEIGHBOR;
    if (node->neighbor_down == target)
        node->neighbor_down = CSTAR_NO_NEIGHBOR;
    if (node->neighbor_left == target)
        node->neighbor_left = CSTAR_NO_NEIGHBOR;
    if (node->neighbor_right == target)
        node->neighbor_right = CSTAR_NO_NEIGHBOR;
}

void cstar_rcg_init(cstar_rcg_t *rcg)
{
    if (rcg == NULL)
    {
        return;
    }

    rcg->nodes = NULL;
    rcg->node_count = 0;
    rcg->node_capacity = 0;
    rcg->edges = NULL;
    rcg->edge_count = 0;
    rcg->edge_capacity = 0;
}

void cstar_rcg_free(cstar_rcg_t *rcg)
{
    if (rcg == NULL)
    {
        return;
    }

    free(rcg->nodes);
    free(rcg->edges);

    rcg->nodes = NULL;
    rcg->node_count = 0;
    rcg->node_capacity = 0;
    rcg->edges = NULL;
    rcg->edge_count = 0;
    rcg->edge_capacity = 0;
}

int cstar_rcg_add_node(cstar_rcg_t *rcg, point_t pos, int lap_id, bool is_end_node)
{
    if (rcg == NULL)
    {
        return CSTAR_NO_NEIGHBOR;
    }

    if (!cstar_ensure_node_capacity(rcg))
    {
        return CSTAR_NO_NEIGHBOR;
    }

    int id = rcg->node_count;
    cstar_node_t *node = &rcg->nodes[id];

    node->id = id;
    node->pos = pos;
    node->state = CSTAR_NODE_OP;
    node->lap_id = lap_id;
    node->is_end_node = is_end_node;
    node->is_link_node = false;
    node->neighbor_up = CSTAR_NO_NEIGHBOR;
    node->neighbor_down = CSTAR_NO_NEIGHBOR;
    node->neighbor_left = CSTAR_NO_NEIGHBOR;
    node->neighbor_right = CSTAR_NO_NEIGHBOR;

    rcg->node_count++;
    return id;
}

bool cstar_rcg_add_edge(cstar_rcg_t *rcg, int node_a, int node_b,
                        const cstar_environment_t *env)
{
    if (rcg == NULL || node_a < 0 || node_b < 0 ||
        node_a >= rcg->node_count || node_b >= rcg->node_count ||
        node_a == node_b)
    {
        return false;
    }

    if (cstar_edge_exists(rcg, node_a, node_b))
    {
        return true;
    }

    point_t a = rcg->nodes[node_a].pos;
    point_t b = rcg->nodes[node_b].pos;
    if (!cstar_rcg_edge_is_collision_free(a, b, env))
    {
        return false;
    }

    if (!cstar_ensure_edge_capacity(rcg))
    {
        return false;
    }

    cstar_edge_t *edge = &rcg->edges[rcg->edge_count++];
    edge->node_a = node_a;
    edge->node_b = node_b;
    edge->cost = cstar_dist(a, b);

    cstar_node_t *na = &rcg->nodes[node_a];
    cstar_node_t *nb = &rcg->nodes[node_b];

    if (na->lap_id == nb->lap_id)
    {
        if (na->pos.y <= nb->pos.y)
        {
            na->neighbor_up = node_b;
            nb->neighbor_down = node_a;
        }
        else
        {
            na->neighbor_down = node_b;
            nb->neighbor_up = node_a;
        }
    }
    else if (na->lap_id < nb->lap_id)
    {
        na->neighbor_right = node_b;
        nb->neighbor_left = node_a;
    }
    else
    {
        na->neighbor_left = node_b;
        nb->neighbor_right = node_a;
    }

    return true;
}

void cstar_rcg_remove_node(cstar_rcg_t *rcg, int node_id)
{
    (void)rcg;
    (void)node_id;
}

void cstar_rcg_remove_edge(cstar_rcg_t *rcg, int node_a, int node_b)
{
    if (rcg == NULL || node_a < 0 || node_b < 0 ||
        node_a >= rcg->node_count || node_b >= rcg->node_count)
    {
        return;
    }

    for (int i = 0; i < rcg->edge_count; ++i)
    {
        cstar_edge_t edge = rcg->edges[i];
        if (!((edge.node_a == node_a && edge.node_b == node_b) ||
              (edge.node_a == node_b && edge.node_b == node_a)))
        {
            continue;
        }

        cstar_clear_neighbor_reference(&rcg->nodes[node_a], node_b);
        cstar_clear_neighbor_reference(&rcg->nodes[node_b], node_a);

        rcg->edges[i] = rcg->edges[rcg->edge_count - 1];
        rcg->edge_count--;
        return;
    }
}

void cstar_rcg_merge_lap_edge(cstar_rcg_t *rcg, int node_id)
{
    if (rcg == NULL || node_id < 0 || node_id >= rcg->node_count)
    {
        return;
    }

    int up = rcg->nodes[node_id].neighbor_up;
    int down = rcg->nodes[node_id].neighbor_down;
    if (up == CSTAR_NO_NEIGHBOR || down == CSTAR_NO_NEIGHBOR)
    {
        return;
    }

    cstar_rcg_remove_edge(rcg, node_id, up);
    cstar_rcg_remove_edge(rcg, node_id, down);
    cstar_rcg_add_edge(rcg, down, up, NULL);
}

bool cstar_rcg_edge_is_collision_free(point_t a, point_t b,
                                      const cstar_environment_t *env)
{
    if (env == NULL)
    {
        return true;
    }

    for (uint32_t i = 0; i < env->obstacle_count; ++i)
    {
        const polygon_t *obstacle = &env->operationalObstacles[i];
        if (obstacle->vertices == NULL || obstacle->vertex_count < 3u)
        {
            continue;
        }

        if (cstar_point_in_polygon(a, obstacle) || cstar_point_in_polygon(b, obstacle))
        {
            return false;
        }

        for (uint32_t e = 0; e < obstacle->vertex_count; ++e)
        {
            uint32_t next = (e + 1u) % obstacle->vertex_count;
            point_t p = obstacle->vertices[e];
            point_t q = obstacle->vertices[next];
            if (cstar_segments_intersect(a, b, p, q))
            {
                return false;
            }
        }
    }

    return true;
}

void cstar_rcg_close_node(cstar_rcg_t *rcg, int node_id)
{
    if (rcg == NULL || node_id < 0 || node_id >= rcg->node_count)
    {
        return;
    }

    rcg->nodes[node_id].state = CSTAR_NODE_CL;
}
