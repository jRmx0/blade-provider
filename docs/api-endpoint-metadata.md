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
          "appHandler": "environment.format"
        },
        {
          "id": 4,
          "name": "Type",
          "paramType": "Enum",
          "enumValues": ["Off-Line"],
          "defaultValue": "Off-Line",
          "section": "Environment",
          "appHandler": "environment.type"
        },
        {
          "id": 5,
          "name": "Coordinate System",
          "paramType": "Enum",
          "enumValues": ["Decimal"],
          "defaultValue": "Decimal",
          "section": "Environment",
          "appHandler": "environment.coordsystem"
        }
      ],
      "debugLayers": [
        {
          "id": 1,
          "key": "eventList",
          "name": "Event List",
          "type": "Point",
          "style": [
            { "id": 10, "name": "pointShape",        "value": "circle"     },
            { "id": 11, "name": "pointRadius",        "value": "4"          },
            { "id": 20, "name": "pointBorderColor",   "value": "purple-500" },
            { "id": 21, "name": "pointBorderWidth",   "value": "1"          },
            { "id": 22, "name": "pointBorderStyle",   "value": "solid"      },
            { "id": 23, "name": "pointBorderOpacity", "value": null         },
            { "id": 30, "name": "pointFillColor",     "value": "purple-500" },
            { "id": 31, "name": "pointFillOpacity",   "value": null         },
            { "id": 40, "name": "pointIdPlacement",   "value": null         },
            { "id": 41, "name": "pointIdColor",       "value": null         },
            { "id": 42, "name": "pointIdFontSize",    "value": null         },
            { "id": 43, "name": "pointIdOffset",      "value": null         },
            { "id": 50, "name": "pointTextPlacement", "value": "outside-bottom" },
            { "id": 51, "name": "pointTextColor",     "value": "purple-500" },
            { "id": 52, "name": "pointTextFontSize",  "value": "11"         },
            { "id": 53, "name": "pointTextOffset",    "value": "6"          }
          ],
          "label": {
            "key": "eventType",
            "values": [
              { "value": "B_IN",       "color": "green-600"  },
              { "value": "B_SIDE_IN",  "color": "teal-600"   },
              { "value": "B_INIT",     "color": "green-200"  },
              { "value": "B_OUT",      "color": "red-600"    },
              { "value": "B_SIDE_OUT", "color": "pink-600"   },
              { "value": "B_DEINIT",   "color": "red-200"    },
              { "value": "IN",         "color": "green-400"  },
              { "value": "SIDE_IN",    "color": "teal-400"   },
              { "value": "OUT",        "color": "red-400"    },
              { "value": "SIDE_OUT",   "color": "pink-400"   },
              { "value": "FLOOR",      "color": "blue-400"   },
              { "value": "CEILING",    "color": "orange-400" },
              { "value": "NONE",       "color": null         }
            ]
          }
        },
        {
          "id": 2,
          "key": "cellList",
          "name": "Cell List",
          "type": "Polygon",
          "style": [],
          "label": null
        },
        {
          "id": 3,
          "key": "cellVisitOrder",
          "name": "Cell Visit Order",
          "type": "Line",
          "style": [],
          "label": null
        }
      ]
    }
  ]
}
```

`appHandler` values tell the consumer that this parameter is bound to an environment-level property (format, type, coordinate system) rather than being a free-form algorithm input. Consumers should resolve these from the environment state rather than prompting the user separately.

`debugLayers` is present on algorithms that produce debug output as part of their compute result. Each entry declares the default display style for one named debug layer. Consumers should use this style as the initial rendering config but may override it locally.

---

## Types

### `DebugLayerMetadata`

| Field | Type | Notes |
|---|---|---|
| `id` | `number` | Sequential identifier for this debug layer |
| `key` | `string` | Matches the field name under `result.debug` in the compute response |
| `name` | `string` | Human-readable display name |
| `type` | `DebugLayerType` | Geometry primitive type of the objects produced by this layer |
| `style` | `DebugLayerStyleAttribute[]` | Default rendering attributes — see [Debug Layer Styles](./api-debug-layer-styles.md) |
| `label` | `DebugLayerLabel \| null` | Data binding and per-value text color overrides — `null` for layers with no text label |

### `DebugLayerType`

| Value | Description |
|---|---|
| `"Point"` | Layer items are rendered as individual point markers |
| `"Line"` | Layer items are rendered as line segments or polylines |
| `"Polygon"` | Layer items are rendered as filled or stroked polygon shapes |

### `DebugLayerStyleAttribute`

`style` is an ordered array of `DebugLayerStyleAttribute` objects. The attribute set depends on the layer's `type`. See [Debug Layer Styles](./api-debug-layer-styles.md) for the full per-type attribute registry.

| Field | Type | Notes |
|---|---|---|
| `id` | `number` | Attribute identifier, unique within the layer type's attribute family |
| `name` | `string` | Attribute key — matches the family's attribute registry |
| `value` | `string \| null` | Attribute value serialised as a string, or `null` when inactive |

All attributes in a type's registry are always present in the array. `null` means the attribute produces no output — the renderer should skip it.

### `DebugLayerLabel`

Declares the data field to use as the text label and provides per-value color overrides. Present on layers that carry a text label; `null` on all others.

| Field | Type | Notes |
|---|---|---|
| `key` | `string` | Name of the field on each data item to use as the rendered text value |
| `values` | `DebugLayerLabelValue[]` | All possible values the field can take, in definition order |

### `DebugLayerLabelValue`

| Field | Type | Notes |
|---|---|---|
| `value` | `string` | The data value — matches what the algorithm emits in the compute result |
| `color` | `string \| null` | Tailwind color token that overrides `pointTextColor` for points with this label value; `null` falls back to `pointTextColor` |
