/**
 * @file transport.c
 * @brief Implementation of the transport layer.
 */

#include "transport.h"
#include <stddef.h>

/* 
 * These functions are implemented in the hardware-specific 
 * layer (agent/hal/stm32/uart_dma.c). 
 */
extern int  stm32_uart_send_dma(const uint8_t *data, uint16_t len);
extern void stm32_uart_init(void);

static volatile uint32_t s_tx_drop_count = 0;

void transport_init(void) {
    stm32_uart_init();
}

int transport_send(const uint8_t *packet, uint16_t len) {
    if (packet == NULL || len == 0) {
        return -1;
    }

    /* 
     * Attempt to initiate a DMA transfer.
     * stm32_uart_send_dma returns 0 on success, non-zero if busy.
     */
    if (stm32_uart_send_dma(packet, len) != 0) {
        s_tx_drop_count++;
        return -1;
    }

    return 0;
}

uint32_t transport_get_drop_count(void) {
    return s_tx_drop_count;
}
