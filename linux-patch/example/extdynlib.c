#include <logger.h>
#include <config.h>

int32b_t inject_function(void_t)
{
    plogger(log_debug, "This is a inject function!\n");

    return 0;
}

__attribute__((constructor)) void_t load_extdynlib()
{
   plogger(log_notice, "%s load success ...\n", __FUNCTION__);
}

__attribute__((destructor)) void_t unload_extdynlib()
{
    plogger(log_notice, "%s unload success ...\n", __FUNCTION__);
}
