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

The repo also ships `vnv_final/agent/include/rtostwin_config.h` for in-repo
examples, but real firmware integrations should copy and own their project-local
config file.

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
