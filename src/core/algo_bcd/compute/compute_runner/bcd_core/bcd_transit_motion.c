#include <stdbool.h>
#include <math.h>
#include "../../../../../../dependencies/cvector/cvector.h"

#include "bcd_cell_computation.h"
#include "bcd_geometry.h"
#include "bcd_transit_motion.h"

// --- COMPUTE_CONNECTION_MOTION

static cvector_vector_type(int) extract_cell_chain(const cvector_vector_type(int) * path_list,
                                                   int begin_path_pos,
                                                   int end_path_pos);

// ---

static point_t compute_crossing_waypoint(const bcd_cell_t *cell_a, const bcd_cell_t *cell_b);

// ---

static void append_cell_spine_waypoints(cvector_vector_type(point_t) * nav,
                                        const bcd_cell_t *cell,
                                        float x_from, float x_to);

// IMPLEMENTATION --- compute_connection_motion ----------------------

cvector_vector_type(point_t) compute_connection_motion(const cvector_vector_type(bcd_cell_t) * cell_list,
                                                       const cvector_vector_type(int) * path_list,
                                                       int begin_path_pos,
                                                       point_t begin_point,
                                                       int end_path_pos,
                                                       point_t end_point)
{
    cvector_vector_type(point_t) nav = NULL;

    if (cell_list == NULL || path_list == NULL)
        return nav;

    // Extract the ordered chain of cell indices by slicing path_list at the
    // exact known positions — never by searching for a value, because the same
    // cell index can appear multiple times (as a BFS transit insertion and as a
    // primary coverage cell) and value-search would pick the wrong occurrence.
    cvector_vector_type(int) chain = extract_cell_chain(path_list, begin_path_pos, end_path_pos);

    int chain_len = (int)cvector_size(chain);

    // Single-cell transit: trace the medial-axis spine within the cell.
    // This handles the headland → first coverage entry where both endpoints
    // map to the same BCD cell.
    if (chain_len == 1)
    {
        int idx = chain[0];
        const bcd_cell_t *cell = &(*cell_list)[idx];
        cvector_push_back(nav, begin_point);
        append_cell_spine_waypoints(&nav, cell, begin_point.x, end_point.x);
        cvector_push_back(nav, end_point);
        cvector_free(chain);
        return nav;
    }

    // Need at least two cells (begin + end)
    if (chain_len < 1)
    {
        cvector_free(chain);
        return nav;
    }

    // Start at the begin point
    cvector_push_back(nav, begin_point);

    float current_x = begin_point.x;

    for (int i = 0; i < chain_len - 1; ++i)
    {
        int idx_a = chain[i];
        int idx_b = chain[i + 1];

        const bcd_cell_t *cell_a = &(*cell_list)[idx_a];
        const bcd_cell_t *cell_b = &(*cell_list)[idx_b];

        // Compute crossing waypoint: midpoint of the shared vertical cell boundary.
        point_t crossing = compute_crossing_waypoint(cell_a, cell_b);
        float x_cross = crossing.x;

        // Insert medial-axis spine waypoints for cell_a between current_x and
        // x_cross.  For each kink (slope change in ceiling/floor) strictly
        // between those x-values, the midpoint of the boundary gap is added.
        // This guarantees the piecewise-linear path stays inside cell_a even
        // when the boundary is concave due to obstacle deflection.
        append_cell_spine_waypoints(&nav, cell_a, current_x, x_cross);

        cvector_push_back(nav, crossing);
        current_x = x_cross;
    }

    // Handle the final cell: from its entry crossing to end_point.
    // The end cell may also have concave boundaries between those x-values.
    int idx_end = chain[chain_len - 1];
    const bcd_cell_t *end_cell = &(*cell_list)[idx_end];
    append_cell_spine_waypoints(&nav, end_cell, current_x, end_point.x);

    // Arrive at the end point (start of the next coverage sweep)
    cvector_push_back(nav, end_point);

    cvector_free(chain);
    return nav;
}

// --- COMPUTE_CONNECTION_MOTION

static cvector_vector_type(int) extract_cell_chain(const cvector_vector_type(int) * path_list,
                                                   int begin_path_pos,
                                                   int end_path_pos)
{
    cvector_vector_type(int) chain = NULL;

    if (path_list == NULL)
        return chain;

    int path_size = (int)cvector_size(*path_list);
    if (begin_path_pos < 0 || begin_path_pos >= path_size ||
        end_path_pos < begin_path_pos || end_path_pos >= path_size)
        return chain;

    // Slice path_list directly by position — no value search needed.
    // This is safe even when the same cell index appears multiple times
    // in path_list (e.g. as both a BFS transit insertion and a primary
    // coverage cell), because we always have the exact positions.
    for (int i = begin_path_pos; i <= end_path_pos; ++i)
        cvector_push_back(chain, (*path_list)[i]);

    return chain;
}

// ---

