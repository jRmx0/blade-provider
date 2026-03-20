// Generates events list, cells and adjacency graph using Boustrophedon Cellular Decomposition (BCD)

#ifndef BOUSTROPHEDON_CELLULAR_DECOMPOSITION_H
#define BOUSTROPHEDON_CELLULAR_DECOMPOSITION_H

#include <stdbool.h>
#include "../bcd_runner.h"

typedef enum {
    B_IN,
    B_SIDE_IN,
    B_INIT,
    
    B_OUT,
    B_SIDE_OUT,
    B_DEINIT,

    IN,                 
    SIDE_IN,
    
    OUT,
    SIDE_OUT,
    
    FLOOR,
    CEILING,

    NONE
} bcd_event_type_t;

typedef struct {
    polygon_type_t polygon_type;
    point_t polygon_vertex;
    bcd_event_type_t bcd_event_type;
    polygon_edge_t floor_edge;          // If IN: edge terminating at the event
                                        // IF OUT: edge emanating from the event
                                        // IF FLOOR: edge terminating at the event
    
    polygon_edge_t ceiling_edge;        // IF IN: edge emanating from the event 
                                        // IF OUT: edge terminating at the event
                                        // IF CEILING: edge emanating from the event
} bcd_event_t;

typedef struct {
    bcd_event_t *bcd_events;
    int length;
    int capacity;
} bcd_event_list_t;

int build_bcd_event_list(const input_environment_t *env, 
                         bcd_event_list_t *event_list);

void free_bcd_event_list(bcd_event_list_t *event_list);

#endif // BOUSTROPHEDON_CELLULAR_DECOMPOSITION_H
