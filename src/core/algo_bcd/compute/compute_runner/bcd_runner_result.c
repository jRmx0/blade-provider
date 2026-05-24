#include <stdlib.h>
#include <string.h>
#include "bcd_runner_result.h"

/* Full definition of the opaque bcd_result_t — visible here and to bcd.c
 * via the C amalgamation include chain. Type headers (bcd_event_list_building.h,
 * bcd_cell_computation.h, bcd_motion_planning.h, headland.h, path_finder.h)
 * are included by bcd_runner.c before this file is amalgamated. */
struct bcd_result_t
{
    bcd_event_list_t event_list;
    cvector_vector_type(bcd_cell_t) cell_list;
    cvector_vector_type(int) path_list;
    bcd_motion_plan_t motion_plan;
    headland_t headland;
    bool has_headland;
    cvector_vector_type(point_t) start_nav;
};

static bcd_result_t *bcd_result_create(void)
{
    bcd_result_t *result = (bcd_result_t *)malloc(sizeof(bcd_result_t));
    if (result == NULL)
    {
        return NULL;
    }

    result->event_list.bcd_events = NULL;
    result->event_list.length = 0;
    result->cell_list = NULL;
    result->path_list = NULL;
    result->motion_plan = (bcd_motion_plan_t){0};
    memset(&result->headland, 0, sizeof(headland_t));
    result->has_headland = false;
    result->start_nav = NULL;

    return result;
}

static void bcd_result_populate(bcd_result_t *result,
                                bcd_event_list_t event_list,
                                cvector_vector_type(bcd_cell_t) cell_list,
                                cvector_vector_type(int) path_list,
                                bcd_motion_plan_t motion_plan,
                                bool has_headland,
                                headland_t headland,
                                cvector_vector_type(point_t) start_nav)
{
    result->event_list = event_list;
    result->cell_list = cell_list;
    result->path_list = path_list;
    result->motion_plan = motion_plan;
    result->has_headland = has_headland;
    if (has_headland)
        result->headland = headland;
    result->start_nav = start_nav;
}

static void bcd_result_free(bcd_result_t *result)
{
    if (result == NULL)
        return;
    free_bcd_event_list(&result->event_list);
    free_bcd_cell_list(&result->cell_list);
    cvector_free(result->path_list);
    free_bcd_motion(&result->motion_plan);
    cvector_free(result->start_nav);
    if (result->has_headland)
        free_headland(&result->headland);
    free(result);
}
