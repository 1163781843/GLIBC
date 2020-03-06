#ifndef __LOGGER_H__
#define __LOGGER_H__

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#include <config.h>

enum {
    log_error,
    log_warning,
    log_notice,
    log_verbose,
    log_debug,
};

void_t patch_logger(int32b_t level, const int8b_t *file, int32b_t line, const int8b_t *format, ...);

#define plogger(level, ...) do {                            \
    patch_logger(level, __FILE__, __LINE__, __VA_ARGS__);   \
} while (0)

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
