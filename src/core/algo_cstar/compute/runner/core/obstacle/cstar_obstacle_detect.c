#include <math.h>
#include <stdlib.h>
#include "cstar_obstacle_detect.h"

#define CSTAR_OBSTACLE_EPSILON 1e-6f

// -------------------------------------------------------------------------
// Internal: 2D parametric line-segment intersection
// -------------------------------------------------------------------------

/**
 * Tests whether segment P (from + t*(to-from)) intersects segment Q (a + s*(b-a)).
 * Both t and s must be in [0, 1] for a true crossing.
 * Fills *t_out with the parametric position along P.
 */
static bool cstar_obstacle_segment_intersect(point_t from, point_t to,
                                             point_t a, point_t b,
                                             float *t_out)
{
    float dx = to.x - from.x;
    float dy = to.y - from.y;
    float ex = b.x - a.x;
    float ey = b.y - a.y;

    float denom = dx * ey - dy * ex;
    if (fabsf(denom) < CSTAR_OBSTACLE_EPSILON)
        return false; /* parallel */

    float fx = a.x - from.x;
    float fy = a.y - from.y;

    float t = (fx * ey - fy * ex) / denom;
    float s = (fx * dy - fy * dx) / denom;

    if (t < -CSTAR_OBSTACLE_EPSILON || t > 1.0f + CSTAR_OBSTACLE_EPSILON)
        return false;
    if (s < -CSTAR_OBSTACLE_EPSILON || s > 1.0f + CSTAR_OBSTACLE_EPSILON)
        return false;

    *t_out = (t < 0.0f) ? 0.0f : (t > 1.0f ? 1.0f : t);
    return true;
}

// -------------------------------------------------------------------------
// Internal: find exact entry point of segment into obstacle polygon
// -------------------------------------------------------------------------

/**
 * Finds the earliest parametric crossing of segment from→to with any edge of
 * the polygon. Fills *entry_out with the intersection point and *edge_idx_out
 * with the polygon edge index (edge i spans vertex[i] → vertex[(i+1)%n]).
 */
static bool cstar_obstacle_find_entry(point_t from, point_t to,
                                      const polygon_t *polygon,
                                      point_t *entry_out,
                                      int *edge_idx_out)
{
    if (polygon == NULL || polygon->vertices == NULL || polygon->vertex_count < 3u)
        return false;

    float best_t = 2.0f;
    int best_edge = -1;
    uint32_t n = polygon->vertex_count;

    for (uint32_t i = 0; i < n; ++i)
    {
        uint32_t j = (i + 1u) % n;
        float t;
        if (cstar_obstacle_segment_intersect(from, to,
                                             polygon->vertices[i],
                                             polygon->vertices[j], &t))
        {
            if (t < best_t)
            {
                best_t = t;
                best_edge = (int)i;
            }
        }
    }

    if (best_edge < 0)
        return false;

    entry_out->x = from.x + best_t * (to.x - from.x);
    entry_out->y = from.y + best_t * (to.y - from.y);
    *edge_idx_out = best_edge;
    return true;
}

// -------------------------------------------------------------------------
// Internal: check if segment from→to is clear of a single obstacle
// -------------------------------------------------------------------------

/**
 * Probes segment from→to at w-intervals; returns false if any probe lies
 * inside the polygon. Uses cstar_sampling_point_in_polygon (accessible in
 * the amalgamation unit).
 */
static bool cstar_obstacle_segment_clear(point_t from, point_t to,
                                         const polygon_t *obstacle, float w)
{
    float dx = to.x - from.x;
    float dy = to.y - from.y;
    float len = sqrtf(dx * dx + dy * dy);

    if (len < CSTAR_OBSTACLE_EPSILON)
        return !cstar_sampling_point_in_polygon(from, obstacle);

    int steps = (int)ceilf(len / w);
    for (int i = 0; i <= steps; ++i)
    {
        float t = (i < steps) ? ((float)i * w / len) : 1.0f;
        point_t probe = {from.x + t * dx, from.y + t * dy};
        if (cstar_sampling_point_in_polygon(probe, obstacle))
            return false;
    }
    return true;
}

// -------------------------------------------------------------------------
// Public: probe path for obstacle collision
// -------------------------------------------------------------------------

