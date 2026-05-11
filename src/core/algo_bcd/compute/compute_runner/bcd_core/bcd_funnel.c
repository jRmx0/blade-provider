#include "bcd_funnel.h"

#include <math.h>
#include <stdbool.h>
#include <stdlib.h>

#include "../../../../../../dependencies/cvector/cvector.h"

/* --------------------------------------------------------------------------
 * BCD Portal extraction
 *
 * A "portal" between two adjacent BCD cells is the shared vertical boundary
 * segment.  We use the shorter of the two cells' vertical spans at that
 * x-coordinate (same logic as compute_crossing_waypoint) because it represents
 * the tightest opening both cells physically share.
 *
 * portal_left  = top of that span (higher y value in BCD convention)
 * portal_right = bottom of that span (lower y value)
 *
 * The funnel algorithm threads a string through these portals left/right edges.
 * -------------------------------------------------------------------------- */

typedef struct
{
    point_t left;  /* left  edge of portal (typically higher y) */
    point_t right; /* right edge of portal (typically lower  y) */
} bcd_portal_t;

static bcd_portal_t extract_portal(const bcd_cell_t *cell_a, const bcd_cell_t *cell_b)
{
    point_t top_a, bottom_a;
    point_t top_b, bottom_b;

    if (cell_a->c_end.x <= cell_b->c_begin.x + 1e-3f &&
        cell_a->c_end.x >= cell_b->c_begin.x - 1e-3f)
    {
        top_a = cell_a->c_end;
        bottom_a = cell_a->f_begin;
        top_b = cell_b->c_begin;
        bottom_b = cell_b->f_end;
    }
    else if (cell_a->c_begin.x <= cell_b->c_end.x + 1e-3f &&
             cell_a->c_begin.x >= cell_b->c_end.x - 1e-3f)
    {
        top_a = cell_a->c_begin;
        bottom_a = cell_a->f_end;
        top_b = cell_b->c_end;
        bottom_b = cell_b->f_begin;
    }
    else if (cell_a->c_end.x < cell_b->c_end.x)
    {
        top_a = cell_a->c_end;
        bottom_a = cell_a->f_begin;
        top_b = cell_b->c_begin;
        bottom_b = cell_b->f_end;
    }
    else
    {
        top_a = cell_a->c_begin;
        bottom_a = cell_a->f_end;
        top_b = cell_b->c_end;
        bottom_b = cell_b->f_begin;
    }

    float span_a = fabsf(top_a.y - bottom_a.y);
    float span_b = fabsf(top_b.y - bottom_b.y);

    point_t top = (span_a <= span_b) ? top_a : top_b;
    point_t bottom = (span_a <= span_b) ? bottom_a : bottom_b;

    /* BCD ceiling has higher y, floor has lower y.
     * Assign left = ceiling side, right = floor side for funnel convention. */
    bcd_portal_t portal;
    if (top.y >= bottom.y)
    {
        portal.left = top;
        portal.right = bottom;
    }
    else
    {
        portal.left = bottom;
        portal.right = top;
    }
    return portal;
}

/* --------------------------------------------------------------------------
 * 2-D cross product of vectors (O→A) and (O→B).
 * Positive: B is to the left of O→A
 * Negative: B is to the right of O→A
 * -------------------------------------------------------------------------- */
static float cross2d(point_t O, point_t A, point_t B)
{
    return (A.x - O.x) * (B.y - O.y) - (A.y - O.y) * (B.x - O.x);
}

/* --------------------------------------------------------------------------
 * Funnel algorithm (SSFA)
 *
 * References:
 *   Mikko Mononen, "Simple Stupid Funnel Algorithm", 2010.
 *   http://digestingduck.blogspot.com/2010/03/simple-stupid-funnel-algorithm.html
 *
 * The corridor is represented as a sequence of portals (left/right edge pairs
 * at each cell boundary).  We add a degenerate "portal" at the start (both
 * edges = from_point) and at the end (both edges = to_point) so the algorithm
 * naturally emits the start and end as waypoints.
 * -------------------------------------------------------------------------- */

