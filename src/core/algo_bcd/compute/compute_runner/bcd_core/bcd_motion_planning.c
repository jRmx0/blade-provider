#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include "../../../../../../dependencies/cvector/cvector.h"

#include "bcd_cell_computation.h"
#include "bcd_motion_planning.h"

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
                                     int end_edge_index);

// --- ---

static float compute_sweep_x(float cell_start_x, int line_index, float step_size, float cell_end_x);

// --- ---

static bool compute_sweep_endpoints(float x,
                                    const polygon_edge_t *ceiling_edges, int ceiling_count,
                                    const polygon_edge_t *floor_edges, int floor_count,
                                    bool going_down,
                                    point_t *out_start, point_t *out_end);

// --- ---

static bool append_sweep_line_connection(cvector_vector_type(point_t) *ox,
                                         float current_x, float next_x,
                                         const bcd_cell_t *cell, bool going_down);

// --- --- COMPUTE_BOUSTROPHEDON_MOTION

static cvector_vector_type(point_t) compute_connection_motion(const cvector_vector_type(bcd_cell_t) * cell_list,
                                                              const cvector_vector_type(int) * path_list,
                                                              int begin_cell_index,
                                                              point_t begin_point,
                                                              int end_cell_index,
                                                              point_t end_point);

// IMPLEMENTATION --- compute_bcd_motion ----------------------------

int compute_bcd_motion(cvector_vector_type(bcd_cell_t) * cell_list,
                       const cvector_vector_type(int) * path_list,
                       bcd_motion_plan_t *motion_plan,
                       float step_size)
{
    int begin_cell_index;
    point_t begin_point = {0};
    int end_cell_index;
    point_t end_point = {0};
    bool compute_nav = false;

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

        cvector_vector_type(point_t) nav = NULL;

        if (compute_nav)
        {
            end_cell_index = (*path_list)[i];
            end_point = *cvector_front(ox);

            nav = compute_connection_motion((const cvector_vector_type(bcd_cell_t) *)cell_list,
                                            (const cvector_vector_type(int) *)path_list,
                                            begin_cell_index,
                                            begin_point,
                                            end_cell_index,
                                            end_point);
        }

        cell_motion_plan_t curr_section;
        curr_section.ox = ox;
        curr_section.nav = nav; // Navigation not implemented yet

        cvector_push_back(motion_plan->section, curr_section);

        (*cell_list)[(*path_list)[i]].cleaned = true;

        begin_cell_index = (*path_list)[i];
        begin_point = *cvector_back(ox);
        compute_nav = true;
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
    float cell_end_x   = cell->c_end.x;
    float cell_width   = cell_end_x - cell_start_x;

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
                                     cell->floor_edge_list,   (int)cvector_size(cell->floor_edge_list),
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
                                     int end_edge_index)
{
    if (start_edge_index == end_edge_index || start_edge_index == -1 || end_edge_index == -1)
    {
        return; // No transition needed
    }

    // Determine direction of traversal
    bool forward = start_edge_index < end_edge_index;

    if (forward)
    {
        // Moving forward (left to right): the kink vertex between edge[i] and
        // edge[i+1] is edge[i].begin (= edge[i-1].end by chain invariant).
        // Pushing only edge[i].begin per step avoids duplicating the shared
        // boundary point that the previous iteration already emitted.
        for (int i = start_edge_index; i < end_edge_index; i++)
        {
            cvector_push_back(*path, edge_list[i].begin);
        }
    }
    else
    {
        // Moving backward (right to left): the kink vertex between edge[i] and
        // edge[i-1] is edge[i].end.
        for (int i = start_edge_index; i > end_edge_index; i--)
        {
            cvector_push_back(*path, edge_list[i].end);
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
    int start_count                   = going_down ? ceiling_count : floor_count;
    const polygon_edge_t *end_edges   = going_down ? floor_edges   : ceiling_edges;
    int end_count                     = going_down ? floor_count   : ceiling_count;

    if (!try_find_intersection_point(x, start_edges, start_count, out_start))
        return false;
    if (!try_find_intersection_point(x, end_edges, end_count, out_end))
        return false;

    return true;
}

static bool append_sweep_line_connection(cvector_vector_type(point_t) *ox,
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
    int next_edge_index    = find_intersecting_edge_index(next_x,    active_edges, active_count);

    if (current_edge_index != -1 && next_edge_index != -1 &&
        current_edge_index != next_edge_index)
    {
        add_edge_transition_path(ox, active_edges, active_count,
                                 current_edge_index, next_edge_index);
    }

    point_t next_start;
    if (!try_find_intersection_point(next_x, active_edges, active_count, &next_start))
        return false;

    cvector_push_back(*ox, next_start);
    return true;
}

// --- --- COMPUTE_BOUSTROPHEDON_MOTION

static cvector_vector_type(point_t) compute_connection_motion(const cvector_vector_type(bcd_cell_t) * cell_list,
                                                              const cvector_vector_type(int) * path_list,
                                                              int begin_cell_index,
                                                              point_t begin_point,
                                                              int end_cell_index,
                                                              point_t end_point)
{
    return NULL;
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