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
#include "../../dependencies/cJSON/cJSON.c"
#include "algo_bcd/bcd.c"
#include "algo_mock/mock_algo.c"

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
	if (response == NULL)
	{
		return NULL;
	}
	cJSON_AddStringToObject(response, "status", "error");
	cJSON_AddStringToObject(response, "code", code);
	cJSON_AddStringToObject(response, "message", message);

	char *json = cJSON_PrintUnformatted(response);
	cJSON_Delete(response);
	return json;
}

static const char *parse_requested_algorithm_name(const char *request_json, char **error_json_out)
{
	if (request_json == NULL)
	{
		*error_json_out = create_dispatch_error_json("invalid_request", "Compute request JSON is required.");
		return NULL;
	}

	cJSON *root = cJSON_Parse(request_json);
	if (!cJSON_IsObject(root))
	{
		cJSON_Delete(root);
		*error_json_out = create_dispatch_error_json("invalid_request", "Compute request body must be a JSON object.");
		return NULL;
	}

	const cJSON *algorithm_name = cJSON_GetObjectItemCaseSensitive(root, "algorithmName");
	if (!cJSON_IsString(algorithm_name) || algorithm_name->valuestring == NULL || algorithm_name->valuestring[0] == '\0')
	{
		cJSON_Delete(root);
		*error_json_out = create_dispatch_error_json("missing_algorithm", "algorithmName is required.");
		return NULL;
	}

	const char *result = algorithm_name->valuestring;
	cJSON_Delete(root);
	return result;
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

char *dispatch_compute_json(const char *request_json)
{
	char *error_json = NULL;
	const char *algorithm_name = parse_requested_algorithm_name(request_json, &error_json);
	if (algorithm_name == NULL)
	{
		return error_json;
	}

	if (strcmp(algorithm_name, "Boustrophedon Cellular Decomposition") == 0)
	{
		return bcd_compute(request_json);
	}

	if (strcmp(algorithm_name, "Mock Metadata Matrix") == 0)
	{
		return mock_algo_compute(request_json);
	}

	return create_dispatch_error_json("unknown_algorithm", "Unknown algorithm requested.");
}

void dispatch_string_free(char *value)
{
	free(value);
}