#ifndef BCD_CHECK_H
#define BCD_CHECK_H

#include "../bcd.h"

typedef struct
{
	bool ok;
	const char *code;
	const char *message;
} bcd_check_result_t;

bool bcd_check_request_json(const char *request_json, input_environment_t *environment, bcd_check_result_t *result);

#endif // BCD_CHECK_H
