/**
 * test_allocator.c
 *
 * Exercises every public function in the VirtualAlloc-backed allocator
 * under the same Bun/TCC toolchain used by the production dispatcher.
 *
 * Compiled as a DLL by cc() — no main(); entry point is run_allocator_tests().
 */

#include <stdio.h>
#include <string.h>
#include <windows.h>

/* allocator.h must precede allocator.c so the cvector_clib_* macros are
 * defined before cvector.h is pulled in by the unity build. */
#include "../dependencies/allocator/allocator.h"
#include "../dependencies/allocator/allocator.c"

/* --------------------------------------------------------------------------
 * Test helpers
 * -------------------------------------------------------------------------- */

static void print_usage(void)
{
    long base  = va_get_baseline();
    long delta = va_get_last_tracking_value();
    printf("  usage: baseline=%ld  delta=%ld\n", base, delta);
}

/* --------------------------------------------------------------------------
 * Tests (verbatim logic from performace-metrics/src/main.c)
 * -------------------------------------------------------------------------- */

static void test_virtual_alloc(void)
{
    size_t size = 1024 * 100;

    printf("Usage: %ld + %ld\n", va_get_baseline(), va_get_last_tracking_value());

    char *p1 = va_malloc(size);
    va_memset(p1, 1, size);
    printf("Usage: %ld + %ld\n", va_get_baseline(), va_get_last_tracking_value());

    char *p2 = va_malloc(size);
    va_memset(p2, 1, size);
    printf("Usage: %ld + %ld\n", va_get_baseline(), va_get_last_tracking_value());

    va_free(p1);
    printf("Usage: %ld + %ld\n", va_get_baseline(), va_get_last_tracking_value());

    va_free(p2);
    printf("Usage: %ld + %ld\n", va_get_baseline(), va_get_last_tracking_value());
}

static void test_cvector(void)
{
    cvector(int) vec = NULL;

    for (int i = 0; i < 50000; i++) {
        cvector_push_back(vec, i);
    }
    printf("After 50 000 push_back: %ld + %ld bytes  (size=%zu, capacity=%zu)\n",
           va_get_baseline(),
           va_get_last_tracking_value(),
           cvector_size(vec),
           cvector_capacity(vec));

    for (int i = 0; i < 25000; i++) {
        cvector_pop_back(vec);
    }
    printf("After 25 000 pop_back:  %ld + %ld bytes  (size=%zu, capacity=%zu)\n",
           va_get_baseline(),
           va_get_last_tracking_value(),
           cvector_size(vec),
           cvector_capacity(vec));

    cvector_shrink_to_fit(vec);
    printf("After shrink_to_fit:    %ld + %ld bytes  (size=%zu, capacity=%zu)\n",
           va_get_baseline(),
           va_get_last_tracking_value(),
           cvector_size(vec),
           cvector_capacity(vec));

    cvector_free(vec);
    printf("After cvector_free:     %ld + %ld bytes\n",
           va_get_baseline(),
           va_get_last_tracking_value());
}

static void test_realloc(void)
{
    size_t size = 1024 * 100;

    char *p = va_malloc(size);
    va_memset(p, 1, size);
    printf("After malloc+memset(100KB):   %ld + %ld\n", va_get_baseline(), va_get_last_tracking_value());

    p = va_realloc(p, size * 2);
    /* va_realloc records 3 snapshots internally; print the last (settled) one */
    printf("After realloc to 200KB (settled): %ld + %ld\n", va_get_baseline(), va_get_last_tracking_value());

    va_free(p);
    printf("After free:                   %ld + %ld\n", va_get_baseline(), va_get_last_tracking_value());
}

