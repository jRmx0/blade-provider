#include <string.h>
#include "cstar_debug.h"
#include "../../../../../common/debug_serialize.h"
#include "../../core/preprocess/cstar_lap.h"

static void cstar_debug_add_point_entry(cJSON *arr, int id, point_t p, const char *label)
{
    cJSON *entry = cJSON_CreateObject();
    cJSON *point = cJSON_CreateObject();
    if (entry == NULL || point == NULL)
    {
        cJSON_Delete(entry);
        cJSON_Delete(point);
        return;
    }

    cJSON_AddNumberToObject(entry, "id", id);
    cJSON_AddNumberToObject(point, "x", p.x);
    cJSON_AddNumberToObject(point, "y", p.y);
    cJSON_AddItemToObject(entry, "point", point);
    if (label != NULL)
    {
        cJSON_AddStringToObject(entry, "pointLabel", label);
    }
    cJSON_AddItemToArray(arr, entry);
}

static bool cstar_debug_add_layer(cJSON *layers_arr, int id, const char *source, const cJSON *list)
{
    if (layers_arr == NULL || source == NULL || list == NULL)
    {
        return false;
    }

    cJSON *layer = cJSON_CreateObject();
    cJSON *list_copy = cJSON_Duplicate((cJSON *)list, 1);
    if (layer == NULL || list_copy == NULL)
    {
        cJSON_Delete(layer);
        cJSON_Delete(list_copy);
        return false;
    }

    cJSON_AddNumberToObject(layer, "id", id);
    cJSON_AddStringToObject(layer, "source", source);
    cJSON_AddItemToObject(layer, "list", list_copy);
    cJSON_AddItemToArray(layers_arr, layer);
    return true;
}

bool cstar_debug_init(cstar_debug_t *debug_state, cJSON *root)
{
    if (debug_state == NULL || root == NULL)
    {
        return false;
    }

    memset(debug_state, 0, sizeof(*debug_state));

    cJSON *rcg_link_nodes = cJSON_CreateArray();
    cJSON *rcg_end_nodes = cJSON_CreateArray();
    cJSON *rcg_edges = cJSON_CreateArray();
    cJSON *lap_list = cJSON_CreateArray();
    cJSON *sampling_front_list = cJSON_CreateArray();
    cJSON *frontier_sample_list = cJSON_CreateArray();
    cJSON *retreat_node_list = cJSON_CreateArray();
    cJSON *coverage_hole_list = cJSON_CreateArray();
    cJSON *debug = cJSON_CreateObject();
    cJSON *layers = cJSON_CreateArray();

    if (rcg_link_nodes == NULL || rcg_end_nodes == NULL || rcg_edges == NULL ||
        lap_list == NULL || sampling_front_list == NULL || frontier_sample_list == NULL ||
        retreat_node_list == NULL || coverage_hole_list == NULL || debug == NULL || layers == NULL)
    {
        cJSON_Delete(rcg_link_nodes);
        cJSON_Delete(rcg_end_nodes);
        cJSON_Delete(rcg_edges);
        cJSON_Delete(lap_list);
        cJSON_Delete(sampling_front_list);
        cJSON_Delete(frontier_sample_list);
        cJSON_Delete(retreat_node_list);
        cJSON_Delete(coverage_hole_list);
        cJSON_Delete(debug);
        cJSON_Delete(layers);
        return false;
    }

    cJSON_AddItemToObject(root, "rcgLinkNodeList", rcg_link_nodes);
    cJSON_AddItemToObject(root, "rcgEndNodeList", rcg_end_nodes);
    cJSON_AddItemToObject(root, "rcgEdgeList", rcg_edges);
    cJSON_AddItemToObject(root, "lapList", lap_list);
    cJSON_AddItemToObject(root, "samplingFrontList", sampling_front_list);
    cJSON_AddItemToObject(root, "frontierSampleList", frontier_sample_list);
    cJSON_AddItemToObject(root, "retreatNodeList", retreat_node_list);
    cJSON_AddItemToObject(root, "coverageHoleList", coverage_hole_list);

    debug_state->rcg_link_nodes = rcg_link_nodes;
    debug_state->rcg_end_nodes = rcg_end_nodes;
    debug_state->rcg_edges = rcg_edges;
    debug_state->lap_list = lap_list;
    debug_state->sampling_front_list = sampling_front_list;
    debug_state->frontier_sample_list = frontier_sample_list;
    debug_state->retreat_node_list = retreat_node_list;
    debug_state->coverage_hole_list = coverage_hole_list;
    debug_state->debug = debug;
    debug_state->layers = layers;
    return true;
}

