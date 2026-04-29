#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include "../../../../../../dependencies/cvector/cvector.h"

#include "bcd_cell_computation.h"
#include "bcd_motion_planning.h"
#include "bcd_geometry.h"

// --- COMPUTE_BCD_MOTION

static cvector_vector_type(point_t) compute_boustrophedon_motion(const cvector_vector_type(bcd_cell_t) * cell_list,
                                                                 int cell_index,
                                                                 float step_size);

// --- --- COMPUTE_BOUSTROPHEDON_MOTION

static int find_intersecting_edge_index(float x,
                                        const polygon_edge_t *edge_list,
                                        int edge_count);

// --- --- --- FIND_INTERSECTING_EDGE_INDEX

static bool is_x_in_edge_range(float x,
                               const polygon_edge_t *edge);

// --- ---

static bool try_find_intersection_point(float x,
                                        const polygon_edge_t *edge_list,
                                        int edge_count,
                                        point_t *out_point);

// --- --- --- TRY_FIND_INTERSECTION_POINT

static float find_y_intersection(float x,
                                 const polygon_edge_t *edge);

// --- ---

static void add_edge_transition_path(cvector_vector_type(point_t) * path,
                                     const polygon_edge_t *edge_list,
                                     int edge_count,
                                     int start_edge_index,
                                     int end_edge_index,
                                     bool chain_reversed);

// --- ---

static float compute_sweep_x(float cell_start_x, int line_index, float step_size, float cell_end_x);

// --- ---

static bool compute_sweep_endpoints(float x,
                                    const polygon_edge_t *ceiling_edges, int ceiling_count,
                                    const polygon_edge_t *floor_edges, int floor_count,
                                    bool going_down,
                                    point_t *out_start, point_t *out_end);

// --- ---

static bool append_sweep_line_connection(cvector_vector_type(point_t) * ox,
                                         float current_x, float next_x,
                                         const bcd_cell_t *cell, bool going_down);

// --- --- COMPUTE_BOUSTROPHEDON_MOTION

static point_t compute_crossing_waypoint(const bcd_cell_t *cell_a, const bcd_cell_t *cell_b);

// ---

static void append_cell_spine_waypoints(cvector_vector_type(point_t) * nav,
                                        const bcd_cell_t *cell,
                                        float x_from, float x_to);

// ---

static cvector_vector_type(int) extract_cell_chain(const cvector_vector_type(int) * path_list,
                                                   int begin_path_pos,
                                                   int end_path_pos);

// ---

static cvector_vector_type(point_t) compute_connection_motion(const cvector_vector_type(bcd_cell_t) * cell_list,
                                                              const cvector_vector_type(int) * path_list,
                                                              int begin_path_pos,
                                                              point_t begin_point,
                                                              int end_path_pos,
                                                              point_t end_point);

// IMPLEMENTATION --- compute_bcd_motion ----------------------------

