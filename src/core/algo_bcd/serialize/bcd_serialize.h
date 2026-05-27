/**
 * bcd_serialize.h
 *
 * Declares the BCD result serializer.
 * Converts a bcd_result_t into a cJSON tree ready for printing.
 *
 * Included by: compute/runner/bcd_runner.h (transitively by bcd.c)
 */

#ifndef BCD_SERIALIZE_H
#define BCD_SERIALIZE_H

#include "../bcd.h"
#include "../../../../dependencies/cJSON/cJSON.h"

/* Forward declaration — full struct definition is in compute/runner/utils/bcd_runner_result.c */
typedef struct bcd_result_t bcd_result_t;

/* Builds the cJSON result tree from a successful bcd_result_t.
 * Returns a cJSON object owned by the caller (must be cJSON_Delete'd). */
cJSON *bcd_build_result_json_tree(const bcd_result_t *result);

#endif // BCD_SERIALIZE_H
