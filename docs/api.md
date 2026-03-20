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
          "style": 
          { 
            "className": "text-purple-500 stroke-purple-500", 
            "pointRadius": 4 
          } 
        },
        { 
          "id": 2,
          "key": "cellList",
          "name": "Cell List",
          "style": 
          { 
            "className": "text-blue-400 stroke-blue-400 fill-blue-400/10" 
          } 
        },
        { 
          "id": 3,
          "key": "cellVisitOrder",
          "name": "Cell Visit Order",
          "style": 
          { 
            "className": "text-yellow-300", 
            "fontSize": 12 
          } 
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
| `style` | `DebugLayerStyle` | Default rendering style for this layer |

#### `DebugLayerStyle`

| Field | Type | Notes |
|---|---|---|
| `className` | `string` | Tailwind utility classes for color, stroke, fill, and opacity |
| `pointRadius` | `number?` | Point marker radius in pixels; present on point-rendered layers |
| `fontSize` | `number?` | Label font size in pixels; present on text-rendered layers |

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
        "polygonType": "BOUNDARY",
        "vertex": { "x": 0, "y": 0 },
        "eventType": "SIDE_IN",
        "floorEdge": { "begin": { "x": 0, "y": 0 }, "end": { "x": 0, "y": 0 } },
        "ceilingEdge": { "begin": { "x": 0, "y": 0 }, "end": { "x": 0, "y": 0 } }
      }
    ],
    "cellList": [
        {
          "cellNumber": 0,
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
| `eventType` | `"SIDE_IN"` \| `"CEILING"` \| `"SIDE_OUT"` | BCD sweep-line event classification |
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
