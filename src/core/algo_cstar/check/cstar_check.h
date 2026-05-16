// Validates C* compute request JSON payload.

#ifndef CSTAR_CHECK_H
#define CSTAR_CHECK_H

#include "../internal.h"

typedef struct
{
	bool ok;
	const char *code;
	const char *message;
} cstar_check_result_t;

bool cstar_validate_request_json(const char *request_json, cstar_check_result_t *result);

#endif // CSTAR_CHECK_H
