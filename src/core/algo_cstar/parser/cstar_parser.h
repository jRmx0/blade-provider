// Parses validated C* compute request JSON payload into environment structures.

#ifndef CSTAR_PARSER_H
#define CSTAR_PARSER_H

#include "../cstar.h"
#include "../check/cstar_check.h"

bool cstar_parse_request_json(const char *request_json, cstar_environment_t *environment, cstar_check_result_t *result);
void cstar_parser_free_environment(cstar_environment_t *environment);

#endif // CSTAR_PARSER_H
