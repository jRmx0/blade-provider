/**
 * @file timer.h
 * @brief QueryPerformanceCounter-backed timer with time-tracking.
 *
 * Provides a lightweight, high-resolution timer built on the Win32
 * QueryPerformanceCounter / QueryPerformanceFrequency APIs.  Elapsed time
 * samples are accumulated into an internal cvector so that callers can
 * examine a time-series of stage durations after the measured code completes.
 *
 * The API mirrors the allocator tracking API (allocator.h) so that both
 * libraries can be used side-by-side with a consistent programming model:
 *
 * @code
 *   tm_reset();               // clear any prior session, re-enable tracking
 *   tm_set_unit(TM_UNIT_MS);
 *   tm_start();               // anchor the baseline counter
 *   // ... work ...
 *   tm_mark("phase A");       // record elapsed + attach label
 *   // ... more work ...
 *   tm_mark("phase B");
 *   tm_stop();                // record final sample
 *
 *   double *samples      = tm_get_tracking_data();
 *   size_t  sample_count = tm_get_tracking_count();
 *   const tm_stage_marker_t *markers = tm_get_stage_markers();
 *   size_t  marker_count = tm_get_stage_marker_count();
 *
 *   tm_free_tracking_data();
 *   tm_free_stage_markers();
 * @endcode
 *
 * @note Not thread-safe.  All functions assume single-threaded access to
 *       the global timer state.
 */
#ifndef TIMER_H_
#define TIMER_H_

#include <stddef.h>
#include <windows.h>

/* ---- unit of measure ---- */

/**
 * @brief Time unit used for all elapsed-time values returned by the timer.
 *
 * Set via tm_set_unit() before calling tm_start().  The default unit is
 * TM_UNIT_MS.  The active unit affects tm_elapsed(), tm_stop(), tm_mark(),
 * and the values stored in the tracking vector.
 */
typedef enum
{
    TM_UNIT_NS = 0, /**< Nanoseconds  (1e-9 s) */
    TM_UNIT_US,     /**< Microseconds (1e-6 s) */
    TM_UNIT_MS,     /**< Milliseconds (1e-3 s) */
    TM_UNIT_S,      /**< Seconds      (1 s)    */
} tm_unit_t;

/**
 * @brief Set the unit used for all elapsed-time values.
 *
 * Call before tm_start() to ensure consistent units across the entire
 * sample series recorded in a session.
 *
 * @param unit One of TM_UNIT_NS, TM_UNIT_US, TM_UNIT_MS, TM_UNIT_S.
 */
void tm_set_unit(tm_unit_t unit);

/**
 * @brief Return the currently active time unit.
 * @return Current tm_unit_t value.
 */
tm_unit_t tm_get_unit(void);

/**
 * @brief Return a short string label for the current unit.
 *
 * Returns one of @c "ns", @c "us", @c "ms", or @c "s".  The returned
 * pointer is a string literal and must not be freed.
 *
 * @return Null-terminated unit label string.
 */
const char *tm_unit_label(void);

/* ---- stage markers ---- */

/**
 * @brief A labelled boundary in the tracking sample stream.
 *
 * @c sample_index is the zero-based index into the tracking array at the
 * moment the marker was recorded.  @c label is a string literal (not owned
 * by the timer).
 */
typedef struct
{
    size_t sample_index; /**< Index of the associated sample. */
    const char *label;   /**< Stage name supplied to tm_mark(). */
} tm_stage_marker_t;

/* ---- timer control ---- */

/**
 * @brief Reset all timer state in preparation for a new tracking session.
 *
 * Performs the following in order:
 *   1. Clears the sample tracking vector (tm_free_tracking_data).
 *   2. Clears all stage markers (tm_free_stage_markers).
 *   3. Re-enables tracking (tm_tracking_enable).
 *
 * Does NOT anchor the baseline counter; call tm_start() immediately
 * after tm_reset() when ready to begin measurement.
 */