int compute_bcd_motion(cvector_vector_type(bcd_cell_t) * cell_list,
                       const cvector_vector_type(int) * path_list,
                       bcd_motion_plan_t *motion_plan,
                       float step_size)
{
    int begin_path_pos = 0;
    point_t begin_point = {0};
    bool compute_nav = false;

    // Track the very first coverage point for the closing return nav.
    int first_path_pos = 0;
    point_t first_point = {0};
    bool first_recorded = false;

    size_t i;
    for (i = 0; i < cvector_size(*path_list); ++i)
    {
        if ((*cell_list)[(*path_list)[i]].cleaned == true)
        {
            continue;
        }

        cvector_vector_type(point_t) ox = NULL;
        ox = compute_boustrophedon_motion((const cvector_vector_type(bcd_cell_t) *)cell_list,
                                          (*path_list)[i],
                                          step_size);
        if (ox == NULL)
        {
            return -1;
        }

        if (compute_nav)
        {
            // Nav connects the previous section's coverage END to this section's
            // coverage START.  Assign it to the already-pushed previous section
            // so that each section's nav describes how to LEAVE that section,
            // not how to arrive at it.
            point_t end_point = *cvector_front(ox);

            cvector_vector_type(point_t) nav =
                compute_connection_motion((const cvector_vector_type(bcd_cell_t) *)cell_list,
                                          (const cvector_vector_type(int) *)path_list,
                                          begin_path_pos,
                                          begin_point,
                                          (int)i,
                                          end_point);

            int prev_section_idx = (int)cvector_size(motion_plan->section) - 1;
            motion_plan->section[prev_section_idx].nav = nav;
        }

        // Push current section with nav=NULL; nav will be filled in on the
        // next iteration once the next cell's start point is known.
        cell_motion_plan_t curr_section;
        curr_section.ox = ox;
        curr_section.nav = NULL;

        cvector_push_back(motion_plan->section, curr_section);

        (*cell_list)[(*path_list)[i]].cleaned = true;

        // Record the very first coverage point for the closing return nav.
        if (!first_recorded)
        {
            first_path_pos = (int)i;
            first_point = *cvector_front(ox);
            first_recorded = true;
        }

        begin_path_pos = (int)i;
        begin_point = *cvector_back(ox);
        compute_nav = true;
    }

    // Generate the closing nav: last coverage end → first coverage start.
    // path_list already encodes the return route: compute_bcd_path_list appends
    // a shortest-path back to starting_cell_index at the tail of path_list.
    // Slicing forward from begin_path_pos to path_list.size()-1 gives the
    // correct cell-visit-order return journey without reversing anything.
    if (first_recorded && cvector_size(motion_plan->section) > 0)
    {
        int last_section_idx = (int)cvector_size(motion_plan->section) - 1;
        int path_list_end = (int)cvector_size(*path_list) - 1;

        cvector_vector_type(point_t) return_nav =
            compute_connection_motion((const cvector_vector_type(bcd_cell_t) *)cell_list,
                                      (const cvector_vector_type(int) *)path_list,
                                      begin_path_pos,
                                      begin_point,
                                      path_list_end,
                                      first_point);

        motion_plan->section[last_section_idx].nav = return_nav;
    }

    return 0;
}

// --- COMPUTE_BCD_MOTION

static cvector_vector_type(point_t) compute_boustrophedon_motion(const cvector_vector_type(bcd_cell_t) * cell_list,
                                                                 int cell_index,
                                                                 float step_size) // the distance between two parallel line segments
{
    cvector_vector_type(point_t) ox = NULL;

    if (cell_list == NULL || cell_index < 0 || cell_index >= cvector_size(*cell_list))
    {
        return ox;
    }

    const bcd_cell_t *cell = &(*cell_list)[cell_index];

    float cell_start_x = cell->c_begin.x;
    float cell_end_x = cell->c_end.x;
    float cell_width = cell_end_x - cell_start_x;

    if (cell_width <= 0 || step_size <= 0)
    {
        return ox;
    }

    int num_lines = (int)(cell_width / step_size) + 1;

    bool going_down = true;

    for (int i = 0; i < num_lines; i++)
    {
        float current_x = compute_sweep_x(cell_start_x, i, step_size, cell_end_x);

        point_t start_point, end_point;
        if (!compute_sweep_endpoints(current_x,
                                     cell->ceiling_edge_list, (int)cvector_size(cell->ceiling_edge_list),
                                     cell->floor_edge_list, (int)cvector_size(cell->floor_edge_list),
                                     going_down, &start_point, &end_point))
        {
            cvector_free(ox);
            return NULL;
        }

        if (i == 0)
        {
            cvector_push_back(ox, start_point);
        }

        cvector_push_back(ox, end_point);

        if (i < num_lines - 1)
        {
            float next_x = compute_sweep_x(cell_start_x, i + 1, step_size, cell_end_x);
            if (!append_sweep_line_connection(&ox, current_x, next_x, cell, going_down))
            {
                cvector_free(ox);
                return NULL;
            }
        }

        going_down = !going_down;
    }

    return ox;
}

// --- --- COMPUTE_BOUSTROPHEDON_MOTION

static int find_intersecting_edge_index(float x, const polygon_edge_t *edge_list, int edge_count)
{
    if (edge_list == NULL || edge_count == 0)
    {
        return -1;
    }

    for (int i = 0; i < edge_count; i++)
    {
        if (is_x_in_edge_range(x, &edge_list[i]))
        {
            return i;
        }
    }

    return -1; // No intersecting edge found
}

