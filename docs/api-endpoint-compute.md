# POST /compute · GET /compute/:jobId

← [Back to API Overview](./api.md)

---

## `POST /compute`

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

## `GET /compute/:jobId`

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

---

## Types

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
    "coveragePathPlan": {
      "segments": ["..."]
    },
    "debug": {
      "layers": ["..."]
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
  "coveragePathPlan": {
    "segments": [
      {
        "id": 0,
        "type": "coverage",
        "path": [
          { "x": 5, "y": 5 },
          { "x": 95, "y": 5 },
          { "x": 95, "y": 20 },
          { "x": 5, "y": 20 }
        ]
      },
      {
        "id": 1,
        "type": "transit",
        "path": [
          { "x": 5, "y": 20 },
          { "x": 5, "y": 5 }
        ]
      },
      {
        "id": 2,
        "type": "coverage",
        "path": [
          { "x": 5, "y": 5 },
          { "x": 95, "y": 5 },
          { "x": 95, "y": 20 },
          { "x": 5, "y": 20 }
        ]
      }
    ]
  },
  "debug": {
    "layers": [
      {
        "id": 1,
        "source": "eventList",
        "list": [
          /*event*/
          {
            "id": 1,
            "pointLabel": "SIDE_IN",
            "point": { "x": 0, "y": 0 },
          }
        ]
      },
      {
        "id": 2,
        "source": "cellList",
        "winding": "clockwise",
        "list": [
          { 
            /*cell*/
            "id": 1,
            "centroidPoint": { "x": 50, "y": 50 },
            "vertices": [
              { "x": 0, "y": 0 },
              { "x": 100, "y": 0 },
              { "x": 100, "y": 100 },
              { "x": 0, "y": 100 }
            ]
          }
        ],
      },
      {
        "id": 3,
        "source": "cellVisitOrder",
        "list": [
          { 
            /*sequence entry*/
            "id": 1, 
            "pointLabel": 1, // cellId 
            "point": { "x": 50, "y": 50 } 
          }
        ]
      }
    ]
  }
}
```

#### Top-level fields

| Field | Type | Notes |
|---|---|---|
| `coveragePathPlan` | `CoveragePathPlan` | Primary output container |
| `debug` | `AlgorithmDebug` | Algorithm-specific debug output — see [`AlgorithmDebug`](#algorithmdebug) |

### `CoveragePathPlan`

| Field | Type | Notes |
|---|---|---|
| `segments` | `CoveragePathPlanSegment[]` | Ordered execution sequence; robot walks it start-to-end |

### `CoveragePathPlanSegment`

| Field | Type | Notes |
|---|---|---|
| `id` | `number` | Sequential index (0-based) |
| `type` | `CoveragePathPlanSegmentType` | Segment role — see [`CoveragePathPlanSegmentType`](#coveragepathplansegmenttype) |
| `path` | `Point[]` | Ordered waypoints, in execution order |

### `CoveragePathPlanSegmentType`

Extensible string enum. Providers must emit only defined values. Consumers must accept unknown values without error and render them neutrally.

| Value | Description |
|---|---|
| `"coverage"` | Active working path — tool engaged |
| `"transit"` | Repositioning path — tool not engaged |

### `AlgorithmDebug`

| Field | Type | Notes |
|---|---|---|
| `layers` | `DebugResultLayer[]` | All debug output layers for this run, in declaration order |

### `DebugResultLayer`

Base shape shared by every debug layer. Algorithm-specific layers may carry additional top-level fields alongside `id`, `source`, and `list`.

| Field | Type | Notes |
|---|---|---|
| `id` | `number` | Matches `layer.id` from `GET /metadata` — use to look up style and display config |
| `source` | `string` | Identifies the layer kind (e.g. `"eventList"`, `"cellList"`) — matches `layer.debugLayer` in metadata |
| `list` | `object[]` | Data items for this layer; shape is algorithm and layer-kind specific |

---

## BCD-Specific Types

The following types describe the `debug.layers` entries when `algorithmName` is `"Boustrophedon Cellular Decomposition"`.

### `source: "eventList"` layer

| Field | Type | Notes |
|---|---|---|
| `id` | `number` | Matches `layer.id` from metadata |
| `source` | `"eventList"` | — |
| `list` | `BcdEvent[]` | Sweep-line events produced by the BCD algorithm |

### `source: "cellList"` layer

| Field | Type | Notes |
|---|---|---|
| `id` | `number` | Matches `layer.id` from metadata |
| `source` | `"cellList"` | — |
| `winding` | `"clockwise"` \| `"counter-clockwise"` | Winding direction shared by all cells in this run — currently always `"clockwise"` |
| `list` | `BcdCell[]` | Decomposed cells in discovery order |

### `source: "cellVisitOrder"` layer

| Field | Type | Notes |
|---|---|---|
| `id` | `number` | Matches `layer.id` from metadata |
| `source` | `"cellVisitOrder"` | — |
| `list` | `CellVisitEntry[]` | Planned cell visit sequence, in visit order |

### `CellVisitEntry`

| Field | Type | Notes |
|---|---|---|
| `id` | `number` | Sequential visit index (0-based); unique across the array — use as the rendering key |
| `pointLabel` | `number` | The `cellId` this visit targets — index into `cellList`. May repeat if the same cell is visited more than once. Used as the canvas point label. Renderers are responsible for offsetting markers when multiple entries share a `pointLabel` |
| `point` | `Point` | Centroid of the cell's four span corners (`ceilingBegin`, `ceilingEnd`, `floorBegin`, `floorEnd`). Always the same value for a given `pointLabel` |

### `BcdEvent`

| Field | Type | Notes |
|---|---|---|
| `id` | `number` | Sequential index of the event |
| `point` | `Point` | The polygon vertex that triggered the event |
| `pointLabel` | `"B_IN"` \| `"B_SIDE_IN"` \| `"B_INIT"` \| `"B_OUT"` \| `"B_SIDE_OUT"` \| `"B_DEINIT"` \| `"IN"` \| `"SIDE_IN"` \| `"OUT"` \| `"SIDE_OUT"` \| `"FLOOR"` \| `"CEILING"` \| `"NONE"` | BCD sweep-line event classification, used as the canvas point label |

### `BcdCell`

| Field | Type | Notes |
|---|---|---|
| `id` | `number` | Sequential index of the cell |
| `vertices` | `Point[]` | The four span corners in order: `ceilingBegin`, `ceilingEnd`, `floorEnd`, `floorBegin` |
| `centroidPoint` | `Point` | Centroid of the four span corners — average of `ceilingBegin`, `ceilingEnd`, `floorBegin`, `floorEnd`. Useful for label placement and hit-testing |
