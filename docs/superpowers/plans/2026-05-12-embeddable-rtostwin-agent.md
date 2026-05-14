# Embeddable RTOSTwin Agent Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Turn the validated STM32 telemetry agent into a small embeddable C library that another FreeRTOS firmware project can include with `rtostwin_config.h`, `#include "rtostwin.h"`, `rtostwin_init()`, and `rtostwin_start()`.

**Architecture:** Keep the current validated `vnv_final/agent/` implementation in place and add a professional public API layer on top of it. Move the telemetry task ownership into a lifecycle module, keep `StartTelemetryAgent()` as a compatibility wrapper, and make the feature compile out cleanly when `RTOSTWIN_ENABLE=0`. The bridge, wire format, STM32 evidence, and existing validated command paths remain unchanged.

**Tech Stack:** C99, FreeRTOS, STM32 HAL transport backend, existing RTOSTwin C agent modules, host-side GCC smoke tests, STM32CubeIDE integration documentation.

---

## File Structure

### Files to create

- `vnv_final/agent/include/rtostwin.h`
  Public API used by application firmware.
- `vnv_final/agent/include/rtostwin_config_template.h`
  Copyable configuration template for firmware projects.
- `vnv_final/agent/rtostwin.c`
  Lifecycle implementation: init/start/stop/is-running/version and the telemetry task.
- `vnv_final/agent/tests/test_rtostwin_disabled.c`
  Host compile/run test proving disabled mode compiles without FreeRTOS.
- `vnv_final/agent/tests/test_rtostwin_lifecycle.c`
  Host compile/run test proving init/start/stop behavior and compatibility wrapper.
- `vnv_final/agent/README.md`
  Embeddable-library guide for firmware users.
- `docs/guides/embed_rtostwin_in_stm32.md`
  STM32CubeIDE integration guide with include paths, source files, and app calls.

### Files to modify

- `vnv_final/agent/main.c`
  Replace the current telemetry task implementation with a compatibility wrapper that calls the new public API.
- `vnv_final/agent/tests/mocks/task.h`
  Add minimal FreeRTOS task API stubs needed by lifecycle tests.
- `README.md`
  Add a short note that the STM32 agent can now be integrated as an embeddable library.
- `vnv_final/README.md`
  Add a pointer to `agent/README.md`.
- `docs/repository_layout.md`
  Mention that `vnv_final/agent/include/` is the public C integration surface.

### Files to inspect but preserve

- `vnv_final/agent/core/snapshot.c`
- `vnv_final/agent/core/encoder.c`
- `vnv_final/agent/core/framer.c`
- `vnv_final/agent/core/transport.c`
- `vnv_final/agent/hal/stm32/uart_dma.c`
- `agent/core/wire_format.h`

### Verification targets

- `vnv_final/agent/tests/test_rtostwin_disabled.c`
- `vnv_final/agent/tests/test_rtostwin_lifecycle.c`
- existing agent tests:
  - `vnv_final/agent/tests/test_measurement.c`
  - `vnv_final/agent/tests/test_profiler.c`
  - `vnv_final/agent/tests/test_snapshot.c`
  - `vnv_final/agent/tests/test_framer.c`
  - `vnv_final/agent/tests/test_encoder.c`
- existing bridge tests:
  - `vnv_final/bridge/tests`

---

### Task 1: Add Public API And Config Template

**Files:**
- Create: `vnv_final/agent/include/rtostwin.h`
- Create: `vnv_final/agent/include/rtostwin_config_template.h`
- Create: `vnv_final/agent/tests/test_rtostwin_disabled.c`
- Test: host compile/run for disabled mode

- [ ] **Step 1: Create `vnv_final/agent/include/rtostwin_config_template.h`**

Create the file with this full content:

```c
#ifndef RTOSTWIN_CONFIG_H
#define RTOSTWIN_CONFIG_H

/*
 * Copy this file into your firmware project as rtostwin_config.h and adjust the
 * values below. Keep this template in the RTOSTwin repo unchanged.
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
```

- [ ] **Step 2: Create `vnv_final/agent/include/rtostwin.h`**

Create the file with this full content:

```c
#ifndef RTOSTWIN_H
#define RTOSTWIN_H

#include <stdbool.h>
#include <stdint.h>
#include "rtostwin_config.h"

#ifndef RTOSTWIN_ENABLE
#define RTOSTWIN_ENABLE 1
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

#endif

#if RTOSTWIN_ENABLE_COMPAT_API
void StartTelemetryAgent(void);
#endif

#endif /* RTOSTWIN_H */
```

- [ ] **Step 3: Create disabled-mode test config**

