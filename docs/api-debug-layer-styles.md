# blade-provider Debug Layer Styles

Style attributes are the **visual layer** of the debug layer metadata returned by `GET /metadata`. They are separate from the algorithm data — they tell consumers how to render each debug layer by default.

Consumers should use these as the initial rendering configuration but may override individual attributes locally.

---

## `DebugLayerStyleAttribute`

Each element of the `style` array on a `DebugLayerMetadata` object is a `DebugLayerStyleAttribute`:

| Field | Type | Notes |
|---|---|---|
| `id` | `number` | Attribute identifier, unique within the layer type's attribute family |
| `name` | `string` | Attribute key — matches the entries in the family's attribute registry below |
| `value` | `string \| null` | Attribute value serialised as a string, or `null` when the attribute is inactive |

`value` is always a string regardless of the attribute's logical type (e.g. radius `4` is serialised as `"4"`). A `null` value means the attribute produces no visual output — the renderer should skip it.

All attributes in a type's registry are **always** present in the array. No attribute is omitted even when inactive.

---

## Point Style Attributes

Applies to layers with `type: "Point"`. All 18 attributes are always emitted.

Color values are Tailwind color tokens; opacity is expressed inline via slash notation (e.g. `"purple-500"`, `"purple-500/50"`). Font size values are Tailwind text size classes (e.g. `"text-xs"`, `"text-sm"`, `"text-base"`). Spatial values (radius, width, offset) are Tailwind spacing tokens (e.g. `"1"`, `"2"`, `"4"`).

### Marker Shape

| id | name | Allowed values |
|----|------|----------------|
| 10 | Point Shape | `"circle"` \| `"square"` \| `"diamond"` \| `"cross"` \| `"triangle"` \| `null` |
| 11 | Point Radius | Tailwind spacing token (e.g. `"4"`) \| `null` |

### Overlap

Controls how the renderer spreads apart multiple points that share the same canvas position (e.g. multiple visit entries on the same cell centroid).

| id | name | Allowed values |
|----|------|----------------|
| 12 | Point Overlap Spacing | Tailwind spacing token — distance between stacked points (e.g. `"8"`) \| `null` (renderer default) |
| 13 | Point Overlap Layout | `"grid"` \| `null` (no spread) |

### Border

| id | name | Allowed values |
|----|------|----------------|
| 20 | Point Border Color | Tailwind color token with optional opacity (e.g. `"purple-500"`, `"purple-500/50"`) \| `null` (no border) |
| 21 | Point Border Width | Tailwind spacing token (e.g. `"1"`) \| `null` |
| 22 | Point Border Style | `"solid"` \| `"dashed"` \| `"dotted"` \| `null` |

### Fill

| id | name | Allowed values |
|----|------|----------------|
| 30 | Point Fill Color | Tailwind color token with optional opacity (e.g. `"purple-500"`, `"purple-500/50"`) \| `null` (transparent) |

### Id Label

Renders the point's own index within the layer array (e.g. `0`, `1`, `2`…).

| id | name | Allowed values |
|----|------|----------------|
| 40 | Point ID Placement | `"inside"` \| `"outside-left"` \| `"outside-right"` \| `"outside-top"` \| `"outside-bottom"` \| `null` (hidden) |
| 41 | Point ID Color | Tailwind color token \| `null` |
| 42 | Point ID Font Size | Tailwind text size class (e.g. `"text-xs"`, `"text-sm"`, `"text-base"`) \| `null` |
| 43 | Point ID Font Weight | Tailwind font weight class (e.g. `"font-normal"`, `"font-medium"`, `"font-bold"`) \| `null` |
| 44 | Point ID Offset | Tailwind spacing token for the offset from point center when an outside placement is active (e.g. `"2"`) \| `null` (renderer default) |

### Text Label

Renders a data value carried by the point (e.g. an algorithm-specific annotation). Placement must differ from **Point ID Placement** when both are active.

**Point Label Color** is the default text label color. When the layer's [`label`](./api.md#debuglayerlabel) declares a non-null `color` for the current text value, that color overrides **Point Label Color** for that point only.

| id | name | Allowed values |
|----|------|----------------|
| 50 | Point Label Placement | `"inside"` \| `"outside-left"` \| `"outside-right"` \| `"outside-top"` \| `"outside-bottom"` \| `null` (hidden) |
| 51 | Point Label Color | Tailwind color token \| `null` |
| 52 | Point Label Font Size | Tailwind text size class (e.g. `"text-xs"`, `"text-sm"`, `"text-base"`) \| `null` |
| 53 | Point Label Font Weight | Tailwind font weight class (e.g. `"font-normal"`, `"font-medium"`, `"font-bold"`) \| `null` |
| 54 | Point Label Offset | Tailwind spacing token for the offset from point center when an outside placement is active (e.g. `"2"`) \| `null` (renderer default) |

