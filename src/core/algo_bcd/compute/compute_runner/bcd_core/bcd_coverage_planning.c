#include <stdio.h>
#include <stdbool.h>
#include "../../../../../../dependencies/cvector/cvector.h"
#include "bcd_coverage_planning.h"
#include "bcd_pathfinding.h"

// COMPUTE_BCD_PATH_LIST

static void add_cell_to_path(cvector_vector_type(int) * path_list,
                             cvector_vector_type(bcd_cell_t) * cell_list,
                             int cell_index,
                             int *visited_count);

static bool all_cells_visited(int visited_count, int total_cells);

int find_unvisited_neighbor(int curr_cell_index,
                            cvector_vector_type(bcd_cell_t) * cell_list);

static void add_shortest_path_to_list(cvector_vector_type(int) * path_list,
                                      cvector_vector_type(bcd_cell_t) * cell_list,
                                      int target_cell_index,
                                      int *visited_count);

// ---

static bool should_backtrack(int *curr_path_index);

// IMPLEMENTATION --- compute_bcd_path_list -------------------------

int compute_bcd_path_list(cvector_vector_type(bcd_cell_t) * cell_list,
                          int starting_cell_index,
                          cvector_vector_type(int) * path_list)
{
    if (cell_list == NULL || path_list == NULL)
    {
        printf("compute_bcd_path_list: Invalid input parameters\n");
        return -1;
    }

    int cell_count = cvector_size(*cell_list);
    if (cell_count == 0)
    {
        printf("compute_bcd_path_list: No cells to process\n");
        return 0;
    }

    if (starting_cell_index == -1)
        starting_cell_index = 0;
    if (starting_cell_index < 0 || starting_cell_index >= cell_count)
    {
        printf("compute_bcd_path_list: Invalid starting_cell_index=%d (cell_count=%d)\n",
               starting_cell_index, cell_count);
        return -3;
    }

    int visited_count = 0;
    bool search_shortest_path = false;
    int curr_path_index = 0;

    // Initialize with starting cell
    add_cell_to_path(path_list,
                     cell_list,
                     starting_cell_index,
                     &visited_count);

    while (!all_cells_visited(visited_count, cell_count))
    {
        if (curr_path_index < 0 || curr_path_index >= (int)cvector_size(*path_list))
        {
            printf("compute_bcd_path_list: Invalid curr_path_index=%d (path_size=%zu)\n",
                   curr_path_index, cvector_size(*path_list));
            return -4;
        }

        int current_cell = (*path_list)[curr_path_index];
        if (current_cell < 0 || current_cell >= cell_count)
        {
            printf("compute_bcd_path_list: Invalid current_cell=%d (cell_count=%d)\n",
                   current_cell, cell_count);
            return -5;
        }
        int next_cell = find_unvisited_neighbor(current_cell, cell_list);

        if (next_cell != -1)
        {
            if (search_shortest_path)
            {
                add_shortest_path_to_list(path_list,
                                          cell_list,
                                          next_cell,
                                          &visited_count);
                search_shortest_path = false;
            }
            else
            {
                add_cell_to_path(path_list,
                                 cell_list,
                                 next_cell,
                                 &visited_count);
            }

            curr_path_index = cvector_size(*path_list) - 1;
        }
        else
        {
            search_shortest_path = true;

            if (should_backtrack(&curr_path_index))
            {
                printf("compute_bcd_path_list: backtracking exhausted, cell graph may be disconnected\n");
                return -2;
            }
        }
    }

    return 0;
}

// --- COMPUTE_BCD_PATH_LIST

static void add_cell_to_path(cvector_vector_type(int) * path_list,
                             cvector_vector_type(bcd_cell_t) * cell_list,
                             int cell_index,
                             int *visited_count)
{
    if (path_list == NULL || cell_list == NULL || *cell_list == NULL || visited_count == NULL)
    {
        return;
    }

    int cell_count = cvector_size(*cell_list);
    if (cell_index < 0 || cell_index >= cell_count)
    {
        return;
    }

    cvector_push_back(*path_list, cell_index);
    if ((*cell_list)[cell_index].visited == false)
    {
        (*cell_list)[cell_index].visited = true;
        (*visited_count)++;
    }
}

static bool all_cells_visited(int visited_count, int total_cells)
{
    return visited_count == total_cells;
}

int find_unvisited_neighbor(int curr_cell_index,
                            cvector_vector_type(bcd_cell_t) * cell_list)
{
    if (cell_list == NULL || *cell_list == NULL)
    {
        return -1;
    }

    int cell_count = cvector_size(*cell_list);
    if (curr_cell_index < 0 || curr_cell_index >= cell_count)
    {
        return -1;
    }

    bcd_neighbor_node_t *node = (*cell_list)[curr_cell_index].neighbor_list.head;
    while (node != NULL)
    {
        int neighbor_index = node->cell_index;
        if (neighbor_index >= 0 && neighbor_index < cell_count)
        {
            if ((*cell_list)[neighbor_index].visited == false)
            {
                return neighbor_index;
            }
        }

        node = node->next;
    }

    return -1;
}

static void add_shortest_path_to_list(cvector_vector_type(int) * path_list,
                                      cvector_vector_type(bcd_cell_t) * cell_list,
                                      int target_cell_index,
                                      int *visited_count)
{
    if (path_list == NULL || *path_list == NULL || cvector_size(*path_list) == 0)
        return;

    int last_cell_index = (*path_list)[cvector_size(*path_list) - 1];
    cvector_vector_type(int) corridor = bcd_astar(last_cell_index, target_cell_index, cell_list);

    if (corridor == NULL || cvector_size(corridor) < 2)
    {
        cvector_free(corridor);
        add_cell_to_path(path_list, cell_list, target_cell_index, visited_count);
        return;
    }

    // Add intermediate cells from A* corridor (skip first — already in path — and last)
    for (size_t i = 1; i < cvector_size(corridor) - 1; ++i)
        cvector_push_back(*path_list, corridor[i]);

    cvector_free(corridor);
    add_cell_to_path(path_list, cell_list, target_cell_index, visited_count);
}

// ---

static bool should_backtrack(int *curr_path_index)
{
    (*curr_path_index)--;
    return (*curr_path_index) < 0;
}
