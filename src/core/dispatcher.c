/**
 * dispatcher.c
 *
 * Central routing layer between the API and core algorithms.
 * Receives incoming requests from api/, determines which algorithm
 * to invoke based on input type, and delegates to the corresponding
 * algorithm's metadata or compute function.
 *
 * Adding a new algorithm:
 *   1. Include its header
 *   2. Add a case to dispatch_metadata() and dispatch_compute()
 *
 * Dependencies: algorithm public headers (bcd.h, ...)
 * Exposed to: api/
 */

#include <stdlib.h>
#include <string.h>

#include "dispatcher.h"
#include "../../dependencies/cJSON/cjson_compat.h"
#include "boustrophedon_cellular_decomposition/bcd.h"
#include "mock_algo/mock_algo.h"

static cJSON *parse_algorithm_metadata_json(char *algorithm_json)
{
	if (algorithm_json == NULL)
	{
		return NULL;
	}

	cJSON *algorithm = cJSON_Parse(algorithm_json);
	free(algorithm_json);
	return algorithm;
}

static char *create_dispatch_error_json(const char *code, const char *message)
{
	cJSON *response = cJSON_CreateObject();
	cJSON_AddStringToObject(response, "status", "error");
	cJSON_AddStringToObject(response, "code", code);
	cJSON_AddStringToObject(response, "message", message);

	char *json = cJSON_PrintUnformatted(response);
	cJSON_Delete(response);
	return json;
}

char *dispatch_metadata_json(void)
{
	cJSON *response = cJSON_CreateObject();
	cJSON *algorithms = cJSON_CreateArray();
	cJSON *bcd_algorithm = NULL;
	cJSON *mock_algorithm = NULL;
	if (response == NULL || algorithms == NULL)
	{
		cJSON_Delete(response);
		cJSON_Delete(algorithms);
		return NULL;
	}

	cJSON_AddItemToObject(response, "algorithms", algorithms);

	bcd_algorithm = parse_algorithm_metadata_json(bcd_get_metadata_json());
	if (bcd_algorithm == NULL)
	{
		cJSON_Delete(response);
		return NULL;
	}

	cJSON_AddItemToArray(algorithms, bcd_algorithm);

	mock_algorithm = parse_algorithm_metadata_json(mock_algo_get_metadata_json());
	if (mock_algorithm == NULL)
	{
		cJSON_Delete(response);
		return NULL;
	}

	cJSON_AddItemToArray(algorithms, mock_algorithm);

	char *json = cJSON_PrintUnformatted(response);
	cJSON_Delete(response);
	return json;
}

char *dispatch_compute_json(const char *algorithm_name, const char *input_environment_json)
{
	if (algorithm_name == NULL)
	{
		return create_dispatch_error_json("unknown_algorithm", "Unknown algorithm requested.");
	}

	if (strcmp(algorithm_name, "mock_cpp_bcd") == 0 || strcmp(algorithm_name, "bcd") == 0)
	{
		return bcd_compute(input_environment_json);
	}

	if (strcmp(algorithm_name, "mock_metadata_matrix") == 0)
	{
		return mock_algo_compute(input_environment_json);
	}

	return create_dispatch_error_json("unknown_algorithm", "Unknown algorithm requested.");
}

void dispatch_string_free(char *value)
{
	free(value);
}