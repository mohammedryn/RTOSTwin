#ifndef RTOSTWIN_CONFIG_H
#define RTOSTWIN_CONFIG_H

/*
 * Repo-local baseline configuration used by the validated examples and the
 * compatibility implementation. Downstream firmware can override this file by
 * providing its own rtostwin_config.h earlier on the include path.
 */

#define RTOSTWIN_ENABLE                         1
#define RTOSTWIN_VERSION_STRING                 "0.1.0-stm32-baseline"

#define RTOSTWIN_TELEMETRY_TASK_NAME            "RTOSTwin"
#define RTOSTWIN_TELEMETRY_TASK_STACK_WORDS     512
#define RTOSTWIN_TELEMETRY_TASK_PRIORITY_OFFSET 2
#define RTOSTWIN_TELEMETRY_PERIOD_MS            100

#define RTOSTWIN_PAYLOAD_BUFFER_BYTES           512
#define RTOSTWIN_FRAME_BUFFER_BYTES             540
#define RTOSTWIN_PROFILE_REPORT_INTERVAL        100

#define RTOSTWIN_ENABLE_PROFILING               1
#define RTOSTWIN_ENABLE_COMPAT_API              1

#endif /* RTOSTWIN_CONFIG_H */