bool cstar_path_has_collision(point_t from,
                              point_t to,
                              float w,
                              const cstar_environment_t *env,
                              point_t *entry_pt_out,
                              int *obstacle_idx_out)
{
    if (env == NULL || env->operationalObstacles == NULL ||
        env->obstacle_count == 0u || w < CSTAR_OBSTACLE_EPSILON)
        return false;

    float dx = to.x - from.x;
    float dy = to.y - from.y;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < CSTAR_OBSTACLE_EPSILON)
        return false;

    /* Start probing from step 1 (skip from itself — it is a valid free node). */
    int step_count = (int)ceilf(len / w);
    for (int step = 1; step <= step_count; ++step)
    {
        float t = (step < step_count) ? ((float)step * w / len) : 1.0f;
        point_t probe = {from.x + t * dx, from.y + t * dy};

        for (uint32_t k = 0; k < env->obstacle_count; ++k)
        {
            if (cstar_sampling_point_in_polygon(probe, &env->operationalObstacles[k]))
            {
                point_t entry;
                int edge_idx;
                if (cstar_obstacle_find_entry(from, to,
                                              &env->operationalObstacles[k],
                                              &entry, &edge_idx))
                {
                    *entry_pt_out = entry;
                }
                else
                {
                    *entry_pt_out = probe; /* fallback */
                }
                *obstacle_idx_out = (int)k;
                return true;
            }
        }
    }
    return false;
}

// -------------------------------------------------------------------------
// Public: follow obstacle boundary CCW from entry_pt
// -------------------------------------------------------------------------

cvector_vector_type(point_t) cstar_obstacle_follow_boundary(point_t entry_pt,
                                                            point_t destination,
                                                            int obstacle_idx,
                                                            float w,
                                                            const cstar_environment_t *env)
{
    if (env == NULL || obstacle_idx < 0 ||
        (uint32_t)obstacle_idx >= env->obstacle_count)
        return NULL;

    const polygon_t *obstacle = &env->operationalObstacles[obstacle_idx];
    uint32_t n = obstacle->vertex_count;
    if (n < 3u || obstacle->vertices == NULL)
        return NULL;

    cvector_vector_type(point_t) waypoints = NULL;
    cvector_push_back(waypoints, entry_pt);

    /* Find which edge the entry_pt lies on (closest by point-to-segment dist). */
    float min_dist = INFINITY;
    int start_edge = 0;
    for (uint32_t i = 0; i < n; ++i)
    {
        uint32_t j = (i + 1u) % n;
        float d = cstar_sampling_dist_point_segment(entry_pt,
                                                    obstacle->vertices[i],
                                                    obstacle->vertices[j]);
        if (d < min_dist)
        {
            min_dist = d;
            start_edge = (int)i;
        }
    }

    /*
     * CCW walk (forward vertex order, matching POLYGON_WINDING_CCW):
     * The collision edge is start_edge → start_edge+1.
     * Begin at vertex[(start_edge + 1) % n] and proceed forward.
     *
     * Guard: visit at most n vertices to prevent infinite loops on
     * degenerate inputs where destination is always inside the obstacle.
     */
    int start_vertex = (start_edge + 1) % (int)n;
    point_t prev = entry_pt;

    for (int step = 0; step < (int)n; ++step)
    {
        int vi = (start_vertex + step) % (int)n;
        point_t vertex = obstacle->vertices[vi];

        /* Interpolate intermediate points along prev → vertex at w-spacing. */
        float ex = vertex.x - prev.x;
        float ey = vertex.y - prev.y;
        float edge_len = sqrtf(ex * ex + ey * ey);

        if (edge_len > w + CSTAR_OBSTACLE_EPSILON)
        {
            int sub_steps = (int)floorf(edge_len / w);
            for (int s = 1; s <= sub_steps; ++s)
            {
                float st = (float)s * w / edge_len;
                point_t mid = {prev.x + st * ex, prev.y + st * ey};
                cvector_push_back(waypoints, mid);
            }
        }

        cvector_push_back(waypoints, vertex);
        prev = vertex;

        /* Exit: clear sight-line from this vertex to the destination. */
        if (cstar_obstacle_segment_clear(vertex, destination, obstacle, w))
            return waypoints;
    }

    /* Full circumnavigation: return all collected waypoints. */
    return waypoints;
}