void cstar_debug_dispose(cstar_debug_t *debug_state)
{
    if (debug_state == NULL)
    {
        return;
    }

    cJSON_Delete(debug_state->debug);
    cJSON_Delete(debug_state->layers);
    memset(debug_state, 0, sizeof(*debug_state));
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
        cstar_debug_add_point_entry(debug_state->frontier_sample_list, i + 1, rcg->nodes[i].pos, NULL);
        if (rcg->nodes[i].is_end_node)
        {
            cstar_debug_add_point_entry(debug_state->rcg_end_nodes, i + 1, rcg->nodes[i].pos, NULL);
        }
        if (rcg->nodes[i].is_link_node)
        {
            cstar_debug_add_point_entry(debug_state->rcg_link_nodes, i + 1, rcg->nodes[i].pos, NULL);
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
        point_t a = rcg->nodes[edge->node_a].pos;
        point_t b = rcg->nodes[edge->node_b].pos;
        float xs[2] = {a.x, b.x};
        float ys[2] = {a.y, b.y};
        cJSON *segment = debug_build_segment(i + 1, "rcgEdge", xs, ys, 2);
        if (segment == NULL)
        {
            return false;
        }
        cJSON_AddItemToArray(debug_state->rcg_edges, segment);
    }

    return true;
}

bool cstar_debug_export_laps(cstar_debug_t *debug_state,
                             const cstar_sampling_front_t *front,
                             const input_environment_t *env)
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
        float xs[2] = {x, x};
        float ys[2] = {min_y, max_y};
        cJSON *segment = debug_build_segment(i + 1, "lap", xs, ys, 2);
        if (segment == NULL)
        {
            return false;
        }
        cJSON_AddItemToArray(debug_state->lap_list, segment);
    }

    return true;
}

bool cstar_debug_export_sampling_front_polygon(cstar_debug_t *debug_state,
                                               const input_environment_t *env)
{
    if (debug_state == NULL || env == NULL)
    {
        return false;
    }

    cJSON *front_polygon = cJSON_CreateObject();
    cJSON *front_vertices = cJSON_CreateArray();
    if (front_polygon == NULL || front_vertices == NULL)
    {
        cJSON_Delete(front_polygon);
        cJSON_Delete(front_vertices);
        return false;
    }

    cJSON_AddNumberToObject(front_polygon, "id", 1);
    cJSON_AddItemToObject(front_polygon, "vertices", front_vertices);
    for (uint32_t i = 0; i < env->boundary.vertex_count; ++i)
    {
        cJSON *jv = cJSON_CreateObject();
        if (jv == NULL)
        {
            cJSON_Delete(front_polygon);
            return false;
        }
        cJSON_AddNumberToObject(jv, "x", env->boundary.vertices[i].x);
        cJSON_AddNumberToObject(jv, "y", env->boundary.vertices[i].y);
        cJSON_AddItemToArray(front_vertices, jv);
    }

    cJSON_AddItemToArray(debug_state->sampling_front_list, front_polygon);
    return true;
}

bool cstar_debug_attach_layers(cstar_debug_t *debug_state, cJSON *root)
{
    if (debug_state == NULL || root == NULL || debug_state->debug == NULL || debug_state->layers == NULL)
    {
        return false;
    }

    bool debug_ok = true;
    debug_ok = debug_ok && cstar_debug_add_layer(debug_state->layers, 10, "rcgLinkNodeList", debug_state->rcg_link_nodes);
    debug_ok = debug_ok && cstar_debug_add_layer(debug_state->layers, 11, "rcgEndNodeList", debug_state->rcg_end_nodes);
    debug_ok = debug_ok && cstar_debug_add_layer(debug_state->layers, 12, "rcgEdgeList", debug_state->rcg_edges);
    debug_ok = debug_ok && cstar_debug_add_layer(debug_state->layers, 13, "lapList", debug_state->lap_list);
    debug_ok = debug_ok && cstar_debug_add_layer(debug_state->layers, 14, "samplingFrontList", debug_state->sampling_front_list);
    debug_ok = debug_ok && cstar_debug_add_layer(debug_state->layers, 15, "frontierSampleList", debug_state->frontier_sample_list);
    debug_ok = debug_ok && cstar_debug_add_layer(debug_state->layers, 16, "retreatNodeList", debug_state->retreat_node_list);
    debug_ok = debug_ok && cstar_debug_add_layer(debug_state->layers, 17, "coverageHoleList", debug_state->coverage_hole_list);

    if (!debug_ok)
    {
        return false;
    }

    cJSON_AddItemToObject(debug_state->debug, "layers", debug_state->layers);
    cJSON_AddItemToObject(root, "debug", debug_state->debug);
    debug_state->layers = NULL;
    debug_state->debug = NULL;
    return true;
}
