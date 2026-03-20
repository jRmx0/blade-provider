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

Color values are Tailwind color tokens (e.g. `"purple-500"`, `"blue-400/10"`). Opacity values are integers in the range `"0"`–`"100"`.

### Marker shape

| id | name | Allowed values |
|----|------|----------------|
| 10 | `pointShape` | `"circle"` \| `"square"` \| `"diamond"` \| `"cross"` \| `"triangle"` \| `null` |
| 11 | `pointRadius` | Pixel radius as a string (e.g. `"4"`) \| `null` |

### Border

| id | name | Allowed values |
|----|------|----------------|
| 20 | `pointBorderColor` | Tailwind color token \| `null` (no border) |
| 21 | `pointBorderWidth` | Pixel width as a string (e.g. `"1"`) \| `null` |
| 22 | `pointBorderStyle` | `"solid"` \| `"dashed"` \| `"dotted"` \| `null` |
| 23 | `pointBorderOpacity` | `"0"`–`"100"` \| `null` (full opacity) |

### Fill

| id | name | Allowed values |
|----|------|----------------|
| 30 | `pointFillColor` | Tailwind color token \| `null` (transparent) |
| 31 | `pointFillOpacity` | `"0"`–`"100"` \| `null` (full opacity) |

### Id label

Renders the point's own index within the layer array (e.g. `0`, `1`, `2`…).

| id | name | Allowed values |
|----|------|----------------|
| 40 | `pointIdPlacement` | `"inside"` \| `"outside-left"` \| `"outside-right"` \| `"outside-top"` \| `"outside-bottom"` \| `null` (hidden) |
| 41 | `pointIdColor` | Tailwind color token \| `null` |
| 42 | `pointIdFontSize` | Pixel font size as a string (e.g. `"12"`) \| `null` |
| 43 | `pointIdOffset` | Pixel offset from point center when an outside placement is active, as a string (e.g. `"8"`) \| `null` (renderer default) |

### Text label

Renders a data value carried by the point (e.g. an algorithm-specific annotation). Placement must differ from `pointIdPlacement` when both are active.

`pointTextColor` is the default text label color. When the layer's [`label`](./api.md#debuglayerlabel) declares a non-null `color` for the current text value, that color overrides `pointTextColor` for that point only.

| id | name | Allowed values |
|----|------|----------------|
| 50 | `pointTextPlacement` | `"inside"` \| `"outside-left"` \| `"outside-right"` \| `"outside-top"` \| `"outside-bottom"` \| `null` (hidden) |
| 51 | `pointTextColor` | Tailwind color token \| `null` |
| 52 | `pointTextFontSize` | Pixel font size as a string (e.g. `"12"`) \| `null` |
| 53 | `pointTextOffset` | Pixel offset from point center when an outside placement is active, as a string (e.g. `"8"`) \| `null` (renderer default) |

---

## Line Style Attributes

_Not yet defined. `style` is an empty array for Line layers._

---

## Polygon Style Attributes

_Not yet defined. `style` is an empty array for Polygon layers._
