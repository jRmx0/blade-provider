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
        return "Coverage Path";
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

typedef enum {
    METADATA_STYLE_TYPE_INTEGER = 0,
    METADATA_STYLE_TYPE_COLOR,
    METADATA_STYLE_TYPE_SPACING,
    METADATA_STYLE_TYPE_POINT_SHAPE_ENUM,
    METADATA_STYLE_TYPE_POLYGON_ID_SHAPE_ENUM,
    METADATA_STYLE_TYPE_STROKE_STYLE_ENUM,
    METADATA_STYLE_TYPE_FILL_STYLE_ENUM,
    METADATA_STYLE_TYPE_OVERLAP_LAYOUT_ENUM,
    METADATA_STYLE_TYPE_PLACEMENT_ENUM,
    METADATA_STYLE_TYPE_FONT_SIZE_ENUM,
    METADATA_STYLE_TYPE_FONT_WEIGHT_ENUM,
    METADATA_STYLE_TYPE_LINE_ARROW_START_ENUM,
    METADATA_STYLE_TYPE_LINE_ARROW_END_ENUM,
    METADATA_STYLE_TYPE_LINE_ARROW_MID_ENUM
} metadata_style_type_t;

static inline const char *metadata_style_type_to_string(metadata_style_type_t style_type)
{
    switch (style_type)
    {
    case METADATA_STYLE_TYPE_INTEGER:               return "Integer";
    case METADATA_STYLE_TYPE_COLOR:                 return "Color";
    case METADATA_STYLE_TYPE_SPACING:               return "Spacing";
    case METADATA_STYLE_TYPE_POINT_SHAPE_ENUM:      return "PointShapeEnum";
    case METADATA_STYLE_TYPE_POLYGON_ID_SHAPE_ENUM: return "PolygonIDShapeEnum";
    case METADATA_STYLE_TYPE_STROKE_STYLE_ENUM:     return "StrokeStyleEnum";
    case METADATA_STYLE_TYPE_FILL_STYLE_ENUM:       return "FillStyleEnum";
    case METADATA_STYLE_TYPE_OVERLAP_LAYOUT_ENUM:   return "OverlapLayoutEnum";
    case METADATA_STYLE_TYPE_PLACEMENT_ENUM:        return "PlacementEnum";
    case METADATA_STYLE_TYPE_FONT_SIZE_ENUM:        return "FontSizeEnum";
    case METADATA_STYLE_TYPE_FONT_WEIGHT_ENUM:      return "FontWeightEnum";
    case METADATA_STYLE_TYPE_LINE_ARROW_START_ENUM: return "LineArrowStartEnum";
    case METADATA_STYLE_TYPE_LINE_ARROW_END_ENUM:   return "LineArrowEndEnum";
    case METADATA_STYLE_TYPE_LINE_ARROW_MID_ENUM:   return "LineArrowMidEnum";
    default:                                        return "Integer";
    }
}

typedef enum {
    BCD_METADATA_DEBUG_LAYER_TYPE_POINT = 0,
    BCD_METADATA_DEBUG_LAYER_TYPE_LINE,
    BCD_METADATA_DEBUG_LAYER_TYPE_POLYGON
} bcd_metadata_debug_layer_type_t;

static inline const char *metadata_debug_layer_type_to_string(bcd_metadata_debug_layer_type_t layer_type)
{
    switch (layer_type)
    {
    case BCD_METADATA_DEBUG_LAYER_TYPE_POINT:
        return "Point";
    case BCD_METADATA_DEBUG_LAYER_TYPE_LINE:
        return "Line";
    case BCD_METADATA_DEBUG_LAYER_TYPE_POLYGON:
        return "Polygon";
    default:
        return "Point";
    }
}

#endif // METADATA_TYPES_H