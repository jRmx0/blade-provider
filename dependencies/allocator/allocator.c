/**
 * @file allocator.c
 * @brief VirtualAlloc-backed allocator implementation.
 *
 * Every public function in this translation unit calls the static helper
 * record_op() after it completes its primary work.  record_op() queries
 * the process working set and appends a baseline-relative sample to the
 * internal cvector.  The reentrancy guard inside record_op() is
 * necessary because cvector_push_back may itself invoke va_realloc,
 * which would otherwise recurse back into record_op() unboundedly.
 */
#include "allocator.h"
#include "../../dependencies/cvector/cvector.h"
#include <string.h>
#include <windows.h>
#include <psapi.h>

static void record_op(void);

/* --------------------------------------------------------------------------
 * Core allocators
 * -------------------------------------------------------------------------- */

void *va_malloc(size_t size)
{
    void *ptr = VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    record_op();
    return ptr;
}

void va_free(void *ptr)
{
    if (ptr)
        VirtualFree(ptr, 0, MEM_RELEASE);
    record_op();
}

void *va_calloc(size_t count, size_t size)
{
    /* MEM_COMMIT guarantees newly committed pages are zero-filled by the OS,
     * so no explicit memset is needed here. */
    void *ptr = VirtualAlloc(NULL, count * size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    record_op();
    return ptr;
}

void *va_realloc(void *ptr, size_t new_size)
{
    if (!ptr)
        return va_malloc(new_size);

    void *new_ptr = VirtualAlloc(NULL, new_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!new_ptr)
        return NULL;
    /* Snapshot 1 — peak: both the old and new regions are simultaneously
     * resident, so working set is at its highest point. */
    record_op();

    /* VirtualQuery gives us the committed region size so we copy only what
     * exists, capped at new_size to avoid an over-read. */
    MEMORY_BASIC_INFORMATION mbi;
    VirtualQuery(ptr, &mbi, sizeof(mbi));
    size_t copy_size = mbi.RegionSize < new_size ? mbi.RegionSize : new_size;
    memcpy(new_ptr, ptr, copy_size);
    /* Snapshot 2 — fault-in: the memcpy touches every new page, causing the
     * OS to fault them into the working set. */
    record_op();

    VirtualFree(ptr, 0, MEM_RELEASE);
    /* Snapshot 3 — settle: releasing the old region trims the working set;
     * the delta should drop back toward the pre-realloc level. */
    record_op();
    return new_ptr;
}

/* --------------------------------------------------------------------------
 * Copy + memory ops
 * -------------------------------------------------------------------------- */

void *va_memset(void *ptr, int value, size_t size)
{
    void *result = memset(ptr, value, size);
    record_op();
    return result;
}

void *va_memcpy(void *dst, const void *src, size_t size)
{
    void *result = memcpy(dst, src, size);
    record_op();
    return result;
}

void *va_memmove(void *dst, const void *src, size_t size)
{
    void *result = memmove(dst, src, size);
    record_op();
    return result;
}

char *va_strdup(const char *str)
{
    if (!str)
        return NULL;
    size_t len = strlen(str) + 1;
    char *dst = VirtualAlloc(NULL, len, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!dst)
        return NULL;
    memcpy(dst, str, len);
    record_op(); /* combined alloc + touch */
    return dst;
}

char *va_strndup(const char *str, size_t n)
{
    if (!str)
        return NULL;
    size_t len = strnlen(str, n);
    char *dst = VirtualAlloc(NULL, len + 1, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!dst)
        return NULL;
    memcpy(dst, str, len);
    dst[len] = '\0';
    record_op(); /* combined alloc + touch */
    return dst;
}

/* --------------------------------------------------------------------------
 * VirtualAlloc-specific ops
 * -------------------------------------------------------------------------- */

int va_decommit(void *ptr, size_t size)
{
    int ok = VirtualFree(ptr, size, MEM_DECOMMIT) != 0;
    record_op();
    return ok;
}

int va_protect(void *ptr, size_t size, DWORD flags, DWORD *old_protect)
{
    DWORD old = 0;
    int ok = VirtualProtect(ptr, size, flags, &old) != 0;
    if (old_protect)
        *old_protect = old;
    record_op();
    return ok;
}

/* --------------------------------------------------------------------------
 * Memory usage query
 * -------------------------------------------------------------------------- */

long get_mem_usage(void)
{
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
        return (long)(pmc.WorkingSetSize);
    return -1;
}

/* --------------------------------------------------------------------------
 * Tracking state
 * -------------------------------------------------------------------------- */

static cvector(long) g_tracking_vec = NULL;
static int g_tracking_enabled = 1;
static long g_baseline = 0;

/**
 * @brief Append a working-set snapshot to the tracking vector.
 *
 * @internal
 * Called after every allocator and memory operation.  The function is
 * a no-op when tracking is disabled.
 *
 * The static @c recording flag prevents unbounded recursion: cvector_push_back
 * may internally call va_realloc (to grow the vector's backing region), which
 * itself calls record_op().  The guard causes the nested call to return
 * immediately, so only the outermost invocation records a sample.
 *
 * Stored value: working_set_bytes − g_baseline, or -1 on query failure.
 */
static void record_op(void)
{
    if (!g_tracking_enabled)
        return;

    static int recording = 0;
    if (recording)
        return;
    recording = 1;

    long usage = get_mem_usage();
    long value = (usage >= 0) ? (usage - g_baseline) : -1;
    cvector_push_back(g_tracking_vec, value);

    recording = 0;
}

/* --------------------------------------------------------------------------
 * Tracking API
 * -------------------------------------------------------------------------- */

void va_tracking_enable(void) { g_tracking_enabled = 1; }
void va_tracking_disable(void) { g_tracking_enabled = 0; }
int va_tracking_is_enabled(void) { return g_tracking_enabled; }

void va_tracking_set_baseline(void)
{
    long usage = get_mem_usage();
    g_baseline = (usage >= 0) ? usage : 0;
}

long va_get_baseline(void) { return g_baseline; }

long *va_get_tracking_data(void) { return g_tracking_vec; }
size_t va_get_tracking_count(void) { return cvector_size(g_tracking_vec); }

long va_get_last_tracking_value(void)
{
    size_t count = cvector_size(g_tracking_vec);
    return count > 0 ? g_tracking_vec[count - 1] : 0;
}

void va_free_tracking_data(void)
{
    cvector_free(g_tracking_vec);
    g_tracking_vec = NULL;
}

/* --------------------------------------------------------------------------
 * Stage markers
 * -------------------------------------------------------------------------- */

#define VA_TRACKING_MAX_MARKERS 32

static va_stage_marker_t g_markers[VA_TRACKING_MAX_MARKERS];
static size_t g_marker_count = 0;

void va_tracking_mark(const char *label)
{
    if (!g_tracking_enabled)
        return;
    /* Take a snapshot at the stage boundary first. */
    record_op();
    if (g_marker_count >= VA_TRACKING_MAX_MARKERS)
        return;
    size_t count = cvector_size(g_tracking_vec);
    g_markers[g_marker_count].sample_index = count > 0 ? count - 1 : 0;
    g_markers[g_marker_count].label = label;
    g_marker_count++;
}

const va_stage_marker_t *va_get_stage_markers(void) { return g_marker_count > 0 ? g_markers : NULL; }
size_t va_get_stage_marker_count(void) { return g_marker_count; }
void va_free_stage_markers(void) { g_marker_count = 0; }
