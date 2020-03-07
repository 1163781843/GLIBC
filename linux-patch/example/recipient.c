#include <logger.h>
#include <header.h>
#include <config.h>

static int32b_t recipient_function(void_t)
{
    link_dynamic_library();

    return 0;
}

int32b_t main()
{
    while (1) {
        recipient_function();
        sleep(5);
    }

    return 0;
}
