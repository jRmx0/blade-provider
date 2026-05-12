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

    char *json = cJSON_PrintUnformatted(algorithm);
    cJSON_Delete(algorithm);
    return json;
}