cvector_vector_type(point_t) bcd_funnel(
    const cvector_vector_type(bcd_cell_t) * cell_list,
    const cvector_vector_type(int) * cell_corridor,
    point_t from_point,
    point_t to_point)
{
    cvector_vector_type(point_t) result = NULL;

    if (cell_list == NULL || *cell_list == NULL ||
        cell_corridor == NULL || *cell_corridor == NULL)
        return result;

    int corridor_len = (int)cvector_size(*cell_corridor);
    int cell_count = (int)cvector_size(*cell_list);

    if (corridor_len == 0)
        return result;

    /* Validate all cell indices in corridor */
    for (int i = 0; i < corridor_len; ++i)
    {
        if ((*cell_corridor)[i] < 0 || (*cell_corridor)[i] >= cell_count)
            return result;
    }

    /* Build portal list.
     * Portal 0         : degenerate (from_point, from_point)
     * Portals 1..n-1   : shared boundaries between adjacent corridor cells
     * Portal n (last)  : degenerate (to_point, to_point)
     */
    int portal_count = corridor_len; /* n-1 boundaries + 1 degenerate end = corridor_len */
    /* We need corridor_len - 1 real portals + 2 degenerate = corridor_len + 1 total */
    portal_count = corridor_len + 1;

    bcd_portal_t *portals =
        (bcd_portal_t *)malloc((size_t)portal_count * sizeof(bcd_portal_t));
    if (portals == NULL)
        return result;

    /* Portal 0: start degenerate */
    portals[0].left = from_point;
    portals[0].right = from_point;

    /* Real portals between adjacent cells */
    for (int i = 0; i < corridor_len - 1; ++i)
    {
        const bcd_cell_t *ca = &(*cell_list)[(*cell_corridor)[i]];
        const bcd_cell_t *cb = &(*cell_list)[(*cell_corridor)[i + 1]];
        portals[i + 1] = extract_portal(ca, cb);
    }

    /* Last portal: end degenerate */
    portals[corridor_len].left = to_point;
    portals[corridor_len].right = to_point;

    /* --- SSFA ----------------------------------------------------------- */

    cvector_push_back(result, from_point);

    point_t apex = from_point;
    point_t portal_left = from_point;
    point_t portal_right = from_point;
    int apex_index = 0;
    int left_index = 0;
    int right_index = 0;

    for (int i = 1; i < portal_count; ++i)
    {
        point_t new_left = portals[i].left;
        point_t new_right = portals[i].right;

        /* --- Process right edge ---------------------------------------- */
        if (cross2d(apex, portal_right, new_right) <= 0.0f)
        {
            if (apex.x == portal_right.x && apex.y == portal_right.y ||
                cross2d(apex, portal_left, new_right) > 0.0f)
            {
                /* Tighten funnel right */
                portal_right = new_right;
                right_index = i;
            }
            else
            {
                /* Right edge crosses over left — emit left as waypoint, restart */
                cvector_push_back(result, portal_left);
                apex = portal_left;
                apex_index = left_index;
                portal_left = apex;
                portal_right = apex;
                left_index = apex_index;
                right_index = apex_index;
                /* Reprocess this portal from the new apex */
                i = apex_index;
                continue;
            }
        }

        /* --- Process left edge ----------------------------------------- */
        if (cross2d(apex, portal_left, new_left) >= 0.0f)
        {
            if (apex.x == portal_left.x && apex.y == portal_left.y ||
                cross2d(apex, portal_right, new_left) < 0.0f)
            {
                /* Tighten funnel left */
                portal_left = new_left;
                left_index = i;
            }
            else
            {
                /* Left edge crosses over right — emit right as waypoint, restart */
                cvector_push_back(result, portal_right);
                apex = portal_right;
                apex_index = right_index;
                portal_left = apex;
                portal_right = apex;
                left_index = apex_index;
                right_index = apex_index;
                i = apex_index;
                continue;
            }
        }
    }

    /* Emit to_point if not already the last emitted waypoint */
    int res_size = (int)cvector_size(result);
    if (res_size == 0 ||
        result[res_size - 1].x != to_point.x ||
        result[res_size - 1].y != to_point.y)
    {
        cvector_push_back(result, to_point);
    }

    free(portals);
    return result;
}