// -------------------------------------------------------------------------
// Public: full CCW circumnavigation of an obstacle
// -------------------------------------------------------------------------

cvector_vector_type(point_t) cstar_obstacle_circumnavigate(point_t entry_pt,
                                                           int obstacle_idx,
                                                           float w,
                                                           const cstar_environment_t *env)
{
    if (env == NULL || obstacle_idx < 0 ||
        (uint32_t)obstacle_idx >= env->obstacle_count)
        return NULL;

    const polygon_t *obstacle = &env->operationalObstacles[obstacle_idx];
    uint32_t n = obstacle->vertex_count;
    if (n < 3u || obstacle->vertices == NULL)
        return NULL;

    cvector_vector_type(point_t) waypoints = NULL;
    cvector_push_back(waypoints, entry_pt);

    /* Find which edge the entry_pt lies on (closest by point-to-segment dist). */
    float min_dist = INFINITY;
    int start_edge = 0;
    for (uint32_t i = 0; i < n; ++i)
    {
        uint32_t j = (i + 1u) % n;
        float d = cstar_sampling_dist_point_segment(entry_pt,
                                                    obstacle->vertices[i],
                                                    obstacle->vertices[j]);
        if (d < min_dist)
        {
            min_dist = d;
            start_edge = (int)i;
        }
    }

    /*
     * CCW walk (forward vertex index order): start at vertex[(start_edge+1)%n]
     * and visit all n vertices, interpolating at w-spacing.
     */
    int start_vertex = (start_edge + 1) % (int)n;
    point_t prev = entry_pt;

    for (int step = 0; step < (int)n; ++step)
    {
        int vi = (start_vertex + step) % (int)n;
        point_t vertex = obstacle->vertices[vi];

        float ex = vertex.x - prev.x;
        float ey = vertex.y - prev.y;
        float edge_len = sqrtf(ex * ex + ey * ey);

        if (edge_len > w + CSTAR_OBSTACLE_EPSILON)
        {
            int sub_steps = (int)floorf(edge_len / w);
            for (int s = 1; s <= sub_steps; ++s)
            {
                float st = (float)s * w / edge_len;
                point_t mid = {prev.x + st * ex, prev.y + st * ey};
                cvector_push_back(waypoints, mid);
            }
        }

        cvector_push_back(waypoints, vertex);
        prev = vertex;
    }

    /* Close the loop: return to entry_pt. */
    cvector_push_back(waypoints, entry_pt);

    return waypoints;
}

// -------------------------------------------------------------------------
// Public: inject boundary waypoints as RCG nodes and reconnect graph
// -------------------------------------------------------------------------

