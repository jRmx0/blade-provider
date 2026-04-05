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
        { "x": 40, "y": 20 },
        { "x": 40, "y": 40 },
        { "x": 20, "y": 40 }
      ]
    ]
  },
  "parameters": {
    "<parameter name>": "<value>",
    "...": "..."
  }
}
```

### Fields

| Field | Type | Required | Notes |
|---|---|---|---|
| `algorithmId` | `number` | yes | Must match `algorithm.id` from `GET /metadata`. The native dispatcher routes by integer ID. |
| `requestId` | `string` | no | Client-supplied trace ID; echoed back in the job state |
| `environment` | `object` | yes | |
| `environment.zones` | `Point[][]` | yes | One or more boundary polygons. Each polygon ≥ 3 vertices. **Clockwise winding** in screen coordinates (Y increases downward). Exact zone count constraints are algorithm-specific — check the algorithm's error codes. |
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

Parameter constraints (allowed ranges, required enum values, etc.) are algorithm-specific. They are validated by the algorithm's native core and reported as job-level error codes when violated — see [api-errors.md](./api-errors.md).

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
  "algorithmName": "<algorithm name>",
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
  "algorithmName": "<algorithm name>",
  "createdAt": "2026-03-20T12:00:00.000Z",
  "startedAt": "2026-03-20T12:00:00.050Z",
  "completedAt": "2026-03-20T12:00:00.060Z",
  "error": {
    "code": "<algorithm-specific error code>",
    "message": "<human-readable description>"
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
        "id": 1,
        "type": "coverage",
        "path": [
          { "id": 1, "point": { "x": 5, "y": 5 } },
          { "id": 2, "point": { "x": 95, "y": 5 } },
          "..."
        ]
      },
      {
        "id": 2,
        "type": "transit",
        "path": [ "..." ]
      },
      "..."
    ]
  },
  "debug": {
    "layers": [
      {
        "id": 1,
        "source": "<debugLayerName>",
        "list": [ "..." ]
      },
      "..."
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
| `id` | `number` | Sequential index (1-based) |
| `type` | `CoveragePathPlanSegmentType` | Segment role — see [`CoveragePathPlanSegmentType`](#coveragepathplansegmenttype) |
| `path` | `IndexedPoint[]` | Ordered waypoints, in execution order |

### `IndexedPoint`

| Field | Type | Notes |
|---|---|---|
| `id` | `number` | Sequential index (1-based) within the segment |
| `point` | `Point` | Waypoint coordinates |

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
| `source` | `string` | Identifies the layer kind — matches `layer.computeLayer` in metadata |
| `list` | `object[]` | Data items for this layer; shape is algorithm and layer-kind specific |

---

## Algorithm-Specific Types

Each algorithm defines its own `debug.layers` item shapes. The exact fields in each `list` entry depend on which algorithm ran and which debug layer the `source` identifies. Consult the specific algorithm's documentation for details.

The generic base fields (`id`, `source`, `list`) are described by [`DebugResultLayer`](#debugresultlayer) above. Algorithm implementations may add further top-level fields alongside these.

