#ifndef RTOSTWIN_H
#define RTOSTWIN_H

#include <stdbool.h>
#include <stdint.h>

#include <rtostwin_config.h>

#ifndef RTOSTWIN_ENABLE
#define RTOSTWIN_ENABLE 1
#endif

#ifndef RTOSTWIN_ENABLE_COMPAT_API
#define RTOSTWIN_ENABLE_COMPAT_API 1
#endif

#ifndef RTOSTWIN_VERSION_STRING
#define RTOSTWIN_VERSION_STRING "0.1.0"
#endif

typedef enum {
    RTOSTWIN_STATUS_OK = 0,
    RTOSTWIN_STATUS_DISABLED = 1,
    RTOSTWIN_STATUS_ALREADY_STARTED = 2,
    RTOSTWIN_STATUS_ERROR = 3
} rtostwin_status_t;

#if RTOSTWIN_ENABLE

rtostwin_status_t rtostwin_init(void);
rtostwin_status_t rtostwin_start(void);
rtostwin_status_t rtostwin_stop(void);
bool rtostwin_is_running(void);
const char *rtostwin_version(void);

#if RTOSTWIN_ENABLE_COMPAT_API
void StartTelemetryAgent(void);
#endif

#else

static inline rtostwin_status_t rtostwin_init(void)
{
    return RTOSTWIN_STATUS_DISABLED;
}

static inline rtostwin_status_t rtostwin_start(void)
{
    return RTOSTWIN_STATUS_DISABLED;
}

static inline rtostwin_status_t rtostwin_stop(void)
{
    return RTOSTWIN_STATUS_DISABLED;
}

static inline bool rtostwin_is_running(void)
{
    return false;
}

static inline const char *rtostwin_version(void)
{
    return RTOSTWIN_VERSION_STRING;
}

#if RTOSTWIN_ENABLE_COMPAT_API
static inline void StartTelemetryAgent(void)
{
}
#endif

#endif /* RTOSTWIN_ENABLE */

#endif /* RTOSTWIN_H */
