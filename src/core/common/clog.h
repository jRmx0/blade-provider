/* clog.h — Simple levelled logging for the blade-provider C core.
 *
 * Output goes to stderr so it does not pollute the JSON written to stdout
 * by the compute child process.
 *
 * Log level is controlled by the LOG_LEVEL environment variable
 * (error | warn | info | debug). Defaults to "info" when unset.
 *
 *   LOG_ERROR   always visible
 *   LOG_WARN    always visible
 *   LOG_INFO    always visible (use for important process status)
 *   LOG_DEBUG   visible only when LOG_LEVEL=debug (use for incremental steps)
 */

#pragma once

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>

static inline void _clog_emit(const char *level, const char *fmt, ...)
{
    char buffer[2048];

    int prefix_len = snprintf(buffer, sizeof(buffer), "[%s] ", level);
    if (prefix_len < 0)
        return;

    if (prefix_len >= (int)sizeof(buffer))
        prefix_len = (int)sizeof(buffer) - 1;

    va_list args;
    va_start(args, fmt);
    int body_len = vsnprintf(buffer + prefix_len,
                             sizeof(buffer) - (size_t)prefix_len,
                             fmt,
                             args);
    va_end(args);

    if (body_len < 0)
        return;

    size_t total_len = (size_t)prefix_len + (size_t)body_len;
    if (total_len >= sizeof(buffer))
        total_len = sizeof(buffer) - 1;

    if (total_len + 1 < sizeof(buffer))
    {
        buffer[total_len++] = '\n';
        buffer[total_len] = '\0';
    }
    else
    {
        buffer[sizeof(buffer) - 2] = '\n';
        buffer[sizeof(buffer) - 1] = '\0';
        total_len = sizeof(buffer) - 1;
    }

    (void)_write(2, buffer, (unsigned int)total_len);
}

static inline int _clog_is_debug(void)
{
    static int cached = -1;
    if (cached < 0)
    {
        const char *env = getenv("LOG_LEVEL");
        cached = (env && strcmp(env, "debug") == 0) ? 1 : 0;
    }
    return cached;
}

/* clang-format off */
#define LOG_ERROR(fmt, ...) _clog_emit("error", fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  _clog_emit("warn", fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)  _clog_emit("info", fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) \
    do { if (_clog_is_debug()) _clog_emit("debug", fmt, ##__VA_ARGS__); } while (0)
/* clang-format on */
