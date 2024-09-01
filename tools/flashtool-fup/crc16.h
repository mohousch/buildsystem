#ifndef __CRC16_H__
#define __CRC16_H__

#include <stdint.h>

uint16_t crc16(uint16_t crc, const uint8_t *buffer, uint32_t len);

#endif /* __CRC16_H__ */
