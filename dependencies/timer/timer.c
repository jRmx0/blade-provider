/**
 * @file timer.c
 * @brief QueryPerformanceCounter-backed timer implementation.
 *
 * Internal state is stored in module-level static variables.  The tracking
 * vector is backed by cvector using the standard CRT heap (malloc / free);
 * timer.h does not define any cvector_clib_* overrides, so this translation
 * unit receives the CRT defaults when cvector.h is included below.  This
 * deliberately keeps the timer independent of the VirtualAlloc-backed
 * allocator (allocator.h) so that both libraries can be used side-by-side
 * without circular dependencies.
 *
 * Unlike the allocator's record_op(), the internal record_sample() helper
 * here requires no reentrancy guard because cvector_push_back expands via
 * CRT realloc, which never calls back into any timer function.
 */
#include "timer.h"
#include "../cvector/cvector.h"

/* --------------------------------------------------------------------------
 * Constants
 * -------------------------------------------------------------------------- */

/** Maximum number of stage markers that can be stored in a single session. */
#define TM_TRACKING_MAX_MARKERS 32

/* --------------------------------------------------------------------------
 * Static state
 * -------------------------------------------------------------------------- */

/** Baseline counter set by tm_start(). */
static LARGE_INTEGER g_start = {0};

/** QPC frequency in counts-per-second, populated by tm_start(). */
static LARGE_INTEGER g_freq = {0};

/** Non-zero after the first successful tm_start() call. */
static int g_started = 0;

/** Active time unit; default is TM_UNIT_MS. */
static tm_unit_t g_unit = TM_UNIT_MS;

/** Non-zero when sample recording is enabled (default on). */
static int g_tracking_enabled = 1;

/** Dynamic array of recorded elapsed-time samples. */
static cvector(double) g_tracking_vec = NULL;

/** Fixed-size array of stage markers. */
static tm_stage_marker_t g_markers[TM_TRACKING_MAX_MARKERS];

/** Number of stage markers recorded so far. */
static size_t g_marker_count = 0;

/** Non-zero when the last marker in g_markers is still open (no duration yet). */
static int g_last_marker_open = 0;

/* --------------------------------------------------------------------------
 * Internal helpers
 * -------------------------------------------------------------------------- */

/**
 * @brief Convert a raw QPC tick delta to the active time unit.
 *
 * @param ticks Difference between two QueryPerformanceCounter readings.
 * @return Elapsed time expressed in g_unit, or 0.0 if the frequency is
 *         not yet known.
 */
static double ticks_to_unit(LONGLONG ticks)
{
    if (g_freq.QuadPart == 0)
        return 0.0;

    double freq = (double)g_freq.QuadPart;
    switch (g_unit)
    {
    case TM_UNIT_NS:
        return (double)ticks * 1e9 / freq;
    case TM_UNIT_US:
        return (double)ticks * 1e6 / freq;
    case TM_UNIT_MS:
        return (double)ticks * 1e3 / freq;
    case TM_UNIT_S:
        return (double)ticks / freq;
    default:
        return (double)ticks * 1e3 / freq;
    }
}

/**
 * @brief Return the tick count elapsed since tm_start() was called.
 *
 * @return Tick delta, or 0 if tm_start() has not been called yet.
 */
static LONGLONG elapsed_ticks(void)
{
    if (!g_started)
        return 0;

    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return now.QuadPart - g_start.QuadPart;
}

/**
 * @brief Compute the current elapsed time and, when tracking is enabled,
 *        append it to the tracking vector.
 *
 * @return The elapsed value in the active unit.
 */
static double push_sample(void)
{
    double value = ticks_to_unit(elapsed_ticks());
    if (g_tracking_enabled)
        cvector_push_back(g_tracking_vec, value);
    return value;
}

/**
 * @brief Close the last open marker by computing its duration.
 *
 * @param now The elapsed time at the closing boundary.
 */
static void close_last_marker(double now)
{
    if (g_last_marker_open && g_marker_count > 0)
    {
        g_markers[g_marker_count - 1].duration = now - g_markers[g_marker_count - 1].start;
        g_last_marker_open = 0;
    }
}

/* --------------------------------------------------------------------------
 * Unit control
 * -------------------------------------------------------------------------- */

void tm_set_unit(tm_unit_t unit) { g_unit = unit; }

tm_unit_t tm_get_unit(void) { return g_unit; }

const char *tm_unit_label(void)
{
    switch (g_unit)
    {
    case TM_UNIT_NS:
        return "ns";
    case TM_UNIT_US:
        return "us";
    case TM_UNIT_MS:
        return "ms";
    case TM_UNIT_S:
        return "s";
    default:
        return "ms";
    }
}

/* --------------------------------------------------------------------------
 * Timer control
 * -------------------------------------------------------------------------- */

void tm_reset(void)
{
    tm_free_tracking_data();
    tm_free_stage_markers();
    tm_tracking_enable();
}

void tm_start(void)
{
    QueryPerformanceFrequency(&g_freq);
    QueryPerformanceCounter(&g_start);
    g_started = 1;
}

double tm_elapsed(void)
{
    return ticks_to_unit(elapsed_ticks());
}

double tm_stop(void)
{
    double now = push_sample();
    close_last_marker(now);
    return now;
}

/* --------------------------------------------------------------------------
 * Tracking control
 * -------------------------------------------------------------------------- */

void tm_tracking_enable(void) { g_tracking_enabled = 1; }
void tm_tracking_disable(void) { g_tracking_enabled = 0; }
int tm_tracking_is_enabled(void) { return g_tracking_enabled; }

/* --------------------------------------------------------------------------
 * Tracking data
 * -------------------------------------------------------------------------- */

double *tm_get_tracking_data(void) { return g_tracking_vec; }

size_t tm_get_tracking_count(void) { return cvector_size(g_tracking_vec); }

double tm_get_last_tracking_value(void)
{
    size_t count = cvector_size(g_tracking_vec);
    return count > 0 ? g_tracking_vec[count - 1] : 0.0;
}

void tm_free_tracking_data(void)
{
    cvector_free(g_tracking_vec);
    g_tracking_vec = NULL;
}

/* --------------------------------------------------------------------------
 * Stage markers
 * -------------------------------------------------------------------------- */

void tm_mark(const char *label)
{
    if (!g_tracking_enabled)
        return;

    double now = push_sample();
    close_last_marker(now);

    if (g_marker_count >= TM_TRACKING_MAX_MARKERS)
        return;

    g_markers[g_marker_count].label = label;
    g_markers[g_marker_count].start = now;
    g_markers[g_marker_count].duration = 0.0;
    g_marker_count++;
    g_last_marker_open = 1;
}

const tm_stage_marker_t *tm_get_stage_markers(void)
{
    return g_marker_count > 0 ? g_markers : NULL;
}

size_t tm_get_stage_marker_count(void) { return g_marker_count; }

void tm_free_stage_markers(void)
{
    g_marker_count = 0;
    g_last_marker_open = 0;
}
