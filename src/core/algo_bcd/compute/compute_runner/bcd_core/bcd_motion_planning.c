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

static point_t find_intersection_point(float x,
                                       const polygon_edge_t *edge_list,
                                       int edge_count);

// --- --- --- FIND_INTERSECTION__POINT

static float find_y_intersection(float x,
                                 const polygon_edge_t *edge);

// --- ---

static void add_edge_transition_path(cvector_vector_type(point_t) * path,
                                     const polygon_edge_t *edge_list,
                                     int edge_count,
                                     int start_edge_index,
                                     int end_edge_index);

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

    // Get cell boundaries
    point_t ceiling_start = cell->c_begin;
    point_t ceiling_end = cell->c_end;
    point_t floor_start = cell->f_begin;
    point_t floor_end = cell->f_end;

    // Calculate the sweep direction (perpendicular to the cell)
    // Assuming cells are oriented vertically, sweep horizontally
    float cell_width = ceiling_end.x - ceiling_start.x;

    if (cell_width <= 0 || step_size <= 0)
    {
        return ox;
    }

    // Calculate number of sweep lines
    int num_lines = (int)(cell_width / step_size) + 1;

    // Generate boustrophedon pattern as a continuous path
    bool going_down = true; // Start by going from ceiling to floor

    for (int i = 0; i < num_lines; i++)
    {
        float x_offset = i * step_size;
        float current_x = ceiling_start.x + x_offset;

        // Don't exceed the cell boundary
        if (current_x > ceiling_end.x)
        {
            current_x = ceiling_end.x;
        }

        // Find which edges we're intersecting with
        int current_ceiling_edge_index = find_intersecting_edge_index(current_x, cell->ceiling_edge_list, cvector_size(cell->ceiling_edge_list));
        int current_floor_edge_index = find_intersecting_edge_index(current_x, cell->floor_edge_list, cvector_size(cell->floor_edge_list));

        point_t start_point, end_point;

        if (going_down)
        {
            // Going from ceiling to floor
            start_point = find_intersection_point(current_x, cell->ceiling_edge_list, cvector_size(cell->ceiling_edge_list));
            end_point = find_intersection_point(current_x, cell->floor_edge_list, cvector_size(cell->floor_edge_list));
        }
        else
        {
            // Going from floor to ceiling
            start_point = find_intersection_point(current_x, cell->floor_edge_list, cvector_size(cell->floor_edge_list));
            end_point = find_intersection_point(current_x, cell->ceiling_edge_list, cvector_size(cell->ceiling_edge_list));
        }

        // For the first line, add the start point
        if (i == 0)
        {
            cvector_push_back(ox, start_point);
        }

        // Always add the end point (this completes the current sweep line)
        cvector_push_back(ox, end_point);

        // Handle edge transitions and connecting to next line
        if (i < num_lines - 1)
        {
            // Calculate the next line position
            float next_x_offset = (i + 1) * step_size;
            float next_x = ceiling_start.x + next_x_offset;

            // Don't exceed the cell boundary
            if (next_x > ceiling_end.x)
            {
                next_x = ceiling_end.x;
            }

            // Find which edges the next line will intersect with
            int next_ceiling_edge_index = find_intersecting_edge_index(next_x, cell->ceiling_edge_list, cvector_size(cell->ceiling_edge_list));
            int next_floor_edge_index = find_intersecting_edge_index(next_x, cell->floor_edge_list, cvector_size(cell->floor_edge_list));

            // Determine which boundary we need to follow for transition.
            // Guard uses current_*_edge_index (not last_*) so the first transition
            // (i=0 -> i=1) is correctly handled even before any "last" index exists.
            if (going_down)
            {
                // Current sweep ended at floor, next will start at floor.
                // Follow the floor boundary between the two x positions if the
                // edge changes (i.e. a kink/obstacle vertex lies between them).
                if (current_floor_edge_index != -1 && next_floor_edge_index != -1 &&
                    current_floor_edge_index != next_floor_edge_index)
                {
                    add_edge_transition_path(&ox, cell->floor_edge_list, cvector_size(cell->floor_edge_list),
                                             current_floor_edge_index, next_floor_edge_index);
                }
            }
            else
            {
                // Current sweep ended at ceiling, next will start at ceiling.
                if (current_ceiling_edge_index != -1 && next_ceiling_edge_index != -1 &&
                    current_ceiling_edge_index != next_ceiling_edge_index)
                {
                    add_edge_transition_path(&ox, cell->ceiling_edge_list, cvector_size(cell->ceiling_edge_list),
                                             current_ceiling_edge_index, next_ceiling_edge_index);
                }
            }

            // Calculate the start point of the next line
            point_t next_start;
            if (!going_down) // Next line will go down (ceiling to floor)
            {
                next_start = find_intersection_point(next_x, cell->ceiling_edge_list, cvector_size(cell->ceiling_edge_list));
            }
            else // Next line will go up (floor to ceiling)
            {
                next_start = find_intersection_point(next_x, cell->floor_edge_list, cvector_size(cell->floor_edge_list));
            }

            // Add the start point of next line
            cvector_push_back(ox, next_start);
        }

        // Alternate direction for next line
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

static point_t find_intersection_point(float x, const polygon_edge_t *edge_list, int edge_count)
{
    point_t result = {x, 0.0f}; // Default point with x coordinate and y=0

    int edge_index = find_intersecting_edge_index(x, edge_list, edge_count);
    if (edge_index >= 0)
    {
        result.y = find_y_intersection(x, &edge_list[edge_index]);
    }
    else
    {
        // Fallback: if no edge found, try to use the first or last edge
        if (edge_list != NULL && edge_count > 0)
        {
            // Use the first edge as fallback
            result.y = find_y_intersection(x, &edge_list[0]);
        }
    }

    return result;
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