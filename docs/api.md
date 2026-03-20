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

| Method | Path | Description | Reference |
|---|---|---|---|
| `GET` | `/` | Service discovery | below |
| `GET` | `/health` | Liveness check | below |
| `GET` | `/metadata` | Algorithm metadata & parameter schemas | [api-endpoint-metadata.md](./api-endpoint-metadata.md) |
| `POST` | `/compute` | Submit a compute job | [api-endpoint-compute.md](./api-endpoint-compute.md) |
| `GET` | `/compute/:jobId` | Poll a compute job | [api-endpoint-compute.md](./api-endpoint-compute.md) |

For error response shape and all error codes see [api-errors.md](./api-errors.md).  
For debug layer style attributes see [api-debug-layer-styles.md](./api-debug-layer-styles.md).

---

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