void tm_reset(void);

/**
 * @brief Anchor the baseline counter for elapsed-time calculations.
 *
 * Queries QueryPerformanceFrequency and QueryPerformanceCounter and stores
 * the results as the reference point for all subsequent tm_elapsed(),
 * tm_mark(), and tm_stop() calls.
 *
 * Does NOT record a tracking sample and does NOT clear prior tracking data;
 * call tm_reset() before tm_start() to begin a clean session.
 */
void tm_start(void);

/**
 * @brief Return the elapsed time since tm_start() without recording a sample.
 *
 * The returned value is expressed in the unit set by tm_set_unit()
 * (default: milliseconds).
 *
 * @return Elapsed time in the current unit, or 0.0 if tm_start() has
 *         not been called.
 */
double tm_elapsed(void);

/**
 * @brief Record a tracking sample and return the elapsed time.
 *
 * Appends the current elapsed time to the internal tracking vector and
 * returns the same value.  This is the non-labelled counterpart to
 * tm_mark(); use it to close a measurement session without attaching a label.
 *
 * No sample is appended when tracking is disabled, but the elapsed value
 * is still computed and returned.
 *
 * @return Elapsed time in the current unit at the moment of the call,
 *         or 0.0 if tm_start() has not been called.
 */
double tm_stop(void);

/* ---- tracking control ---- */

/**
 * @brief Enable elapsed-time sample recording.
 *
 * Recording is enabled by default at program start and after tm_reset().
 * Each tm_mark() or tm_stop() call appends one sample when enabled.
 */
void tm_tracking_enable(void);

/**
 * @brief Disable elapsed-time sample recording.
 *
 * While disabled, tm_mark() and tm_stop() still compute the elapsed time
 * and return it, but do not append samples to the tracking vector.
 */
void tm_tracking_disable(void);

/**
 * @brief Return non-zero if tracking is currently enabled.
 * @return 1 if enabled, 0 if disabled.
 */
int tm_tracking_is_enabled(void);

/* ---- tracking data ---- */

/**
 * @brief Return a pointer to the raw tracking sample array.
 *
 * Each element is a @c double representing the elapsed time (in the unit
 * active when the sample was recorded) since the last tm_start() call.
 * The pointer is valid until the next tm_mark() or tm_stop() call that
 * triggers a cvector reallocation, or until tm_free_tracking_data() is
 * called.  Do NOT pass this pointer to @c free() or @c va_free() directly.
 *
 * @return Pointer to the first sample, or NULL if no samples exist.
 */
double *tm_get_tracking_data(void);

/**
 * @brief Return the number of tracking samples recorded so far.
 * @return Sample count, or 0 if no samples exist.
 */
size_t tm_get_tracking_count(void);

/**
 * @brief Return the most recently recorded tracking sample.
 * @return The last sample value in the current unit, or 0.0 if no samples
 *         exist.
 */
double tm_get_last_tracking_value(void);

/**
 * @brief Release the internal tracking vector and reset the sample count.
 *
 * Tracking remains in whatever enabled/disabled state it was in before
 * this call.
 */
void tm_free_tracking_data(void);

/* ---- stage markers ---- */

/**
 * @brief Record a tracking sample and attach a stage label to it.
 *
 * No-op when tracking is disabled.  At most TM_TRACKING_MAX_MARKERS
 * markers are stored; extras are silently dropped.
 *
 * @param label Stage name (must be a string literal or otherwise outlive
 *              the tracking session).
 */
void tm_mark(const char *label);

/**
 * @brief Return the array of stage markers recorded so far.
 * @return Pointer to the first marker, or NULL if none exist.
 */
const tm_stage_marker_t *tm_get_stage_markers(void);

/**
 * @brief Return the number of stage markers recorded so far.
 */
size_t tm_get_stage_marker_count(void);

/**
 * @brief Clear all recorded stage markers.
 */
void tm_free_stage_markers(void);

#endif /* TIMER_H_ */
