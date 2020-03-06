#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdarg.h>
#include <sys/time.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>

#include <logger.h>

static const char *patch_logstr(int level)
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

void patch_logger(int level, const char *file, int line, const char *format, ...)
{
    time_t cursec;
    struct timeval curtv;
    struct tm *curtm = NULL;
    char *buffer = NULL;
    va_list ap;
    char *filename = NULL;

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