Create `vnv_final/agent/tests/rtostwin_config.h` with this full content:

```c
#ifndef RTOSTWIN_CONFIG_H
#define RTOSTWIN_CONFIG_H

#define RTOSTWIN_ENABLE 0
#define RTOSTWIN_ENABLE_COMPAT_API 1
#define RTOSTWIN_VERSION_STRING "test-disabled"

#endif /* RTOSTWIN_CONFIG_H */
```

- [ ] **Step 4: Create `vnv_final/agent/tests/test_rtostwin_disabled.c`**

Create the file with this full content:

```c
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
```

- [ ] **Step 5: Run disabled-mode compile test**

Run:

```powershell
gcc -std=c99 -Wall -Wextra -Werror -Ivnv_final/agent/tests -Ivnv_final/agent/include vnv_final/agent/tests/test_rtostwin_disabled.c -o C:\tmp\test_rtostwin_disabled.exe
```

Expected:

```text
no compiler errors
```

- [ ] **Step 6: Run disabled-mode executable**

Run:

```powershell
C:\tmp\test_rtostwin_disabled.exe
```

Expected:

```text
rtostwin disabled-mode API test PASSED
```

- [ ] **Step 7: Commit public API skeleton**

Run:

```bash
git add vnv_final/agent/include/rtostwin.h vnv_final/agent/include/rtostwin_config_template.h vnv_final/agent/tests/rtostwin_config.h vnv_final/agent/tests/test_rtostwin_disabled.c
git commit -m "feat(agent): add embeddable public API header"
```

---

### Task 2: Extend FreeRTOS Test Mocks For Lifecycle Testing

**Files:**
- Modify: `vnv_final/agent/tests/mocks/task.h`
- Test: host compile for lifecycle test in Task 3

- [ ] **Step 1: Replace `vnv_final/agent/tests/mocks/task.h` with lifecycle-capable mock declarations**

Replace the file with this full content:

```c
/**
 * @file task.h
 * @brief Minimal FreeRTOS task.h stub for PC unit testing.
 */

#ifndef TASK_H
#define TASK_H

#include <stdint.h>
#include "FreeRTOS.h"

typedef void *TaskHandle_t;
typedef void (*TaskFunction_t)(void *);

typedef enum {
    eRunning = 0,
    eReady,
    eBlocked,
    eSuspended,
    eDeleted
} eTaskState;

typedef struct {
    const char  *pcTaskName;
    eTaskState   eCurrentState;
    UBaseType_t  uxCurrentPriority;
    uint16_t     usStackHighWaterMark;
    uint32_t     ulRunTimeCounter;
} TaskStatus_t;

#define tskIDLE_PRIORITY 0U
#define pdMS_TO_TICKS(ms) ((TickType_t)(ms))

TickType_t   xTaskGetTickCount(void);
UBaseType_t  uxTaskGetSystemState(TaskStatus_t *buf, UBaseType_t max, uint32_t *total);
size_t       xPortGetFreeHeapSize(void);
size_t       xPortGetMinimumEverFreeHeapSize(void);

BaseType_t xTaskCreate(TaskFunction_t pxTaskCode,
                       const char * const pcName,
                       uint16_t usStackDepth,
                       void *pvParameters,
                       UBaseType_t uxPriority,
                       TaskHandle_t *pxCreatedTask);

void vTaskDelete(TaskHandle_t xTaskToDelete);
void vTaskDelay(TickType_t xTicksToDelay);

#endif /* TASK_H */
```

- [ ] **Step 2: Verify existing snapshot test still compiles**

Run:

```powershell
gcc -std=c99 -Wall -Wextra -Werror -Ivnv_final/agent/tests/mocks -Ivnv_final/agent/core vnv_final/agent/tests/test_snapshot.c vnv_final/agent/core/snapshot.c -o C:\tmp\test_snapshot.exe
```

Expected:

```text
no compiler errors
```

- [ ] **Step 3: Run existing snapshot test**

Run:

```powershell
C:\tmp\test_snapshot.exe
```

Expected output contains:

```text
All snapshot tests passed
```

- [ ] **Step 4: Commit FreeRTOS mock extension**

Run:

```bash
git add vnv_final/agent/tests/mocks/task.h
git commit -m "test(agent): extend FreeRTOS mocks for lifecycle API"
```

---

### Task 3: Move Telemetry Lifecycle Behind `rtostwin_*` API

**Files:**
- Create: `vnv_final/agent/rtostwin.c`
- Modify: `vnv_final/agent/main.c`
- Create: `vnv_final/agent/tests/test_rtostwin_lifecycle.c`
- Test: lifecycle compile/run test

