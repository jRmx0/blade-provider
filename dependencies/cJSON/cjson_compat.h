#ifndef CJSON_COMPAT_H
#define CJSON_COMPAT_H

/*
 * clang + MinGW can diagnose a duplicate max_align_t definition when cJSON.h
 * pulls in <stddef.h>: clang's builtin headers and MinGW's stddef.h can both
 * try to typedef max_align_t.
 *
 * For clang-based tooling, suppress clang's builtin max_align_t typedef and let
 * MinGW provide the canonical definition. This is harmless for the real
 * GCC-based build because __clang__ is not defined there.
 */
#if defined(__clang__) && !defined(__CLANG_MAX_ALIGN_T_DEFINED)
#define __CLANG_MAX_ALIGN_T_DEFINED
#endif

#include "cJSON.h" // IWYU pragma: export

#endif // CJSON_COMPAT_H