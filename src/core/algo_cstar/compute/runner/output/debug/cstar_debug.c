#include <string.h>
#include <stdlib.h>

#include "cstar_debug.h"
#include "../../core/preprocess/cstar_lap.h"

#define CSTAR_DEBUG_LAYER_COUNT 8

static bool cstar_debug_point_list_reserve(cstar_debug_point_list_t *list, int required)
{
    if (list == NULL)
    {
        return false;
    }

    if (required <= list->capacity)
    {
        return true;
    }

    int new_capacity = list->capacity == 0 ? 16 : list->capacity;
    while (new_capacity < required)
    {
        new_capacity *= 2;
    }

    cstar_debug_point_entry_t *items =
        (cstar_debug_point_entry_t *)realloc(list->items, (size_t)new_capacity * sizeof(cstar_debug_point_entry_t));
    if (items == NULL)
    {
        return false;
    }

    list->items = items;
    list->capacity = new_capacity;
    return true;
}

static bool cstar_debug_segment_list_reserve(cstar_debug_segment_list_t *list, int required)
{
    if (list == NULL)
    {
        return false;
    }

    if (required <= list->capacity)
    {
        return true;
    }

    int new_capacity = list->capacity == 0 ? 16 : list->capacity;
    while (new_capacity < required)
    {
        new_capacity *= 2;
    }

    cstar_debug_segment_entry_t *items =
        (cstar_debug_segment_entry_t *)realloc(list->items, (size_t)new_capacity * sizeof(cstar_debug_segment_entry_t));
    if (items == NULL)
    {
        return false;
    }

    list->items = items;
    list->capacity = new_capacity;
    return true;
}

static bool cstar_debug_polygon_list_reserve(cstar_debug_polygon_list_t *list, int required)
{
    if (list == NULL)
    {
        return false;
    }

    if (required <= list->capacity)
    {
        return true;
    }

    int new_capacity = list->capacity == 0 ? 8 : list->capacity;
    while (new_capacity < required)
    {
        new_capacity *= 2;
    }

    cstar_debug_polygon_entry_t *items =
        (cstar_debug_polygon_entry_t *)realloc(list->items, (size_t)new_capacity * sizeof(cstar_debug_polygon_entry_t));
    if (items == NULL)
    {
        return false;
    }

    list->items = items;
    list->capacity = new_capacity;
    return true;
}

static bool cstar_debug_append_point(cstar_debug_point_list_t *list, int id, point_t point)
{
    if (!cstar_debug_point_list_reserve(list, list->count + 1))
    {
        return false;
    }

    cstar_debug_point_entry_t *entry = &list->items[list->count];
    entry->id = id;
    entry->point = point;
    list->count++;
    return true;
}

static bool cstar_debug_append_segment(cstar_debug_segment_list_t *list,
                                       int id,
                                       const char *type,
                                       const point_t *path,
                                       int path_count)
{
    if (list == NULL || type == NULL || path == NULL || path_count <= 0)
    {
        return false;
    }

    if (!cstar_debug_segment_list_reserve(list, list->count + 1))
    {
        return false;
    }

    point_t *path_copy = (point_t *)malloc((size_t)path_count * sizeof(point_t));
    if (path_copy == NULL)
    {
        return false;
    }

    for (int i = 0; i < path_count; ++i)
    {
        path_copy[i] = path[i];
    }

    cstar_debug_segment_entry_t *entry = &list->items[list->count];
    entry->id = id;
    entry->type = type;
    entry->path = path_copy;
    entry->path_count = path_count;
    list->count++;
    return true;
}

static bool cstar_debug_append_polygon(cstar_debug_polygon_list_t *list,
                                       int id,
                                       const point_t *vertices,
                                       int vertex_count)
{
    if (list == NULL || vertices == NULL || vertex_count <= 0)
    {
        return false;
    }

    if (!cstar_debug_polygon_list_reserve(list, list->count + 1))
    {
        return false;
    }

    point_t *vertices_copy = (point_t *)malloc((size_t)vertex_count * sizeof(point_t));
    if (vertices_copy == NULL)
    {
        return false;
    }

    for (int i = 0; i < vertex_count; ++i)
    {
        vertices_copy[i] = vertices[i];
    }

    cstar_debug_polygon_entry_t *entry = &list->items[list->count];
    entry->id = id;
    entry->vertices = vertices_copy;
    entry->vertex_count = vertex_count;
    list->count++;
    return true;
}

