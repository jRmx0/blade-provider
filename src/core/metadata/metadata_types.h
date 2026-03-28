#ifndef METADATA_TYPES_H
#define METADATA_TYPES_H

typedef enum {
    METADATA_STYLE_ATTR_KEY_VISIBLE = 0,
    METADATA_STYLE_ATTR_KEY_Z_INDEX,
    /* Point — Marker Shape */
    METADATA_STYLE_ATTR_KEY_POINT_SHAPE,
    METADATA_STYLE_ATTR_KEY_POINT_RADIUS,
    /* Point — Overlap */
    METADATA_STYLE_ATTR_KEY_POINT_OVERLAP_SPACING,
    METADATA_STYLE_ATTR_KEY_POINT_OVERLAP_LAYOUT,
    /* Point — Border */
    METADATA_STYLE_ATTR_KEY_POINT_BORDER_COLOR,
    METADATA_STYLE_ATTR_KEY_POINT_BORDER_WIDTH,
    METADATA_STYLE_ATTR_KEY_POINT_BORDER_STYLE,
    /* Point — Fill */
    METADATA_STYLE_ATTR_KEY_POINT_FILL_COLOR,
    /* Point — Id Label */
    METADATA_STYLE_ATTR_KEY_POINT_ID_COLOR,
    METADATA_STYLE_ATTR_KEY_POINT_ID_FONT_SIZE,
    METADATA_STYLE_ATTR_KEY_POINT_ID_FONT_WEIGHT,
    METADATA_STYLE_ATTR_KEY_POINT_ID_PLACEMENT,
    METADATA_STYLE_ATTR_KEY_POINT_ID_OFFSET,
    /* Point — Text Label */
    METADATA_STYLE_ATTR_KEY_POINT_LABEL_COLOR,
    METADATA_STYLE_ATTR_KEY_POINT_LABEL_FONT_SIZE,
    METADATA_STYLE_ATTR_KEY_POINT_LABEL_FONT_WEIGHT,
    METADATA_STYLE_ATTR_KEY_POINT_LABEL_PLACEMENT,
    METADATA_STYLE_ATTR_KEY_POINT_LABEL_OFFSET,
    /* Line — Edge */
    METADATA_STYLE_ATTR_KEY_LINE_EDGE_COLOR,
    METADATA_STYLE_ATTR_KEY_LINE_EDGE_WIDTH,
    METADATA_STYLE_ATTR_KEY_LINE_EDGE_STYLE,
    /* Line — Arrow */
    METADATA_STYLE_ATTR_KEY_LINE_ARROW_START,
    METADATA_STYLE_ATTR_KEY_LINE_ARROW_END,
    METADATA_STYLE_ATTR_KEY_LINE_ARROW_MID,
    METADATA_STYLE_ATTR_KEY_LINE_ARROW_MID_SPACING,
    METADATA_STYLE_ATTR_KEY_LINE_ARROW_SIZE,
    /* Polygon — Edge */
    METADATA_STYLE_ATTR_KEY_POLYGON_EDGE_COLOR,
    METADATA_STYLE_ATTR_KEY_POLYGON_EDGE_WIDTH,
    METADATA_STYLE_ATTR_KEY_POLYGON_EDGE_STYLE,
    /* Polygon — Fill */
    METADATA_STYLE_ATTR_KEY_POLYGON_FILL_COLOR,
    METADATA_STYLE_ATTR_KEY_POLYGON_FILL_STYLE,
    /* Polygon — ID */
    METADATA_STYLE_ATTR_KEY_POLYGON_ID_COLOR,
    METADATA_STYLE_ATTR_KEY_POLYGON_ID_FONT_SIZE,
    METADATA_STYLE_ATTR_KEY_POLYGON_ID_FONT_WEIGHT,
    METADATA_STYLE_ATTR_KEY_POLYGON_ID_SHAPE,
    METADATA_STYLE_ATTR_KEY_POLYGON_ID_RADIUS,
    METADATA_STYLE_ATTR_KEY_POLYGON_ID_BORDER_COLOR,
    METADATA_STYLE_ATTR_KEY_POLYGON_ID_BORDER_WIDTH,
    METADATA_STYLE_ATTR_KEY_POLYGON_ID_BORDER_STYLE,
    METADATA_STYLE_ATTR_KEY_POLYGON_ID_FILL_COLOR,
    METADATA_STYLE_ATTR_KEY_POLYGON_ID_PLACEMENT,
    METADATA_STYLE_ATTR_KEY_POLYGON_ID_OFFSET
} metadata_style_attr_key_t;

