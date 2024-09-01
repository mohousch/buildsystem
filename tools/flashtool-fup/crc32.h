#ifndef CRC32_H_
#define CRC32_H_

#include <stdint.h>

uint32_t crc32(uint32_t crc, uint8_t *buf, uint32_t size);

#endif /* CRC32_H_ */