---

## Line Style Attributes

Applies to layers with `type: "Line"`. All 26 attributes are always emitted.

A Line is composed of **point-vertices** (the dots at each waypoint) and **edges** (the segments connecting them). The point-vertex attribute group reuses the same IDs and names as the [Point Style Attributes](#point-style-attributes) section above — the allowed values and semantics are identical.

### Point Vertex

Point-vertex attributes (IDs 10–54) are identical to the [Point Style Attributes](#point-style-attributes) section above. The same 18 attributes, IDs, names, and allowed values apply.

### Edge

| id | name | Allowed values |
|----|------|----------------|
| 60 | Line Edge Color | Tailwind color token with optional opacity (e.g. `"blue-400"`, `"blue-400/50"`) \| `null` |
| 61 | Line Edge Width | Tailwind spacing token (e.g. `"1"`) \| `null` |
| 62 | Line Edge Style | `"solid"` \| `"dashed"` \| `"dotted"` \| `null` |

### Arrow

Arrow shape values are [Google Material Icon](https://fonts.google.com/icons?icon.query=line) names serialised in kebab-case.

| id | name | Allowed values |
|----|------|----------------|
| 70 | Line Arrow Start | `"line_start_arrow"` \| `"line_start_arrow_notch"` \| `null` (no start arrow) |
| 71 | Line Arrow End | `"line_end_arrow"` \| `"line_end_arrow_notch"` \| `null` (no end arrow) |
| 72 | Line Arrow Mid | `"line_end_arrow"` \| `"line_end_arrow_notch"` \| `null` (no mid arrows) |
| 73 | Line Arrow Mid Spacing | Tailwind spacing token — interval between mid-arrows \| `null` |
| 74 | Line Arrow Size | Tailwind spacing token — arrowhead size \| `null` |

**Mid-arrow count and placement** (applies when `Line Arrow Mid` and `Line Arrow Mid Spacing` are both non-null):

Let `L` = rendered edge length, `S` = `Line Arrow Mid Spacing` value.

Count: `n = max(1, floor(L / S) - 1)`

Position of arrow `i` along the edge: `pos(i) = L/2 + (i − (n−1)/2) × S` for `i = 0 … n−1`

The group is always centered on the edge midpoint. When `n ≥ 2` the minimum-distance rule is enforced — first and last mid-arrow are at least `S` from each terminus. When `n = 1` a single arrow is placed at the midpoint and the minimum-distance rule does not apply.

---

## Polygon Style Attributes

Applies to layers with `type: "Polygon"`. All 19 attributes are always emitted.

A Polygon is composed of **corner vertices** (the points at each boundary corner), a **boundary edge** (the closed stroke outline), and an optional **fill**. The corner-vertex attribute group reuses the same IDs and names as the [Point Style Attributes](#point-style-attributes) section above, **excluding the Overlap group (IDs 12–13) and the Text Label group (IDs 50–54)** which polygons do not carry.

### Corner Vertex

Corner-vertex attributes (IDs 10–44, excluding 12–13) are identical to the [Point Style Attributes](#point-style-attributes) section above — the same 11 attributes, IDs, names, and allowed values apply. The Overlap group (IDs 12–13) and Text Label group (IDs 50–54) are not part of the Polygon attribute set.

### Polygon Edge

| id | name | Allowed values |
|----|------|----------------|
| 60 | Polygon Edge Color | Tailwind color token with optional opacity (e.g. `"slate-400"`, `"slate-400/50"`) \| `null` |
| 61 | Polygon Edge Width | Tailwind spacing token (e.g. `"1"`) \| `null` |
| 62 | Polygon Edge Style | `"solid"` \| `"dashed"` \| `"dotted"` \| `null` |

### Polygon Fill

| id | name | Allowed values |
|----|------|----------------|
| 80 | Polygon Fill Color | Tailwind color token with optional slash opacity (e.g. `"blue-400/20"`) \| `null` (transparent) |
| 81 | Polygon Fill Style | `"solid"` \| `"hatched"` \| `null` |

`"hatched"` renders the fill as a 45° diagonal stripe pattern. When **Polygon Fill Color** is non-null and **Polygon Fill Style** is `null`, the renderer defaults to `"solid"`.

### Polygon ID

Renders the polygon's own index within the layer array (e.g. `0`, `1`, `2`…) at the polygon centroid.

| id | name | Allowed values |
|----|------|----------------|
| 82 | Polygon ID Color | Tailwind color token \| `null` |
| 83 | Polygon ID Font Size | Tailwind text size class (e.g. `"text-xs"`, `"text-sm"`, `"text-base"`) \| `null` |
| 84 | Polygon ID Font Weight | Tailwind font weight class (e.g. `"font-normal"`, `"font-medium"`, `"font-bold"`) \| `null` |
