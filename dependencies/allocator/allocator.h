/**
 * @file allocator.h
 * @brief VirtualAlloc-backed allocator with memory-usage tracking.
 *
 * Provides drop-in replacements for the standard C heap functions
 * (malloc / free / calloc / realloc) that are implemented on top of the
 * Win32 VirtualAlloc / VirtualFree APIs.  Every allocation and memory
 * operation records a working-set snapshot into an internal cvector so
 * that callers can plot memory usage over time.
 *
 * The cvector hook defines at the bottom of this header must be
 * included before cvector.h so that the vector used internally by the
 * tracking subsystem is itself backed by VirtualAlloc rather than the
 * CRT heap.
 */
#ifndef ALLOCATOR_H_
#define ALLOCATOR_H_

#include <stddef.h>
#include <windows.h>

/* ---- core allocators ---- */

/**
 * @brief Allocate a block of at least @p size bytes.
 *
 * Commits and reserves a new private region via VirtualAlloc.  The
 * returned memory is zero-initialised by the OS.
 *
 * @param size Number of bytes to allocate.
 * @return Pointer to the allocated block, or NULL on failure.
 */
void *va_malloc(size_t size);

/**
 * @brief Release a block previously obtained from this allocator.
 *
 * Calls VirtualFree with MEM_RELEASE.  Passing NULL is a no-op.
 *
 * @param ptr Pointer returned by va_malloc / va_calloc / va_realloc,
 *            or NULL.
 */
void  va_free(void *ptr);

/**
 * @brief Allocate a zero-initialised array of @p count × @p size bytes.
 *
 * VirtualAlloc with MEM_COMMIT guarantees that committed pages are
 * zeroed by the OS, so no explicit memset is performed.
 *
 * @param count Number of elements.
 * @param size  Size of each element in bytes.
 * @return Pointer to the allocated block, or NULL on failure.
 */
void *va_calloc(size_t count, size_t size);

/**
 * @brief Resize an existing allocation to @p new_size bytes.
 *
 * Because VirtualAlloc regions cannot be grown in place, this always
 * allocates a fresh region, copies the existing data, and releases the
 * old region.  Three working-set snapshots are recorded to capture the
 * distinct phases:
 *   1. *peak*   — both the old and new regions are live simultaneously.
 *   2. *fault-in* — new pages are touched by the copy, raising their
 *                   working-set contribution.
 *   3. *settle* — the old region is released and working set drops.
 *
 * If @p ptr is NULL the call is equivalent to va_malloc(@p new_size).
 *
 * @param ptr      Existing allocation to resize, or NULL.
 * @param new_size Desired size in bytes.
 * @return Pointer to the resized block, or NULL on failure.
 *         The original block is left untouched when NULL is returned.
 */
void *va_realloc(void *ptr, size_t new_size);

/* ---- copy + memory ops ---- */

/**
 * @brief Fill a memory region with a byte value.
 *
 * Thin wrapper around memset that records a tracking snapshot after
 * the operation.
 *
 * @param ptr   Pointer to the start of the region.
 * @param value Byte value to write (cast to unsigned char).
 * @param size  Number of bytes to fill.
 * @return @p ptr.
 */
void *va_memset(void *ptr, int value, size_t size);

/**
 * @brief Copy a non-overlapping memory region.
 *
 * Thin wrapper around memcpy that records a tracking snapshot after
 * the operation.
 *
 * @param dst  Destination pointer.
 * @param src  Source pointer.
 * @param size Number of bytes to copy.
 * @return @p dst.
 */
void *va_memcpy(void *dst, const void *src, size_t size);

/**
 * @brief Copy a potentially-overlapping memory region.
 *
 * Thin wrapper around memmove that records a tracking snapshot after
 * the operation.
 *
 * @param dst  Destination pointer.
 * @param src  Source pointer.
 * @param size Number of bytes to copy.
 * @return @p dst.
 */
void *va_memmove(void *dst, const void *src, size_t size);

/**
 * @brief Duplicate a null-terminated string into a VirtualAlloc region.
 *
 * Unlike the CRT strdup, the returned pointer must be freed with
 * va_free, not free().
 *
 * @param str String to duplicate, or NULL.
 * @return Pointer to the duplicated string, or NULL if @p str is NULL
 *         or if the allocation fails.
 */
char *va_strdup(const char *str);