// --- --- --- FIND_INTERSECTING_EDGE_INDEX

static bool is_x_in_edge_range(float x, const polygon_edge_t *edge)
{
    float min_x = (edge->begin.x < edge->end.x) ? edge->begin.x : edge->end.x;
    float max_x = (edge->begin.x > edge->end.x) ? edge->begin.x : edge->end.x;

    return (x >= min_x && x <= max_x);
}

// --- ---

static bool try_find_intersection_point(float x, const polygon_edge_t *edge_list, int edge_count, point_t *out_point)
{
    int edge_index = find_intersecting_edge_index(x, edge_list, edge_count);
    if (edge_index < 0)
    {
        return false;
    }

    out_point->x = x;
    out_point->y = find_y_intersection(x, &edge_list[edge_index]);
    return true;
}

// --- --- --- FIND_INTERSECTION__POINT

static float find_y_intersection(float x, const polygon_edge_t *edge)
{
    // Linear interpolation between edge start and end points
    if (edge->end.x == edge->begin.x) // Vertical edge
    {
        return edge->begin.y; // Could be either begin or end y, they should be the same for vertical
    }

    float t = (x - edge->begin.x) / (edge->end.x - edge->begin.x);
    return edge->begin.y + t * (edge->end.y - edge->begin.y);
}

// --- ---

static void add_edge_transition_path(cvector_vector_type(point_t) * path,
                                     const polygon_edge_t *edge_list,
                                     int edge_count,
                                     int start_edge_index,
                                     int end_edge_index,
                                     bool chain_reversed)
{
    if (start_edge_index == end_edge_index || start_edge_index == -1 || end_edge_index == -1)
    {
        return; // No transition needed
    }

    // Determine direction of traversal through the index space.
    bool index_increasing = start_edge_index < end_edge_index;

    // Ceiling chain (chain_reversed=false): index 0 = leftmost segment.
    //   .begin = left vertex, .end = right vertex.
    //   Kink between edge[i] and edge[i+1] is at edge[i].end  (increasing index → emit .end).
    //   Kink between edge[i] and edge[i-1] is at edge[i].begin (decreasing index → emit .begin).
    //
    // Floor chain (chain_reversed=true): index 0 = leftmost segment,
    //   but .begin = RIGHT vertex and .end = LEFT vertex (opposite of ceiling).
    //   Kink between floor[i] and floor[i+1] is at floor[i].begin (increasing index → emit .begin).
    //   Kink between floor[i] and floor[i-1] is at floor[i].end   (decreasing index → emit .end).

    if (index_increasing)
    {
        for (int i = start_edge_index; i < end_edge_index; i++)
        {
            point_t kink = chain_reversed ? edge_list[i].begin : edge_list[i].end;
            cvector_push_back(*path, kink);
        }
    }
    else
    {
        for (int i = start_edge_index; i > end_edge_index; i--)
        {
            point_t kink = chain_reversed ? edge_list[i].end : edge_list[i].begin;
            cvector_push_back(*path, kink);
        }
    }
}

// --- ---

static float compute_sweep_x(float cell_start_x, int line_index, float step_size, float cell_end_x)
{
    float x = cell_start_x + (float)line_index * step_size;
    return x > cell_end_x ? cell_end_x : x;
}

static bool compute_sweep_endpoints(float x,
                                    const polygon_edge_t *ceiling_edges, int ceiling_count,
                                    const polygon_edge_t *floor_edges, int floor_count,
                                    bool going_down,
                                    point_t *out_start, point_t *out_end)
{
    const polygon_edge_t *start_edges = going_down ? ceiling_edges : floor_edges;
    int start_count = going_down ? ceiling_count : floor_count;
    const polygon_edge_t *end_edges = going_down ? floor_edges : ceiling_edges;
    int end_count = going_down ? floor_count : ceiling_count;

    if (!try_find_intersection_point(x, start_edges, start_count, out_start))
        return false;
    if (!try_find_intersection_point(x, end_edges, end_count, out_end))
        return false;

    return true;
}

