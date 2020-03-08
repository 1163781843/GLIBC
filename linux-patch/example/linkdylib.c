#include <logger.h>
#include <config.h>

int32b_t link_dynamic_library(void)
{
    plogger(log_debug, "This is a dynamic library\n");
    return 0;
}

void_t *link_dynamic_dlopen(const int8b_t *filename, int32b_t flags)
{
    void_t *handle = NULL;
    plogger(log_notice, "filename[%p], flags[%p], handle[%p]\n", filename, flags, handle);

    handle = dlopen(filename, flags);

    plogger(log_notice, "filename[%s], flags[%02x], handle[%p]\n", filename, flags, handle);

    return handle;
}

int32b_t link_dynamic_dlclose(void_t *handle)
{
    if (dlclose(handle)) {
        plogger(log_error, "dlclose dynamic library failure!\n");
        return -1;
    }

    plogger(log_debug, "dlclose dynamic library success!\n");

    return 0;
}