static point_t compute_crossing_waypoint(const bcd_cell_t *cell_a, const bcd_cell_t *cell_b)
{
    // Use the shorter of the two cells' boundary spans to place the crossing
    // waypoint.  At a BCD event vertex the two cells have asymmetric openings at
    // the shared x: one cell's boundary is clipped by the event vertex (shorter)
    // while the other's extends to the full polygon height (taller).  The shorter
    // span is the actual physical bottleneck both cells share, so its midpoint
    // is guaranteed to lie inside both cells' open space.
    //
    // Edge chains are not used here: they may be shorter than the full cell side
    // when an event vertex terminates them early, making intersection unreliable.
    //
    // Detect which side of the shared boundary each cell contributes by comparing
    // cell_a->c_end.x (right edge of A) with cell_b->c_begin.x (left edge of B),
    // then read both pairs of corners and pick the pair with the smaller span.
    point_t top_a, bottom_a;
    point_t top_b, bottom_b;

    if (cell_a->c_end.x <= cell_b->c_begin.x + 1e-3f &&
        cell_a->c_end.x >= cell_b->c_begin.x - 1e-3f)
    {
        // A's right edge is shared with B's left edge
        top_a = cell_a->c_end;
        bottom_a = cell_a->f_begin;
        top_b = cell_b->c_begin;
        bottom_b = cell_b->f_end;
    }
    else if (cell_a->c_begin.x <= cell_b->c_end.x + 1e-3f &&
             cell_a->c_begin.x >= cell_b->c_end.x - 1e-3f)
    {
        // A's left edge is shared with B's right edge
        top_a = cell_a->c_begin;
        bottom_a = cell_a->f_end;
        top_b = cell_b->c_end;
        bottom_b = cell_b->f_begin;
    }
    else if (cell_a->c_end.x < cell_b->c_end.x)
    {
        // Fallback: A is spatially left of B
        top_a = cell_a->c_end;
        bottom_a = cell_a->f_begin;
        top_b = cell_b->c_begin;
        bottom_b = cell_b->f_end;
    }
    else
    {
        // Fallback: A is spatially right of B
        top_a = cell_a->c_begin;
        bottom_a = cell_a->f_end;
        top_b = cell_b->c_end;
        bottom_b = cell_b->f_begin;
    }

    // Pick the pair with the shorter vertical span — that is the tighter
    // opening both cells must share, so its midpoint fits inside both.
    float span_a = fabsf(top_a.y - bottom_a.y);
    float span_b = fabsf(top_b.y - bottom_b.y);

    point_t top = (span_a <= span_b) ? top_a : top_b;
    point_t bottom = (span_a <= span_b) ? bottom_a : bottom_b;

    point_t waypoint;
    waypoint.x = top.x;
    waypoint.y = (top.y + bottom.y) / 2.0f;
    return waypoint;
}

// ---

static void append_cell_spine_waypoints(cvector_vector_type(point_t) * nav,
                                        const bcd_cell_t *cell,
                                        float x_from, float x_to)
{
    // Collect all kink x-values from the ceiling and floor edge chains that lie
    // strictly between x_from and x_to.  Kinks are the joints between adjacent
    // edge segments — at these x-values the boundary changes slope, so the
    // medial-axis midpoint y(x) also changes slope.  Inserting a waypoint at
    // each kink guarantees the piecewise-linear spine exactly traces the medial
    // axis and cannot exit through a concave boundary between waypoints.
    float x_min = x_from < x_to ? x_from : x_to;
    float x_max = x_from < x_to ? x_to : x_from;

    int ceil_n = (int)cvector_size(cell->ceiling_edge_list);
    int floor_n = (int)cvector_size(cell->floor_edge_list);

    // Ceiling kinks: interior joins at ceiling_edge_list[j].end.x,   j = 0..ceil_n-2
    //   (ceiling chain left→right: .end is the right/shared vertex)
    // Floor kinks:   interior joins at floor_edge_list[j].begin.x,   j = 0..floor_n-2
    //   (floor chain: .begin is the RIGHT vertex of segment j, shared with segment j+1)
    cvector_vector_type(float) kinks = NULL;

    for (int j = 0; j < ceil_n - 1; ++j)
    {
        float kx = cell->ceiling_edge_list[j].end.x;
        if (kx > x_min && kx < x_max)
            cvector_push_back(kinks, kx);
    }
    for (int j = 0; j < floor_n - 1; ++j)
    {
        float kx = cell->floor_edge_list[j].begin.x;
        if (kx > x_min && kx < x_max)
            cvector_push_back(kinks, kx);
    }

    if (kinks == NULL)
        return;

    // Insertion-sort kinks in direction of travel.
    // Kink count is always small (typically 0–3), so this is fast.
    int k_count = (int)cvector_size(kinks);
    bool ascending = (x_from < x_to);

    for (int a = 1; a < k_count; ++a)
    {
        float key = kinks[a];
        int b = a - 1;
        while (b >= 0 && (ascending ? kinks[b] > key : kinks[b] < key))
        {
            kinks[b + 1] = kinks[b];
            b--;
        }
        kinks[b + 1] = key;
    }

    for (int k = 0; k < k_count; ++k)
    {
        point_t spine_pt = bcd_cell_midpoint_at_x(cell, kinks[k]);
        cvector_push_back(*nav, spine_pt);
    }

    cvector_free(kinks);
}