static bool append_sweep_line_connection(cvector_vector_type(point_t) * ox,
                                         float current_x, float next_x,
                                         const bcd_cell_t *cell, bool going_down)
{
    // Active boundary: where the current sweep ended and where the next sweep starts.
    // going_down=true  -> ended at floor, next starts at floor.
    // going_down=false -> ended at ceiling, next starts at ceiling.
    const polygon_edge_t *active_edges = going_down ? cell->floor_edge_list : cell->ceiling_edge_list;
    int active_count = going_down
                           ? (int)cvector_size(cell->floor_edge_list)
                           : (int)cvector_size(cell->ceiling_edge_list);

    int current_edge_index = find_intersecting_edge_index(current_x, active_edges, active_count);
    int next_edge_index = find_intersecting_edge_index(next_x, active_edges, active_count);

    if (current_edge_index != -1 && next_edge_index != -1 &&
        current_edge_index != next_edge_index)
    {
        // Floor edge list is stored right→left (chain_reversed=true).
        // Ceiling edge list is stored left→right (chain_reversed=false).
        bool chain_reversed = going_down; // going_down=true means active boundary is floor
        add_edge_transition_path(ox, active_edges, active_count,
                                 current_edge_index, next_edge_index, chain_reversed);
    }

    point_t next_start;
    if (!try_find_intersection_point(next_x, active_edges, active_count, &next_start))
        return false;

    cvector_push_back(*ox, next_start);
    return true;
}

// --- --- COMPUTE_BOUSTROPHEDON_MOTION

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

// ---

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

// IMPLEMENTATION --- compute_connection_motion ----------------------

static cvector_vector_type(point_t) compute_connection_motion(const cvector_vector_type(bcd_cell_t) * cell_list,
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

    // Need at least two cells (begin + end)
    if (chain_len < 2)
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

// MOTION_PLAN HELPERS

void log_bcd_motion(const bcd_motion_plan_t motion_plan)
{
    printf("BCD Motion Plan:\n");

    if (motion_plan.section == NULL)
    {
        printf("  (NULL motion plan)\n");
        return;
    }

    int section_count = cvector_size(motion_plan.section);
    printf("  Total sections: %d\n", section_count);

    if (section_count == 0)
    {
        printf("  (no sections)\n");
        return;
    }

    for (int i = 0; i < section_count; i++)
    {
        const cell_motion_plan_t *section = &motion_plan.section[i];
        printf("  Section %d:\n", i);

        // Log coverage motion (ox)
        if (section->ox == NULL)
        {
            printf("    Coverage: (NULL point list)\n");
        }
        else
        {
            int point_count = cvector_size(section->ox);
            printf("    Coverage points: %d (continuous path)\n", point_count);

            if (point_count == 0)
            {
                printf("    Coverage: (no points)\n");
            }
            else
            {
                // Log the continuous path points
                printf("    Path: ");
                for (int j = 0; j < point_count; j++)
                {
                    point_t point = section->ox[j];
                    printf("(%.2f, %.2f)", point.x, point.y);
                    if (j < point_count - 1)
                    {
                        printf(" -> ");
                    }

                    // Break line every 4 points for readability
                    if ((j + 1) % 4 == 0 && j < point_count - 1)
                    {
                        printf("\n          ");
                    }
                }
                printf("\n");
            }
        }

        // Log navigation motion (nav)
        if (section->nav == NULL)
        {
            printf("    Navigation: (NULL point list)\n");
        }
        else
        {
            int nav_count = cvector_size(section->nav);
            printf("    Navigation points: %d\n", nav_count);

            if (nav_count == 0)
            {
                printf("    Navigation: (no points)\n");
            }
            else
            {
                for (int j = 0; j < nav_count; j++)
                {
                    point_t nav_point = section->nav[j];
                    printf("      Nav %d: (%.2f, %.2f)\n", j, nav_point.x, nav_point.y);
                }
            }
        }
    }
}

void free_bcd_motion(bcd_motion_plan_t *motion_plan)
{
    if (motion_plan == NULL)
        return;

    if (motion_plan->section != NULL)
    {
        // Free each section's point vectors
        for (int i = 0; i < cvector_size(motion_plan->section); i++)
        {
            cvector_free(motion_plan->section[i].ox);
            cvector_free(motion_plan->section[i].nav);
        }

        cvector_free(motion_plan->section);
        motion_plan->section = NULL;
    }
}