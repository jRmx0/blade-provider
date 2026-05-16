// Validates and parses C* compute request JSON payload.

#ifndef CSTAR_CHECK_H
#define CSTAR_CHECK_H

#include "../internal.h"

typedef struct
{
	bool ok;
	const char *code;
	const char *message;
} cstar_check_result_t;

bool cstar_check_request_json(const char *request_json, input_environment_t *environment, cstar_check_result_t *result);
void cstar_check_free_environment(input_environment_t *environment);

#endif // CSTAR_CHECK_H
