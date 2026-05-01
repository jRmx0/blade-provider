#include <stdio.h>
#include <stdbool.h>
#include "../../../../../../dependencies/cvector/cvector.h"
#include "bcd_coverage_planning.h"

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

// --- ADD_SHORTEST_PATH_TO_LIST

cvector_vector_type(int) find_shortest_path(int cell_index_from,
                                            int cell_index_to,
                                            cvector_vector_type(bcd_cell_t) * cell_list);

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
        int current_cell = (*path_list)[curr_path_index];
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
                return -2;
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
    cvector_push_back(*path_list, cell_index);
    (*cell_list)[cell_index].visited = true;
    (*visited_count)++;
}

static bool all_cells_visited(int visited_count, int total_cells)
{
    return visited_count == total_cells;
}

int find_unvisited_neighbor(int curr_cell_index,
                            cvector_vector_type(bcd_cell_t) * cell_list)
{
    bcd_neighbor_node_t curr_neighbor_node;
    int curr_neighbor_index = -1;

    if ((*cell_list)[curr_cell_index].neighbor_list.count == 0)
    {
        return -1;
    }

    curr_neighbor_node = *(*cell_list)[curr_cell_index].neighbor_list.head;
    curr_neighbor_index = curr_neighbor_node.cell_index;

    if ((*cell_list)[curr_neighbor_index].visited == false)
    {
        return curr_neighbor_index;
    }

    for (int i = 1; i < (*cell_list)[curr_cell_index].neighbor_list.count; i++)
    {
        curr_neighbor_node = *curr_neighbor_node.next;
        curr_neighbor_index = curr_neighbor_node.cell_index;

        if ((*cell_list)[curr_neighbor_index].visited == false)
        {
            return curr_neighbor_index;
        }
    }

    return -1;
}

static void add_shortest_path_to_list(cvector_vector_type(int) * path_list,
                                      cvector_vector_type(bcd_cell_t) * cell_list,
                                      int target_cell_index,
                                      int *visited_count)
{
    int last_cell_index = (*path_list)[cvector_size(*path_list) - 1];
    cvector_vector_type(int) shortest_path = find_shortest_path(last_cell_index, target_cell_index, cell_list);

    // Add intermediate cells from shortest path (skip first and last)
    for (size_t i = 1; i < cvector_size(shortest_path) - 1; ++i)
    {
        cvector_push_back(*path_list, shortest_path[i]);
    }

    cvector_free(shortest_path);
    add_cell_to_path(path_list, cell_list, target_cell_index, visited_count);
}

// --- --- ADD_SHORTEST_PATH_TO_LIST

cvector_vector_type(int) find_shortest_path(int cell_index_from,
                                            int cell_index_to,
                                            cvector_vector_type(bcd_cell_t) * cell_list)
{
    cvector_vector_type(int) path = NULL;

    if (cell_list == NULL || cell_index_from < 0 || cell_index_to < 0)
    {
        return path;
    }

    int cell_count = cvector_size(*cell_list);
    if (cell_index_from >= cell_count || cell_index_to >= cell_count)
    {
        return path;
    }

    // If source and destination are the same
    if (cell_index_from == cell_index_to)
    {
        cvector_push_back(path, cell_index_from);
        return path;
    }

    // BFS data structures
    cvector_vector_type(int) queue = NULL;
    cvector_vector_type(int) parent = NULL;
    cvector_vector_type(bool) visited = NULL;

    // Initialize arrays
    for (int i = 0; i < cell_count; i++)
    {
        cvector_push_back(parent, -1);
        cvector_push_back(visited, false);
    }

    // Start BFS
    cvector_push_back(queue, cell_index_from);
    visited[cell_index_from] = true;

    bool found = false;

    while (cvector_size(queue) > 0 && !found)
    {
        int current_cell = queue[0];

        // Remove first element from queue
        for (int i = 0; i < cvector_size(queue) - 1; i++)
        {
            queue[i] = queue[i + 1];
        }
        cvector_pop_back(queue);

        // Check all neighbors
        bcd_neighbor_node_t *neighbor_node = (*cell_list)[current_cell].neighbor_list.head;

        while (neighbor_node != NULL)
        {
            int neighbor_index = neighbor_node->cell_index;

            if (!visited[neighbor_index])
            {
                visited[neighbor_index] = true;
                parent[neighbor_index] = current_cell;
                cvector_push_back(queue, neighbor_index);

                if (neighbor_index == cell_index_to)
                {
                    found = true;
                    break;
                }
            }

            neighbor_node = neighbor_node->next;
        }
    }

    // Reconstruct path if found
    if (found)
    {
        cvector_vector_type(int) temp_path = NULL;
        int current = cell_index_to;

        while (current != -1)
        {
            cvector_push_back(temp_path, current);
            current = parent[current];
        }

        // Reverse the path
        int path_length = cvector_size(temp_path);
        for (int i = path_length - 1; i >= 0; i--)
        {
            cvector_push_back(path, temp_path[i]);
        }

        cvector_free(temp_path);
    }

    // Cleanup
    cvector_free(queue);
    cvector_free(parent);
    cvector_free(visited);

    return path;
}

// ---

static bool should_backtrack(int *curr_path_index)
{
    (*curr_path_index)--;
    return (*curr_path_index) < 0;
}

// PATH_LIST HELPERS

void log_bcd_path_list(const cvector_vector_type(int) * path_list)
{
    if (path_list == NULL)
    {
        printf("BCD Path List: NULL\n");
        return;
    }

    int path_count = cvector_size(*path_list);
    printf("BCD Path List (%d cells):\n", path_count);

    if (path_count == 0)
    {
        printf("  (empty)\n");
        return;
    }

    for (int i = 0; i < path_count; i++)
    {
        printf("  [%d]: Cell %d\n", i, (*path_list)[i]);
    }
}
