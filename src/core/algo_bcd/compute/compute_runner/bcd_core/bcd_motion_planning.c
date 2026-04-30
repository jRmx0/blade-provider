#include <stdio.h>
#include <stdbool.h>
#include "../../../../../../dependencies/cvector/cvector.h"

#include "bcd_cell_computation.h"
#include "bcd_motion_planning.h"
#include "bcd_ox_motion.h"
#include "bcd_transit_motion.h"

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
