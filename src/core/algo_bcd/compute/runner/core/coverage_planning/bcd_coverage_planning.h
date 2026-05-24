#ifndef BCD_COVERAGE_H
#define BCD_COVERAGE_H

#include "../../../../../../../dependencies/cvector/cvector.h"
#include "../cells/bcd_cell_computation.h"

int compute_bcd_path_list(cvector_vector_type(bcd_cell_t) * cell_list,
                          int starting_cell_index,
                          cvector_vector_type(int) * path_list);

#endif // BCD_COVERAGE_H
