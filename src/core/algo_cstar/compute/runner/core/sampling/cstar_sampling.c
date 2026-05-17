#include <math.h>
#include <stdint.h>
#include "cstar_sampling.h"
#include "../geometry/cstar_geometry.h"
#include "../rcg/cstar_rcg.h"
#include "../../../../../../../dependencies/cvector/cvector.h"

#define CSTAR_EPSILON 1e-6f

static float cstar_sampling_dist_point_segment(point_t p, point_t a, point_t b)
{
    float vx = b.x - a.x;
    float vy = b.y - a.y;
    float wx = p.x - a.x;
    float wy = p.y - a.y;

    float vv = vx * vx + vy * vy;
    if (vv <= CSTAR_EPSILON)
    {
        float dx = p.x - a.x;
        float dy = p.y - a.y;
        return sqrtf(dx * dx + dy * dy);
    }

    float t = (wx * vx + wy * vy) / vv;
    if (t < 0.0f)
        t = 0.0f;
    if (t > 1.0f)
        t = 1.0f;

    float cx = a.x + t * vx;
    float cy = a.y + t * vy;
    float dx = p.x - cx;
    float dy = p.y - cy;
    return sqrtf(dx * dx + dy * dy);
}

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

static bool cstar_sampling_point_on_boundary(point_t point, const polygon_t *polygon, float tol)
{
    if (polygon == NULL || polygon->vertices == NULL || polygon->vertex_count < 2u)
    {
        return false;
    }

    for (uint32_t i = 0; i < polygon->vertex_count; ++i)
    {
        uint32_t next = (i + 1u) % polygon->vertex_count;
        point_t a = polygon->vertices[i];
        point_t b = polygon->vertices[next];

        if (cstar_sampling_dist_point_segment(point, a, b) <= tol)
        {
            return true;
        }
    }

    return false;
}

static bool cstar_sampling_point_is_free(point_t point, const cstar_environment_t *env)
{
    if (env == NULL)
    {
        return false;
    }

    if (!cstar_sampling_point_in_polygon(point, &env->operationalBoundary) &&
        !cstar_sampling_point_on_boundary(point, &env->operationalBoundary, CSTAR_EPSILON))
    {
        return false;
    }

    return true;
}

bool cstar_is_frontier_sample(point_t s, float w, const cstar_environment_t *env)
{
    if (env == NULL || env->operationalBoundary.vertices == NULL || env->operationalBoundary.vertex_count < 3u)
    {
        return false;
    }

    /*
     * Boundary-only frontier criterion (current phase):
     * a sample is considered frontier if it lies within distance w from the
     * operational boundary edges.
     */
    float frontier_tol = (w > CSTAR_EPSILON) ? w : CSTAR_EPSILON;
    return cstar_sampling_point_on_boundary(s, &env->operationalBoundary, frontier_tol);
}

int cstar_generate_frontier_samples(cstar_rcg_t *rcg,
                                    float w,
                                    int delta,
                                    const cstar_environment_t *env)
{
    if (env == NULL || env->laps == NULL || rcg == NULL)
    {
        return 0;
    }

    cstar_lap_t *laps = (cstar_lap_t *)env->laps;
    int lap_count = (int)cvector_size(laps);
    if (lap_count <= 0)
    {
        return 0;
    }

    float min_x = 0.0f, max_x = 0.0f, min_y = 0.0f, max_y = 0.0f;
    cstar_lap_boundary_bbox(env, &min_x, &max_x, &min_y, &max_y);

    float step = ((delta > 0 ? (float)delta : 1.0f) * ((w > CSTAR_EPSILON) ? w : 1.0f));
    int total_added = 0;

    for (int lap_index = 0; lap_index < lap_count; ++lap_index)
    {
        cstar_lap_t *lap = &laps[lap_index];

        if (lap->node_ids != NULL)
        {
            cvector_free(lap->node_ids);
            lap->node_ids = NULL;
            lap->node_count = 0;
            lap->node_capacity = 0;
        }

        for (float y = min_y; y <= max_y + CSTAR_EPSILON; y += step)
        {
            point_t sample = {lap->x, y};

            if (!cstar_sampling_point_is_free(sample, env))
            {
                continue;
            }

            if (!cstar_is_frontier_sample(sample, w, env))
            {
                continue;
            }

            int node_id = cstar_rcg_add_node(rcg, sample, lap->id, false);
            if (node_id == CSTAR_NO_NEIGHBOR)
            {
                continue;
            }

            cvector_push_back(lap->node_ids, node_id);
            total_added++;
        }

        lap->node_count = (int)cvector_size(lap->node_ids);
        lap->node_capacity = cvector_capacity(lap->node_ids);

        if (lap->node_count > 0)
        {
            rcg->nodes[lap->node_ids[0]].is_end_node = true;
            rcg->nodes[lap->node_ids[lap->node_count - 1]].is_end_node = true;
        }
    }

    return total_added;
}
