# Memory Tracking — Integration Guide

Reference implementation: `src/core/algo_bcd`.

---

## Overview

The VirtualAlloc-backed allocator (`dependencies/allocator/allocator.h`) records working-set snapshots on every allocation call. When an algorithm opts in, the compute orchestrator opens a tracking window around the pipeline, collects the sample array and labelled stage markers, then serialises a `performance` object into the JSON response.

---

## Concepts

| Concept | What it is |
|---|---|
| **Sample** | Working-set delta in bytes relative to the baseline, captured after every `va_malloc` / `va_free` / `va_realloc` call while tracking is enabled. |
| **Baseline** | Working-set size snapshot taken just before the compute window opens. All samples are `current_working_set − baseline`. |
| **Stage marker** | A named boundary in the sample stream (`va_tracking_mark("Label")`). Each marker stores the index of the next sample into the sample array so that the UI can annotate the chart. |

---

## Step-by-step integration

### 1. Add the parameter to `core_types.h`

`input_environment_t` already carries a `track_memory_usage` field that is shared by all algorithms:

```c
// src/core/core_types.h  (already present — do not duplicate)
bool track_memory_usage;
```

No change is required here unless your algorithm uses a different environment struct.

---

### 2. Declare the parameter in `<algo>_metadata.c`

Add one entry in the `METADATA_PARAM_SECTION_PERFORMANCE` section. Copy the exact call from BCD:

```c
metadata_add_parameter(
    parameters,
    <next_id>,          // sequential parameter id
    "Track Memory Usage",
    METADATA_PARAM_TYPE_BOOLEAN,
    NULL, 0,
    "true",             // default value
    METADATA_PARAM_SECTION_PERFORMANCE,
    METADATA_APP_HANDLER_NONE,
    0, 0.0, 0, 0.0, NULL
);
```

---

### 3. Parse the parameter in `<algo>_check.c`

