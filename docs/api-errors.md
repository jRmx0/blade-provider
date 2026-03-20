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
