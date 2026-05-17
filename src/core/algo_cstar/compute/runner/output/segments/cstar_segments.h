#ifndef CSTAR_SEGMENTS_H
#define CSTAR_SEGMENTS_H

#include "../../../../cstar.h"
#include "../../../../../../../dependencies/cJSON/cJSON.h"

bool cstar_segments_append(cJSON *segments_arr,
                           cJSON *typed_arr,
                           int *segment_id,
                           const char *type,
                           const float *xs,
                           const float *ys,
                           int count);

bool cstar_segments_add_start_transit(cJSON *segments_arr,
                                      cJSON *coverage_transit_arr,
                                      int *segment_id,
                                      point_t start_point,
                                      point_t current_point);

bool cstar_segments_add_initial_coverage(cJSON *segments_arr,
                                         cJSON *coverage_arr,
                                         int *segment_id,
                                         point_t current_point,
                                         point_t goal_point);

#endif // CSTAR_SEGMENTS_H
