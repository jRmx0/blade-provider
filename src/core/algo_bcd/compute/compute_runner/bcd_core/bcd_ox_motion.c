#include <stdbool.h>
#include <math.h>
#include "../../../../../../dependencies/cvector/cvector.h"

#include "bcd_cell_computation.h"
#include "bcd_ox_motion.h"

// --- COMPUTE_BOUSTROPHEDON_MOTION

static int find_intersecting_edge_index(float x,
                                        const polygon_edge_t *edge_list,
                                        int edge_count);

// --- --- FIND_INTERSECTING_EDGE_INDEX

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

// IMPLEMENTATION --- compute_boustrophedon_motion ------------------

cvector_vector_type(point_t) compute_boustrophedon_motion(const cvector_vector_type(bcd_cell_t) * cell_list,
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

// --- COMPUTE_BOUSTROPHEDON_MOTION

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

// --- --- FIND_INTERSECTING_EDGE_INDEX

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

// --- --- --- TRY_FIND_INTERSECTION_POINT

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
