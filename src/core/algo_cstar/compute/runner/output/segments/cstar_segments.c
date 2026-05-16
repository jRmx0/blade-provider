#include "cstar_segments.h"
#include "../../../../../common/debug_serialize.h"

bool cstar_segments_append(cJSON *segments_arr,
                           cJSON *typed_arr,
                           int *segment_id,
                           const char *type,
                           const float *xs,
                           const float *ys,
                           int count)
{
    cJSON *segment = debug_build_segment(*segment_id, type, xs, ys, count);
    if (segment == NULL)
    {
        return false;
    }

    cJSON *typed_copy = cJSON_Duplicate(segment, 1);
    if (typed_copy == NULL)
    {
        cJSON_Delete(segment);
        return false;
    }

    cJSON_AddItemToArray(segments_arr, segment);
    cJSON_AddItemToArray(typed_arr, typed_copy);
    (*segment_id)++;
    return true;
}

bool cstar_segments_add_start_transit(cJSON *segments_arr,
                                      cJSON *coverage_transit_arr,
                                      int *segment_id,
                                      point_t start_point,
                                      point_t current_point)
{
    float xs[2] = {start_point.x, current_point.x};
    float ys[2] = {start_point.y, current_point.y};
    return cstar_segments_append(segments_arr,
                                 coverage_transit_arr,
                                 segment_id,
                                 "coverageTransit",
                                 xs,
                                 ys,
                                 2);
}

bool cstar_segments_add_initial_coverage(cJSON *segments_arr,
                                         cJSON *coverage_arr,
                                         int *segment_id,
                                         point_t current_point,
                                         point_t goal_point)
{
    float xs[2] = {current_point.x, goal_point.x};
    float ys[2] = {current_point.y, goal_point.y};
    return cstar_segments_append(segments_arr,
                                 coverage_arr,
                                 segment_id,
                                 "coverage",
                                 xs,
                                 ys,
                                 2);
}