- [ ] **Step 1: Create `vnv_final/agent/rtostwin.c`**

Create the file with this full content:

```c
/**
 * @file rtostwin.c
 * @brief Embeddable lifecycle entry point for the RTOSTwin telemetry agent.
 */

#include "include/rtostwin.h"

#if RTOSTWIN_ENABLE

#include "FreeRTOS.h"
#include "task.h"
#include "../../agent/core/wire_format.h"
#include "core/snapshot.h"
#include "core/encoder.h"
#include "core/framer.h"
#include "core/measurement.h"
#include "core/transport.h"
#include "core/profiler.h"
#include <stdio.h>

#ifndef RTOSTWIN_TELEMETRY_TASK_NAME
#define RTOSTWIN_TELEMETRY_TASK_NAME "RTOSTwin"
#endif

#ifndef RTOSTWIN_TELEMETRY_TASK_STACK_WORDS
#define RTOSTWIN_TELEMETRY_TASK_STACK_WORDS 512
#endif

#ifndef RTOSTWIN_TELEMETRY_TASK_PRIORITY_OFFSET
#define RTOSTWIN_TELEMETRY_TASK_PRIORITY_OFFSET 2
#endif

#ifndef RTOSTWIN_TELEMETRY_PERIOD_MS
#define RTOSTWIN_TELEMETRY_PERIOD_MS 100
#endif

#ifndef RTOSTWIN_PAYLOAD_BUFFER_BYTES
#define RTOSTWIN_PAYLOAD_BUFFER_BYTES 512
#endif

#ifndef RTOSTWIN_FRAME_BUFFER_BYTES
#define RTOSTWIN_FRAME_BUFFER_BYTES 540
#endif

#ifndef RTOSTWIN_PROFILE_REPORT_INTERVAL
#define RTOSTWIN_PROFILE_REPORT_INTERVAL 100
#endif

#ifndef RTOSTWIN_ENABLE_PROFILING
#define RTOSTWIN_ENABLE_PROFILING 1
#endif

static full_snapshot_t s_current_snapshot;
static uint8_t s_payload_buffer[RTOSTWIN_PAYLOAD_BUFFER_BYTES];
static uint8_t s_framed_buffer[RTOSTWIN_FRAME_BUFFER_BYTES];
static measurement_stats_t s_cycle_stats;
static TaskHandle_t s_telemetry_task_handle;
static bool s_initialised;

static void rtostwin_telemetry_task(void *pvParameters)
{
    (void)pvParameters;

    profiler_stats_t snap_stats = {0};
    uint32_t loop_count = 0u;

    while (1) {
        uint32_t cycle_start = profiler_start();

        uint32_t t_start = profiler_start();
        snapshot_capture(&s_current_snapshot);
        uint32_t t_elapsed = profiler_stop(t_start);
        profiler_record(&snap_stats, t_elapsed);

        bool force_keyframe = ((loop_count % WF_KEYFRAME_INTERVAL) == 0u);
        uint16_t enc_len = encoder_encode(&s_current_snapshot,
                                          s_payload_buffer,
                                          sizeof(s_payload_buffer),
                                          force_keyframe);

        if (enc_len > 0u) {
            uint8_t pkt_type = encoder_last_was_keyframe() ? WF_TYPE_KEYFRAME : WF_TYPE_DELTA;
            uint16_t frame_len = frame_packet(s_payload_buffer,
                                              enc_len,
                                              pkt_type,
                                              s_current_snapshot.sequence_num,
                                              s_current_snapshot.timestamp_ticks,
                                              s_framed_buffer,
                                              sizeof(s_framed_buffer));

            if (frame_len > 0u) {
                transport_send(s_framed_buffer, frame_len);
            }
        }

        measurement_record(&s_cycle_stats, profiler_stop(cycle_start));

#if RTOSTWIN_ENABLE_PROFILING
        if ((++loop_count % RTOSTWIN_PROFILE_REPORT_INTERVAL) == 0u) {
            profiler_report(&snap_stats, "snapshot_capture");
            printf("[MEASURE] telemetry_cycle min=%lu max=%lu mean=%lu cycles samples=%lu\n",
                   (unsigned long)s_cycle_stats.min_cycles,
                   (unsigned long)s_cycle_stats.max_cycles,
                   (unsigned long)measurement_mean_cycles(&s_cycle_stats),
                   (unsigned long)s_cycle_stats.sample_count);
            measurement_reset(&s_cycle_stats);
        }
#else
        ++loop_count;
#endif

        vTaskDelay(pdMS_TO_TICKS(RTOSTWIN_TELEMETRY_PERIOD_MS));
    }
}

rtostwin_status_t rtostwin_init(void)
{
    if (s_initialised) {
        return RTOSTWIN_STATUS_OK;
    }

    profiler_init();
    snapshot_init();
    encoder_init();
    transport_init();
    measurement_reset(&s_cycle_stats);

    s_initialised = true;
    return RTOSTWIN_STATUS_OK;
}

rtostwin_status_t rtostwin_start(void)
{
    if (!s_initialised) {
        rtostwin_status_t init_status = rtostwin_init();
        if (init_status != RTOSTWIN_STATUS_OK) {
            return init_status;
        }
    }

    if (s_telemetry_task_handle != NULL) {
        return RTOSTWIN_STATUS_ALREADY_STARTED;
    }

    BaseType_t created = xTaskCreate(rtostwin_telemetry_task,
                                     RTOSTWIN_TELEMETRY_TASK_NAME,
                                     RTOSTWIN_TELEMETRY_TASK_STACK_WORDS,
                                     NULL,
                                     tskIDLE_PRIORITY + RTOSTWIN_TELEMETRY_TASK_PRIORITY_OFFSET,
                                     &s_telemetry_task_handle);

    if (created != pdTRUE) {
        s_telemetry_task_handle = NULL;
        return RTOSTWIN_STATUS_ERROR;
    }

    return RTOSTWIN_STATUS_OK;
}

rtostwin_status_t rtostwin_stop(void)
{
    if (s_telemetry_task_handle == NULL) {
        return RTOSTWIN_STATUS_OK;
    }

    vTaskDelete(s_telemetry_task_handle);
    s_telemetry_task_handle = NULL;
    return RTOSTWIN_STATUS_OK;
}

bool rtostwin_is_running(void)
{
    return s_telemetry_task_handle != NULL;
}

const char *rtostwin_version(void)
{
    return RTOSTWIN_VERSION_STRING;
}

#if RTOSTWIN_ENABLE_COMPAT_API
void StartTelemetryAgent(void)
{
    (void)rtostwin_init();
    (void)rtostwin_start();
}
#endif

#endif /* RTOSTWIN_ENABLE */
```