/**
 * @brief Duplicate at most @p n characters of a string into a
 *        VirtualAlloc region.
 *
 * The result is always null-terminated.  Unlike the CRT strndup, the
 * returned pointer must be freed with va_free, not free().
 *
 * @param str String to duplicate, or NULL.
 * @param n   Maximum number of characters to copy (excluding the
 *            terminating null byte).
 * @return Pointer to the duplicated string, or NULL if @p str is NULL
 *         or if the allocation fails.
 */
char *va_strndup(const char *str, size_t n);

/* ---- VirtualAlloc-specific ops ---- */

/**
 * @brief Decommit a range of pages without releasing the reserved
 *        address space.
 *
 * Calls VirtualFree with MEM_DECOMMIT.  The virtual-address range
 * remains reserved and can be re-committed later without changing the
 * base pointer.
 *
 * @param ptr  Base address of the region to decommit (must be
 *             page-aligned).
 * @param size Number of bytes to decommit.
 * @return Non-zero on success, zero on failure.
 */
int   va_decommit(void *ptr, size_t size);

/**
 * @brief Change the access-protection flags of a memory region.
 *
 * Thin wrapper around VirtualProtect.
 *
 * @param ptr         Base address of the region (must be page-aligned).
 * @param size        Size of the region in bytes.
 * @param flags       New protection flags (e.g. PAGE_READONLY,
 *                    PAGE_READWRITE, PAGE_NOACCESS).
 * @param old_protect Receives the previous protection flags.  May be
 *                    NULL if the previous value is not needed.
 * @return Non-zero on success, zero on failure.
 */
int   va_protect(void *ptr, size_t size, DWORD flags, DWORD *old_protect);

/* ---- memory usage query ---- */

/**
 * @brief Return the current working-set size of the process in bytes.
 *
 * Uses GetProcessMemoryInfo / PROCESS_MEMORY_COUNTERS.
 *
 * @return Working-set size in bytes, or -1 if the query fails.
 */
long get_mem_usage(void);

/* ---- tracking API ---- */

/**
 * @brief Enable working-set snapshot recording.
 *
 * Recording is enabled by default at program start.  Each allocator
 * call appends one sample to the internal tracking vector.
 */
void   va_tracking_enable(void);

/**
 * @brief Disable working-set snapshot recording.
 *
 * While disabled, allocator calls still perform their work but do not
 * append samples to the tracking vector.
 */
void   va_tracking_disable(void);

/**
 * @brief Return non-zero if tracking is currently enabled.
 * @return 1 if enabled, 0 if disabled.
 */
int    va_tracking_is_enabled(void);

/**
 * @brief Capture the current working-set size as the baseline.
 *
 * All subsequent tracking samples are stored as deltas relative to
 * this baseline (sample = working_set − baseline).  Call this once
 * before the code under measurement to normalise the series.
 */
void   va_tracking_set_baseline(void);

/**
 * @brief Return the baseline working-set size captured by the most
 *        recent call to va_tracking_set_baseline().
 * @return Baseline in bytes, or 0 if no baseline has been set.
 */
long   va_get_baseline(void);

/**
 * @brief Return a pointer to the raw tracking sample array.
 *
 * Each element is a signed long representing the working-set delta in
 * bytes relative to the baseline (-1 indicates a failed query).
 * The pointer is valid until the next allocator call that triggers a
 * cvector reallocation, or until va_free_tracking_data() is called.
 * Do NOT pass this pointer to free() or va_free() directly.
 *
 * @return Pointer to the first sample, or NULL if no samples exist.
 */
long  *va_get_tracking_data(void);

/**
 * @brief Return the number of samples currently in the tracking vector.
 * @return Sample count as size_t.
 */
size_t va_get_tracking_count(void);

/**
 * @brief Return the most recently recorded tracking sample.
 * @return Last sample value in bytes, or 0 if the vector is empty.
 */
long   va_get_last_tracking_value(void);

/**
 * @brief Release the memory used by the internal tracking vector and
 *        reset the sample count to zero.
 *
 * After this call va_get_tracking_data() returns NULL and
 * va_get_tracking_count() returns 0.  Recording continues normally if
 * tracking is still enabled.
 */
void   va_free_tracking_data(void);

/**
 * @note The defines below redirect the cvector library to use this
 * allocator instead of the CRT heap.  They must appear in this header
 * (before any inclusion of cvector.h) so that the internal tracking
 * vector is itself backed by VirtualAlloc/VirtualFree.
 */
#define cvector_clib_malloc  va_malloc
#define cvector_clib_free    va_free
#define cvector_clib_calloc  va_calloc
#define cvector_clib_realloc va_realloc

#endif /* ALLOCATOR_H_ */
