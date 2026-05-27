#ifndef CSTAR_SERIALIZE_H
#define CSTAR_SERIALIZE_H

#include "../check/cstar_check.h"
#include "../parser/cstar_parser.h"
#include "../cstar.h"

// Serializes a successful compute result to a cJSON tree (caller must cJSON_Delete it).
// Returns NULL on failure. Does not take ownership of result.
cJSON *cstar_build_result_json_tree(const cstar_coverage_path_result_t *result);

// Serializes a successful compute result to JSON. Returns a malloc'd string.
// Does not take ownership; caller retains responsibility for freeing the result.
char *cstar_serialize_result_json(const cstar_coverage_path_result_t *result);

// Serializes an error to JSON. Returns a malloc'd string.
char *cstar_serialize_error_json(const char *code, const char *message);

// Frees all resources in a coverage path result struct.
void cstar_result_free(cstar_coverage_path_result_t *result);

#endif // CSTAR_SERIALIZE_H