static void test_copy_ops(void)
{
    size_t size = 1024 * 64;

    char *src = va_malloc(size);
    va_memset(src, 0xAB, size);
    printf("src malloc+memset(64KB):  %ld + %ld\n", va_get_baseline(), va_get_last_tracking_value());

    char *dst = va_malloc(size);
    printf("dst malloc (untouched):   %ld + %ld\n", va_get_baseline(), va_get_last_tracking_value());

    va_memcpy(dst, src, size);
    printf("after va_memcpy:          %ld + %ld\n", va_get_baseline(), va_get_last_tracking_value());

    /* shift src one byte to force overlap-safe path via memmove */
    va_memmove(dst, dst + 1, size - 1);
    printf("after va_memmove(overlap):%ld + %ld\n", va_get_baseline(), va_get_last_tracking_value());

    va_free(src);
    va_free(dst);
    printf("after free both:          %ld + %ld\n", va_get_baseline(), va_get_last_tracking_value());
}

static void test_strdup(void)
{
    const char *orig = "Hello, VirtualAlloc world! This is a test string for strdup.";

    char *dup = va_strdup(orig);
    printf("va_strdup:  %ld + %ld  (content match: %d)\n",
           va_get_baseline(), va_get_last_tracking_value(),
           strcmp(orig, dup) == 0);

    char *ndup = va_strndup(orig, 5);
    printf("va_strndup(5): %ld + %ld  (result: \"%s\")\n",
           va_get_baseline(), va_get_last_tracking_value(), ndup);

    va_free(dup);
    va_free(ndup);
    printf("after free both: %ld + %ld\n", va_get_baseline(), va_get_last_tracking_value());
}

static void test_decommit(void)
{
    size_t size = 1024 * 64;

    char *p = va_malloc(size);
    va_memset(p, 1, size);
    printf("After malloc+memset(64KB):  %ld + %ld\n", va_get_baseline(), va_get_last_tracking_value());

    va_decommit(p, size);
    printf("After va_decommit:          %ld + %ld  (VA reservation kept)\n",
           va_get_baseline(), va_get_last_tracking_value());

    /* Re-commit the same VA range and touch it again */
    VirtualAlloc(p, size, MEM_COMMIT, PAGE_READWRITE);
    va_memset(p, 2, size);
    printf("After re-commit+memset:     %ld + %ld\n", va_get_baseline(), va_get_last_tracking_value());

    va_free(p);
    printf("After free:                 %ld + %ld\n", va_get_baseline(), va_get_last_tracking_value());
}

static void test_protect(void)
{
    size_t size = 1024 * 64;

    char *p = va_malloc(size);
    va_memset(p, 1, size);
    printf("After malloc+memset(64KB):    %ld + %ld\n", va_get_baseline(), va_get_last_tracking_value());

    DWORD old = 0;
    va_protect(p, size, PAGE_READONLY, &old);
    printf("After PAGE_READONLY protect:  %ld + %ld  (old flags: 0x%lX)\n",
           va_get_baseline(), va_get_last_tracking_value(), old);

    va_protect(p, size, PAGE_NOACCESS, &old);
    printf("After PAGE_NOACCESS protect:  %ld + %ld  (old flags: 0x%lX)\n",
           va_get_baseline(), va_get_last_tracking_value(), old);

    /* Restore before free to avoid access violation in VirtualFree */
    va_protect(p, size, PAGE_READWRITE, &old);
    va_free(p);
    printf("After restore+free:           %ld + %ld\n", va_get_baseline(), va_get_last_tracking_value());
}

/* --------------------------------------------------------------------------
 * Entry point (exported symbol, called from test-allocator.ts)
 * -------------------------------------------------------------------------- */

void run_allocator_tests(void)
{
    va_tracking_set_baseline();
    printf("Baseline: %ld bytes\n\n", va_get_baseline());

    printf("=== VirtualAlloc test ===\n");
    test_virtual_alloc();

    printf("\n=== cvector test ===\n");
    test_cvector();

    printf("\n=== realloc test ===\n");
    test_realloc();

    printf("\n=== copy ops test ===\n");
    test_copy_ops();

    printf("\n=== strdup test ===\n");
    test_strdup();

    printf("\n=== decommit test ===\n");
    test_decommit();

    printf("\n=== protect test ===\n");
    test_protect();

    printf("\nDone.\n");
}
