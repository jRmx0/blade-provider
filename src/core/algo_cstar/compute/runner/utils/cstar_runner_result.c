#include <stdlib.h>
#include <string.h>
#include "cstar_runner_result.h"

static cstar_coverage_path_result_t *cstar_result_create(void)
{
    cstar_coverage_path_result_t *result =
        (cstar_coverage_path_result_t *)malloc(sizeof(cstar_coverage_path_result_t));
    if (result == NULL)
    {
        return NULL;
    }

    result->all_segments = NULL;
    result->segment_count = 0;
    result->segment_capacity = 0;

    result->coverage.segments = NULL;
    result->coverage.segment_count = 0;
    result->coverage.segment_capacity = 0;

    result->coverage_transit.segments = NULL;
    result->coverage_transit.segment_count = 0;
    result->coverage_transit.segment_capacity = 0;

    result->retreat_transit.segments = NULL;
    result->retreat_transit.segment_count = 0;
    result->retreat_transit.segment_capacity = 0;

    result->hole_coverage.segments = NULL;
    result->hole_coverage.segment_count = 0;
    result->hole_coverage.segment_capacity = 0;

    result->hole_transit.segments = NULL;
    result->hole_transit.segment_count = 0;
    result->hole_transit.segment_capacity = 0;

    result->debug_layers.layers = NULL;
    result->debug_layers.layer_count = 0;
    result->debug_layers.layer_capacity = 0;

    return result;
}

/**
 * Add a segment to the result struct and to its appropriate collection.
 * Makes a copy of the segment data.
 */
static bool cstar_result_add_segment(cstar_coverage_path_result_t *result,
                                     int segment_id,
                                     const char *type,
                                     const point_t *path,
                                     int path_count)
{
    if (result == NULL || type == NULL || path == NULL || path_count <= 0)
    {
        return false;
    }

    // Grow segments array if needed
    if (result->segment_count >= result->segment_capacity)
    {
        int new_capacity = result->segment_capacity == 0 ? 16 : result->segment_capacity * 2;
        cstar_segment_t *new_segments =
            (cstar_segment_t *)realloc(result->all_segments, new_capacity * sizeof(cstar_segment_t));
        if (new_segments == NULL)
        {
            return false;
        }
        result->all_segments = new_segments;
        result->segment_capacity = new_capacity;
    }

    // Allocate and populate segment
    cstar_segment_t *seg = &result->all_segments[result->segment_count];
    seg->id = segment_id;

    seg->type = (char *)malloc(strlen(type) + 1);
    if (seg->type == NULL)
    {
        return false;
    }
    strcpy(seg->type, type);

    seg->path = (cstar_path_point_t *)malloc(path_count * sizeof(cstar_path_point_t));
    if (seg->path == NULL)
    {
        free(seg->type);
        return false;
    }

    for (int i = 0; i < path_count; ++i)
    {
        seg->path[i].id = i;
        seg->path[i].point = path[i];
    }
    seg->path_count = path_count;

    result->segment_count++;

    return true;
}

static void cstar_result_cleanup_partial(cstar_coverage_path_result_t *result)
{
    if (result == NULL)
    {
        return;
    }

    if (result->all_segments != NULL)
    {
        for (int i = 0; i < result->segment_count; ++i)
        {
            cstar_segment_t *seg = &result->all_segments[i];
            free(seg->type);
            seg->type = NULL;
            free(seg->path);
            seg->path = NULL;
            seg->path_count = 0;
        }

        free(result->all_segments);
        result->all_segments = NULL;
        result->segment_count = 0;
        result->segment_capacity = 0;
    }

    if (result->debug_layers.layers != NULL)
    {
        for (int i = 0; i < result->debug_layers.layer_count; ++i)
        {
            cstar_debug_layer_t *layer = &result->debug_layers.layers[i];

            if (layer->list_type == CSTAR_DEBUG_LIST_SEGMENTS)
            {
                for (int j = 0; j < layer->list.segments.count; ++j)
                {
                    free(layer->list.segments.items[j].path);
                    layer->list.segments.items[j].path = NULL;
                    layer->list.segments.items[j].path_count = 0;
                }
                free(layer->list.segments.items);
            }
            else if (layer->list_type == CSTAR_DEBUG_LIST_POLYGONS)
            {
                for (int j = 0; j < layer->list.polygons.count; ++j)
                {
                    free(layer->list.polygons.items[j].vertices);
                    layer->list.polygons.items[j].vertices = NULL;
                    layer->list.polygons.items[j].vertex_count = 0;
                }
                free(layer->list.polygons.items);
            }
            else
            {
                free(layer->list.points.items);
            }
        }

        free(result->debug_layers.layers);
        result->debug_layers.layers = NULL;
        result->debug_layers.layer_count = 0;
        result->debug_layers.layer_capacity = 0;
    }

    free(result);
}
