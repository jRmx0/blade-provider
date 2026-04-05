# HTTP Errors

← [Back to API Overview](./api.md)

---

## Error Response Shape

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

#### Algorithm-specific errors (example)

Each algorithm validates its own request and emits unique error codes. The following is an example from one algorithm — actual codes depend on which algorithm ran. Check the `algorithmName` field on the failed job to identify the relevant algorithm.

| Code | Description |
|---|---|
| `missing_environment` | `environment` field is missing |
| `missing_parameters` | `parameters` field is missing |
| `invalid_zones` | `environment.zones` is missing or not an array |
| `invalid_zone_count` | Zone count does not meet algorithm requirements |
| `invalid_boundary` | Boundary polygon has fewer than 3 valid vertices |
| `invalid_obstacles` | `environment.obstacles` is not an array |
| `invalid_obstacle` | One or more obstacle polygons have fewer than 3 valid vertices |
| `missing_<paramName>` | A required parameter is absent |
| `invalid_<paramName>` | A parameter value fails the algorithm's constraint |
| `unsupported_<paramName>` | A parameter value is not in the allowed set for this algorithm |
| `not_implemented` | Compute path is not yet wired (validation passed, stub only) |
| `allocation_failed` | Memory allocation failure in native core |
