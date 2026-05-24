/**
 * bcd_parser.h
 *
 * Declares the BCD request parser/validator.
 * Parses and validates a raw JSON string into an input_environment_t.
 *
 * Included by: bcd.c (via bcd_parser.c amalgamation)
 */

#ifndef BCD_PARSER_H
#define BCD_PARSER_H

#include "../bcd.h"
#include "../check/bcd_check.h"

/* Parses and validates the BCD compute request JSON.
 * On success returns true and populates *environment.
 * On failure returns false and populates *result with an error code/message.
 * Caller must call free_input_environment(environment) after use. */
bool bcd_parse_request_json(const char *request_json, input_environment_t *environment, bcd_check_result_t *result);

#endif // BCD_PARSER_H
