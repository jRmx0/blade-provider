# blade-provider HTTP API

blade-provider is a local HTTP service that exposes algorithm metadata and runs coverage-path-planning computations. Compute jobs are asynchronous: submitting a request returns a job ID, which the caller polls until the job reaches a terminal state.

**Default base URL:** `http://localhost:8080`  
Controlled by the `PORT` environment variable (`BLADE_PROVIDER_NAME` sets the service display name).

---

## Auth & Headers

| Header | When required | Value |
|---|---|---|
| `Content-Type` | POST requests with a body | `application/json` |
| `Authorization` | When provider has an API key configured | `Bearer <apiKey>` |

All responses carry open CORS headers (`Access-Control-Allow-Origin: *`). Preflight `OPTIONS` requests to any route return `204` with no body.

---

## Endpoints

### `GET /`

Service discovery. Returns the list of exposed endpoints and names of available algorithms.

**Response `200`**
```json
{
  "service": "blade-provider",
  "endpoints": {
    "health": "/health",
    "metadata": "/metadata",
    "compute": "/compute",
    "computeStatus": "/compute/:jobId"
  },
  "algorithms": [
    { "name": "Boustrophedon Cellular Decomposition" }
  ]
}
```

---

### `GET /health`

Liveness check.

**Response `200`**
```json
{
  "status": "ok",
  "service": "blade-provider",
  "timestamp": "2026-03-20T12:00:00.000Z"
}
```

---

### `GET /metadata`

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

#### `DebugLayerMetadata`

| Field | Type | Notes |
|---|---|---|
| `id` | `number` | Sequential identifier for this debug layer |
| `key` | `string` | Matches the field name under `result.debug` in the compute response |
| `name` | `string` | Human-readable display name |
| `type` | `DebugLayerType` | Geometry primitive type of the objects produced by this layer |
| `style` | `DebugLayerStyleAttribute[]` | Default rendering attributes — see [Debug Layer Styles](./debug-layer-styles.md) |
| `label` | `DebugLayerLabel \| null` | Data binding and per-value text color overrides — `null` for layers with no text label |

#### `DebugLayerType`

| Value | Description |
|---|---|
| `"Point"` | Layer items are rendered as individual point markers |
| `"Line"` | Layer items are rendered as line segments or polylines |
| `"Polygon"` | Layer items are rendered as filled or stroked polygon shapes |

#### `DebugLayerStyleAttribute`

`style` is an ordered array of `DebugLayerStyleAttribute` objects. The attribute set depends on the layer's `type`. See [Debug Layer Styles](./debug-layer-styles.md) for the full per-type attribute registry.

| Field | Type | Notes |
|---|---|---|
| `id` | `number` | Attribute identifier, unique within the layer type's attribute family |
| `name` | `string` | Attribute key — matches the family's attribute registry |
| `value` | `string \| null` | Attribute value serialised as a string, or `null` when inactive |

All attributes in a type's registry are always present in the array. `null` means the attribute produces no output — the renderer should skip it.

#### `DebugLayerLabel`

Declares the data field to use as the text label and provides per-value color overrides. Present on layers that carry a text label; `null` on all others.

| Field | Type | Notes |
|---|---|---|
| `key` | `string` | Name of the field on each data item to use as the rendered text value |
| `values` | `DebugLayerLabelValue[]` | All possible values the field can take, in definition order |

#### `DebugLayerLabelValue`

| Field | Type | Notes |
|---|---|---|
| `value` | `string` | The data value — matches what the algorithm emits in the compute result |
| `color` | `string \| null` | Tailwind color token that overrides `pointTextColor` for points with this label value; `null` falls back to `pointTextColor` |

---

### `POST /compute`

Submits a compute job. Returns immediately with a job ID — computation runs asynchronously.

