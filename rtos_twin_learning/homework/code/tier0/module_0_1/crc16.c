/*
 * Module 0.1 — Assignment 3: crc16.c
 *
 * OBJECTIVE:
 *   Implement CRC-16-CCITT — the exact error detection used in RTOSTwin packets.
 *
 * INSTRUCTIONS:
 *   gcc -Wall -Wextra -std=c99 -o crc16 crc16.c
 *   ./crc16
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>

/*
 * TODO: Implement CRC-16-CCITT
 *
 * Algorithm:
 *   1. Initial CRC value = 0xFFFF
 *   2. For each byte in data:
 *      a. XOR the byte into the upper 8 bits of CRC: crc ^= (uint16_t)byte << 8
 *      b. For each of the 8 bits:
 *         - If the MSB (bit 15) of CRC is set:
 *           CRC = (CRC << 1) ^ 0x1021
 *         - Otherwise:
 *           CRC = CRC << 1
 *   3. Return final CRC value
 *
 * Polynomial: 0x1021 (x^16 + x^12 + x^5 + 1)
 */
uint16_t crc16_ccitt(const uint8_t *data, uint16_t length) {
    // YOUR CODE HERE
    return 0;
}


int main(void) {
    printf("=== CRC-16-CCITT Exercise ===\n\n");
    
    // Test 1: "Hello"
    const char *test1 = "Hello";
    uint16_t crc1 = crc16_ccitt((const uint8_t *)test1, strlen(test1));
    printf("CRC of \"Hello\" = 0x%04X\n", crc1);
    // TODO: Verify against a known-good CRC-16-CCITT calculator online
    
    // Test 2: All zeros
    uint8_t zeros[5] = {0, 0, 0, 0, 0};
    uint16_t crc2 = crc16_ccitt(zeros, 5);
    printf("CRC of {0,0,0,0,0} = 0x%04X\n", crc2);
    
    // Test 3: Empty data
    uint16_t crc3 = crc16_ccitt(NULL, 0);
    printf("CRC of empty = 0x%04X (should be 0xFFFF)\n", crc3);
    
    // Test 4: Verify integrity
    printf("\n=== Integrity Verification ===\n\n");
    
    // Simulate a packet: data + CRC appended
    uint8_t packet[7];  // 5 bytes data + 2 bytes CRC
    memcpy(packet, "Hello", 5);
    
    // Calculate CRC over data
    uint16_t packet_crc = crc16_ccitt(packet, 5);
    packet[5] = (uint8_t)(packet_crc >> 8);    // CRC high byte
    packet[6] = (uint8_t)(packet_crc & 0xFF);  // CRC low byte
    
    // Receiver: recompute CRC and compare
    uint16_t verify_crc = crc16_ccitt(packet, 5);
    uint16_t received_crc = ((uint16_t)packet[5] << 8) | packet[6];
    
    if (verify_crc == received_crc) {
        printf("Packet integrity: PASS ✓\n");
    } else {
        printf("Packet integrity: FAIL ✗\n");
    }
    
    // Corrupt one byte and re-verify
    packet[2] ^= 0x01;  // Flip one bit in 'l' → different character
    verify_crc = crc16_ccitt(packet, 5);
    
    if (verify_crc == received_crc) {
        printf("Corrupted packet:  FAIL (should have detected!) ✗\n");
    } else {
        printf("Corrupted packet:  Detected corruption ✓\n");
    }
    
    return 0;
}
