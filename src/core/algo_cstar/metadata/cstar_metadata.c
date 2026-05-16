#include "../../metadata/metadata_json.h"
#include "../../metadata/metadata_types.h"

char *cstar_build_metadata_json(void)
{
    cJSON *algorithm = cJSON_CreateObject();
    if (algorithm == NULL)
    {
        return NULL;
    }

    cJSON_AddNumberToObject(algorithm, "id", 3);
    cJSON_AddStringToObject(algorithm, "name", "C Star");

    cJSON *parameters = cJSON_CreateArray();
    if (parameters == NULL)
    {
        cJSON_Delete(algorithm);
        return NULL;
    }

    cJSON_AddItemToObject(algorithm, "parameters", parameters);

    metadata_add_parameter(parameters, 1, "Path Width", BCD_METADATA_PARAM_TYPE_DECIMAL, NULL, 0, "20", BCD_METADATA_PARAM_SECTION_COVERAGE_PATH, BCD_METADATA_APP_HANDLER_NONE, 1, 0.0, 0, 0.0, NULL);
    metadata_add_parameter(parameters, 2, "Sensor Range", BCD_METADATA_PARAM_TYPE_DECIMAL, NULL, 0, "300", BCD_METADATA_PARAM_SECTION_COVERAGE_PATH, BCD_METADATA_APP_HANDLER_NONE, 1, 0.0, 0, 0.0, NULL);
    metadata_add_parameter(parameters, 3, "Frontier Spacing Multiplier", BCD_METADATA_PARAM_TYPE_INTEGER, NULL, 0, "1", BCD_METADATA_PARAM_SECTION_COVERAGE_PATH, BCD_METADATA_APP_HANDLER_NONE, 1, 1.0, 0, 0.0, NULL);

    char *json = cJSON_PrintUnformatted(algorithm);
    cJSON_Delete(algorithm);
    return json;
}