/*
 * cstar_sampling.c  --  Progressive sampling front and frontier samples
 *
 * At each C* iteration i, the robot has moved from prev_pos to curr_pos and
 * has discovered a new area of the environment.  The sampling front F_i is
 * the obstacle-free, previously-unsampled portion of that discovery
 * (Definition III.2).
 *
 * Laps inside F_i
 * ---------------
 * F_i is partitioned into parallel laps spaced w apart, aligned with lap_dir.
 * Nodes are placed on each lap at intervals >= delta*w, filtered by the
 * frontier-sample predicate cstar_is_frontier_sample().
 *
 * Frontier sample (Definition III.3)
 * ------------------------------------
 * A point s is a frontier sample if the ball B(s, w) contains unknown space
 * or an obstacle boundary; i.e. B(s, w) is not entirely inside known,
 * obstacle-free space.
 *
 * References: Section III.A, Definitions III.2-III.5, Algorithm 4 (lines 3-6)
 */

#include <stdlib.h>
#include <math.h>
#include "cstar_sampling.h"
#include "preprocess/cstar_lap.h"

#define CSTAR_EPSILON 1e-6f

static bool cstar_sampling_point_in_polygon(point_t point, const polygon_t *polygon)
{
    if (polygon == NULL || polygon->vertices == NULL || polygon->vertex_count < 3u)
    {
        return false;
    }

    bool inside = false;
    uint32_t j = polygon->vertex_count - 1u;
    for (uint32_t i = 0; i < polygon->vertex_count; ++i)
    {
        point_t vi = polygon->vertices[i];
        point_t vj = polygon->vertices[j];

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

static float cstar_dist_point_segment(point_t p, point_t a, point_t b)
{
    float vx = b.x - a.x;
    float vy = b.y - a.y;
    float len_sq = vx * vx + vy * vy;
    if (len_sq < CSTAR_EPSILON)
    {
        float dx = p.x - a.x;
        float dy = p.y - a.y;
        return sqrtf(dx * dx + dy * dy);
    }

    float t = ((p.x - a.x) * vx + (p.y - a.y) * vy) / len_sq;
    if (t < 0.0f)
        t = 0.0f;
    if (t > 1.0f)
        t = 1.0f;

    point_t proj = {a.x + t * vx, a.y + t * vy};
    float dx = p.x - proj.x;
    float dy = p.y - proj.y;
    return sqrtf(dx * dx + dy * dy);
}

static bool cstar_point_is_free(point_t p, const input_environment_t *env)
{
    if (!cstar_sampling_point_in_polygon(p, &env->boundary))
    {
        return false;
    }

    for (uint32_t i = 0; i < env->obstacle_count; ++i)
    {
        if (cstar_sampling_point_in_polygon(p, &env->obstacles[i]))
        {
            return false;
        }
    }

    return true;
}

cstar_sampling_front_t cstar_create_sampling_front(point_t prev_pos,
                                                   point_t curr_pos,
                                                   float rd,
                                                   float w,
                                                   point_t lap_dir,
                                                   const input_environment_t *env)
{
    (void)prev_pos;
    (void)rd;
    (void)lap_dir;

    cstar_sampling_front_t front = {0};

    if (env == NULL || env->boundary.vertices == NULL || env->boundary.vertex_count < 3u)
    {
        return front;
    }

    cstar_lap_generate_full_width(&front, curr_pos, w, env);

    return front;
}

int cstar_generate_frontier_samples(cstar_sampling_front_t *front,
                                    cstar_rcg_t *rcg,
                                    float w,
                                    int delta,
                                    const input_environment_t *env)
{
    if (front == NULL || rcg == NULL || env == NULL || front->laps == NULL)
    {
        return 0;
    }

    float min_x = 0.0f, max_x = 0.0f, min_y = 0.0f, max_y = 0.0f;
    cstar_lap_boundary_bbox(env, &min_x, &max_x, &min_y, &max_y);

    float step = (delta > 0 ? (float)delta : 1.0f) * ((w > CSTAR_EPSILON) ? w : 1.0f);
    int total_added = 0;

    int lap_count = (int)cvector_size(front->laps);
    for (int i = 0; i < lap_count; ++i)
    {
        cstar_lap_t *lap = &front->laps[i];
        int first_id = CSTAR_NO_NEIGHBOR;
        int last_id = CSTAR_NO_NEIGHBOR;

        for (float y = min_y; y <= max_y + CSTAR_EPSILON; y += step)
        {
            point_t s = {lap->x, y};
            if (!cstar_point_is_free(s, env))
            {
                continue;
            }

            if (!cstar_is_frontier_sample(s, w, env))
            {
                continue;
            }

            int id = cstar_rcg_add_node(rcg, s, lap->id, false);
            if (id == CSTAR_NO_NEIGHBOR)
            {
                continue;
            }

            cvector_push_back(lap->node_ids, id);
            lap->node_count = (int)cvector_size(lap->node_ids);

            if (first_id == CSTAR_NO_NEIGHBOR)
            {
                first_id = id;
            }
            last_id = id;
            total_added++;
        }

        if (first_id != CSTAR_NO_NEIGHBOR)
        {
            rcg->nodes[first_id].is_end_node = true;
            rcg->nodes[last_id].is_end_node = true;
        }
    }

    return total_added;
}

bool cstar_is_frontier_sample(point_t s, float w, const input_environment_t *env)
{
    if (env == NULL || env->boundary.vertices == NULL || env->boundary.vertex_count < 3u)
    {
        return false;
    }

    float range = (w > CSTAR_EPSILON) ? w : 1.0f;

    for (uint32_t i = 0; i < env->boundary.vertex_count; ++i)
    {
        uint32_t next = (i + 1u) % env->boundary.vertex_count;
        if (cstar_dist_point_segment(s, env->boundary.vertices[i], env->boundary.vertices[next]) <= range)
        {
            return true;
        }
    }

    for (uint32_t o = 0; o < env->obstacle_count; ++o)
    {
        const polygon_t *obstacle = &env->obstacles[o];
        if (obstacle->vertices == NULL || obstacle->vertex_count < 3u)
        {
            continue;
        }

        for (uint32_t i = 0; i < obstacle->vertex_count; ++i)
        {
            uint32_t next = (i + 1u) % obstacle->vertex_count;
            if (cstar_dist_point_segment(s, obstacle->vertices[i], obstacle->vertices[next]) <= range)
            {
                return true;
            }
        }
    }

    return false;
}

void cstar_sampling_front_free(cstar_sampling_front_t *front)
{
    if (front == NULL || front->laps == NULL)
    {
        return;
    }

    int lap_count = (int)cvector_size(front->laps);
    for (int i = 0; i < lap_count; ++i)
    {
        cvector_free(front->laps[i].node_ids);
        front->laps[i].node_ids = NULL;
        front->laps[i].node_count = 0;
        front->laps[i].node_capacity = 0;
    }

    cvector_free(front->laps);
    front->laps = NULL;
}
