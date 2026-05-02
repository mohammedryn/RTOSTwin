#include <stdint.h>
#include <stdio.h>
#include "agent/core/framer.h"

int main(void)
{
    const uint8_t p[] = {0x10,0x20,0x30};
    uint8_t out[64] = {0};
    uint16_t n = frame_packet(p, 3U, out, sizeof(out));
    uint16_t crc = crc16_ccitt((const uint8_t*)"123456789", 9U);
    printf("LEN=%u TYPE=%u CRCVEC=0x%04X TS=%u\n", n, out[3], crc, (unsigned)(out[6] | (out[7]<<8) | (out[8]<<16) | (out[9]<<24)));
    return 0;
}
