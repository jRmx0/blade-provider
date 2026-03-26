# GET /metadata

← [Back to API Overview](./api.md)

Returns all algorithms the provider exposes, with their parameter schemas. Consumers should fetch this once on connection and use it to build parameter input forms.

**Response `200`**
```json
{
  "algorithms": [
    {
      "id": 1,
      "name": "Boustrophedon Cellular Decomposition",
      "parameters": [
        {
          "id": 1,
          "name": "Path Width",
          "paramType": "Decimal",
          "enumValues": [],
          "defaultValue": "15",
          "section": "Coverage Path",
          "appHandler": null
        },
        {
          "id": 2,
          "name": "Path Overlap",
          "paramType": "Decimal",
          "enumValues": [],
          "defaultValue": "5",
          "section": "Coverage Path",
          "appHandler": null
        },
        {
          "id": 3,
          "name": "Format",
          "paramType": "Enum",
          "enumValues": ["Polygon"],
          "defaultValue": "Polygon",
          "section": "Environment",
          "appHandler": "env.format"
        },
        {
          "id": 4,
          "name": "Type",
          "paramType": "Enum",
          "enumValues": ["Off-Line"],
          "defaultValue": "Off-Line",
          "section": "Environment",
          "appHandler": "env.type"
        },
        {
          "id": 5,
          "name": "Coordinate System",
          "paramType": "Enum",
          "enumValues": ["Decimal"],
          "defaultValue": "Decimal",
          "section": "Environment",
          "appHandler": "env.coordsystem"
        }
      ],
      "layers": [
        {
          "id": 1,
          "computeLayer": "coveragePathPlan",
          "name": "Coverage Path",
          "layerType": "Line",
          "style": {
            "universalStyleAttributes": [
              { "name": "Z-Index",                "styleType": "Integer",           "defaultValue": "100"            },
            ], 
            "pointStyleAttributes": [ 
              { "name": "Point Shape",            "styleType": "PointShapeEnum",    "defaultValue": null             },
              { "name": "Point Radius",           "styleType": "Spacing",           "defaultValue": null             },
              { "name": "Point Overlap Spacing",  "styleType": "Spacing",           "defaultValue": null             },
              { "name": "Point Overlap Layout",   "styleType": "OverlapLayoutEnum", "defaultValue": null             },
              { "name": "Point Border Color",     "styleType": "Color",             "defaultValue": null             },
              { "name": "Point Border Width",     "styleType": "Spacing",           "defaultValue": null             },
              { "name": "Point Border Style",     "styleType": "StrokeStyleEnum",   "defaultValue": null             },
              { "name": "Point Fill Color",       "styleType": "Color",             "defaultValue": null             },
              { "name": "Point ID Color",         "styleType": "Color",             "defaultValue": null             },
              { "name": "Point ID Font Size",     "styleType": "FontSizeEnum",      "defaultValue": null             },
              { "name": "Point ID Font Weight",   "styleType": "FontWeightEnum",    "defaultValue": null             },
              { "name": "Point ID Placement",     "styleType": "PlacementEnum",     "defaultValue": null             },
              { "name": "Point ID Offset",        "styleType": "Spacing",           "defaultValue": null             },
              { "name": "Point Label Color",      "styleType": "Color",             "defaultValue": null             },
              { "name": "Point Label Font Size",  "styleType": "FontSizeEnum",      "defaultValue": null             },
              { "name": "Point Label Font Weight","styleType": "FontWeightEnum",    "defaultValue": null             },
              { "name": "Point Label Placement",  "styleType": "PlacementEnum",     "defaultValue": null             },
              { "name": "Point Label Offset",     "styleType": "Spacing",           "defaultValue": null             },
            ],
            "lineStyleAttributes": [
              { "name": "Line Edge Color",        "styleType": "Color",             "defaultValue": "#60a5fa"        },
              { "name": "Line Edge Width",        "styleType": "Spacing",           "defaultValue": "1"              },
              { "name": "Line Edge Style",        "styleType": "StrokeStyleEnum",   "defaultValue": "solid"          },
              { "name": "Line Arrow Start",       "styleType": "LineArrowStartEnum","defaultValue": null             },
              { "name": "Line Arrow End",         "styleType": "LineArrowEndEnum",  "defaultValue": null             },
              { "name": "Line Arrow Mid",         "styleType": "LineArrowMidEnum",  "defaultValue": "line_end_arrow" },
              { "name": "Line Arrow Mid Spacing", "styleType": "Spacing",           "defaultValue": "12"             },
              { "name": "Line Arrow Size",        "styleType": "Spacing",           "defaultValue": "3"              }
            ]
          }
        },
        {
          "id": 10,
          "computeLayer": "eventList",
          "name": "Event List",
          "layerType": "Point",
          "pointLabelEnumValues": ["B_IN", "B_SIDE_IN", "B_INIT", "B_OUT", "B_SIDE_OUT", "B_DEINIT", "IN", "SIDE_IN", "OUT", "SIDE_OUT", "FLOOR", "CEILING", "NONE"],
          "style": 
          {
            "universalStyleAttributes": [
              { "name": "Z-Index",                "styleType": "Integer",          "defaultValue": "110"            },
            ],
            "pointStyleAttributes": [
              { "name": "Point Shape",            "styleType": "PointShapeEnum",   "defaultValue": "circle"         },
              { "name": "Point Radius",           "styleType": "Spacing",          "defaultValue": "4"              },
              { "name": "Point Overlap Spacing",  "styleType": "Spacing",          "defaultValue": null             },
              { "name": "Point Overlap Layout",   "styleType": "OverlapLayoutEnum","defaultValue": null             },
              { "name": "Point Border Color",     "styleType": "Color",            "defaultValue": "#a855f7"        },
              { "name": "Point Border Width",     "styleType": "Spacing",          "defaultValue": "1"              },
              { "name": "Point Border Style",     "styleType": "StrokeStyleEnum",  "defaultValue": "solid"          },
              { "name": "Point Fill Color",       "styleType": "Color",            "defaultValue": "#a855f7"        },
              { "name": "Point ID Color",         "styleType": "Color",            "defaultValue": null             },
              { "name": "Point ID Font Size",     "styleType": "FontSizeEnum",     "defaultValue": null             },
              { "name": "Point ID Font Weight",   "styleType": "FontWeightEnum",   "defaultValue": null             },
              { "name": "Point ID Placement",     "styleType": "PlacementEnum",    "defaultValue": null             },
              { "name": "Point ID Offset",        "styleType": "Spacing",          "defaultValue": null             },
              { "name": "Point Label Color",      "styleType": "Color",            "defaultValue": "#a855f7"        },
              { "name": "Point Label Font Size",  "styleType": "FontSizeEnum",     "defaultValue": "text-xs"        },
              { "name": "Point Label Font Weight","styleType": "FontWeightEnum",   "defaultValue": "font-medium"    },
              { "name": "Point Label Placement",  "styleType": "PlacementEnum",    "defaultValue": "outside-bottom" },
              { "name": "Point Label Offset",     "styleType": "Spacing",          "defaultValue": "6"              }
            ],
            "pointLabelColorMapping": [
              { "value": "B_IN",       "color": "#16a34a"   },
              { "value": "B_SIDE_IN",  "color": "#16a34a"   },
              { "value": "B_INIT",     "color": "#16a34a"   },
              { "value": "B_OUT",      "color": "#dc2626"   },
              { "value": "B_SIDE_OUT", "color": "#dc2626"   },
              { "value": "B_DEINIT",   "color": "#dc2626"   },
              { "value": "IN",         "color": "#16a34a"   },
              { "value": "SIDE_IN",    "color": "#16a34a"   },
              { "value": "OUT",        "color": "#dc2626"   },
              { "value": "SIDE_OUT",   "color": "#dc2626"   },
              { "value": "FLOOR",      "color": "#60a5fa"   },
              { "value": "CEILING",    "color": "#fb923c"   },
              { "value": "NONE",       "color": "#FF00FF"   }
            ]
          }          
        },
        {
          "id": 11,
          "computeLayer": "cellList",
          "name": "Cell List",
          "layerType": "Polygon",
          "style":  
          {
            "universalStyleAttributes": [
              { "name": "Z-Index",                "styleType": "Integer",           "defaultValue": "120"        },
            ],
            "pointStyleAttributes": [
              { "name": "Point Shape",            "styleType": "PointShapeEnum",    "defaultValue": null         },
              { "name": "Point Radius",           "styleType": "Spacing",           "defaultValue": null         },
              { "name": "Point Border Color",     "styleType": "Color",             "defaultValue": null         },
              { "name": "Point Border Width",     "styleType": "Spacing",           "defaultValue": null         },
              { "name": "Point Border Style",     "styleType": "StrokeStyleEnum",   "defaultValue": null         },
              { "name": "Point Fill Color",       "styleType": "Color",             "defaultValue": null         },
              { "name": "Point ID Color",         "styleType": "Color",             "defaultValue": null         },
              { "name": "Point ID Font Size",     "styleType": "FontSizeEnum",      "defaultValue": null         },
              { "name": "Point ID Font Weight",   "styleType": "FontWeightEnum",    "defaultValue": null         },
              { "name": "Point ID Placement",     "styleType": "PlacementEnum",     "defaultValue": null         },
              { "name": "Point ID Offset",        "styleType": "Spacing",           "defaultValue": null         },
            ],
            "polygonStyleAttributes": [
              { "name": "Polygon Edge Color",     "styleType": "Color",             "defaultValue": "#94a3b8"    },
              { "name": "Polygon Edge Width",     "styleType": "Spacing",           "defaultValue": "1"          },
              { "name": "Polygon Edge Style",     "styleType": "StrokeStyleEnum",   "defaultValue": "solid"      },
              { "name": "Polygon Fill Color",     "styleType": "Color",             "defaultValue": null         },
              { "name": "Polygon Fill Style",     "styleType": "FillStyleEnum",     "defaultValue": null         },
              { "name": "Polygon ID Color",       "styleType": "Color",             "defaultValue": "#64748b"    },
              { "name": "Polygon ID Font Size",   "styleType": "FontSizeEnum",      "defaultValue": "text-xs"    },
              { "name": "Polygon ID Font Weight", "styleType": "FontWeightEnum",    "defaultValue": "font-medium"},
              { "name": "Polygon ID Shape",       "styleType": "PolygonIDShapeEnum","defaultValue": null         },
              { "name": "Polygon ID Radius",      "styleType": "Spacing",           "defaultValue": null         },
              { "name": "Polygon ID Border Color","styleType": "Color",             "defaultValue": null         },
              { "name": "Polygon ID Border Width","styleType": "Spacing",           "defaultValue": null         },
              { "name": "Polygon ID Border Style","styleType": "StrokeStyleEnum",   "defaultValue": null         },
              { "name": "Polygon ID Fill Color",  "styleType": "Color",             "defaultValue": null         },
              { "name": "Polygon ID Placement",   "styleType": "PlacementEnum",     "defaultValue": "inside"     },
              { "name": "Polygon ID Offset",      "styleType": "Spacing",           "defaultValue": null         }
            ]
          }
        },
        {
          "id": 12,
          "computeLayer": "cellVisitOrder",
          "name": "Cell Visit Order",
          "layerType": "Line",
          "style": 
          {
            "universalStyleAttributes": [
              { "name": "Z-Index",                "styleType": "Integer",           "defaultValue": "130"             },
            ],
            "pointStyleAttributes": [
              { "name": "Point Shape",            "styleType": "PointShapeEnum",    "defaultValue": "circle"          },
              { "name": "Point Radius",           "styleType": "Spacing",           "defaultValue": "3"               },
              { "name": "Point Overlap Spacing",  "styleType": "Spacing",           "defaultValue": "8"               },
              { "name": "Point Overlap Layout",   "styleType": "OverlapLayoutEnum", "defaultValue": "grid"            },
              { "name": "Point Border Color",     "styleType": "Color",             "defaultValue": null              },
              { "name": "Point Border Width",     "styleType": "Spacing",           "defaultValue": null              },
              { "name": "Point Border Style",     "styleType": "StrokeStyleEnum",   "defaultValue": null              },
              { "name": "Point Fill Color",       "styleType": "Color",             "defaultValue": "#60a5fa"         },
              { "name": "Point ID Color",         "styleType": "Color",             "defaultValue": "#60a5fa"         },
              { "name": "Point ID Font Size",     "styleType": "FontSizeEnum",      "defaultValue": "text-xs"         },
              { "name": "Point ID Font Weight",   "styleType": "FontWeightEnum",    "defaultValue": "font-medium"     },
              { "name": "Point ID Placement",     "styleType": "PlacementEnum",     "defaultValue": "inside"          },
              { "name": "Point ID Offset",        "styleType": "Spacing",           "defaultValue": null              },
              { "name": "Point Label Color",      "styleType": "Color",             "defaultValue": null              },
              { "name": "Point Label Font Size",  "styleType": "FontSizeEnum",      "defaultValue": null              },
              { "name": "Point Label Font Weight","styleType": "FontWeightEnum",    "defaultValue": null              },
              { "name": "Point Label Placement",  "styleType": "PlacementEnum",     "defaultValue": null              },
              { "name": "Point Label Offset",     "styleType": "Spacing",           "defaultValue": null              },
            ],
            "lineStyleAttributes": [
              { "name": "Line Edge Color",        "styleType": "Color",             "defaultValue": "#60a5fa"         },
              { "name": "Line Edge Width",        "styleType": "Spacing",           "defaultValue": "1"               },
              { "name": "Line Edge Style",        "styleType": "StrokeStyleEnum",   "defaultValue": "solid"           },
              { "name": "Line Arrow Start",       "styleType": "LineArrowStartEnum","defaultValue": null              },
              { "name": "Line Arrow End",         "styleType": "LineArrowEndEnum",  "defaultValue": null              },
              { "name": "Line Arrow Mid",         "styleType": "LineArrowMidEnum",  "defaultValue": "line_end_arrow"  },
              { "name": "Line Arrow Mid Spacing", "styleType": "Spacing",           "defaultValue": "12"              },
              { "name": "Line Arrow Size",        "styleType": "Spacing",           "defaultValue": "3"               }
            ]
          }
        }
      ]
    }
  ]
}
```