static inline const char *metadata_style_attr_key_to_string(metadata_style_attr_key_t key)
{
    switch (key)
    {
    case METADATA_STYLE_ATTR_KEY_VISIBLE:                  return "Visible";
    case METADATA_STYLE_ATTR_KEY_Z_INDEX:                  return "Z-Index";
    case METADATA_STYLE_ATTR_KEY_POINT_SHAPE:              return "Point Shape";
    case METADATA_STYLE_ATTR_KEY_POINT_RADIUS:             return "Point Radius";
    case METADATA_STYLE_ATTR_KEY_POINT_OVERLAP_SPACING:    return "Point Overlap Spacing";
    case METADATA_STYLE_ATTR_KEY_POINT_OVERLAP_LAYOUT:     return "Point Overlap Layout";
    case METADATA_STYLE_ATTR_KEY_POINT_BORDER_COLOR:       return "Point Border Color";
    case METADATA_STYLE_ATTR_KEY_POINT_BORDER_WIDTH:       return "Point Border Width";
    case METADATA_STYLE_ATTR_KEY_POINT_BORDER_STYLE:       return "Point Border Style";
    case METADATA_STYLE_ATTR_KEY_POINT_FILL_COLOR:         return "Point Fill Color";
    case METADATA_STYLE_ATTR_KEY_POINT_ID_COLOR:           return "Point ID Color";
    case METADATA_STYLE_ATTR_KEY_POINT_ID_FONT_SIZE:       return "Point ID Font Size";
    case METADATA_STYLE_ATTR_KEY_POINT_ID_FONT_WEIGHT:     return "Point ID Font Weight";
    case METADATA_STYLE_ATTR_KEY_POINT_ID_PLACEMENT:       return "Point ID Placement";
    case METADATA_STYLE_ATTR_KEY_POINT_ID_OFFSET:          return "Point ID Offset";
    case METADATA_STYLE_ATTR_KEY_POINT_LABEL_COLOR:        return "Point Label Color";
    case METADATA_STYLE_ATTR_KEY_POINT_LABEL_FONT_SIZE:    return "Point Label Font Size";
    case METADATA_STYLE_ATTR_KEY_POINT_LABEL_FONT_WEIGHT:  return "Point Label Font Weight";
    case METADATA_STYLE_ATTR_KEY_POINT_LABEL_PLACEMENT:    return "Point Label Placement";
    case METADATA_STYLE_ATTR_KEY_POINT_LABEL_OFFSET:       return "Point Label Offset";
    case METADATA_STYLE_ATTR_KEY_LINE_EDGE_COLOR:          return "Line Edge Color";
    case METADATA_STYLE_ATTR_KEY_LINE_EDGE_WIDTH:          return "Line Edge Width";
    case METADATA_STYLE_ATTR_KEY_LINE_EDGE_STYLE:          return "Line Edge Style";
    case METADATA_STYLE_ATTR_KEY_LINE_ARROW_START:         return "Line Arrow Start";
    case METADATA_STYLE_ATTR_KEY_LINE_ARROW_END:           return "Line Arrow End";
    case METADATA_STYLE_ATTR_KEY_LINE_ARROW_MID:           return "Line Arrow Mid";
    case METADATA_STYLE_ATTR_KEY_LINE_ARROW_MID_SPACING:   return "Line Arrow Mid Spacing";
    case METADATA_STYLE_ATTR_KEY_LINE_ARROW_SIZE:          return "Line Arrow Size";
    case METADATA_STYLE_ATTR_KEY_POLYGON_EDGE_COLOR:       return "Polygon Edge Color";
    case METADATA_STYLE_ATTR_KEY_POLYGON_EDGE_WIDTH:       return "Polygon Edge Width";
    case METADATA_STYLE_ATTR_KEY_POLYGON_EDGE_STYLE:       return "Polygon Edge Style";
    case METADATA_STYLE_ATTR_KEY_POLYGON_FILL_COLOR:       return "Polygon Fill Color";
    case METADATA_STYLE_ATTR_KEY_POLYGON_FILL_STYLE:       return "Polygon Fill Style";
    case METADATA_STYLE_ATTR_KEY_POLYGON_ID_COLOR:         return "Polygon ID Color";
    case METADATA_STYLE_ATTR_KEY_POLYGON_ID_FONT_SIZE:     return "Polygon ID Font Size";
    case METADATA_STYLE_ATTR_KEY_POLYGON_ID_FONT_WEIGHT:   return "Polygon ID Font Weight";
    case METADATA_STYLE_ATTR_KEY_POLYGON_ID_SHAPE:         return "Polygon ID Shape";
    case METADATA_STYLE_ATTR_KEY_POLYGON_ID_RADIUS:        return "Polygon ID Radius";
    case METADATA_STYLE_ATTR_KEY_POLYGON_ID_BORDER_COLOR:  return "Polygon ID Border Color";
    case METADATA_STYLE_ATTR_KEY_POLYGON_ID_BORDER_WIDTH:  return "Polygon ID Border Width";
    case METADATA_STYLE_ATTR_KEY_POLYGON_ID_BORDER_STYLE:  return "Polygon ID Border Style";
    case METADATA_STYLE_ATTR_KEY_POLYGON_ID_FILL_COLOR:    return "Polygon ID Fill Color";
    case METADATA_STYLE_ATTR_KEY_POLYGON_ID_PLACEMENT:     return "Polygon ID Placement";
    case METADATA_STYLE_ATTR_KEY_POLYGON_ID_OFFSET:        return "Polygon ID Offset";
    default:                                               return "Visible";
    }
}

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
    METADATA_STYLE_TYPE_BOOLEAN = 0,
    METADATA_STYLE_TYPE_INTEGER,
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
    case METADATA_STYLE_TYPE_BOOLEAN:               return "Boolean";
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