static void cstar_debug_point_list_free(cstar_debug_point_list_t *list)
{
    if (list == NULL)
    {
        return;
    }

    free(list->items);
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

static void cstar_debug_segment_list_free(cstar_debug_segment_list_t *list)
{
    if (list == NULL)
    {
        return;
    }

    for (int i = 0; i < list->count; ++i)
    {
        free(list->items[i].path);
        list->items[i].path = NULL;
        list->items[i].path_count = 0;
    }

    free(list->items);
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

static void cstar_debug_polygon_list_free(cstar_debug_polygon_list_t *list)
{
    if (list == NULL)
    {
        return;
    }

    for (int i = 0; i < list->count; ++i)
    {
        free(list->items[i].vertices);
        list->items[i].vertices = NULL;
        list->items[i].vertex_count = 0;
    }

    free(list->items);
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

static void cstar_debug_move_point_list(cstar_debug_point_list_t *dst,
                                        cstar_debug_point_list_t *src)
{
    *dst = *src;
    src->items = NULL;
    src->count = 0;
    src->capacity = 0;
}

static void cstar_debug_move_segment_list(cstar_debug_segment_list_t *dst,
                                          cstar_debug_segment_list_t *src)
{
    *dst = *src;
    src->items = NULL;
    src->count = 0;
    src->capacity = 0;
}

static void cstar_debug_move_polygon_list(cstar_debug_polygon_list_t *dst,
                                          cstar_debug_polygon_list_t *src)
{
    *dst = *src;
    src->items = NULL;
    src->count = 0;
    src->capacity = 0;
}

bool cstar_debug_init(cstar_debug_t *debug_state)
{
    if (debug_state == NULL)
    {
        return false;
    }

    memset(debug_state, 0, sizeof(*debug_state));
    return true;
}

void cstar_debug_dispose(cstar_debug_t *debug_state)
{
    if (debug_state == NULL)
    {
        return;
    }

    cstar_debug_point_list_free(&debug_state->rcg_link_nodes);
    cstar_debug_point_list_free(&debug_state->rcg_end_nodes);
    cstar_debug_segment_list_free(&debug_state->rcg_edges);
    cstar_debug_segment_list_free(&debug_state->lap_list);
    cstar_debug_polygon_list_free(&debug_state->sampling_front_list);
    cstar_debug_point_list_free(&debug_state->frontier_sample_list);
    cstar_debug_point_list_free(&debug_state->retreat_node_list);
    cstar_debug_polygon_list_free(&debug_state->coverage_hole_list);

    memset(debug_state, 0, sizeof(*debug_state));
}

bool cstar_debug_finalize_layers(cstar_debug_t *debug_state,
                                 cstar_debug_layers_t *out_layers)
{
    if (debug_state == NULL || out_layers == NULL)
    {
        return false;
    }

    cstar_debug_layer_t *layers =
        (cstar_debug_layer_t *)calloc((size_t)CSTAR_DEBUG_LAYER_COUNT, sizeof(cstar_debug_layer_t));
    if (layers == NULL)
    {
        return false;
    }

    layers[0].id = 10;
    layers[0].source = "rcgLinkNodeList";
    layers[0].list_type = CSTAR_DEBUG_LIST_POINTS;
    cstar_debug_move_point_list(&layers[0].list.points, &debug_state->rcg_link_nodes);

    layers[1].id = 11;
    layers[1].source = "rcgEndNodeList";
    layers[1].list_type = CSTAR_DEBUG_LIST_POINTS;
    cstar_debug_move_point_list(&layers[1].list.points, &debug_state->rcg_end_nodes);

    layers[2].id = 12;
    layers[2].source = "rcgEdgeList";
    layers[2].list_type = CSTAR_DEBUG_LIST_SEGMENTS;
    cstar_debug_move_segment_list(&layers[2].list.segments, &debug_state->rcg_edges);

    layers[3].id = 13;
    layers[3].source = "lapList";
    layers[3].list_type = CSTAR_DEBUG_LIST_SEGMENTS;
    cstar_debug_move_segment_list(&layers[3].list.segments, &debug_state->lap_list);

    layers[4].id = 14;
    layers[4].source = "samplingFrontList";
    layers[4].list_type = CSTAR_DEBUG_LIST_POLYGONS;
    cstar_debug_move_polygon_list(&layers[4].list.polygons, &debug_state->sampling_front_list);

    layers[5].id = 15;
    layers[5].source = "frontierSampleList";
    layers[5].list_type = CSTAR_DEBUG_LIST_POINTS;
    cstar_debug_move_point_list(&layers[5].list.points, &debug_state->frontier_sample_list);

    layers[6].id = 16;
    layers[6].source = "retreatNodeList";
    layers[6].list_type = CSTAR_DEBUG_LIST_POINTS;
    cstar_debug_move_point_list(&layers[6].list.points, &debug_state->retreat_node_list);

    layers[7].id = 17;
    layers[7].source = "coverageHoleList";
    layers[7].list_type = CSTAR_DEBUG_LIST_POLYGONS;
    cstar_debug_move_polygon_list(&layers[7].list.polygons, &debug_state->coverage_hole_list);

    out_layers->layers = layers;
    out_layers->layer_count = CSTAR_DEBUG_LAYER_COUNT;
    out_layers->layer_capacity = CSTAR_DEBUG_LAYER_COUNT;
    return true;
}

bool cstar_debug_export_rcg_nodes(cstar_debug_t *debug_state,
                                  const cstar_rcg_t *rcg)
{
    if (debug_state == NULL || rcg == NULL)
    {
        return false;
    }

    for (int i = 0; i < rcg->node_count; ++i)
    {
        if (!cstar_debug_append_point(&debug_state->frontier_sample_list, i + 1, rcg->nodes[i].pos))
        {
            return false;
        }

        if (rcg->nodes[i].is_end_node)
        {
            if (!cstar_debug_append_point(&debug_state->rcg_end_nodes, i + 1, rcg->nodes[i].pos))
            {
                return false;
            }
        }

        if (rcg->nodes[i].is_link_node)
        {
            if (!cstar_debug_append_point(&debug_state->rcg_link_nodes, i + 1, rcg->nodes[i].pos))
            {
                return false;
            }
        }
    }

    return true;
}

bool cstar_debug_export_rcg_edges(cstar_debug_t *debug_state,
                                  const cstar_rcg_t *rcg)
{
    if (debug_state == NULL || rcg == NULL)
    {
        return false;
    }

    for (int i = 0; i < rcg->edge_count; ++i)
    {
        const cstar_edge_t *edge = &rcg->edges[i];
        point_t path[2] = {
            rcg->nodes[edge->node_a].pos,
            rcg->nodes[edge->node_b].pos};

        if (!cstar_debug_append_segment(&debug_state->rcg_edges, i + 1, "rcgEdge", path, 2))
        {
            return false;
        }
    }

    return true;
}

bool cstar_debug_export_laps(cstar_debug_t *debug_state,
                             const cstar_sampling_front_t *front,
                             const cstar_environment_t *env)
{
    if (debug_state == NULL || front == NULL || env == NULL)
    {
        return false;
    }

    float min_x = 0.0f, max_x = 0.0f, min_y = 0.0f, max_y = 0.0f;
    cstar_lap_boundary_bbox(env, &min_x, &max_x, &min_y, &max_y);

    if (front->laps == NULL)
    {
        return true;
    }

    int lap_count = (int)cvector_size(front->laps);
    for (int i = 0; i < lap_count; ++i)
    {
        float x = front->laps[i].x;
        point_t path[2] = {
            {x, min_y},
            {x, max_y}};

        if (!cstar_debug_append_segment(&debug_state->lap_list, i + 1, "lap", path, 2))
        {
            return false;
        }
    }

    return true;
}

bool cstar_debug_export_sampling_front_polygon(cstar_debug_t *debug_state,
                                               const cstar_environment_t *env)
{
    if (debug_state == NULL || env == NULL)
    {
        return false;
    }

    if (env->operationalBoundary.vertices == NULL || env->operationalBoundary.vertex_count == 0)
    {
        return true;
    }

    return cstar_debug_append_polygon(&debug_state->sampling_front_list,
                                      1,
                                      env->operationalBoundary.vertices,
                                      (int)env->operationalBoundary.vertex_count);
}