- [ ] **Step 2: Replace `vnv_final/agent/main.c` with compatibility-only wrapper**

Replace the file with this full content:

```c
/**
 * @file main.c
 * @brief Backward-compatible entry point for existing RTOSTwin examples.
 */

#include "include/rtostwin.h"

#if RTOSTWIN_ENABLE && !RTOSTWIN_ENABLE_COMPAT_API
void StartTelemetryAgent(void)
{
    (void)rtostwin_init();
    (void)rtostwin_start();
}
#endif
```

- [ ] **Step 3: Create lifecycle test config**

Create `vnv_final/agent/tests/rtostwin_lifecycle_config/rtostwin_config.h` with this full content:

```c
#ifndef RTOSTWIN_CONFIG_H
#define RTOSTWIN_CONFIG_H

#define RTOSTWIN_ENABLE 1
#define RTOSTWIN_ENABLE_COMPAT_API 1
#define RTOSTWIN_VERSION_STRING "test-lifecycle"
#define RTOSTWIN_TELEMETRY_TASK_NAME "RTOSTwinTest"
#define RTOSTWIN_TELEMETRY_TASK_STACK_WORDS 256
#define RTOSTWIN_TELEMETRY_TASK_PRIORITY_OFFSET 3
#define RTOSTWIN_TELEMETRY_PERIOD_MS 100
#define RTOSTWIN_PAYLOAD_BUFFER_BYTES 512
#define RTOSTWIN_FRAME_BUFFER_BYTES 540
#define RTOSTWIN_PROFILE_REPORT_INTERVAL 100
#define RTOSTWIN_ENABLE_PROFILING 0

#endif /* RTOSTWIN_CONFIG_H */
```

- [ ] **Step 4: Create `vnv_final/agent/tests/test_rtostwin_lifecycle.c`**

Create the file with this full content:

