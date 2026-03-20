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

### `CoveragePlan`

| Field | Type | Notes |
|---|---|---|
| `sections` | `CoverageSection[]` | One entry per BCD cell visited, in visit order |

### `CoverageSection`

| Field | Type | Notes |
|---|---|---|
| `coveragePath` | `Point[]` | Ordered boustrophedon waypoints for this cell |
| `transitPath` | `Point[]` | Waypoints from the end of the previous section to the start of this one; currently always `[]` (not yet computed) |

### `BcdDebug`

| Field | Type | Notes |
|---|---|---|
| `eventList` | `BcdEvent[]` | Sweep-line events produced by the BCD algorithm |
| `cellList` | `BcdCell[]` | Decomposed cells in discovery order |
| `cellVisitOrder` | `number[]` | Cell indices in planned visit order; values are indices into `cellList` |

### `BcdEvent`

| Field | Type | Notes |
|---|---|---|
| `polygonType` | `"BOUNDARY"` \| `"OBSTACLE"` | Whether the vertex belongs to the boundary or an obstacle |
| `vertex` | `Point` | The polygon vertex that triggered the event |
| `eventType` | `"B_IN"` \| `"B_SIDE_IN"` \| `"B_INIT"` \| `"B_OUT"` \| `"B_SIDE_OUT"` \| `"B_DEINIT"` \| `"IN"` \| `"SIDE_IN"` \| `"OUT"` \| `"SIDE_OUT"` \| `"FLOOR"` \| `"CEILING"` \| `"NONE"` | BCD sweep-line event classification |
| `floorEdge` | `Edge` | Active floor edge at the event; `{begin:{x:0,y:0},end:{x:0,y:0}}` when not applicable |
| `ceilingEdge` | `Edge` | Active ceiling edge at the event; `{begin:{x:0,y:0},end:{x:0,y:0}}` when not applicable |

### `BcdCell`

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

### `Edge`

| Field | Type |
|---|---|
| `begin` | `Point` |
| `end` | `Point` |