`appHandler` values tell the consumer that this parameter is bound to an environment-level property (format, type, coordinate system) rather than being a free-form algorithm input. Consumers should resolve these from the environment state rather than prompting the user separately.

`layers` declares all renderable output layers for an algorithm. All layers carry a `computeLayer` field: coverage layers point to the field name in `result.coveragePathPlan`; debug layers match the `source` value in `result.debug.layers`. Consumers should use the `style` as the initial rendering config but may override it locally.

---

## Types

### `AlgorithmMetadata`

| Field | Type | Notes |
|---|---|---|
| `id` | `number` | Unique numeric identifier for the algorithm — pass as `algorithmId` in compute requests |
| `name` | `string` | Human-readable display name |
| `parameters` | `ParameterMetadata[]` | Parameter schema declarations; use to build input forms |
| `layers` | `LayerMetadata[]` | All renderable output layers declared by this algorithm |

### `ParameterMetadata`

| Field | Type | Notes |
|---|---|---|
| `id` | `number` | Unique numeric identifier for this parameter |
| `name` | `string` | Parameter name — used as the key in the compute request `parameters` object |
| `paramType` | `ParameterType` | Value data type — see [`ParameterType`](#parametertype) |
| `enumValues` | `string[]` | Allowed values when `paramType` is `"Enum"`; empty array for all other types |
| `defaultValue` | `string` | Default value serialised as a string |
| `section` | `string` | UI grouping label |
| `appHandler` | `string \| null` | When non-null, binds this parameter to an environment-level property — see the `appHandler` note above |

### `ParameterType`

| Value | Description |
|---|---|
| `"Integer"` | Whole number |
| `"Decimal"` | Floating-point |
| `"Boolean"` | Boolean |
| `"Enum"` | One of `enumValues` |
| `"String"` | Arbitrary string |

### `LayerMetadata`

| Field | Type | Notes |
|---|---|---|
| `id` | `number` | Unique numeric identifier for this layer |
| `computeLayer` | `string` | Names the result field this layer binds to. On coverage path plan layers, names the field in `result.coveragePathPlan`. On debug layers, matches the `source` value on the corresponding entry in `result.debug.layers` |
| `name` | `string` | Human-readable display name |
| `layerType` | `LayerType` | Geometry primitive type of the objects produced by this layer |
| `style` | `LayerStyle` | Default rendering attributes grouped by attribute family — see [`LayerStyle`](#layerstyle) |
| `pointLabelEnumValues` | `string[]?` | All valid `pointLabel` values for this layer, in definition order. Present only on Point layers whose compute items carry a `pointLabel` field |

### `LayerType`

| Value | Description |
|---|---|
| `"Point"` | Layer items are rendered as individual point markers |
| `"Line"` | Layer items are rendered as line segments or polylines |
| `"Polygon"` | Layer items are rendered as filled or stroked polygon shapes |

### `LayerStyle`

Style attributes are grouped into sub-arrays by attribute family. All families applicable to the layer's `layerType` are always included in the object.

| Field | Type | Notes |
|---|---|---|
| `universalStyleAttributes` | `LayerStyleAttribute[]` | Attributes that apply to all layer types (e.g. Z-Index) |
| `pointStyleAttributes` | `LayerStyleAttribute[]?` | Point marker attributes — present on `Point`, `Line`, and `Polygon` layers. On `Line` layers these are the point-vertex attributes; on `Polygon` layers these are the corner-vertex attributes (Overlap and Text Label groups excluded) |
| `lineStyleAttributes` | `LayerStyleAttribute[]?` | Line stroke and arrow attributes — present on `Line` layers |
| `polygonStyleAttributes` | `LayerStyleAttribute[]?` | Polygon fill and stroke attributes — present on `Polygon` layers |
| `pointLabelColorMapping` | `PointLabelColorEntry[]?` | Per-value color overrides for the `pointLabel` field. Present only on layers that carry `pointLabelEnumValues`. Always targets the `pointLabel` field on each compute item |

### `LayerStyleAttribute`

Each entry in an attribute family array.

| Field | Type | Notes |
|---|---|---|
| `name` | `string` | Human-readable display name for the attribute — see [Layer Styles](./api-layer-styles.md) for the full registry |
| `styleType` | `StyleType` | The value category of this attribute — see [Layer Styles StyleType](./api-layer-styles.md#styletype) |
| `defaultValue` | `string \| null` | Default attribute value serialised as a string, or `null` when inactive. `null` means the attribute produces no output — the renderer should skip it |

### `PointLabelColorEntry`

| Field | Type | Notes |
|---|---|---|
| `value` | `string` | A `pointLabel` value — matches what the algorithm emits in the compute result |
| `color` | `string \| null` | CSS hex string (e.g. `"#16a34a"`) that overrides **Point Label Color** for points with this label value; `null` falls back to **Point Label Color** |
