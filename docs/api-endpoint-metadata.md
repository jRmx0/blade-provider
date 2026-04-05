# GET /metadata

← [Back to API Overview](./api.md)

Returns all algorithms the provider exposes, with their parameter schemas. Consumers should fetch this once on connection and use it to build parameter input forms.

**Response `200`** *(abbreviated — actual algorithms, parameters, and style values depend on the provider)*
```json
{
  "algorithms": [
    {
      "id": 1,
      "name": "<algorithm name>",
      "parameters": [
        {
          "id": 1,
          "name": "<parameter name>",
          "paramType": "Decimal",
          "enumValues": [],
          "defaultValue": "15",
          "minValue": 0,
          "section": "<section label>",
          "appHandler": null
        },
        {
          "id": 2,
          "name": "<env-bound parameter>",
          "paramType": "Enum",
          "enumValues": ["<value>"],
          "defaultValue": "<value>",
          "section": "<section label>",
          "appHandler": "env.<property>"
        },
        "..."
      ],
      "layers": [
        {
          "id": 1,
          "computeLayer": "<resultFieldName>",
          "name": "<layer display name>",
          "layerType": "Line",
          "style": {
            "generalStyleAttributes": [
              { "key": "Visible", "styleType": "Boolean", "defaultValue": "true" },
              { "key": "Z-Index", "styleType": "Integer", "defaultValue": "100" }
            ],
            "pointStyleAttributes": [ "..." ],
            "lineStyleAttributes": [ "..." ]
          }
        },
        {
          "id": 2,
          "computeLayer": "<debugLayerName>",
          "name": "<debug layer display name>",
          "layerType": "Point",
          "pointLabelEnumValues": ["<label>", "..."],
          "style": {
            "generalStyleAttributes": [ "..." ],
            "pointStyleAttributes": [ "..." ],
            "pointLabelColorMapping": [
              { "value": "<label>", "color": "#rrggbb" },
              "..."
            ]
          }
        },
        "..."
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
| `minValue` | `number \| null` | Minimum allowed numeric value. `null` means no lower bound. Only meaningful for `Integer` and `Decimal` parameters; ignored for all other types |
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
| `generalStyleAttributes` | `LayerStyleAttribute[]` | Attributes that apply to all layer types (e.g. Z-Index) |
| `pointStyleAttributes` | `LayerStyleAttribute[]?` | Point marker attributes — present on `Point`, `Line`, and `Polygon` layers. On `Line` layers these are the point-vertex attributes; on `Polygon` layers these are the corner-vertex attributes (Overlap and Text Label groups excluded) |
| `lineStyleAttributes` | `LayerStyleAttribute[]?` | Line stroke and arrow attributes — present on `Line` layers |
| `polygonStyleAttributes` | `LayerStyleAttribute[]?` | Polygon fill and stroke attributes — present on `Polygon` layers |
| `pointLabelColorMapping` | `PointLabelColorEntry[]?` | Per-value color overrides for the `pointLabel` field. Present only on layers that carry `pointLabelEnumValues`. Always targets the `pointLabel` field on each compute item |

### `LayerStyleAttribute`

Each entry in an attribute family array.

| Field | Type | Notes |
|---|---|---|
| `key` | `string` | Attribute key — matches the entries in the attribute registry in [Layer Styles](./api-layer-styles.md) |
| `styleType` | `StyleType` | The value category of this attribute — see [Layer Styles StyleType](./api-layer-styles.md#styletype) |
| `defaultValue` | `string \| null` | Default attribute value serialised as a string, or `null` when inactive. `null` means the attribute produces no output — the renderer should skip it |

### `PointLabelColorEntry`

| Field | Type | Notes |
|---|---|---|
| `value` | `string` | A `pointLabel` value — matches what the algorithm emits in the compute result |
| `color` | `string \| null` | CSS hex string (e.g. `"#16a34a"`) that overrides **Point Label Color** for points with this label value; `null` falls back to **Point Label Color** |
