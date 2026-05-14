#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "../include/rtostwin.h"

int main(void)
{
    assert(rtostwin_init() == RTOSTWIN_STATUS_DISABLED);
    assert(rtostwin_start() == RTOSTWIN_STATUS_DISABLED);
    assert(rtostwin_stop() == RTOSTWIN_STATUS_DISABLED);
    assert(rtostwin_is_running() == false);
    assert(strcmp(rtostwin_version(), "test-disabled") == 0);

    printf("rtostwin disabled-mode API test PASSED\n");
    return 0;
}