int cstar_obstacle_inject_boundary_nodes(cstar_rcg_t *rcg,
                                         const cvector_vector_type(point_t) waypoints,
                                         int from_node_id,
                                         float w,
                                         const cstar_environment_t *env)
{
    if (rcg == NULL || waypoints == NULL)
        return CSTAR_NO_NEIGHBOR;

    int wp_count = (int)cvector_size(waypoints);
    if (wp_count == 0)
        return CSTAR_NO_NEIGHBOR;

    float cross_lap_threshold = sqrtf(2.0f) * w;

    /*
     * Phase 1: Add all waypoints as RCG nodes and collect their IDs.
     * We collect IDs separately so we can safely mark is_link_node after
     * all cstar_rcg_add_node calls (each push may reallocate rcg->nodes).
     */
    cvector_vector_type(int) new_ids = NULL;

    for (int i = 0; i < wp_count; ++i)
    {
        int id = cstar_rcg_add_node(rcg, waypoints[i], -1,
                                    false, false, false, false);
        cvector_push_back(new_ids, id);
    }

    /* Phase 2: Mark all new nodes as link nodes (nodes array is now stable). */
    for (int i = 0; i < (int)cvector_size(new_ids); ++i)
    {
        cstar_node_t *node = cstar_rcg_get_node_by_id_mut(rcg, new_ids[i]);
        if (node != NULL)
            node->is_link_node = true;
    }

    /* Phase 3: Chain edges — from_node_id → new_ids[0] → new_ids[1] → … */
    int prev_id = from_node_id;
    int exit_node_id = CSTAR_NO_NEIGHBOR;

    for (int i = 0; i < (int)cvector_size(new_ids); ++i)
    {
        int cur_id = new_ids[i];
        if (cur_id == CSTAR_NO_NEIGHBOR)
        {
            prev_id = cur_id;
            continue;
        }

        if (prev_id != CSTAR_NO_NEIGHBOR)
        {
            const cstar_node_t *pn = cstar_rcg_get_node_by_id(rcg, prev_id);
            const cstar_node_t *cn = cstar_rcg_get_node_by_id(rcg, cur_id);
            if (pn != NULL && cn != NULL)
            {
                float ddx = pn->pos.x - cn->pos.x;
                float ddy = pn->pos.y - cn->pos.y;
                float cost = sqrtf(ddx * ddx + ddy * ddy);
                cstar_rcg_add_edge(rcg, prev_id, cur_id, cost);
            }
        }

        prev_id = cur_id;
        exit_node_id = cur_id;
    }

    cvector_free(new_ids);

    if (exit_node_id == CSTAR_NO_NEIGHBOR)
        return CSTAR_NO_NEIGHBOR;

    /*
     * Phase 4: Connect exit node to nearby Open RCG nodes within √2·w.
     * Snapshot node_count before the loop; new nodes were already added above.
     */
    int snapshot_count = rcg->node_count;
    const cstar_node_t *exit_node = cstar_rcg_get_node_by_id(rcg, exit_node_id);

    if (exit_node != NULL)
    {
        float ex_x = exit_node->pos.x;
        float ex_y = exit_node->pos.y;

        for (int i = 0; i < snapshot_count; ++i)
        {
            cstar_node_t *other = &rcg->nodes[i];
            if (other->id == exit_node_id || other->state == CSTAR_NODE_CL)
                continue;

            float ddx = other->pos.x - ex_x;
            float ddy = other->pos.y - ex_y;
            float dist = sqrtf(ddx * ddx + ddy * ddy);

            if (dist <= cross_lap_threshold + CSTAR_OBSTACLE_EPSILON)
            {
                cstar_rcg_add_unique_edge(&rcg->edges, exit_node_id,
                                          other->id, dist);
            }
        }

        rcg->edge_count = (int)cvector_size(rcg->edges);
        rcg->edge_capacity = (int)cvector_capacity(rcg->edges);
    }

    /* Phase 5: Rebuild all neighbor pointers from the authoritative edge list. */
    cstar_rcg_rebuild_links_from_edges(rcg->nodes, rcg->node_count,
                                       rcg->edges, rcg->edge_count);

    return exit_node_id;
}

// -------------------------------------------------------------------------
// Public: add frontier samples on laps adjacent to a newly-discovered obstacle
// -------------------------------------------------------------------------

