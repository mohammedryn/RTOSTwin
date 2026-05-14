/**
 * @file main.c
 * @brief Backward-compatible entry point for existing RTOSTwin examples.
 */

#include "rtostwin.h"

#if RTOSTWIN_ENABLE && !RTOSTWIN_ENABLE_COMPAT_API
void StartTelemetryAgent(void)
{
    (void)rtostwin_init();
    (void)rtostwin_start();
}
#endif