**Request body** — see [Compute Request Payload](#compute-request-payload).

**Response `202`**
```json
{
  "jobId": "550e8400-e29b-41d4-a716-446655440000",
  "status": "queued",
  "createdAt": "2026-03-20T12:00:00.000Z",
  "pollUrl": "http://localhost:8080/compute/550e8400-e29b-41d4-a716-446655440000"
}
```

**Errors**

| Status | `error.code` | Cause |
|---|---|---|
| `400` | `invalid_json` | Request body is not valid JSON |

---

### `GET /compute/:jobId`

Polls the status of a previously submitted job.

**Response `200`** — [`ComputeJobState`](#computejobstate)

**Errors**

| Status | `error.code` | Cause |
|---|---|---|
| `404` | `job_not_found` | No job exists for the given ID |

---

## Compute Request Payload

```json
{
  "algorithmId": 1,
  "requestId": "optional-client-trace-id",
  "environment": {
    "zones": [
      [
        { "x": 0, "y": 0 },
        { "x": 100, "y": 0 },
        { "x": 100, "y": 100 },
        { "x": 0, "y": 100 }
      ]
    ],
    "obstacles": [
      [
        { "x": 20, "y": 20 },
        { "x": 20, "y": 40 },
        { "x": 40, "y": 40 },
        { "x": 40, "y": 20 }
      ]
    ]
  },
  "parameters": {
    "Path Width": 15,
    "Path Overlap": 5,
    "Format": "Polygon",
    "Type": "Off-Line",
    "Coordinate System": "Decimal"
  }
}
```

### Fields

| Field | Type | Required | Notes |
|---|---|---|---|
| `algorithmId` | `number` | yes | Must match `algorithm.id` from `GET /metadata`. The native dispatcher routes by integer ID. |
| `requestId` | `string` | no | Client-supplied trace ID; echoed back in the job state |
| `environment` | `object` | yes | |
| `environment.zones` | `Point[][]` | yes | One or more boundary polygons. Each polygon ≥ 3 vertices. **Clockwise winding** in screen coordinates (Y increases downward). BCD currently requires exactly one zone. |
| `environment.obstacles` | `Point[][]` | no | Zero or more obstacle polygons. Each polygon ≥ 3 vertices. **Counter-clockwise winding** in screen coordinates. |
| `parameters` | `object` | yes | Key = `parameter.name` from metadata. Values must conform to `paramType` (see below). |

`Point` is `{ "x": number, "y": number }`.

### Parameter value types

| `paramType` | Expected JSON type | Notes |
|---|---|---|
| `Integer` | `number` | Whole number |
| `Decimal` | `number` | Floating-point |
| `Boolean` | `boolean` | |
| `Enum` | `string` | Must be one of `enumValues` from metadata |
| `String` | `string` | |

### BCD-specific constraints

| Parameter | Constraint |
|---|---|
| `Path Width` | Must be `> 0` |
| `Path Overlap` | Must be `>= 0` and `< Path Width` |
| `Format` | `"Polygon"` only |
| `Type` | `"Off-Line"` only |
| `Coordinate System` | `"Decimal"` only |

---

## Job Lifecycle

```
POST /compute ──► 202 (jobId)
                      │
                      ▼
                 status: "queued"
                      │
                      ▼
                 status: "running"
                      │
            ┌─────────┴─────────┐
            ▼                   ▼
   status: "completed"   status: "failed"
   result: {...}          error: { code, message }
```

Poll `GET /compute/:jobId` until `status` is `"completed"` or `"failed"`.

### `ComputeJobState`

**Completed job**
```json
{
  "jobId": "550e8400-e29b-41d4-a716-446655440000",
  "status": "completed",
  "algorithmName": "Boustrophedon Cellular Decomposition",
  "createdAt": "2026-03-20T12:00:00.000Z",
  "startedAt": "2026-03-20T12:00:00.050Z",
  "completedAt": "2026-03-20T12:00:01.200Z",
  "requestId": "optional-client-trace-id",
  "result": {
    "coveragePlan": {
      "sections": [
        {
          "coveragePath": [
            { "x": 5, "y": 5 },
            { "x": 95, "y": 5 },
            { "x": 95, "y": 20 },
            { "x": 5, "y": 20 }
          ],
          "transitPath": []
        }
      ]
    },
    "debug": {
      "eventList":      ["..."],
      "cellList":       ["..."],
      "cellVisitOrder": [0]
    }
  }
}
```

**Failed job**
```json
{
  "jobId": "550e8400-e29b-41d4-a716-446655440000",
  "status": "failed",
  "algorithmName": "Boustrophedon Cellular Decomposition",
  "createdAt": "2026-03-20T12:00:00.000Z",
  "startedAt": "2026-03-20T12:00:00.050Z",
  "completedAt": "2026-03-20T12:00:00.060Z",
  "error": {
    "code": "invalid_path_width",
    "message": "Path Width must be a number greater than 0."
  }
}
```

| Field | Type | Notes |
|---|---|---|
| `jobId` | `string` | UUID |
| `status` | `string` | `"queued"` \| `"running"` \| `"completed"` \| `"failed"` |
| `algorithmName` | `string` | Display name of the algorithm |
| `createdAt` | `string` | ISO 8601 |
| `startedAt` | `string?` | ISO 8601; set when execution starts |
| `completedAt` | `string?` | ISO 8601; set on terminal state |
| `requestId` | `string?` | Echoed from request body if provided |
| `result` | `ComputeResult?` | Present when `status === "completed"` |
| `error` | `object?` | Present when `status === "failed"` |

### `ComputeResult`

```json
{
  "coveragePlan": {
    "sections": [
      {
        "coveragePath": [
          { "x": 5, "y": 5 },
          { "x": 95, "y": 5 },
          { "x": 95, "y": 20 },
          { "x": 5, "y": 20 }
        ],
        "transitPath": []
      }
    ]
  },
  "debug": {
    "eventList": [
      {
        // add id
        "polygonType": "BOUNDARY",
        "vertex": { "x": 0, "y": 0 },
        "eventType": "SIDE_IN",
        "floorEdge": { "begin": { "x": 0, "y": 0 }, "end": { "x": 0, "y": 0 } },
        "ceilingEdge": { "begin": { "x": 0, "y": 0 }, "end": { "x": 0, "y": 0 } }
      }
    ],
    "cellList": [
        {
          "cellNumber": 0, // Replace by id
          "ceilingBegin": { "x": 0, "y": 0 },
          "ceilingEnd": { "x": 100, "y": 0 },
          "floorBegin": { "x": 100, "y": 100 },
          "floorEnd": { "x": 0, "y": 100 },
          "ceilingEdges": [
            { "begin": { "x": 0, "y": 0 }, "end": { "x": 100, "y": 0 } }
          ],
          "floorEdges": [
            { "begin": { "x": 100, "y": 100 }, "end": { "x": 0, "y": 100 } }
          ],
          "open": false,
          "visited": true,
          "cleaned": true
        }
    ],
    "cellVisitOrder": [0]
  }
}
```

#### Top-level fields

| Field | Type | Notes |
|---|---|---|
| `coveragePlan` | `CoveragePlan` | Primary output — the computed coverage path |
| `debug` | `BcdDebug` | Internal algorithm data; useful for visualization and debugging |

#### `CoveragePlan`

| Field | Type | Notes |
|---|---|---|
| `sections` | `CoverageSection[]` | One entry per BCD cell visited, in visit order |

#### `CoverageSection`

| Field | Type | Notes |
|---|---|---|
| `coveragePath` | `Point[]` | Ordered boustrophedon waypoints for this cell |
| `transitPath` | `Point[]` | Waypoints from the end of the previous section to the start of this one; currently always `[]` (not yet computed) |

#### `BcdDebug`

| Field | Type | Notes |
|---|---|---|
| `eventList` | `BcdEvent[]` | Sweep-line events produced by the BCD algorithm |
| `cellList` | `BcdCell[]` | Decomposed cells in discovery order |
| `cellVisitOrder` | `number[]` | Cell indices in planned visit order; values are indices into `cellList` |

#### `BcdEvent`

| Field | Type | Notes |
|---|---|---|
| `polygonType` | `"BOUNDARY"` \| `"OBSTACLE"` | Whether the vertex belongs to the boundary or an obstacle |
| `vertex` | `Point` | The polygon vertex that triggered the event |
| `eventType` | `"B_IN"` \| `"B_SIDE_IN"` \| `"B_INIT"` \| `"B_OUT"` \| `"B_SIDE_OUT"` \| `"B_DEINIT"` \| `"IN"` \| `"SIDE_IN"` \| `"OUT"` \| `"SIDE_OUT"` \| `"FLOOR"` \| `"CEILING"` \| `"NONE"` | BCD sweep-line event classification |
| `floorEdge` | `Edge` | Active floor edge at the event; `{begin:{x:0,y:0},end:{x:0,y:0}}` when not applicable |
| `ceilingEdge` | `Edge` | Active ceiling edge at the event; `{begin:{x:0,y:0},end:{x:0,y:0}}` when not applicable |

#### `BcdCell`

| Field | Type | Notes |
|---|---|---|
| `cellNumber` | `number` | Sequential index of the cell |
| `ceilingBegin` | `Point` | Left endpoint of the ceiling span |
| `ceilingEnd` | `Point` | Right endpoint of the ceiling span |
| `floorBegin` | `Point` | Left endpoint of the floor span |
| `floorEnd` | `Point` | Right endpoint of the floor span |
| `ceilingEdges` | `Edge[]` | Polygon edges forming the ceiling boundary of this cell |
| `floorEdges` | `Edge[]` | Polygon edges forming the floor boundary of this cell |
| `open` | `boolean` | `true` while the cell has no closing sweep event yet |
| `visited` | `boolean` | `true` once the path planner has scheduled this cell |
| `cleaned` | `boolean` | `true` once the motion planner has generated coverage for this cell |

#### `Edge`

| Field | Type |
|---|---|
| `begin` | `Point` |
| `end` | `Point` |

---

## HTTP Error Response Shape

All HTTP-layer errors use this structure:

```json
{
  "error": {
    "code": "error_code",
    "message": "Human-readable description.",
    "details": {}
  }
}
```

`details` is only present when additional context is available (e.g. `allowedMethods` on a 405).

---

## Error Codes

### HTTP layer

| Code | HTTP status | Description |
|---|---|---|
| `invalid_json` | 400 | Request body is not valid JSON |
| `not_found` | 404 | Route does not exist |
| `job_not_found` | 404 | No compute job found for the given ID |
| `method_not_allowed` | 405 | HTTP method not allowed for this route |

### Compute job errors (appear in `job.error.code`)

#### Dispatcher

| Code | Description |
|---|---|
| `invalid_request` | Compute request body is missing or not a JSON object |
| `missing_algorithm` | `algorithmId` field is missing or not a number |
| `unknown_algorithm` | `algorithmId` does not match any registered algorithm |
| `internal_error` | Unexpected server-side failure |

#### Boustrophedon Cellular Decomposition

| Code | Description |
|---|---|
| `missing_environment` | `environment` field is missing |
| `missing_parameters` | `parameters` field is missing |
| `invalid_zones` | `environment.zones` is missing or not an array |
| `invalid_zone_count` | `environment.zones` must contain exactly one polygon |
| `invalid_boundary` | Boundary polygon has fewer than 3 valid vertices |
| `invalid_obstacles` | `environment.obstacles` is not an array |
| `invalid_obstacle` | One or more obstacle polygons have fewer than 3 valid vertices |
| `missing_path_width` | `Path Width` parameter is missing |
| `invalid_path_width` | `Path Width` must be a number greater than 0 |
| `missing_path_overlap` | `Path Overlap` parameter is missing |
| `invalid_path_overlap` | `Path Overlap` must be ≥ 0 and < `Path Width` |
| `missing_format` | `Format` parameter is missing |
| `unsupported_format` | `Format` value is not supported (only `"Polygon"`) |
| `missing_type` | `Type` parameter is missing |
| `unsupported_type` | `Type` value is not supported (only `"Off-Line"`) |
| `missing_coordinate_system` | `Coordinate System` parameter is missing |
| `unsupported_coordinate_system` | `Coordinate System` value is not supported (only `"Decimal"`) |
| `not_implemented` | BCD compute path is not yet wired (validation passed, stub only) |
| `allocation_failed` | Memory allocation failure in native core |
