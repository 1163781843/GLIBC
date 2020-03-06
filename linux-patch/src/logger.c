#include <config.h>
#include <logger.h>

static const int8b_t *patch_logstr(int32_t level)
{
    switch (level) {
    case log_error:
        return "\033[31;1merror\33[0m";
    case log_warning:
        return "\033[32;31;1mwarning\33[0m";
    case log_notice:
        return "\033[33;1mnotice\33[0m";
    case log_verbose:
        return "\033[32mverbose\33[0m";
    case log_debug:
    default:
        break;
    }

    return "\033[32;1mdebug\33[0m";
}

void_t patch_logger(int32b_t level, const int8b_t *file, int32b_t line, const int8b_t *format, ...)
{
    time_t cursec;
    struct timeval curtv;
    struct tm *curtm = NULL;
    int8b_t *buffer = NULL;
    va_list ap;
    int8b_t *filename = NULL;

    va_start(ap, format);
    vasprintf(&buffer, format, ap);
    va_end(ap);

    cursec = time(NULL);
    curtm = localtime(&cursec);
    gettimeofday(&curtv, NULL);

    if (file) {
        filename = strrchr(file, '/');
    }

    fprintf(stdout, "[%04d-%02d-%02d %02d:%02d:%02d:%03ld] %s [%05ld]  -- %s:%d  %s\n",
        curtm->tm_year + 1900, curtm->tm_mon + 1, curtm->tm_mday,
        curtm->tm_hour, curtm->tm_min, curtm->tm_sec, curtv.tv_usec,
        patch_logstr(level), syscall(SYS_gettid), filename ? ++filename : file,
        line, buffer);

    if (buffer) {
        free(buffer);
        buffer = NULL;
    }
}