int cstar_generate_obstacle_adjacent_samples(cstar_rcg_t *rcg,
                                             int obstacle_idx,
                                             float w,
                                             int delta,
                                             const cstar_environment_t *env)
{
    if (rcg == NULL || env == NULL || obstacle_idx < 0 ||
        (uint32_t)obstacle_idx >= env->obstacle_count ||
        env->laps == NULL || w < CSTAR_OBSTACLE_EPSILON)
        return 0;

    const polygon_t *obstacle = &env->operationalObstacles[obstacle_idx];
    if (obstacle->vertices == NULL || obstacle->vertex_count < 3u)
        return 0;

    /* Obstacle X bounding box. */
    float obs_min_x = obstacle->vertices[0].x;
    float obs_max_x = obstacle->vertices[0].x;
    for (uint32_t i = 1; i < obstacle->vertex_count; ++i)
    {
        if (obstacle->vertices[i].x < obs_min_x)
            obs_min_x = obstacle->vertices[i].x;
        if (obstacle->vertices[i].x > obs_max_x)
            obs_max_x = obstacle->vertices[i].x;
    }

    float margin = sqrtf(2.0f) * w;
    float dedup_eps = w * 0.05f; /* 5% of path width — enough to skip near-duplicates */

    /* Y-range and sample step (mirrors initial frontier sampling logic). */
    float min_x, max_x, min_y, max_y;
    cstar_lap_boundary_bbox(env, &min_x, &max_x, &min_y, &max_y);

    float step = ((delta > 0 ? (float)delta : 1.0f) * w);
    float anchor_y = env->start_point.y;
    int first_sample_index = (int)ceilf(((min_y - anchor_y) / step) - CSTAR_OBSTACLE_EPSILON);
    int last_sample_index = (int)floorf(((max_y - anchor_y) / step) + CSTAR_OBSTACLE_EPSILON);

    /* Snapshot of existing node count for the connection phase below. */
    int existing_node_count = rcg->node_count;

    cstar_lap_t *laps = (cstar_lap_t *)env->laps;
    int lap_count = (int)cvector_size(laps);

    cvector_vector_type(int) new_ids = NULL;
    int total_added = 0;

    for (int li = 0; li < lap_count; ++li)
    {
        cstar_lap_t *lap = &laps[li];

        /* Skip laps outside the obstacle's X neighbourhood. */
        if (lap->x < obs_min_x - margin || lap->x > obs_max_x + margin)
            continue;

        /* Snapshot the lap's current node count before any additions. */
        int existing_lap_node_count = lap->node_count;

        /* Determine whether the section of this lap that overlaps the
           obstacle's y-range was already fully traversed.
           A lap can contain multiple disconnected sections separated by
           earlier obstacles, so we examine only the section whose end-nodes
           bracket the new sample y-range — not the entire lap.

           Pass 1: find the nearest end-node (top, bottom, or both) whose y
           is <= sample_y_lo (lower fence) and the nearest one whose y is
           >= sample_y_hi (upper fence).  These two end-nodes delimit the
           section that the new obstacle-adjacent nodes will land in.

           Pass 2: check whether every existing node within [section_lo,
           section_hi] is Closed; nodes outside that range belong to a
           different section and are ignored. */
        float sample_y_lo = anchor_y + (float)first_sample_index * step;
        float sample_y_hi = anchor_y + (float)last_sample_index * step;

        float section_lo = -INFINITY; /* greatest end-node y <= sample_y_lo */
        float section_hi = INFINITY;  /* smallest end-node y >= sample_y_hi */

        for (int ni = 0; ni < existing_lap_node_count; ++ni)
        {
            int nid = lap->node_ids[ni];
            int nidx = cstar_rcg_index_from_node_id(rcg, nid);
            if (nidx == CSTAR_NO_NEIGHBOR)
                continue;
            const cstar_node_t *nd = &rcg->nodes[nidx];
            bool is_end = nd->is_top_end_node ||
                          nd->is_bottom_end_node ||
                          nd->is_top_and_bottom_end_node;
            if (!is_end)
                continue;
            float ny = nd->pos.y;
            if (ny <= sample_y_lo && ny > section_lo)
                section_lo = ny;
            if (ny >= sample_y_hi && ny < section_hi)
                section_hi = ny;
        }

        int section_node_count = 0;
        bool lap_fully_covered = true;
        for (int ni = 0; ni < existing_lap_node_count; ++ni)
        {
            int nid = lap->node_ids[ni];
            int nidx = cstar_rcg_index_from_node_id(rcg, nid);
            if (nidx == CSTAR_NO_NEIGHBOR)
                continue;
            const cstar_node_t *nd = &rcg->nodes[nidx];
            float ny = nd->pos.y;
            if (ny < section_lo || ny > section_hi)
                continue; /* different section */
            section_node_count++;
            if (nd->state != CSTAR_NODE_CL)
            {
                lap_fully_covered = false;
                break;
            }
        }
        if (section_node_count == 0)
            lap_fully_covered = false;

        for (int si = first_sample_index; si <= last_sample_index; ++si)
        {
            float y = anchor_y + ((float)si * step);
            point_t sample = {lap->x, y};

            if (!cstar_sampling_point_is_free(sample, env))
                continue;

            /* Accept samples that are either near the operational boundary
               OR near the hit obstacle's own perimeter.  The standard
               boundary-distance filter rejects interior nodes because they
               are far from the outer boundary polygon — so we also check the
               obstacle's edges. */
            {
                float boundary_dist = cstar_sampling_dist_to_operational_boundary(sample, env);
                float obs_dist = INFINITY;
                for (uint32_t vi = 0; vi < obstacle->vertex_count; ++vi)
                {
                    uint32_t vn = (vi + 1u) % obstacle->vertex_count;
                    float d = cstar_sampling_dist_point_segment(
                        sample,
                        obstacle->vertices[vi],
                        obstacle->vertices[vn]);
                    if (d < obs_dist)
                        obs_dist = d;
                }
                if (!(boundary_dist < w) && !(obs_dist < w))
                    continue;
            }

            /* Deduplication: skip if an existing node sits within dedup_eps. */
            bool duplicate = false;
            for (int ni = 0; ni < rcg->node_count; ++ni)
            {
                float dx = rcg->nodes[ni].pos.x - sample.x;
                float dy = rcg->nodes[ni].pos.y - sample.y;
                if (dx * dx + dy * dy <= dedup_eps * dedup_eps)
                {
                    duplicate = true;
                    break;
                }
            }
            if (duplicate)
                continue;

            bool is_top = cstar_sampling_is_top_end_node_vertical(sample, w, env);
            bool is_bottom = cstar_sampling_is_bottom_end_node_vertical(sample, w, env);
            bool is_both = cstar_sampling_is_top_and_bottom_end_node_vertical(sample, w, env);

            /* The gap between the obstacle and the zone boundary is narrower
               than w on this lap section.  A top-and-bottom end node here
               would be isolated in the chain (no edges in either direction)
               and unreachable by A*, causing an infinite dead-end loop.
               Skip it — the existing zone-boundary end node already acts as
               the effective section terminus for this corridor. */
            if (is_both)
                continue;

            int node_id = cstar_rcg_add_node(rcg, sample, lap->id,
                                             is_top, is_bottom, is_both, false);
            if (node_id == CSTAR_NO_NEIGHBOR)
                continue;

            /* Pre-close if the lap was already fully traversed: every
               pre-existing same-lap node was Closed before this pass,
               meaning the robot swept through this y-position already. */
            if (lap_fully_covered)
            {
                cstar_node_t *new_node = cstar_rcg_get_node_by_id_mut(rcg, node_id);
                if (new_node != NULL)
                    new_node->state = CSTAR_NODE_CL;
            }

            cvector_push_back(lap->node_ids, node_id);
            lap->node_count = (int)cvector_size(lap->node_ids);
            lap->node_capacity = (int)cvector_capacity(lap->node_ids);

            cvector_push_back(new_ids, node_id);
            total_added++;
        }
    }

    if (total_added == 0)
    {
        cvector_free(new_ids);
        return 0;
    }

    /*
     * Connect new nodes to existing nodes within margin.
     * Iterate only the pre-existing nodes so we don't double-count;
     * connections between the new nodes themselves are handled by the
     * second loop below.
     */
    for (int ni = 0; ni < (int)cvector_size(new_ids); ++ni)
    {
        int new_id = new_ids[ni];
        const cstar_node_t *nn = cstar_rcg_get_node_by_id(rcg, new_id);
        if (nn == NULL)
            continue;

        for (int ei = 0; ei < existing_node_count; ++ei)
        {
            const cstar_node_t *other = &rcg->nodes[ei];
            float dx = other->pos.x - nn->pos.x;
            float dy = other->pos.y - nn->pos.y;
            float dist = sqrtf(dx * dx + dy * dy);
            if (dist <= margin + CSTAR_OBSTACLE_EPSILON)
                cstar_rcg_add_unique_edge(&rcg->edges, new_id, other->id, dist);
        }

        /* Also connect new nodes to each other. */
        for (int nj = ni + 1; nj < (int)cvector_size(new_ids); ++nj)
        {
            int other_id = new_ids[nj];
            const cstar_node_t *on = cstar_rcg_get_node_by_id(rcg, other_id);
            if (on == NULL)
                continue;
            float dx = on->pos.x - nn->pos.x;
            float dy = on->pos.y - nn->pos.y;
            float dist = sqrtf(dx * dx + dy * dy);
            if (dist <= margin + CSTAR_OBSTACLE_EPSILON)
                cstar_rcg_add_unique_edge(&rcg->edges, new_id, other_id, dist);
        }
    }

    cvector_free(new_ids);

    rcg->edge_count = (int)cvector_size(rcg->edges);
    rcg->edge_capacity = (int)cvector_capacity(rcg->edges);

    cstar_rcg_rebuild_links_from_edges(rcg->nodes, rcg->node_count,
                                       rcg->edges, rcg->edge_count);

    return total_added;
}
