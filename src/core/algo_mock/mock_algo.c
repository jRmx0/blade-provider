/**
 * mock_algo.c
 *
 * Public entry points for the metadata-coverage mock algorithm.
 * Keeps the interface shape aligned with the BCD module while
	* delegating metadata and compute details to metadata/ and compute/
	* helper translation units.
 */

#include "check/mock_algo_check.c"
#include "metadata/mock_algo_metadata.c"
#include "compute/mock_algo_compute.c"

char *mock_algo_build_metadata_json(void);
char *mock_algo_run_compute(const char *input_environment_json);

char *mock_algo_get_metadata_json(void)
{
	return mock_algo_build_metadata_json();
}

char *mock_algo_compute(const char *input_environment_json)
{
	return mock_algo_run_compute(input_environment_json);
}
