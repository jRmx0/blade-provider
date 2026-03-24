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
          "cppLayer": "segments",
          "name": "Coverage Path",
          "type": "Line",
          "style": [
            { "id": 5,  "name": "Z-Index",                  "value": "100"        },
            { "id": 10, "name": "Point Shape",              "value":  null              },
            { "id": 11, "name": "Point Radius",              "value": null              },
            { "id": 12, "name": "Point Overlap Spacing",     "value": null              },
            { "id": 13, "name": "Point Overlap Layout",      "value": null              },
            { "id": 20, "name": "Point Border Color",        "value": null              },
            { "id": 21, "name": "Point Border Width",        "value": null              },
            { "id": 22, "name": "Point Border Style",        "value": null              },
            { "id": 30, "name": "Point Fill Color",          "value": null              },
            { "id": 40, "name": "Point ID Color",            "value": null              },
            { "id": 41, "name": "Point ID Font Size",        "value": null              },
            { "id": 42, "name": "Point ID Font Weight",      "value": null              },
            { "id": 43, "name": "Point ID Placement",        "value": null              },
            { "id": 44, "name": "Point ID Offset",           "value": null              },
            { "id": 50, "name": "Point Label Placement",     "value": null              },
            { "id": 51, "name": "Point Label Color",         "value": null              },
            { "id": 52, "name": "Point Label Font Size",     "value": null              },
            { "id": 53, "name": "Point Label Font Weight",   "value": null              },
            { "id": 54, "name": "Point Label Offset",        "value": null              },
            { "id": 60, "name": "Line Edge Color",           "value": "#60a5fa"         },
            { "id": 61, "name": "Line Edge Width",           "value": "1"               },
            { "id": 62, "name": "Line Edge Style",           "value": "solid"           },
            { "id": 70, "name": "Line Arrow Start",          "value": null              },
            { "id": 71, "name": "Line Arrow End",            "value": null              },
            { "id": 72, "name": "Line Arrow Mid",            "value": "line_end_arrow"  },
            { "id": 73, "name": "Line Arrow Mid Spacing",    "value": "12"              },
            { "id": 74, "name": "Line Arrow Size",           "value": "3"               }
          ],
          "label": null
        },
        {
          "id": 10,
          "debugLayer": "eventList",
          "name": "Event List",
          "type": "Point",
          "style": [
            { "id": 5,  "name": "Z-Index",                "value": "110"        },
            { "id": 10, "name": "Point Shape",              "value": "circle"     },
            { "id": 11, "name": "Point Radius",              "value": "4"          },
            { "id": 12, "name": "Point Overlap Spacing",     "value": null         },
            { "id": 13, "name": "Point Overlap Layout",      "value": null         },
            { "id": 20, "name": "Point Border Color",        "value": "#a855f7"   },
            { "id": 21, "name": "Point Border Width",    "value": "1"          },
            { "id": 22, "name": "Point Border Style",    "value": "solid"      },
            { "id": 30, "name": "Point Fill Color",      "value": "#a855f7"   },
            { "id": 40, "name": "Point ID Color",        "value": null         },
            { "id": 41, "name": "Point ID Font Size",    "value": null         },
            { "id": 42, "name": "Point ID Font Weight",  "value": null         },
            { "id": 43, "name": "Point ID Placement",    "value": null         },
            { "id": 44, "name": "Point ID Offset",       "value": null         },
            { "id": 50, "name": "Point Label Placement", "value": "outside-bottom" },
            { "id": 51, "name": "Point Label Color",     "value": "#a855f7"    },
            { "id": 52, "name": "Point Label Font Size", "value": "text-xs"       },
            { "id": 53, "name": "Point Label Font Weight","value": "font-medium"  },
            { "id": 54, "name": "Point Label Offset",    "value": "6"             }
          ],
          "label": {
            "key": "pointLabel",
            "enumValues": [
              { "value": "B_IN",       "color": "#16a34a"   },
              { "value": "B_SIDE_IN",  "color": "#0d9488"   },
              { "value": "B_INIT",     "color": "#bbf7d0"   },
              { "value": "B_OUT",      "color": "#dc2626"   },
              { "value": "B_SIDE_OUT", "color": "#db2777"   },
              { "value": "B_DEINIT",   "color": "#fecaca"   },
              { "value": "IN",         "color": "#4ade80"   },
              { "value": "SIDE_IN",    "color": "#2dd4bf"   },
              { "value": "OUT",        "color": "#f87171"   },
              { "value": "SIDE_OUT",   "color": "#f472b6"   },
              { "value": "FLOOR",      "color": "#60a5fa"   },
              { "value": "CEILING",    "color": "#fb923c"   },
              { "value": "NONE",       "color": null        }
            ]
          }
        },
        {
          "id": 11,
          "debugLayer": "cellList",
          "name": "Cell List",
          "type": "Polygon",
          "style": [
            { "id": 5,  "name": "Z-Index",                "value": "120"        },
            { "id": 10, "name": "Point Shape",              "value": null        },
            { "id": 11, "name": "Point Radius",              "value": null        },
            { "id": 20, "name": "Point Border Color",        "value": null        },
            { "id": 21, "name": "Point Border Width",     "value": null        },
            { "id": 22, "name": "Point Border Style",     "value": null        },
            { "id": 30, "name": "Point Fill Color",       "value": null        },
            { "id": 40, "name": "Point ID Color",         "value": null        },
            { "id": 41, "name": "Point ID Font Size",     "value": null        },
            { "id": 42, "name": "Point ID Font Weight",   "value": null        },
            { "id": 43, "name": "Point ID Placement",     "value": null        },
            { "id": 44, "name": "Point ID Offset",        "value": null        },
            { "id": 60, "name": "Polygon Edge Color",     "value": "#94a3b8"  },
            { "id": 61, "name": "Polygon Edge Width",     "value": "1"         },
            { "id": 62, "name": "Polygon Edge Style",     "value": "solid"     },
            { "id": 80, "name": "Polygon Fill Color",     "value": null        },
            { "id": 81, "name": "Polygon Fill Style",     "value": null        },
            { "id": 82, "name": "Polygon ID Color",       "value": "#64748b"   },
            { "id": 83, "name": "Polygon ID Font Size",   "value": "text-xs"   },
            { "id": 84, "name": "Polygon ID Font Weight", "value": "font-medium"},
            { "id": 85, "name": "Polygon ID Shape",       "value": null        },
            { "id": 86, "name": "Polygon ID Radius",      "value": null        },
            { "id": 87, "name": "Polygon ID Border Color","value": null        },
            { "id": 88, "name": "Polygon ID Border Width", "value": null        },
            { "id": 89, "name": "Polygon ID Border Style", "value": null        },
            { "id": 90, "name": "Polygon ID Fill Color",  "value": null        },
            { "id": 91, "name": "Polygon ID Placement",   "value": "inside"    },
            { "id": 92, "name": "Polygon ID Offset",      "value": null        }
          ],
          "label": null
        },
        {
          "id": 12,
          "debugLayer": "cellVisitOrder",
          "name": "Cell Visit Order",
          "type": "Line",
          "style": [
            { "id": 5,  "name": "Z-Index",                "value": "130"        },
            { "id": 10, "name": "Point Shape",              "value": "circle"           },
            { "id": 11, "name": "Point Radius",              "value": "3"               },
            { "id": 12, "name": "Point Overlap Spacing",     "value": "8"               },
            { "id": 13, "name": "Point Overlap Layout",      "value": "grid"            },
            { "id": 20, "name": "Point Border Color",        "value": null              },
            { "id": 21, "name": "Point Border Width",        "value": null              },
            { "id": 22, "name": "Point Border Style",        "value": null              },
            { "id": 30, "name": "Point Fill Color",          "value": "#60a5fa"         },
            { "id": 40, "name": "Point ID Color",            "value": "#60a5fa"         },
            { "id": 41, "name": "Point ID Font Size",        "value": "text-xs"         },
            { "id": 42, "name": "Point ID Font Weight",      "value": "font-medium"     },
            { "id": 43, "name": "Point ID Placement",        "value": "inside"          },
            { "id": 44, "name": "Point ID Offset",           "value": null              },
            { "id": 50, "name": "Point Label Placement",     "value": null              },
            { "id": 51, "name": "Point Label Color",         "value": null              },
            { "id": 52, "name": "Point Label Font Size",     "value": null              },
            { "id": 53, "name": "Point Label Font Weight",   "value": null              },
            { "id": 54, "name": "Point Label Offset",        "value": null              },
            { "id": 60, "name": "Line Edge Color",           "value": "#60a5fa"         },
            { "id": 61, "name": "Line Edge Width",           "value": "1"               },
            { "id": 62, "name": "Line Edge Style",           "value": "solid"           },
            { "id": 70, "name": "Line Arrow Start",          "value": null              },
            { "id": 71, "name": "Line Arrow End",            "value": null              },
            { "id": 72, "name": "Line Arrow Mid",            "value": "line_end_arrow"  },
            { "id": 73, "name": "Line Arrow Mid Spacing",    "value": "12"              },
            { "id": 74, "name": "Line Arrow Size",           "value": "3"               }
          ],
          "label": null
        }
      ]
    }
  ]
}
```

`appHandler` values tell the consumer that this parameter is bound to an environment-level property (format, type, coordinate system) rather than being a free-form algorithm input. Consumers should resolve these from the environment state rather than prompting the user separately.

`layers` declares all renderable output layers for an algorithm. Coverage layers carry a `cppLayer` field pointing to the field name in `result.coveragePathPlan`; debug layers carry a `debugLayer` field matching the `source` value in `result.debug.layers`. Consumers should use the `style` as the initial rendering config but may override it locally.

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
| `cppLayer` | `string?` | Present on coverage path plan layers. Names the field in `result.coveragePathPlan` that holds the data (e.g. `"segments"`) |
| `debugLayer` | `string?` | Present on debug layers. Matches the `source` value on the corresponding entry in `result.debug.layers` |
| `name` | `string` | Human-readable display name |
| `type` | `LayerType` | Geometry primitive type of the objects produced by this layer |
| `style` | `LayerStyleAttribute[]` | Default rendering attributes — see [Layer Styles](./api-layer-styles.md) |
| `label` | `LayerLabel \| null` | Data binding and per-value text color overrides — `null` for layers with no text label |

Exactly one of `cppLayer` or `debugLayer` is present on any given layer entry.

### `LayerType`

| Value | Description |
|---|---|
| `"Point"` | Layer items are rendered as individual point markers |
| `"Line"` | Layer items are rendered as line segments or polylines |
| `"Polygon"` | Layer items are rendered as filled or stroked polygon shapes |

### `LayerStyleAttribute`

`style` is an ordered array of `LayerStyleAttribute` objects. The attribute set depends on the layer's `type`. See [Layer Styles](./api-layer-styles.md) for the full per-type attribute registry.

| Field | Type | Notes |
|---|---|---|
| `id` | `number` | Attribute identifier, unique within the layer type's attribute family |
| `name` | `string` | Human-readable display name for the attribute; use `id` for programmatic identification |
| `value` | `string \| null` | Attribute value serialised as a string, or `null` when inactive |

All attributes in a type's registry are always present in the array. `null` means the attribute produces no output — the renderer should skip it.

### `LayerLabel`

Declares the data field to use as the text label and provides per-value color overrides. Present on layers that carry a text label; `null` on all others.

| Field | Type | Notes |
|---|---|---|
| `key` | `string` | Name of the field on each data item to use as the rendered text value |
| `enumValues` | `LayerLabelValue[]` | All possible values the field can take, in definition order |

### `LayerLabelValue`

| Field | Type | Notes |
|---|---|---|
| `value` | `string` | The data value — matches what the algorithm emits in the compute result |
| `color` | `string \| null` | CSS hex string with optional alpha (e.g. `"#16a34a"`, `"#16a34acc"`) that overrides **Point Label Color** for points with this label value; `null` falls back to **Point Label Color** |
