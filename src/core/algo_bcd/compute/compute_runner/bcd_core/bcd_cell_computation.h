#ifndef BCD_CELL_COMPUTATION_H
#define BCD_CELL_COMPUTATION_H

#include "../../../../../../dependencies/cvector/cvector.h"
#include "bcd_event_list_building.h"

typedef struct bcd_cell_t bcd_cell_t;
typedef struct bcd_neighbor_node_t bcd_neighbor_node_t;

typedef struct bcd_neighbor_node_t
{
    int cell_index;
    bcd_neighbor_node_t *prev;
    bcd_neighbor_node_t *next;
} bcd_neighbor_node_t;

typedef struct
{
    bcd_neighbor_node_t *head;
    bcd_neighbor_node_t *tail;
    int count;
} bcd_neighbor_list_t;

struct bcd_cell_t
{
    point_t c_begin;
    cvector_vector_type(polygon_edge_t) ceiling_edge_list;
    point_t c_end;
    point_t f_begin;
    cvector_vector_type(polygon_edge_t) floor_edge_list;
    point_t f_end;
    bcd_neighbor_list_t neighbor_list;
    bool open;
    bool visited;
    bool cleaned;
};

int compute_bcd_cells(const bcd_event_list_t *event_list,
                      cvector_vector_type(bcd_cell_t) * cell_list);

void free_bcd_cell_list(cvector_vector_type(bcd_cell_t) * cell_list);

#endif // BCD_CELL_COMPUTATION_H
