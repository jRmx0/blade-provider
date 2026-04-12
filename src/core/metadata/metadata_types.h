#ifndef METADATA_TYPES_H
#define METADATA_TYPES_H

/* ── Style Attribute Keys ─────────────────────────────────────────────────── */

/* General */
#define METADATA_STYLE_ATTR_KEY_VISIBLE                  "Visible"
#define METADATA_STYLE_ATTR_KEY_Z_INDEX                  "Z-Index"
/* Point — Marker Shape */
#define METADATA_STYLE_ATTR_KEY_POINT_SHAPE              "Point Shape"
#define METADATA_STYLE_ATTR_KEY_POINT_RADIUS             "Point Radius"
/* Point — Overlap */
#define METADATA_STYLE_ATTR_KEY_POINT_OVERLAP_SPACING    "Point Overlap Spacing"
#define METADATA_STYLE_ATTR_KEY_POINT_OVERLAP_LAYOUT     "Point Overlap Layout"
/* Point — Border */
#define METADATA_STYLE_ATTR_KEY_POINT_BORDER_COLOR       "Point Border Color"
#define METADATA_STYLE_ATTR_KEY_POINT_BORDER_WIDTH       "Point Border Width"
#define METADATA_STYLE_ATTR_KEY_POINT_BORDER_STYLE       "Point Border Style"
/* Point — Fill */
#define METADATA_STYLE_ATTR_KEY_POINT_FILL_COLOR         "Point Fill Color"
/* Point — Id Label */
#define METADATA_STYLE_ATTR_KEY_POINT_ID_COLOR           "Point ID Color"
#define METADATA_STYLE_ATTR_KEY_POINT_ID_FONT_SIZE       "Point ID Font Size"
#define METADATA_STYLE_ATTR_KEY_POINT_ID_FONT_WEIGHT     "Point ID Font Weight"
#define METADATA_STYLE_ATTR_KEY_POINT_ID_PLACEMENT       "Point ID Placement"
#define METADATA_STYLE_ATTR_KEY_POINT_ID_OFFSET          "Point ID Offset"
/* Point — Text Label */
#define METADATA_STYLE_ATTR_KEY_POINT_LABEL_COLOR        "Point Label Color"
#define METADATA_STYLE_ATTR_KEY_POINT_LABEL_FONT_SIZE    "Point Label Font Size"
#define METADATA_STYLE_ATTR_KEY_POINT_LABEL_FONT_WEIGHT  "Point Label Font Weight"
#define METADATA_STYLE_ATTR_KEY_POINT_LABEL_PLACEMENT    "Point Label Placement"
#define METADATA_STYLE_ATTR_KEY_POINT_LABEL_OFFSET       "Point Label Offset"
/* Line — Edge */
#define METADATA_STYLE_ATTR_KEY_LINE_EDGE_COLOR          "Line Edge Color"
#define METADATA_STYLE_ATTR_KEY_LINE_EDGE_WIDTH          "Line Edge Width"
#define METADATA_STYLE_ATTR_KEY_LINE_EDGE_STYLE          "Line Edge Style"
/* Line — Arrow */
#define METADATA_STYLE_ATTR_KEY_LINE_ARROW_START         "Line Arrow Start"
#define METADATA_STYLE_ATTR_KEY_LINE_ARROW_END           "Line Arrow End"
#define METADATA_STYLE_ATTR_KEY_LINE_ARROW_MID           "Line Arrow Mid"
#define METADATA_STYLE_ATTR_KEY_LINE_ARROW_MID_SPACING   "Line Arrow Mid Spacing"
#define METADATA_STYLE_ATTR_KEY_LINE_ARROW_SIZE          "Line Arrow Size"
/* Polygon — Edge */
#define METADATA_STYLE_ATTR_KEY_POLYGON_EDGE_COLOR       "Polygon Edge Color"
#define METADATA_STYLE_ATTR_KEY_POLYGON_EDGE_WIDTH       "Polygon Edge Width"
#define METADATA_STYLE_ATTR_KEY_POLYGON_EDGE_STYLE       "Polygon Edge Style"
/* Polygon — Fill */
#define METADATA_STYLE_ATTR_KEY_POLYGON_FILL_COLOR       "Polygon Fill Color"
#define METADATA_STYLE_ATTR_KEY_POLYGON_FILL_STYLE       "Polygon Fill Style"
/* Polygon — ID */
#define METADATA_STYLE_ATTR_KEY_POLYGON_ID_COLOR         "Polygon ID Color"
#define METADATA_STYLE_ATTR_KEY_POLYGON_ID_FONT_SIZE     "Polygon ID Font Size"
#define METADATA_STYLE_ATTR_KEY_POLYGON_ID_FONT_WEIGHT   "Polygon ID Font Weight"
#define METADATA_STYLE_ATTR_KEY_POLYGON_ID_SHAPE         "Polygon ID Shape"
#define METADATA_STYLE_ATTR_KEY_POLYGON_ID_RADIUS        "Polygon ID Radius"
#define METADATA_STYLE_ATTR_KEY_POLYGON_ID_BORDER_COLOR  "Polygon ID Border Color"
#define METADATA_STYLE_ATTR_KEY_POLYGON_ID_BORDER_WIDTH  "Polygon ID Border Width"
#define METADATA_STYLE_ATTR_KEY_POLYGON_ID_BORDER_STYLE  "Polygon ID Border Style"
#define METADATA_STYLE_ATTR_KEY_POLYGON_ID_FILL_COLOR    "Polygon ID Fill Color"
#define METADATA_STYLE_ATTR_KEY_POLYGON_ID_PLACEMENT     "Polygon ID Placement"
#define METADATA_STYLE_ATTR_KEY_POLYGON_ID_OFFSET        "Polygon ID Offset"

