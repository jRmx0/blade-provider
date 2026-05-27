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
#include "../../dependencies/cJSON/cJSON.c"
#include "../../dependencies/allocator/allocator.c"
#include "../../dependencies/timer/timer.c"
#include "common/debug_serialize.c"
#include "algo_bcd/bcd.c"
#include "algo_bounce/bounce.c"
#include "algo_cstar/cstar.c"

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

static int parse_requested_algorithm_id(const char *request_json, int *algorithm_id_out, char **error_json_out)
{
	if (request_json == NULL)
	{
		*error_json_out = create_dispatch_error_json("invalid_request", "Compute request JSON is required.");
		return 0;
	}

	cJSON *root = cJSON_Parse(request_json);
	if (!cJSON_IsObject(root))
	{
		cJSON_Delete(root);
		*error_json_out = create_dispatch_error_json("invalid_request", "Compute request body must be a JSON object.");
		return 0;
	}

	const cJSON *algorithm_id = cJSON_GetObjectItemCaseSensitive(root, "algorithmId");
	if (!cJSON_IsNumber(algorithm_id))
	{
		cJSON_Delete(root);
		*error_json_out = create_dispatch_error_json("missing_algorithm", "algorithmId is required.");
		return 0;
	}

	*algorithm_id_out = (int)algorithm_id->valuedouble;
	cJSON_Delete(root);
	return 1;
}

char *dispatch_metadata_json(void)
{
	cJSON *response = cJSON_CreateObject();
	cJSON *algorithms = cJSON_CreateArray();

	cJSON *bcd_algorithm = NULL;
	cJSON *bounce_algorithm = NULL;
	cJSON *cstar_algorithm = NULL;

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
	bounce_algorithm = parse_algorithm_metadata_json(bounce_get_metadata_json());
	if (bounce_algorithm == NULL)
	{
		cJSON_Delete(response);
		return NULL;
	}
	cstar_algorithm = parse_algorithm_metadata_json(cstar_get_metadata_json());
	if (cstar_algorithm == NULL)
	{
		cJSON_Delete(response);
		return NULL;
	}

	cJSON_AddItemToArray(algorithms, bcd_algorithm);
	cJSON_AddItemToArray(algorithms, bounce_algorithm);
	cJSON_AddItemToArray(algorithms, cstar_algorithm);

	char *json = cJSON_PrintUnformatted(response);
	cJSON_Delete(response);
	return json;
}

char *dispatch_compute_json(const char *request_json)
{
	char *error_json = NULL;
	int algorithm_id = 0;
	if (!parse_requested_algorithm_id(request_json, &algorithm_id, &error_json))
	{
		return error_json;
	}

	if (algorithm_id == 1)
	{
		return bcd_compute(request_json);
	}

	if (algorithm_id == 2)
	{
		return bounce_compute(request_json);
	}

	if (algorithm_id == 3)
	{
		return cstar_compute(request_json);
	}

	return create_dispatch_error_json("unknown_algorithm", "Unknown algorithm requested.");
}

void dispatch_string_free(char *value)
{
	free(value);
}