```c
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "../include/rtostwin.h"
#include "../core/snapshot.h"
#include "../core/profiler.h"
#include "../core/measurement.h"

static unsigned profiler_init_calls;
static unsigned snapshot_init_calls;
static unsigned encoder_init_calls;
static unsigned transport_init_calls;
static unsigned x_task_create_calls;
static unsigned v_task_delete_calls;
static TaskHandle_t created_task_handle = (TaskHandle_t)0x1234;

void profiler_init(void) { profiler_init_calls++; }
uint32_t profiler_start(void) { return 10U; }
uint32_t profiler_stop(uint32_t start_cycles) { return start_cycles + 1U; }
void profiler_record(profiler_stats_t *stats, uint32_t elapsed_cycles)
{
    (void)elapsed_cycles;
    stats->call_count++;
}
void profiler_report(const profiler_stats_t *stats, const char *label)
{
    (void)stats;
    (void)label;
}

void snapshot_init(void) { snapshot_init_calls++; }
void snapshot_capture(full_snapshot_t *out)
{
    if (out != NULL) {
        out->sequence_num++;
        out->timestamp_ticks++;
    }
}

void encoder_init(void) { encoder_init_calls++; }
uint16_t encoder_encode(const full_snapshot_t *current,
                        uint8_t *out_buf,
                        uint16_t out_buf_size,
                        bool force_keyframe)
{
    (void)current;
    (void)force_keyframe;
    if (out_buf_size < 1U) {
        return 0U;
    }
    out_buf[0] = 0xA5U;
    return 1U;
}
bool encoder_last_was_keyframe(void) { return true; }

uint16_t frame_packet(const uint8_t *payload,
                      uint16_t payload_len,
                      uint8_t packet_type,
                      uint16_t sequence_num,
                      uint32_t timestamp_ticks,
                      uint8_t *out_buf,
                      uint16_t out_buf_size)
{
    (void)payload;
    (void)payload_len;
    (void)packet_type;
    (void)sequence_num;
    (void)timestamp_ticks;
    if (out_buf_size < 1U) {
        return 0U;
    }
    out_buf[0] = 0x5AU;
    return 1U;
}

void transport_init(void) { transport_init_calls++; }
int transport_send(const uint8_t *packet, uint16_t len)
{
    (void)packet;
    return len > 0U ? 0 : -1;
}
uint32_t transport_get_drop_count(void) { return 0U; }

TickType_t xTaskGetTickCount(void) { return 0U; }
UBaseType_t uxTaskGetSystemState(TaskStatus_t *buf, UBaseType_t max, uint32_t *total)
{
    (void)buf;
    (void)max;
    if (total != NULL) {
        *total = 0U;
    }
    return 0U;
}
size_t xPortGetFreeHeapSize(void) { return 0U; }
size_t xPortGetMinimumEverFreeHeapSize(void) { return 0U; }

BaseType_t xTaskCreate(TaskFunction_t pxTaskCode,
                       const char * const pcName,
                       uint16_t usStackDepth,
                       void *pvParameters,
                       UBaseType_t uxPriority,
                       TaskHandle_t *pxCreatedTask)
{
    (void)pxTaskCode;
    (void)pvParameters;

    assert(strcmp(pcName, "RTOSTwinTest") == 0);
    assert(usStackDepth == 256U);
    assert(uxPriority == (tskIDLE_PRIORITY + 3U));

    x_task_create_calls++;
    *pxCreatedTask = created_task_handle;
    return pdTRUE;
}

void vTaskDelete(TaskHandle_t xTaskToDelete)
{
    assert(xTaskToDelete == created_task_handle);
    v_task_delete_calls++;
}

void vTaskDelay(TickType_t xTicksToDelay)
{
    (void)xTicksToDelay;
}

int main(void)
{
    assert(rtostwin_is_running() == false);
    assert(strcmp(rtostwin_version(), "test-lifecycle") == 0);

    assert(rtostwin_init() == RTOSTWIN_STATUS_OK);
    assert(profiler_init_calls == 1U);
    assert(snapshot_init_calls == 1U);
    assert(encoder_init_calls == 1U);
    assert(transport_init_calls == 1U);

    assert(rtostwin_init() == RTOSTWIN_STATUS_OK);
    assert(profiler_init_calls == 1U);

    assert(rtostwin_start() == RTOSTWIN_STATUS_OK);
    assert(rtostwin_is_running() == true);
    assert(x_task_create_calls == 1U);

    assert(rtostwin_start() == RTOSTWIN_STATUS_ALREADY_STARTED);
    assert(x_task_create_calls == 1U);

    assert(rtostwin_stop() == RTOSTWIN_STATUS_OK);
    assert(rtostwin_is_running() == false);
    assert(v_task_delete_calls == 1U);

    StartTelemetryAgent();
    assert(rtostwin_is_running() == true);
    assert(x_task_create_calls == 2U);

    printf("rtostwin lifecycle API test PASSED\n");
    return 0;
}
```

- [ ] **Step 5: Compile lifecycle test**

Run:

```powershell
gcc -std=c99 -Wall -Wextra -Werror -Ivnv_final/agent/tests/rtostwin_lifecycle_config -Ivnv_final/agent/tests/mocks -Ivnv_final/agent/include -Ivnv_final/agent -Ivnv_final/agent/core -Iagent/core vnv_final/agent/tests/test_rtostwin_lifecycle.c vnv_final/agent/rtostwin.c vnv_final/agent/core/measurement.c -o C:\tmp\test_rtostwin_lifecycle.exe
```

