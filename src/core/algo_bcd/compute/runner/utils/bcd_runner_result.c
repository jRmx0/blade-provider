#include <stdlib.h>
#include <string.h>
#include "bcd_runner_result.h"

/* Redirect cvector's allocator hooks to the plain CRT heap for this
 * compilation unit so that result storage is invisible to the memory
 * usage tracker.  The va_* hooks active in the surrounding amalgamation
 * unit are saved and restored around this file. */
#pragma push_macro("cvector_clib_malloc")
#pragma push_macro("cvector_clib_free")
#pragma push_macro("cvector_clib_realloc")
#pragma push_macro("cvector_clib_calloc")
#undef cvector_clib_malloc
#undef cvector_clib_free
#undef cvector_clib_realloc
#undef cvector_clib_calloc
#define cvector_clib_malloc malloc
#define cvector_clib_free free
#define cvector_clib_realloc realloc
#define cvector_clib_calloc calloc

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
    /* --- event_list: deep copy flat array --- */
    if (event_list.bcd_events != NULL && event_list.length > 0)
    {
        result->event_list.bcd_events =
            (bcd_event_t *)malloc((size_t)event_list.length * sizeof(bcd_event_t));
        memcpy(result->event_list.bcd_events, event_list.bcd_events,
               (size_t)event_list.length * sizeof(bcd_event_t));
        result->event_list.length = event_list.length;
        result->event_list.capacity = event_list.length;
    }

    /* --- cell_list: deep copy cvector + inner cvectors + neighbor linked lists --- */
    result->cell_list = NULL;
    if (cell_list != NULL)
    {
        size_t n = cvector_size(cell_list);
        for (size_t i = 0; i < n; i++)
        {
            bcd_cell_t *src = &cell_list[i];
            bcd_cell_t dst;
            dst.c_begin = src->c_begin;
            dst.c_end = src->c_end;
            dst.f_begin = src->f_begin;
            dst.f_end = src->f_end;
            dst.open = src->open;
            dst.visited = src->visited;
            dst.cleaned = src->cleaned;

            dst.ceiling_edge_list = NULL;
            if (src->ceiling_edge_list != NULL)
            {
                size_t ne = cvector_size(src->ceiling_edge_list);
                for (size_t j = 0; j < ne; j++)
                    cvector_push_back(dst.ceiling_edge_list, src->ceiling_edge_list[j]);
            }

            dst.floor_edge_list = NULL;
            if (src->floor_edge_list != NULL)
            {
                size_t ne = cvector_size(src->floor_edge_list);
                for (size_t j = 0; j < ne; j++)
                    cvector_push_back(dst.floor_edge_list, src->floor_edge_list[j]);
            }

            dst.neighbor_list.head = NULL;
            dst.neighbor_list.tail = NULL;
            dst.neighbor_list.count = 0;
            for (bcd_neighbor_node_t *node = src->neighbor_list.head; node != NULL; node = node->next)
            {
                bcd_neighbor_node_t *new_node =
                    (bcd_neighbor_node_t *)malloc(sizeof(bcd_neighbor_node_t));
                new_node->cell_index = node->cell_index;
                new_node->prev = dst.neighbor_list.tail;
                new_node->next = NULL;
                if (dst.neighbor_list.tail != NULL)
                    dst.neighbor_list.tail->next = new_node;
                else
                    dst.neighbor_list.head = new_node;
                dst.neighbor_list.tail = new_node;
                dst.neighbor_list.count++;
            }

            cvector_push_back(result->cell_list, dst);
        }
    }

    /* --- path_list: deep copy int cvector --- */
    result->path_list = NULL;
    if (path_list != NULL)
    {
        size_t n = cvector_size(path_list);
        for (size_t i = 0; i < n; i++)
            cvector_push_back(result->path_list, path_list[i]);
    }

    /* --- motion_plan: deep copy section cvector + per-section ox/nav cvectors --- */
    result->motion_plan.section = NULL;
    if (motion_plan.section != NULL)
    {
        size_t n = cvector_size(motion_plan.section);
        for (size_t i = 0; i < n; i++)
        {
            cell_motion_plan_t *src = &motion_plan.section[i];
            cell_motion_plan_t dst;
            dst.ox = NULL;
            dst.nav = NULL;
            if (src->ox != NULL)
            {
                size_t np = cvector_size(src->ox);
                for (size_t j = 0; j < np; j++)
                    cvector_push_back(dst.ox, src->ox[j]);
            }
            if (src->nav != NULL)
            {
                size_t np = cvector_size(src->nav);
                for (size_t j = 0; j < np; j++)
                    cvector_push_back(dst.nav, src->nav[j]);
            }
            cvector_push_back(result->motion_plan.section, dst);
        }
    }

    /* --- headland: deep copy sections cvector + per-section path/nav cvectors --- */
    result->has_headland = has_headland;
    memset(&result->headland, 0, sizeof(headland_t));
    if (has_headland && headland.sections != NULL)
    {
        size_t n = cvector_size(headland.sections);
        for (size_t i = 0; i < n; i++)
        {
            headland_section_t *src = &headland.sections[i];
            headland_section_t dst;
            dst.source_index = src->source_index;
            dst.path = NULL;
            dst.nav = NULL;
            if (src->path != NULL)
            {
                size_t np = cvector_size(src->path);
                for (size_t j = 0; j < np; j++)
                    cvector_push_back(dst.path, src->path[j]);
            }
            if (src->nav != NULL)
            {
                size_t np = cvector_size(src->nav);
                for (size_t j = 0; j < np; j++)
                    cvector_push_back(dst.nav, src->nav[j]);
            }
            cvector_push_back(result->headland.sections, dst);
        }
    }

    /* --- start_nav: deep copy point_t cvector --- */
    result->start_nav = NULL;
    if (start_nav != NULL)
    {
        size_t n = cvector_size(start_nav);
        for (size_t i = 0; i < n; i++)
            cvector_push_back(result->start_nav, start_nav[i]);
    }
}

