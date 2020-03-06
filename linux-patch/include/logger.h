#ifndef __LOGGER_H__
#define __LOGGER_H__

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

enum {
    log_error,
    log_warning,
    log_notice,
    log_verbose,
    log_debug,
};

void patch_logger(int level, const char *file, int line, const char *format, ...);

#define plogger(level, ...) do {                            \
    patch_logger(level, __FILE__, __LINE__, __VA_ARGS__);   \
} while (0)

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