Expected:

```text
no compiler errors
```

- [ ] **Step 6: Run lifecycle test**

Run:

```powershell
C:\tmp\test_rtostwin_lifecycle.exe
```

Expected:

```text
rtostwin lifecycle API test PASSED
```

- [ ] **Step 7: Commit lifecycle API implementation**

Run:

```bash
git add vnv_final/agent/rtostwin.c vnv_final/agent/main.c vnv_final/agent/tests/rtostwin_lifecycle_config/rtostwin_config.h vnv_final/agent/tests/test_rtostwin_lifecycle.c
git commit -m "feat(agent): add embeddable lifecycle API"
```

---

### Task 4: Update Example Applications To Use Public API

**Files:**
- Modify: `vnv_final/examples/blinky_twin/main.c`
- Modify: `vnv_final/examples/sensor_system/main.c`
- Test: text audit for old extern-only usage

- [ ] **Step 1: Update `vnv_final/examples/blinky_twin/main.c` include block**

Replace:

```c
extern void StartTelemetryAgent(void);
```

with:

```c
#include "../../agent/include/rtostwin.h"
```

- [ ] **Step 2: Update `vnv_final/examples/blinky_twin/main.c` startup call**

Replace:

```c
StartTelemetryAgent();
```

with:

```c
(void)rtostwin_init();
(void)rtostwin_start();
```

- [ ] **Step 3: Update `vnv_final/examples/sensor_system/main.c` include block**

Replace:

```c
extern void StartTelemetryAgent(void);
```

with:

```c
#include "../../agent/include/rtostwin.h"
```

- [ ] **Step 4: Update `vnv_final/examples/sensor_system/main.c` startup call**

Replace:

```c
StartTelemetryAgent();
```

with:

```c
(void)rtostwin_init();
(void)rtostwin_start();
```

- [ ] **Step 5: Audit remaining compatibility API use**

Run:

```powershell
rg -n "extern void StartTelemetryAgent|StartTelemetryAgent\\(" vnv_final
```

Expected:

```text
vnv_final\agent\rtostwin.c:...:void StartTelemetryAgent(void)
```

It is acceptable if docs mention `StartTelemetryAgent()` as a legacy compatibility API.

- [ ] **Step 6: Commit example migration**

Run:

```bash
git add vnv_final/examples/blinky_twin/main.c vnv_final/examples/sensor_system/main.c
git commit -m "docs(examples): use rtostwin public lifecycle API"
```

---

### Task 5: Document Embedding In Real STM32 Projects

**Files:**
- Create: `vnv_final/agent/README.md`
- Create: `docs/guides/embed_rtostwin_in_stm32.md`
- Modify: `README.md`
- Modify: `vnv_final/README.md`
- Modify: `docs/repository_layout.md`

- [ ] **Step 1: Create `vnv_final/agent/README.md`**

Create the file with this full content:

```md
# RTOSTwin Embeddable Agent

This folder contains the MCU-side telemetry agent as an embeddable FreeRTOS
library.

## Public API

Application firmware should include:

```c
#include "rtostwin.h"
```

Then start telemetry after board peripherals and the RTOS are ready:

```c
(void)rtostwin_init();
(void)rtostwin_start();
```

The legacy `StartTelemetryAgent()` symbol is retained as a compatibility wrapper
for older examples.

## Required Project Config

Copy:

```text
vnv_final/agent/include/rtostwin_config_template.h
```

into your firmware project as:

```text
rtostwin_config.h
```

Then place that file on the compiler include path before `rtostwin.h`.

## Minimum Source Files

For the validated STM32F401RE UART-DMA path, include these files in your
firmware build:

```text
vnv_final/agent/rtostwin.c
vnv_final/agent/core/snapshot.c
vnv_final/agent/core/encoder.c
vnv_final/agent/core/framer.c
vnv_final/agent/core/measurement.c
vnv_final/agent/core/profiler.c
vnv_final/agent/core/transport.c
vnv_final/agent/freertos/hooks.c
vnv_final/agent/hal/stm32/dwt.c
vnv_final/agent/hal/stm32/uart_dma.c
```

The canonical wire-format header remains:

```text
agent/core/wire_format.h
```

## Disable Mode

Set this in `rtostwin_config.h`:

```c
#define RTOSTWIN_ENABLE 0
```

The public API will compile to no-op status-returning inline functions.
```

- [ ] **Step 2: Create `docs/guides/embed_rtostwin_in_stm32.md`**

