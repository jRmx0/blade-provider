#ifndef BCD_RUNNER_RESULT_H
#define BCD_RUNNER_RESULT_H

#include "bcd_runner.h"

/**
 * Create an empty result struct with zero-initialized fields.
 */
static bcd_result_t *bcd_result_create(void);

/**
 * Frees all resources owned by result, then frees the struct itself.
 */
static void bcd_result_free(bcd_result_t *result);

#endif // BCD_RUNNER_RESULT_H