static void bcd_result_free(bcd_result_t *result)
{
    if (result == NULL)
        return;

    /* event_list.bcd_events — plain malloc */
    free(result->event_list.bcd_events);
    result->event_list.bcd_events = NULL;

    /* cell_list — cvectors and neighbor nodes are plain malloc */
    if (result->cell_list != NULL)
    {
        size_t n = cvector_size(result->cell_list);
        for (size_t i = 0; i < n; i++)
        {
            cvector_free(result->cell_list[i].ceiling_edge_list);
            cvector_free(result->cell_list[i].floor_edge_list);
            bcd_neighbor_node_t *node = result->cell_list[i].neighbor_list.head;
            while (node != NULL)
            {
                bcd_neighbor_node_t *next = node->next;
                free(node);
                node = next;
            }
        }
        cvector_free(result->cell_list);
    }

    cvector_free(result->path_list);

    /* motion_plan — inlined free; free_bcd_motion() uses va_free and cannot
     * be called on data allocated with plain malloc */
    if (result->motion_plan.section != NULL)
    {
        size_t n = cvector_size(result->motion_plan.section);
        for (size_t i = 0; i < n; i++)
        {
            cvector_free(result->motion_plan.section[i].ox);
            cvector_free(result->motion_plan.section[i].nav);
        }
        cvector_free(result->motion_plan.section);
        result->motion_plan.section = NULL;
    }

    cvector_free(result->start_nav);

    /* headland — inlined free; free_headland() uses va_free */
    if (result->has_headland && result->headland.sections != NULL)
    {
        size_t n = cvector_size(result->headland.sections);
        for (size_t i = 0; i < n; i++)
        {
            cvector_free(result->headland.sections[i].path);
            cvector_free(result->headland.sections[i].nav);
        }
        cvector_free(result->headland.sections);
        result->headland.sections = NULL;
    }

    free(result);
}

#pragma pop_macro("cvector_clib_malloc")
#pragma pop_macro("cvector_clib_free")
#pragma pop_macro("cvector_clib_realloc")
#pragma pop_macro("cvector_clib_calloc")