Create the file with this full content:

```md
# Embedding RTOSTwin In An STM32CubeIDE Project

This guide shows how to add the validated RTOSTwin STM32 telemetry agent to an
existing FreeRTOS STM32CubeIDE project.

## 1. Add Include Paths

Add these include paths to the STM32CubeIDE project:

```text
<repo>/vnv_final/agent/include
<repo>/vnv_final/agent
<repo>/vnv_final/agent/core
<repo>/vnv_final/agent/hal/stm32
<repo>/agent/core
<your-project>/Core/Inc
```

## 2. Add Source Files

Add these source files to the build:

```text
vnv_final/agent/rtostwin.c
vnv_final/agent/core/snapshot.c
vnv_final/agent/core/encoder.c
vnv_final/agent/core/framer.c
vnv_final/agent/core/measurement.c
vnv_final/agent/core/profiler.c
vnv_final/agent/core/transport.c
vnv_final/agent/freertos/hooks.c
vnv_final/agent/hal/stm32/dwt.c
vnv_final/agent/hal/stm32/uart_dma.c
```

## 3. Add `rtostwin_config.h`

Copy:

```text
vnv_final/agent/include/rtostwin_config_template.h
```

into your project as:

```text
Core/Inc/rtostwin_config.h
```

Recommended STM32 baseline values:

```c
#define RTOSTWIN_ENABLE                         1
#define RTOSTWIN_VERSION_STRING                 "0.1.0-stm32-baseline"
#define RTOSTWIN_TELEMETRY_TASK_STACK_WORDS     512
#define RTOSTWIN_TELEMETRY_TASK_PRIORITY_OFFSET 2
#define RTOSTWIN_TELEMETRY_PERIOD_MS            100
#define RTOSTWIN_PAYLOAD_BUFFER_BYTES           512
#define RTOSTWIN_FRAME_BUFFER_BYTES             540
#define RTOSTWIN_ENABLE_PROFILING               1
```

## 4. Start The Agent

In your application startup code:

```c
#include "rtostwin.h"

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_DMA_Init();
    MX_USART2_UART_Init();

    (void)rtostwin_init();

    osKernelInitialize();

    (void)rtostwin_start();

    osKernelStart();

    while (1) {
    }
}
```

If your project creates tasks before `osKernelStart()`, calling
`rtostwin_start()` before `osKernelStart()` is acceptable because it creates the
RTOSTwin FreeRTOS task and lets the scheduler run it later.

## 5. Collect Telemetry

On the host machine:

```powershell
cd D:\digital_twin\vnv_final
.\.venv\Scripts\Activate.ps1
python bridge/main.py --port COM11 --baud 115200 --device-id my-stm32-device
```

Then open:

```text
http://localhost:8000/metrics
```

## 6. Disable For Production Variants

To compile the public API out without removing call sites:

```c
#define RTOSTWIN_ENABLE 0
```

`rtostwin_init()` and `rtostwin_start()` will return
`RTOSTWIN_STATUS_DISABLED`.
```

- [ ] **Step 3: Add root README pointer**

In `README.md`, add this paragraph under `## Quick Start` before the numbered
steps:

```md
For embedding the agent into another STM32 FreeRTOS firmware project, use the
public C API in `vnv_final/agent/include/rtostwin.h` and the guide at
[docs/guides/embed_rtostwin_in_stm32.md](docs/guides/embed_rtostwin_in_stm32.md).
```

- [ ] **Step 4: Add subtree README pointer**

In `vnv_final/README.md`, add this bullet under `## Start Here`:

```md
- [Embeddable agent guide](agent/README.md)
```

- [ ] **Step 5: Add repository layout note**

In `docs/repository_layout.md`, add this bullet under the `vnv_final/` entry:

```md
  The public C integration surface is `vnv_final/agent/include/rtostwin.h`.
```

- [ ] **Step 6: Commit embedding docs**

Run:

```bash
git add vnv_final/agent/README.md docs/guides/embed_rtostwin_in_stm32.md README.md vnv_final/README.md docs/repository_layout.md
git commit -m "docs(agent): document embeddable STM32 integration"
```

---

### Task 6: Full Verification And Closure

**Files:**
- Verify only

- [ ] **Step 1: Run new disabled-mode test**

Run:

```powershell
gcc -std=c99 -Wall -Wextra -Werror -Ivnv_final/agent/tests -Ivnv_final/agent/include vnv_final/agent/tests/test_rtostwin_disabled.c -o C:\tmp\test_rtostwin_disabled.exe
C:\tmp\test_rtostwin_disabled.exe
```

Expected output contains:

```text
rtostwin disabled-mode API test PASSED
```

