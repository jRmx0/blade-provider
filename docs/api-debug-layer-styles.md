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

Applies to layers with `type: "Point"`. All 16 attributes are always emitted.

Color values are Tailwind color tokens; opacity is expressed inline via slash notation (e.g. `"purple-500"`, `"purple-500/50"`). Font size values are Tailwind text size classes (e.g. `"text-xs"`, `"text-sm"`, `"text-base"`). Spatial values (radius, width, offset) are Tailwind spacing tokens (e.g. `"1"`, `"2"`, `"4"`).

### Marker Shape

| id | name | Allowed values |
|----|------|----------------|
| 10 | Point Shape | `"circle"` \| `"square"` \| `"diamond"` \| `"cross"` \| `"triangle"` \| `null` |
| 11 | Point Radius | Tailwind spacing token (e.g. `"4"`) \| `null` |

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

_Not yet defined. `style` is an empty array for Line layers._

---

## Polygon Style Attributes

_Not yet defined. `style` is an empty array for Polygon layers._
