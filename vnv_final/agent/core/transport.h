/**
 * @file transport.h
 * @brief Transport layer interface for STM32 DMA UART.
 */

#ifndef AGENT_CORE_TRANSPORT_H
#define AGENT_CORE_TRANSPORT_H

#include <stdint.h>

/**
 * @brief Initialize the UART DMA transport.
 * 
 * Sets up USART2 with DMA support at 115200 baud.
 */
void transport_init(void);

/**
 * @brief Send a packet non-blocking via DMA.
 * 
 * If a DMA transfer is already in progress, the packet is 
 * discarded and a drop counter is incremented.
 * 
 * @param packet Pointer to the framed packet.
 * @param len Length of the packet in bytes.
 * @return int 0 on success, -1 if DMA is busy (packet dropped).
 */
int transport_send(const uint8_t *packet, uint16_t len);

/**
 * @brief Get the count of packets dropped due to DMA being busy.
 */
uint32_t transport_get_drop_count(void);

#endif /* AGENT_CORE_TRANSPORT_H */
