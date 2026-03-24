# blade-provider Layer Styles

Style attributes are the **visual layer** of the layer metadata returned by `GET /metadata`. They are separate from the algorithm data — they tell consumers how to render each layer by default.

Consumers should use these as the initial rendering configuration but may override individual attributes locally.

---

## `LayerStyleAttribute`

Each element of the `style` array on a `LayerMetadata` object is a `LayerStyleAttribute`:

| Field | Type | Notes |
|---|---|---|
| `id` | `number` | Attribute identifier, unique within the layer type's attribute family |
| `name` | `string` | Attribute key — matches the entries in the family's attribute registry below |
| `styleType` | `StyleType` | The value category of this attribute — see [`StyleType`](#styletype) below |
| `value` | `string \| null` | Attribute value serialised as a string, or `null` when the attribute is inactive |

`value` is always a string regardless of the attribute's logical type (e.g. radius `4` is serialised as `"4"`). A `null` value means the attribute produces no visual output — the renderer should skip it.

All attributes in a type's registry are **always** present in the array. No attribute is omitted even when inactive.

---

## StyleType

The `styleType` field identifies the value category of a `LayerStyleAttribute`. Consumers use this to determine how to parse, validate, and render each attribute's `value` string.

### Scalar types

| StyleType | Description |
|---|---|
| `"Integer"` | Positive whole number serialised as a string (e.g. `"100"`) |
| `"Color"` | CSS hex string with optional 8-digit alpha (e.g. `"#a855f7"`, `"#a855f7cc"`) |
| `"Spacing"` | Tailwind spacing token (e.g. `"1"`, `"4"`, `"8"`) |

### Enum types

| StyleType | Allowed values |
|---|---|
| `"PointShapeEnum"` | `"circle"` \| `"square"` \| `"diamond"` \| `"cross"` \| `"triangle"` |
| `"PolygonIDShapeEnum"` | `"circle"` \| `"square"` |
| `"StrokeStyleEnum"` | `"solid"` \| `"dashed"` \| `"dotted"` |
| `"FillStyleEnum"` | `"solid"` \| `"hatched"` |
| `"OverlapLayoutEnum"` | `"grid"` |
| `"PlacementEnum"` | `"inside"` \| `"outside-left"` \| `"outside-right"` \| `"outside-top"` \| `"outside-bottom"` |
| `"FontSizeEnum"` | `"text-xs"` \| `"text-sm"` \| `"text-base"` \| `"text-lg"` \| `"text-xl"` \| `"text-2xl"` \| `"text-3xl"` \| `"text-4xl"` \| `"text-5xl"` \| `"text-6xl"` \| `"text-7xl"` \| `"text-8xl"` \| `"text-9xl"` |
| `"FontWeightEnum"` | `"font-thin"` \| `"font-extralight"` \| `"font-light"` \| `"font-normal"` \| `"font-medium"` \| `"font-semibold"` \| `"font-bold"` \| `"font-extrabold"` \| `"font-black"` |
| `"LineArrowStartEnum"` | `"line_start_arrow"` \| `"line_start_arrow_notch"` |
| `"LineArrowEndEnum"` | `"line_end_arrow"` \| `"line_end_arrow_notch"` |
| `"LineArrowMidEnum"` | `"line_start_arrow"` \| `"line_start_arrow_notch"` \| `"line_end_arrow"` \| `"line_end_arrow_notch"` |

---

## Universal Style Attributes

Applies to all layer types. Always emitted as the first element in the `style` array.

| id | name | styleType | Allowed values |
|----|------|-----------|----------------|
| 5 | Z-Index | `"Integer"` | Positive integer serialised as a string (e.g. `"100"`, `"110"`) — lower values are drawn first (bottom) |

---

## Point Style Attributes

Applies to layers with `layerType: "Point"`. All 18 attributes are always emitted.

All color values (border, fill, text ID, text label) are CSS hex strings with optional 8-digit alpha (e.g. `"#a855f7"`, `"#a855f7cc"`). Font size values are Tailwind text size classes (e.g. `"text-xs"`, `"text-sm"`, `"text-base"`). Spatial values (radius, width, offset) are Tailwind spacing tokens (e.g. `"1"`, `"2"`, `"4"`).

### Marker Shape

| id | name | styleType | Allowed values |
|----|------|-----------|----------------|
| 10 | Point Shape | `"PointShapeEnum"` | `"circle"` \| `"square"` \| `"diamond"` \| `"cross"` \| `"triangle"` \| `null` |
| 11 | Point Radius | `"Spacing"` | Tailwind spacing token (e.g. `"4"`) \| `null` |

### Overlap

Controls how the renderer spreads apart multiple points that share the same canvas position (e.g. multiple visit entries on the same cell centroid).

| id | name | styleType | Allowed values |
|----|------|-----------|----------------|
| 12 | Point Overlap Spacing | `"Spacing"` | Tailwind spacing token — distance between stacked points (e.g. `"8"`) \| `null` (renderer default) |
| 13 | Point Overlap Layout | `"OverlapLayoutEnum"` | `"grid"` \| `null` (no spread) |

### Border

| id | name | styleType | Allowed values |
|----|------|-----------|----------------|
| 20 | Point Border Color | `"Color"` | CSS hex string with optional alpha (e.g. `"#a855f7"`, `"#a855f7cc"`) \| `null` (no border) |
| 21 | Point Border Width | `"Spacing"` | Tailwind spacing token (e.g. `"1"`) \| `null` |
| 22 | Point Border Style | `"StrokeStyleEnum"` | `"solid"` \| `"dashed"` \| `"dotted"` \| `null` |

### Fill

| id | name | styleType | Allowed values |
|----|------|-----------|----------------|
| 30 | Point Fill Color | `"Color"` | CSS hex string with optional alpha (e.g. `"#a855f7"`, `"#a855f7cc"`) \| `null` (transparent) |

### Id Label

Renders the point's own index within the layer array (e.g. `1`, `2`, `3`…).

| id | name | styleType | Allowed values |
|----|------|-----------|----------------|
| 40 | Point ID Color | `"Color"` | CSS hex string with optional alpha (e.g. `"#60a5fa"`, `"#60a5fa80"`) \| `null` |
| 41 | Point ID Font Size | `"FontSizeEnum"` | Tailwind text size class (e.g. `"text-xs"`, `"text-sm"`, `"text-base"`) \| `null` |
| 42 | Point ID Font Weight | `"FontWeightEnum"` | Tailwind font weight class (e.g. `"font-normal"`, `"font-medium"`, `"font-bold"`) \| `null` |
| 43 | Point ID Placement | `"PlacementEnum"` | `"inside"` \| `"outside-left"` \| `"outside-right"` \| `"outside-top"` \| `"outside-bottom"` \| `null` (hidden) |
| 44 | Point ID Offset | `"Spacing"` | Tailwind spacing token for the offset from point center when an outside placement is active (e.g. `"2"`) \| `null` (renderer default) |

### Text Label

Renders a data value carried by the point (e.g. an algorithm-specific annotation). Placement must differ from **Point ID Placement** when both are active.

**Point Label Color** is the default text label color. When the layer's [`label`](./api-endpoint-metadata.md#layerlabel) declares a non-null `color` for the current text value, that color overrides **Point Label Color** for that point only.

| id | name | styleType | Allowed values |
|----|------|-----------|----------------|
| 50 | Point Label Placement | `"PlacementEnum"` | `"inside"` \| `"outside-left"` \| `"outside-right"` \| `"outside-top"` \| `"outside-bottom"` \| `null` (hidden) |
| 51 | Point Label Color | `"Color"` | CSS hex string with optional alpha (e.g. `"#a855f7"`, `"#a855f780"`) \| `null` |
| 52 | Point Label Font Size | `"FontSizeEnum"` | Tailwind text size class (e.g. `"text-xs"`, `"text-sm"`, `"text-base"`) \| `null` |
| 53 | Point Label Font Weight | `"FontWeightEnum"` | Tailwind font weight class (e.g. `"font-normal"`, `"font-medium"`, `"font-bold"`) \| `null` |
| 54 | Point Label Offset | `"Spacing"` | Tailwind spacing token for the offset from point center when an outside placement is active (e.g. `"2"`) \| `null` (renderer default) |

---

## Line Style Attributes

Applies to layers with `layerType: "Line"`. All 26 attributes are always emitted.

A Line is composed of **point-vertices** (the dots at each waypoint) and **edges** (the segments connecting them). The point-vertex attribute group reuses the same IDs and names as the [Point Style Attributes](#point-style-attributes) section above — the allowed values and semantics are identical.

### Point Vertex

Point-vertex attributes (IDs 10–54) are identical to the [Point Style Attributes](#point-style-attributes) section above. The same 18 attributes, IDs, names, and allowed values apply.

### Edge

| id | name | styleType | Allowed values |
|----|------|-----------|----------------|
| 60 | Line Edge Color | `"Color"` | CSS hex string with optional alpha (e.g. `"#60a5fa"`, `"#60a5fa80"`) \| `null` |
| 61 | Line Edge Width | `"Spacing"` | Tailwind spacing token (e.g. `"1"`) \| `null` |
| 62 | Line Edge Style | `"StrokeStyleEnum"` | `"solid"` \| `"dashed"` \| `"dotted"` \| `null` |

### Arrow

Arrow shape values are [Google Material Icon](https://fonts.google.com/icons?icon.query=line) names serialised in kebab-case.

| id | name | styleType | Allowed values |
|----|------|-----------|----------------|
| 70 | Line Arrow Start | `"LineArrowStartEnum"` | `"line_start_arrow"` \| `"line_start_arrow_notch"` \| `null` (no start arrow) |
| 71 | Line Arrow End | `"LineArrowEndEnum"` | `"line_end_arrow"` \| `"line_end_arrow_notch"` \| `null` (no end arrow) |
| 72 | Line Arrow Mid | `"LineArrowMidEnum"` | `"line_start_arrow"` \| `"line_start_arrow_notch"` \| `"line_end_arrow"` \| `"line_end_arrow_notch"` \| `null` (no mid arrows) |
| 73 | Line Arrow Mid Spacing | `"Spacing"` | Tailwind spacing token — interval between mid-arrows \| `null` |
| 74 | Line Arrow Size | `"Spacing"` | Tailwind spacing token — arrowhead size \| `null` |

**Mid-arrow count and placement** (applies when `Line Arrow Mid` and `Line Arrow Mid Spacing` are both non-null):

Let `L` = rendered edge length, `S` = `Line Arrow Mid Spacing` value.

Count: `n = max(1, floor(L / S) - 1)`

Position of arrow `i` along the edge: `pos(i) = L/2 + (i − (n−1)/2) × S` for `i = 0 … n−1`

The group is always centered on the edge midpoint. When `n ≥ 2` the minimum-distance rule is enforced — first and last mid-arrow are at least `S` from each terminus. When `n = 1` a single arrow is placed at the midpoint and the minimum-distance rule does not apply.

---

## Polygon Style Attributes

Applies to layers with `layerType: "Polygon"`. All 27 attributes are always emitted.

A Polygon is composed of **corner vertices** (the points at each boundary corner), a **boundary edge** (the closed stroke outline), and an optional **fill**. The corner-vertex attribute group reuses the same IDs and names as the [Point Style Attributes](#point-style-attributes) section above, **excluding the Overlap group (IDs 12–13) and the Text Label group (IDs 50–54)** which polygons do not carry.

### Corner Vertex

Corner-vertex attributes (IDs 10–44, excluding 12–13) are identical to the [Point Style Attributes](#point-style-attributes) section above — the same 11 attributes, IDs, names, and allowed values apply. The Overlap group (IDs 12–13) and Text Label group (IDs 50–54) are not part of the Polygon attribute set.

### Polygon Edge

| id | name | styleType | Allowed values |
|----|------|-----------|----------------|
| 60 | Polygon Edge Color | `"Color"` | CSS hex string with optional alpha (e.g. `"#94a3b8"`, `"#94a3b880"`) \| `null` |
| 61 | Polygon Edge Width | `"Spacing"` | Tailwind spacing token (e.g. `"1"`) \| `null` |
| 62 | Polygon Edge Style | `"StrokeStyleEnum"` | `"solid"` \| `"dashed"` \| `"dotted"` \| `null` |

### Polygon Fill

| id | name | styleType | Allowed values |
|----|------|-----------|----------------|
| 80 | Polygon Fill Color | `"Color"` | CSS hex string with optional alpha (e.g. `"#60a5fa"`, `"#60a5fa33"`) \| `null` (transparent) |
| 81 | Polygon Fill Style | `"FillStyleEnum"` | `"solid"` \| `"hatched"` \| `null` |

`"hatched"` renders the fill as a 45° diagonal stripe pattern. When **Polygon Fill Color** is non-null and **Polygon Fill Style** is `null`, the renderer defaults to `"solid"`.

### Polygon ID

Renders the polygon's own index within the layer array (e.g. `1`, `2`, `3`…) at the polygon centroid.

| id | name | styleType | Allowed values |
|----|------|-----------|----------------|
| 82 | Polygon ID Color | `"Color"` | CSS hex string with optional alpha (e.g. `"#64748b"`, `"#64748b80"`) \| `null` |
| 83 | Polygon ID Font Size | `"FontSizeEnum"` | Tailwind text size class (e.g. `"text-xs"`, `"text-sm"`, `"text-base"`) \| `null` |
| 84 | Polygon ID Font Weight | `"FontWeightEnum"` | Tailwind font weight class (e.g. `"font-normal"`, `"font-medium"`, `"font-bold"`) \| `null` |
| 85 | Polygon ID Shape | `"PolygonIDShapeEnum"` | `"circle"` \| `"square"` \| `null` (no marker background) |
| 86 | Polygon ID Radius | `"Spacing"` | Tailwind spacing token for marker size (e.g. `"4"`, `"6"`) \| `null` |
| 87 | Polygon ID Border Color | `"Color"` | CSS hex string with optional alpha (e.g. `"#94a3b8"`, `"#94a3b880"`) \| `null` |
| 88 | Polygon ID Border Width | `"Spacing"` | Tailwind spacing token (e.g. `"1"`) \| `null` |
| 89 | Polygon ID Border Style | `"StrokeStyleEnum"` | `"solid"` \| `"dashed"` \| `"dotted"` \| `null` |
| 90 | Polygon ID Fill Color | `"Color"` | CSS hex string with optional alpha (e.g. `"#60a5fa"`, `"#60a5fa33"`) \| `null` |
| 91 | Polygon ID Placement | `"PlacementEnum"` | `"inside"` \| `"outside-left"` \| `"outside-right"` \| `"outside-top"` \| `"outside-bottom"` \| `null` (hidden) |
| 92 | Polygon ID Offset | `"Spacing"` | Tailwind spacing token for offset from centroid when an outside placement is active (e.g. `"2"`) \| `null` (renderer default) |