Use `bcd_expect_bool_parameter` (or the equivalent helper in your algorithm's check file) and store the result on the environment:

```c
bool track_memory_usage = false;
if (!<algo>_expect_bool_parameter(
        parameters,
        "Track Memory Usage",
        &track_memory_usage,
        result,
        "missing_track_memory_usage",
        "<ALGO> requires a Track Memory Usage parameter.",
        "invalid_track_memory_usage",
        "<ALGO> Track Memory Usage must be a boolean."))
{
    cJSON_Delete(root);
    free_input_environment(environment);
    return false;
}
environment->track_memory_usage = track_memory_usage;
```

---

### 4. Pre-fetch the flag in `<algo>_compute.c` (before any va-backed parse)

The infrastructure parse that reads the flag must not be tracked. Use a standalone `cJSON_Parse` call with the default CRT allocator — **before** any call to `va_tracking_enable`:

```c
/* Pre-extract the "Track Memory Usage" boolean from the request before any
 * allocator or cJSON hook is configured. Uses the default CRT-backed cJSON
 * so this infrastructure parse is never included in the tracked samples.
 * Defaults to false on any parse failure. */
static bool <algo>_prefetch_track_flag(const char *json)
{
    if (json == NULL)
        return false;

    cJSON *root = cJSON_Parse(json);
    if (!cJSON_IsObject(root))
    {
        cJSON_Delete(root);
        return false;
    }

    const cJSON *parameters = cJSON_GetObjectItemCaseSensitive(root, "parameters");
    if (!cJSON_IsObject(parameters))
    {
        cJSON_Delete(root);
        return false;
    }

    const cJSON *value = cJSON_GetObjectItemCaseSensitive(parameters, "Track Memory Usage");
    bool result = false;
    if (cJSON_IsBool(value))
        result = cJSON_IsTrue(value);
    else if (cJSON_IsString(value) && value->valuestring != NULL)
        result = strcmp(value->valuestring, "true") == 0;

    cJSON_Delete(root);
    return result;
}
```

---

### 5. Open and close the tracking window in `<algo>_run_compute`

```c
char *<algo>_run_compute(const char *input_environment_json)
{
    bool track_memory_usage = <algo>_prefetch_track_flag(input_environment_json);

    /* Parse phase — tracking off so env-struct allocations are excluded. */
    va_tracking_disable();
    va_free_tracking_data();

    input_environment_t environment;
    <algo>_check_result_t check_result;
    if (!<algo>_check_request_json(input_environment_json, &environment, &check_result))
    {
        va_tracking_enable();
        return <algo>_create_error_json(check_result.code, check_result.message);
    }

    /* Open tracking window — baseline set after parse so env structs are
     * already resident and not counted in the compute delta. */
    if (track_memory_usage)
    {
        va_free_tracking_data();
        va_free_stage_markers();
        va_tracking_enable();
        va_tracking_set_baseline();
    }

    /* ---- run the compute pipeline ---- */
    cJSON *root = <algo>_process(&environment);
    free_input_environment(&environment);

    /* Close tracking window and inject performance object. */
    if (track_memory_usage)
    {
        long *samples       = va_get_tracking_data();
        size_t count        = va_get_tracking_count();
        long baseline       = va_get_baseline();
        const va_stage_marker_t *markers = va_get_stage_markers();
        size_t marker_count = va_get_stage_marker_count();
        va_tracking_disable();

        /* Only attach performance to successful results. */
        if (root != NULL && cJSON_GetObjectItemCaseSensitive(root, "<success_key>") != NULL)
        {
            cJSON *perf = cJSON_CreateObject();
            cJSON *metrics_arr = cJSON_CreateArray();
            cJSON_AddItemToObject(perf, "metrics", metrics_arr);

            /* id=1 — working-set delta series (KB) */
            cJSON *metric = cJSON_CreateObject();
            cJSON_AddNumberToObject(metric, "id", 1);
            cJSON *val_arr = cJSON_CreateArray();
            for (size_t i = 0; i < count; ++i)
                cJSON_AddItemToArray(val_arr, cJSON_CreateNumber((double)samples[i] / 1024.0));
            cJSON_AddItemToObject(metric, "value", val_arr);

            if (markers != NULL && marker_count > 0)
            {
                cJSON *stages_arr = cJSON_CreateArray();
                for (size_t i = 0; i < marker_count; ++i)
                {
                    cJSON *jstage = cJSON_CreateObject();
                    cJSON_AddStringToObject(jstage, "label", markers[i].label);
                    cJSON_AddNumberToObject(jstage, "sampleIndex", (double)markers[i].sample_index);
                    cJSON_AddItemToArray(stages_arr, jstage);
                }
                cJSON_AddItemToObject(metric, "stages", stages_arr);
            }
            cJSON_AddItemToArray(metrics_arr, metric);

            /* id=2 — baseline working-set floor (KB), single value */
            cJSON *baseline_metric = cJSON_CreateObject();
            cJSON_AddNumberToObject(baseline_metric, "id", 2);
            cJSON_AddNumberToObject(baseline_metric, "value", (double)baseline / 1024.0);
            cJSON_AddItemToArray(metrics_arr, baseline_metric);

            cJSON_AddItemToObject(root, "performance", perf);
        }
        va_free_tracking_data();
        va_free_stage_markers();
    }

    char *out = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return out;
}
```

Replace `<success_key>` with the top-level key that indicates a successful result (BCD uses `"coveragePathPlan"`).

---

### 6. Add stage markers inside the compute runner

Call `va_tracking_mark` at each logical boundary inside the pipeline runner. The label must be a **string literal** (it is not copied — it must outlive the tracking session):

```c
va_tracking_mark("Stage Name");
do_stage_work(...);

va_tracking_mark("Next Stage");
do_next_stage_work(...);
```

Guidelines for stage placement:
- Mark **before** the work that belongs to that stage, not after.
- Place a marker before the serialisation step and another before the cleanup/free step so the working-set drop from `va_free` calls is visible as its own segment.
- Keep labels short and meaningful. BCD uses Lithuanian names; use whatever convention your team settles on.

---

## Output JSON shape

When `"Track Memory Usage": true` and the compute succeeds, the response gains a top-level `performance` object:

```json
{
  "performance": {
    "metrics": [
      {
        "id": 1,
        "value": [0.0, 12.5, 48.0, ...],
        "stages": [
          { "label": "Preprocessing", "sampleIndex": 3 },
          { "label": "Events",        "sampleIndex": 11 }
        ]
      },
      {
        "id": 2,
        "value": 8192.0
      }
    ]
  }
}
```

| Field | Unit | Description |
|---|---|---|
| `metrics[0].value` | KB | Working-set delta series (one entry per allocator call). |
| `metrics[0].stages[i].sampleIndex` | — | Index into `value` where stage `i` begins. |
| `metrics[1].value` | KB | Absolute baseline working-set before the compute window opened. |

---

## Rules and pitfalls

- **Never enable tracking before calling `va_tracking_set_baseline`.**  Without a baseline, all deltas are relative to 0 and the series will be dominated by the OS working-set noise.
- **Always disable tracking before reading samples.**  `va_get_tracking_data()` returns a pointer into the internal cvector; a concurrent `va_malloc` could trigger a realloc that invalidates the pointer.
- **Free tracking data after use.**  Call `va_free_tracking_data()` and `va_free_stage_markers()` even when the result is an error path, to avoid unbounded growth across multiple compute calls in the same process lifetime.
- **Only attach `performance` to successful results.**  If the pipeline returns an error JSON, skip the `cJSON_AddItemToObject(root, "performance", perf)` call.
- **The infrastructure parse must use vanilla cJSON.**  The `<algo>_prefetch_track_flag` function intentionally calls `cJSON_Parse` before `va_tracking_enable` so that memory used for parsing the request is not included in the tracked window.
- **Stage marker labels must be string literals.**  The allocator stores only the pointer; if you pass a stack or heap string the label will dangle after the tracking session ends.
