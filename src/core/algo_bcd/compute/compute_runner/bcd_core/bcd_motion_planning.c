#include <stdio.h>
#include <stdbool.h>
#include "../../../../../../dependencies/cvector/cvector.h"

#include "bcd_cell_computation.h"
#include "bcd_motion_planning.h"
#include "bcd_ox_motion.h"

// IMPLEMENTATION --- compute_bcd_motion ----------------------------

int compute_bcd_motion(cvector_vector_type(bcd_cell_t) * cell_list,
                       const cvector_vector_type(int) * path_list,
                       bcd_motion_plan_t *motion_plan,
                       float step_size,
                       point_t end_point)
{
    (void)end_point;

    if (cell_list == NULL || *cell_list == NULL ||
        path_list == NULL || *path_list == NULL ||
        motion_plan == NULL)
    {
        return -2;
    }

    int cell_count = (int)cvector_size(*cell_list);

    size_t i;
    for (i = 0; i < cvector_size(*path_list); ++i)
    {
        int path_cell_index = (*path_list)[i];
        if (path_cell_index < 0 || path_cell_index >= cell_count)
        {
            return -3;
        }

        if ((*cell_list)[path_cell_index].cleaned == true)
        {
            continue;
        }

        cvector_vector_type(point_t) ox = NULL;
        ox = compute_boustrophedon_motion((const cvector_vector_type(bcd_cell_t) *)cell_list,
                                          path_cell_index,
                                          step_size);
        if (ox == NULL)
        {
            return -1;
        }

        // Push current section with nav=NULL.
        // Transit paths are computed centrally in bcd_runner.c using
        // find_free_space_path() (visibility-graph A*) for all transit segments.
        cell_motion_plan_t curr_section;
        curr_section.ox = ox;
        curr_section.nav = NULL;

        cvector_push_back(motion_plan->section, curr_section);

        (*cell_list)[path_cell_index].cleaned = true;
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
