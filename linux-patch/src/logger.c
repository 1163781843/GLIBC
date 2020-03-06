#include <config.h>
#include <logger.h>

static int32b_t elim_spec_string(char *data, char delim, char **array, int32b_t arraylen);
#define arraylen(array) (sizeof(array) / sizeof(array[0]))
#define elim_string(data, delim, array) elim_spec_string((data), (delim), (array), arraylen(array))

static int32b_t string_char_delim(int8b_t *data, int8b_t delim, int8b_t **array, int32b_t arraylen)
{
    int32b_t count = 0;
    int8b_t *ptr = data;

    enum tokenizer_state {
        START,
        FIND_DELIM
    } state = START;

    while (*ptr && (count < arraylen)) {
        switch (state) {
        case START:
            array[count++] = ptr;
            state = FIND_DELIM;
            break;
        case FIND_DELIM:
            if (delim == *ptr) {
                *ptr = '\0';
                state = START;
            }
            ++ptr;
            break;
        }
    }

    return count;
}

static int32b_t elim_spec_string(int8b_t *data, int8b_t delim, int8b_t **array, int32b_t arraylen)
{
    if (!data || !array || !arraylen) {
        return 0;
    }

    memset(array, 0, arraylen * sizeof(*array));

    return string_char_delim(data, delim, array, arraylen);
}

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
    int32b_t argc = 0;
    int8b_t *lines[1024] = {0};
    int32b_t len = 0;
    int8b_t strbuffer[65535] = {0};
    int8b_t *dupstr = NULL, *tmpdupstr = NULL;

    va_start(ap, format);
    vasprintf(&buffer, format, ap);
    va_end(ap);

    cursec = time(NULL);
    curtm = localtime(&cursec);
    gettimeofday(&curtv, NULL);

    if (file) {
        filename = strrchr(file, '/');
    }

    tmpdupstr = dupstr = strdup(buffer);
    argc = elim_string(dupstr, '\n', lines);
    for (int32b_t i = 0; i < argc; i++) {
        if (strlen(lines[i])) {
            len += snprintf(strbuffer + len, sizeof(strbuffer) - len, "%s\n", lines[i]);
        }
    }
    if (tmpdupstr) {
        free(tmpdupstr);
        tmpdupstr = NULL;
    }

    fprintf(stdout, "[%04d-%02d-%02d %02d:%02d:%02d:%03ld] %s [%05ld]  -- %s:%d  %s",
        curtm->tm_year + 1900, curtm->tm_mon + 1, curtm->tm_mday,
        curtm->tm_hour, curtm->tm_min, curtm->tm_sec, curtv.tv_usec,
        patch_logstr(level), syscall(SYS_gettid), filename ? ++filename : file,
        line, strbuffer);

    if (buffer) {
        free(buffer);
        buffer = NULL;
    }
}
