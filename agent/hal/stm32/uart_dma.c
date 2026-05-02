/**
 * @file uart_dma.c
 * @brief Hardware Abstraction Layer for STM32 UART DMA.
 */

#include "stm32f4xx_hal.h"
#include <stdint.h>

/* 
 * The UART handle is typically defined and initialized by 
 * STM32CubeMX in main.c. We use it here to trigger DMA.
 */
extern UART_HandleTypeDef huart2;

void stm32_uart_init(void) {
    /* 
     * Most UART and DMA initialization is performed by 
     * generated code in main.c. If we needed specific 
     * transport-only setup, we would put it here.
     */
}

/**
 * @brief Initiates a DMA UART transmission.
 * 
 * @param data Pointer to the buffer to send.
 * @param len Length of the buffer.
 * @return int 0 on success, -1 if the hardware is busy.
 */
int stm32_uart_send_dma(const uint8_t *data, uint16_t len) {
    /* 
     * Verify the UART is ready to transmit. 
     * If a DMA transfer is already active, HAL_UART_GetState will 
     * return HAL_UART_STATE_BUSY_TX.
     */
    HAL_UART_StateTypeDef state = HAL_UART_GetState(&huart2);
    
    if (state == HAL_UART_STATE_BUSY_TX || state == HAL_UART_STATE_BUSY_TX_RX) {
        return -1;
    }

    /* 
     * Trigger the DMA transfer.
     * Note: In a production environment, you should ensure 'data' 
     * remains valid until the transfer is complete (CpltCallback).
     */
    if (HAL_UART_Transmit_DMA(&huart2, (uint8_t*)data, len) != HAL_OK) {
        return -1;
    }

    return 0;
}
