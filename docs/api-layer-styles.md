# blade-provider Layer Styles

Style attributes are the **visual layer** of the layer metadata returned by `GET /metadata`. They are separate from the algorithm data — they tell consumers how to render each layer by default.

Consumers should use these as the initial rendering configuration but may override individual attributes locally.

---

## `LayerStyleAttribute`

Each element of an attribute family sub-array within a [`LayerStyle`](./api-endpoint-metadata.md#layerstyle) object is a `LayerStyleAttribute`:

| Field | Type | Notes |
|---|---|---|
| `key` | `string` | Attribute key — matches the entries in the family's attribute registry below |
| `styleType` | `StyleType` | The value category of this attribute — see [`StyleType`](#styletype) below |
| `defaultValue` | `string \| null` | Default attribute value serialised as a string, or `null` when the attribute is inactive |

`defaultValue` is always a string regardless of the attribute's logical type (e.g. radius `4` is serialised as `"4"`). A `null` value means the attribute produces no visual output — the renderer should skip it.

All attributes in a type's registry are **always** present in the sub-array. No attribute is omitted even when inactive.

---

## StyleType

The `styleType` field identifies the value category of a `LayerStyleAttribute`. Consumers use this to determine how to parse, validate, and render each attribute's `defaultValue` string.

### Scalar types

| StyleType | Description |
|---|---|
| `"Integer"` | Positive whole number serialised as a string (e.g. `"100"`) |
| `"Color"` | CSS hex string with optional 8-digit alpha (e.g. `"#a855f7"`, `"#a855f7cc"`) |
| `"Pixels"` | Direct pixel value serialised as a string (e.g. `"1"`, `"4"`, `"8"`) |

### Enum types

| StyleType | Allowed values |
|---|---|
| `"PointShapeEnum"` | `"circle"` \| `"square"` \| `"diamond"` \| `"cross"` \| `"triangle"` |
| `"PolygonIDShapeEnum"` | `"circle"` \| `"square"` |
| `"StrokeStyleEnum"` | `"solid"` \| `"dashed"` \| `"dotted"` |
| `"FillStyleEnum"` | `"solid"` \| `"hatched"` |
| `"OverlapLayoutEnum"` | `"grid"` |
| `"PlacementEnum"` | `"inside"` \| `"outside-left"` \| `"outside-right"` \| `"outside-top"` \| `"outside-bottom"` |
| `"FontWeightEnum"` | `"font-thin"` \| `"font-extralight"` \| `"font-light"` \| `"font-normal"` \| `"font-medium"` \| `"font-semibold"` \| `"font-bold"` \| `"font-extrabold"` \| `"font-black"` |
| `"LineArrowStartEnum"` | `"line_start_arrow"` \| `"line_start_arrow_notch"` |
| `"LineArrowEndEnum"` | `"line_end_arrow"` \| `"line_end_arrow_notch"` |
| `"LineArrowMidEnum"` | `"line_start_arrow"` \| `"line_start_arrow_notch"` \| `"line_end_arrow"` \| `"line_end_arrow_notch"` |

---

## General Style Attributes

Applies to all layer types. Always the first sub-array in the `LayerStyle` object.

| name | styleType | Allowed values |
|------|-----------|----------------|
| Visible | `"Boolean"` | `"true"` \| `"false"` — whether the layer is visible |
| Z-Index | `"Integer"` | Positive integer serialised as a string (e.g. `"100"`, `"110"`) — lower values are drawn first (bottom) |

---

## Point Style Attributes

Canonical definition of point marker attributes. All 18 attributes are always present in `pointStyleAttributes`. On `Line` layers the same 18 attributes reappear as point-vertex attributes; on `Polygon` layers 11 of them reappear as corner-vertex attributes (Overlap and Text Label groups excluded) — see the respective sections below.

All color values (border, fill, text ID, text label) are CSS hex strings with optional 8-digit alpha (e.g. `"#a855f7"`, `"#a855f7cc"`). Font size values are direct pixel values serialised as a string (e.g. `"12"`, `"14"`, `"16"`). Spatial values (radius, width, offset) are direct pixel values (e.g. `"1"`, `"2"`, `"4"`).

### Marker Shape

| name | styleType | Allowed values |
|------|-----------|----------------|
| Point Shape | `"PointShapeEnum"` | `"circle"` \| `"square"` \| `"diamond"` \| `"cross"` \| `"triangle"` \| `null` |
| Point Radius | `"Pixels"` | Direct pixel value (e.g. `"4"`) \| `null` |

### Overlap

Controls how the renderer spreads apart multiple points that share the same canvas position (e.g. multiple visit entries on the same cell centroid).

| name | styleType | Allowed values |
|------|-----------|----------------|
| Point Overlap Spacing | `"Pixels"` | Pixel distance between stacked points (e.g. `"8"`) \| `null` (renderer default) |
| Point Overlap Layout | `"OverlapLayoutEnum"` | `"grid"` \| `null` (no spread) |

### Border

| name | styleType | Allowed values |
|------|-----------|----------------|
| Point Border Color | `"Color"` | CSS hex string with optional alpha (e.g. `"#a855f7"`, `"#a855f7cc"`) \| `null` (no border) |
| Point Border Width | `"Pixels"` | Direct pixel value (e.g. `"1"`) \| `null` |
| Point Border Style | `"StrokeStyleEnum"` | `"solid"` \| `"dashed"` \| `"dotted"` \| `null` |

### Fill

| name | styleType | Allowed values |
|------|-----------|----------------|
| Point Fill Color | `"Color"` | CSS hex string with optional alpha (e.g. `"#a855f7"`, `"#a855f7cc"`) \| `null` (transparent) |

### Id Label

Renders the point's own index within the layer array (e.g. `1`, `2`, `3`…).

| name | styleType | Allowed values |
|------|-----------|----------------|
| Point ID Color | `"Color"` | CSS hex string with optional alpha (e.g. `"#60a5fa"`, `"#60a5fa80"`) \| `null` |
| Point ID Font Size | `"FontSizeEnum"` | Tailwind text size class (e.g. `"text-xs"`, `"text-sm"`, `"text-base"`) \| `null` |
| Point ID Font Weight | `"FontWeightEnum"` | Tailwind font weight class (e.g. `"font-normal"`, `"font-medium"`, `"font-bold"`) \| `null` |
| Point ID Placement | `"PlacementEnum"` | `"inside"` \| `"outside-left"` \| `"outside-right"` \| `"outside-top"` \| `"outside-bottom"` \| `null` (hidden) |
| Point ID Offset | `"Pixels"` | Pixel offset from point center when an outside placement is active (e.g. `"2"`) \| `null` (renderer default) |

### Text Label

Renders a data value carried by the point (e.g. an algorithm-specific annotation). Placement must differ from **Point ID Placement** when both are active.

**Point Label Color** is the default text label color. When the layer's [`pointLabelColorMapping`](./api-endpoint-metadata.md#pointlabelcolorentry) contains a non-null `color` for the current `pointLabel` value, that color overrides **Point Label Color** for that point only.

| name | styleType | Allowed values |
|------|-----------|----------------|
| Point Label Color | `"Color"` | CSS hex string with optional alpha (e.g. `"#a855f7"`, `"#a855f780"`) \| `null` |
| Point Label Font Size | `"FontSizeEnum"` | Tailwind text size class (e.g. `"text-xs"`, `"text-sm"`, `"text-base"`) \| `null` |
| Point Label Font Weight | `"FontWeightEnum"` | Tailwind font weight class (e.g. `"font-normal"`, `"font-medium"`, `"font-bold"`) \| `null` |
| Point Label Placement | `"PlacementEnum"` | `"inside"` \| `"outside-left"` \| `"outside-right"` \| `"outside-top"` \| `"outside-bottom"` \| `null` (hidden) |
| Point Label Offset | `"Pixels"` | Pixel offset from point center when an outside placement is active (e.g. `"2"`) \| `null` (renderer default) |

---

## Line Style Attributes

Applies to layers with `layerType: "Line"`. All 26 attributes are always present: 18 in `pointStyleAttributes` (point-vertex group) and 8 in `lineStyleAttributes` (Edge and Arrow groups).

A Line is composed of **point-vertices** (the dots at each waypoint) and **edges** (the segments connecting them). The point-vertex attribute group reuses the same names and semantics as the [Point Style Attributes](#point-style-attributes) section above.

### Point Vertex

Point-vertex attributes are identical to the [Point Style Attributes](#point-style-attributes) section above. The same 18 attributes, names, and allowed values apply.

### Edge

| name | styleType | Allowed values |
|------|-----------|----------------|
| Line Edge Color | `"Color"` | CSS hex string with optional alpha (e.g. `"#60a5fa"`, `"#60a5fa80"`) \| `null` |
| Line Edge Width | `"Pixels"` | Direct pixel value (e.g. `"1"`) \| `null` |
| Line Edge Style | `"StrokeStyleEnum"` | `"solid"` \| `"dashed"` \| `"dotted"` \| `null` |

### Arrow

Arrow shape values are [Google Material Icon](https://fonts.google.com/icons?icon.query=line) names serialised in kebab-case.

| name | styleType | Allowed values |
|------|-----------|----------------|
| Line Arrow Start | `"LineArrowStartEnum"` | `"line_start_arrow"` \| `"line_start_arrow_notch"` \| `null` (no start arrow) |
| Line Arrow End | `"LineArrowEndEnum"` | `"line_end_arrow"` \| `"line_end_arrow_notch"` \| `null` (no end arrow) |
| Line Arrow Mid | `"LineArrowMidEnum"` | `"line_start_arrow"` \| `"line_start_arrow_notch"` \| `"line_end_arrow"` \| `"line_end_arrow_notch"` \| `null` (no mid arrows) |
| Line Arrow Mid Spacing | `"Pixels"` | Pixel interval between mid-arrows \| `null` |
| Line Arrow Size | `"Pixels"` | Pixel arrowhead size \| `null` |

**Mid-arrow count and placement** (applies when `Line Arrow Mid` and `Line Arrow Mid Spacing` are both non-null):

Let `L` = rendered edge length, `S` = `Line Arrow Mid Spacing` value.

Count: `n = max(1, floor(L / S) - 1)`

Position of arrow `i` along the edge: `pos(i) = L/2 + (i − (n−1)/2) × S` for `i = 0 … n−1`

The group is always centered on the edge midpoint. When `n ≥ 2` the minimum-distance rule is enforced — first and last mid-arrow are at least `S` from each terminus. When `n = 1` a single arrow is placed at the midpoint and the minimum-distance rule does not apply.

---

## Polygon Style Attributes

Applies to layers with `layerType: "Polygon"`. All 27 attributes are always present: 11 in `pointStyleAttributes` (corner-vertex group, Overlap and Text Label excluded) and 16 in `polygonStyleAttributes` (Edge, Fill, and Polygon ID groups).

A Polygon is composed of **corner vertices** (the points at each boundary corner), a **boundary edge** (the closed stroke outline), and an optional **fill**. The corner-vertex attribute group reuses the same names and semantics as the [Point Style Attributes](#point-style-attributes) section above, **excluding the Overlap group and the Text Label group** which polygons do not carry.

### Corner Vertex

Corner-vertex attributes are identical to the [Point Style Attributes](#point-style-attributes) section above — the same 11 attributes, names, and allowed values apply (Marker Shape, Border, Fill, Id Label). The Overlap and Text Label groups are not part of the Polygon attribute set.

### Polygon Edge

| name | styleType | Allowed values |
|------|-----------|----------------|
| Polygon Edge Color | `"Color"` | CSS hex string with optional alpha (e.g. `"#94a3b8"`, `"#94a3b880"`) \| `null` |
| Polygon Edge Width | `"Pixels"` | Direct pixel value (e.g. `"1"`) \| `null` |
| Polygon Edge Style | `"StrokeStyleEnum"` | `"solid"` \| `"dashed"` \| `"dotted"` \| `null` |

### Polygon Fill

| name | styleType | Allowed values |
|------|-----------|----------------|
| Polygon Fill Color | `"Color"` | CSS hex string with optional alpha (e.g. `"#60a5fa"`, `"#60a5fa33"`) \| `null` (transparent) |
| Polygon Fill Style | `"FillStyleEnum"` | `"solid"` \| `"hatched"` \| `null` |

`"hatched"` renders the fill as a 45° diagonal stripe pattern. When **Polygon Fill Color** is non-null and **Polygon Fill Style** is `null`, the renderer defaults to `"solid"`.

### Polygon ID

Renders the polygon's own index within the layer array (e.g. `1`, `2`, `3`…) at the polygon centroid.

| name | styleType | Allowed values |
|------|-----------|----------------|
| Polygon ID Color | `"Color"` | CSS hex string with optional alpha (e.g. `"#64748b"`, `"#64748b80"`) \| `null` |
| Polygon ID Font Size | `"FontSizeEnum"` | Tailwind text size class (e.g. `"text-xs"`, `"text-sm"`, `"text-base"`) \| `null` |
| Polygon ID Font Weight | `"FontWeightEnum"` | Tailwind font weight class (e.g. `"font-normal"`, `"font-medium"`, `"font-bold"`) \| `null` |
| Polygon ID Shape | `"PolygonIDShapeEnum"` | `"circle"` \| `"square"` \| `null` (no marker background) |
| Polygon ID Radius | `"Pixels"` | Pixel marker size (e.g. `"4"`, `"6"`) \| `null` |
| Polygon ID Border Color | `"Color"` | CSS hex string with optional alpha (e.g. `"#94a3b8"`, `"#94a3b880"`) \| `null` |
| Polygon ID Border Width | `"Pixels"` | Direct pixel value (e.g. `"1"`) \| `null` |
| Polygon ID Border Style | `"StrokeStyleEnum"` | `"solid"` \| `"dashed"` \| `"dotted"` \| `null` |
| Polygon ID Fill Color | `"Color"` | CSS hex string with optional alpha (e.g. `"#60a5fa"`, `"#60a5fa33"`) \| `null` |
| Polygon ID Placement | `"PlacementEnum"` | `"inside"` \| `"outside-left"` \| `"outside-right"` \| `"outside-top"` \| `"outside-bottom"` \| `null` (hidden) |
| Polygon ID Offset | `"Pixels"` | Pixel offset from centroid when an outside placement is active (e.g. `"2"`) \| `null` (renderer default) |
