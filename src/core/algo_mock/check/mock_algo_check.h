#ifndef MOCK_ALGO_CHECK_H
#define MOCK_ALGO_CHECK_H

#include <stdbool.h>

typedef struct
{
	bool ok;
	const char *code;
	const char *message;
} mock_algo_check_result_t;

bool mock_algo_check_request_json(const char *request_json, mock_algo_check_result_t *result);

#endif // MOCK_ALGO_CHECK_H