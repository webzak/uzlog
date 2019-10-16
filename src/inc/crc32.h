#ifndef _CRC32_H_
#define _CRC32_H_
#include <stdlib.h>
#include <stdint.h>

uint32_t crc32(uint32_t crc, const void *buf, size_t size);

#endif