/* ── Parameter Sections ─────────────────────────────────────────────────── */

#define BCD_METADATA_PARAM_SECTION_UNSPECIFIED    NULL
#define BCD_METADATA_PARAM_SECTION_GENERAL        "General"
#define BCD_METADATA_PARAM_SECTION_COVERAGE_PATH  "Coverage Path"
#define BCD_METADATA_PARAM_SECTION_ENVIRONMENT    "Environment"
#define BCD_METADATA_PARAM_SECTION_OBJECT         "Object"
#define BCD_METADATA_PARAM_SECTION_EXECUTION      "Execution"

/* ── Parameter Types ────────────────────────────────────────────────────── */

#define BCD_METADATA_PARAM_TYPE_INTEGER  "Integer"
#define BCD_METADATA_PARAM_TYPE_DECIMAL  "Decimal"
#define BCD_METADATA_PARAM_TYPE_BOOLEAN  "Boolean"
#define BCD_METADATA_PARAM_TYPE_STRING   "String"
#define BCD_METADATA_PARAM_TYPE_ENUM     "Enum"

/* ── App Handlers ───────────────────────────────────────────────────────── */

#define BCD_METADATA_APP_HANDLER_NONE                    NULL
#define BCD_METADATA_APP_HANDLER_ENVIRONMENT_FORMAT      "env.format"
#define BCD_METADATA_APP_HANDLER_ENVIRONMENT_TYPE        "env.type"
#define BCD_METADATA_APP_HANDLER_ENVIRONMENT_COORDSYSTEM "env.coordsystem"

/* ── Environment Parameter Values ──────────────────────────────────────── */

#define BCD_METADATA_FORMAT_POLYGON  "Polygon"
#define BCD_METADATA_FORMAT_GRID     "Grid"

#define BCD_METADATA_TYPE_OFFLINE    "Off-Line"
#define BCD_METADATA_TYPE_ONLINE     "On-Line"

#define BCD_METADATA_COORDSYSTEM_CARTESIAN  "Cartesian"
#define BCD_METADATA_COORDSYSTEM_GEOGRAPHIC  "Geographic"

/* ── Style Types ────────────────────────────────────────────────────────── */

#define METADATA_STYLE_TYPE_BOOLEAN               "Boolean"
#define METADATA_STYLE_TYPE_INTEGER               "Integer"
#define METADATA_STYLE_TYPE_COLOR                 "Color"
#define METADATA_STYLE_TYPE_PIXELS                "Pixels"
#define METADATA_STYLE_TYPE_POINT_SHAPE_ENUM      "PointShapeEnum"
#define METADATA_STYLE_TYPE_POLYGON_ID_SHAPE_ENUM "PolygonIDShapeEnum"
#define METADATA_STYLE_TYPE_STROKE_STYLE_ENUM     "StrokeStyleEnum"
#define METADATA_STYLE_TYPE_FILL_STYLE_ENUM       "FillStyleEnum"
#define METADATA_STYLE_TYPE_OVERLAP_LAYOUT_ENUM   "OverlapLayoutEnum"
#define METADATA_STYLE_TYPE_PLACEMENT_ENUM        "PlacementEnum"
#define METADATA_STYLE_TYPE_FONT_WEIGHT_ENUM      "FontWeightEnum"
#define METADATA_STYLE_TYPE_LINE_ARROW_START_ENUM "LineArrowStartEnum"
#define METADATA_STYLE_TYPE_LINE_ARROW_END_ENUM   "LineArrowEndEnum"
#define METADATA_STYLE_TYPE_LINE_ARROW_MID_ENUM   "LineArrowMidEnum"

/* Line Arrow Start values (LineArrowStartEnum) */
#define METADATA_LINE_ARROW_START_ARROW        "line_start_arrow"
#define METADATA_LINE_ARROW_START_ARROW_NOTCH  "line_start_arrow_notch"

/* Line Arrow End values (LineArrowEndEnum) */
#define METADATA_LINE_ARROW_END_ARROW          "line_end_arrow"
#define METADATA_LINE_ARROW_END_ARROW_NOTCH    "line_end_arrow_notch"

/* Line Arrow Mid values (LineArrowMidEnum) */
#define METADATA_LINE_ARROW_MID_START_ARROW        "line_start_arrow"
#define METADATA_LINE_ARROW_MID_START_ARROW_NOTCH  "line_start_arrow_notch"
#define METADATA_LINE_ARROW_MID_END_ARROW          "line_end_arrow"
#define METADATA_LINE_ARROW_MID_END_ARROW_NOTCH    "line_end_arrow_notch"

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