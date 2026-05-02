/**
 * @file uart_dma.h
 * @brief STM32 HAL UART + DMA low-level driver interface.
 *
 * Provides a hardware-specific abstraction for non-blocking DMA-driven
 * UART transmission. Only this file (and uart_dma.c) should ever call
 * STM32 HAL functions directly. All other agent modules go through
 * transport.h instead.
 */

#ifndef RTOSTWIN_UART_DMA_H
#define RTOSTWIN_UART_DMA_H

#include <stdint.h>

/**
 * @brief Initialize USART2 + DMA for telemetry output.
 *
 * Configures USART2 at 115200 8N1 and links it to its DMA stream.
 * Must be called once before any uart_dma_send() calls.
 */
void uart_dma_init(void);

/**
 * @brief Non-blocking transmit via DMA.
 *
 * Hands the buffer to the DMA controller and returns immediately.
 * The caller must ensure the buffer remains valid until the DMA
 * transfer completes (i.e., it must be a static or persistent buffer).
 *
 * @param buf  Pointer to the data to send.
 * @param len  Number of bytes to send.
 * @return  0  on success (DMA transfer started).
 * @return -1  if DMA is already busy with a previous transfer.
 */
int uart_dma_send(const uint8_t *buf, uint16_t len);

/**
 * @brief Check if the DMA UART peripheral is currently busy.
 *
 * @return 1 if busy (transfer in progress), 0 if idle.
 */
int uart_dma_is_busy(void);

#endif /* RTOSTWIN_UART_DMA_H */
