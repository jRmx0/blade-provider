#ifndef METADATA_TYPES_H
#define METADATA_TYPES_H

typedef enum {
    BCD_METADATA_PARAM_SECTION_UNSPECIFIED = 0,
    BCD_METADATA_PARAM_SECTION_GENERAL,
    BCD_METADATA_PARAM_SECTION_COVERAGE_PATH,
    BCD_METADATA_PARAM_SECTION_ENVIRONMENT,
    BCD_METADATA_PARAM_SECTION_OBJECT,
    BCD_METADATA_PARAM_SECTION_EXECUTION
} bcd_metadata_param_section_t;

typedef enum {
    BCD_METADATA_PARAM_TYPE_INTEGER = 0,
    BCD_METADATA_PARAM_TYPE_DECIMAL,
    BCD_METADATA_PARAM_TYPE_BOOLEAN,
    BCD_METADATA_PARAM_TYPE_STRING,
    BCD_METADATA_PARAM_TYPE_ENUM
} bcd_metadata_param_type_t;

typedef enum {
	BCD_METADATA_APP_HANDLER_NONE = 0,
	BCD_METADATA_APP_HANDLER_ENVIRONMENT_FORMAT,
	BCD_METADATA_APP_HANDLER_ENVIRONMENT_TYPE,
	BCD_METADATA_APP_HANDLER_ENVIRONMENT_COORDSYSTEM
} bcd_metadata_app_handler_t;

typedef enum {
    BCD_METADATA_FORMAT_POLYGON = 0,
    BCD_METADATA_FORMAT_GRID
} bcd_metadata_format_t;

typedef enum {
    BCD_METADATA_TYPE_OFFLINE = 0,
    BCD_METADATA_TYPE_ONLINE
} bcd_metadata_type_t;

typedef enum {
    BCD_METADATA_COORDSYSTEM_DECIMAL = 0,
    BCD_METADATA_COORDSYSTEM_LATLONG
} bcd_metadata_coordsystem_t;

static inline const char *metadata_param_section_to_string(bcd_metadata_param_section_t section)
{
    switch (section)
    {
    case BCD_METADATA_PARAM_SECTION_UNSPECIFIED:
        return (const char *)0;
    case BCD_METADATA_PARAM_SECTION_GENERAL:
        return "General";
    case BCD_METADATA_PARAM_SECTION_COVERAGE_PATH:
        return "Coverage path";
    case BCD_METADATA_PARAM_SECTION_ENVIRONMENT:
        return "Environment";
    case BCD_METADATA_PARAM_SECTION_OBJECT:
        return "Object";
    case BCD_METADATA_PARAM_SECTION_EXECUTION:
        return "Execution";
    default:
        return (const char *)0;
    }
}

static inline const char *metadata_param_type_to_string(bcd_metadata_param_type_t param_type)
{
    switch (param_type)
    {
    case BCD_METADATA_PARAM_TYPE_INTEGER:
        return "Integer";
    case BCD_METADATA_PARAM_TYPE_DECIMAL:
        return "Decimal";
    case BCD_METADATA_PARAM_TYPE_BOOLEAN:
        return "Boolean";
    case BCD_METADATA_PARAM_TYPE_STRING:
        return "String";
    case BCD_METADATA_PARAM_TYPE_ENUM:
        return "Enum";
    default:
        return "String";
    }
}

static inline const char *metadata_app_handler_to_string(bcd_metadata_app_handler_t app_handler)
{
	switch (app_handler)
	{
	case BCD_METADATA_APP_HANDLER_NONE:
		return (const char *)0;
	case BCD_METADATA_APP_HANDLER_ENVIRONMENT_FORMAT:
        return "env.format";
	case BCD_METADATA_APP_HANDLER_ENVIRONMENT_TYPE:
        return "env.type";
	case BCD_METADATA_APP_HANDLER_ENVIRONMENT_COORDSYSTEM:
        return "env.coordsystem";
	default:
		return (const char *)0;
    }
}

static inline const char *metadata_format_to_string(bcd_metadata_format_t format)
{
    switch (format)
    {
    case BCD_METADATA_FORMAT_POLYGON:
        return "Polygon";
    case BCD_METADATA_FORMAT_GRID:
        return "Grid";
    default:
        return "Polygon";
    }
}

static inline const char *metadata_type_to_string(bcd_metadata_type_t type)
{
    switch (type)
    {
    case BCD_METADATA_TYPE_OFFLINE:
        return "Off-Line";
    case BCD_METADATA_TYPE_ONLINE:
        return "On-Line";
    default:
        return "Off-Line";
    }
}

static inline const char *metadata_coordsystem_to_string(bcd_metadata_coordsystem_t coordsystem)
{
    switch (coordsystem)
    {
    case BCD_METADATA_COORDSYSTEM_DECIMAL:
        return "Decimal";
    case BCD_METADATA_COORDSYSTEM_LATLONG:
        return "Lat/Long";
    default:
        return "Decimal";
    }
}

#endif // METADATA_TYPES_H