- [ ] **Step 2: Run new lifecycle test**

Run:

```powershell
gcc -std=c99 -Wall -Wextra -Werror -Ivnv_final/agent/tests/rtostwin_lifecycle_config -Ivnv_final/agent/tests/mocks -Ivnv_final/agent/include -Ivnv_final/agent -Ivnv_final/agent/core -Iagent/core vnv_final/agent/tests/test_rtostwin_lifecycle.c vnv_final/agent/rtostwin.c vnv_final/agent/core/measurement.c -o C:\tmp\test_rtostwin_lifecycle.exe
C:\tmp\test_rtostwin_lifecycle.exe
```

Expected output contains:

```text
rtostwin lifecycle API test PASSED
```

- [ ] **Step 3: Run existing agent unit tests**

Run:

```powershell
gcc -std=c99 -Wall -Wextra -Werror -Ivnv_final/agent/core vnv_final/agent/tests/test_measurement.c vnv_final/agent/core/measurement.c -o C:\tmp\test_measurement.exe
C:\tmp\test_measurement.exe

gcc -std=c99 -Wall -Wextra -Werror vnv_final/agent/tests/test_profiler.c -o C:\tmp\test_profiler.exe
C:\tmp\test_profiler.exe

gcc -std=c99 -Wall -Wextra -Werror -Ivnv_final/agent/tests/mocks -Ivnv_final/agent/core vnv_final/agent/tests/test_snapshot.c vnv_final/agent/core/snapshot.c -o C:\tmp\test_snapshot.exe
C:\tmp\test_snapshot.exe
```

Expected:

```text
each executable exits with code 0
```

- [ ] **Step 4: Run bridge regression tests**

Run:

```powershell
D:\digital_twin\vnv_final\.venv\Scripts\python.exe -m pytest D:\digital_twin\vnv_final\bridge\tests -q
```

Expected:

```text
15 passed
```

- [ ] **Step 5: Audit public API references**

Run:

```powershell
rg -n "rtostwin_init|rtostwin_start|rtostwin_config|StartTelemetryAgent" vnv_final README.md docs
```

Expected:

```text
public API docs and examples reference rtostwin_init/rtostwin_start
StartTelemetryAgent appears only in compatibility implementation or compatibility notes
```

- [ ] **Step 6: Update graphify**

Run:

```powershell
graphify update .
```

Expected output contains:

```text
Code graph updated
```

- [ ] **Step 7: Commit final verification state**

Run:

```bash
git add vnv_final/agent include docs README.md
git status --short
git commit -m "feat(agent): package RTOSTwin as embeddable FreeRTOS library"
```

If `git status --short` shows unrelated user files, stage only the files created or modified by this plan:

```bash
git add vnv_final/agent/include/rtostwin.h
git add vnv_final/agent/include/rtostwin_config_template.h
git add vnv_final/agent/rtostwin.c
git add vnv_final/agent/main.c
git add vnv_final/agent/tests/test_rtostwin_disabled.c
git add vnv_final/agent/tests/test_rtostwin_lifecycle.c
git add vnv_final/agent/tests/rtostwin_config.h
git add vnv_final/agent/tests/rtostwin_lifecycle_config/rtostwin_config.h
git add vnv_final/agent/tests/mocks/task.h
git add vnv_final/examples/blinky_twin/main.c
git add vnv_final/examples/sensor_system/main.c
git add vnv_final/agent/README.md
git add docs/guides/embed_rtostwin_in_stm32.md
git add README.md
git add vnv_final/README.md
git add docs/repository_layout.md
git commit -m "feat(agent): package RTOSTwin as embeddable FreeRTOS library"
```

---

## Self-Review

Spec coverage:

- Small include/API surface: covered by Task 1 and Task 3.
- Compile-time enable/disable directive: covered by `RTOSTWIN_ENABLE` in Task 1 and disabled test.
- Reusable deployment into another STM32 project: covered by Task 5.
- Preserve validated STM32 path: covered by compatibility wrapper and full verification in Task 6.
- Keep telemetry collection path through existing bridge: covered by the guide in Task 5.

Placeholder scan:

- The plan contains no `TBD`, no `TODO`, and no unexpanded "write tests" step.
- Every new source/documentation file has full content.
- Every verification step includes exact commands and expected outputs.

Type consistency:

- Public API names are consistently `rtostwin_init`, `rtostwin_start`,
  `rtostwin_stop`, `rtostwin_is_running`, and `rtostwin_version`.
- Status enum values are consistently prefixed `RTOSTWIN_STATUS_`.
- Compatibility wrapper remains `StartTelemetryAgent()`.
