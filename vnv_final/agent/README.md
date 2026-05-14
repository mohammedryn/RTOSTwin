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

For the validated in-repo examples, `vnv_final/agent/include/rtostwin_config.h`
provides the baseline defaults. External firmware should own its own copied
configuration file instead of editing the repo-local default.

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
