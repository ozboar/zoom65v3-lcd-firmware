#pragma once

#include <stdint.h>

/* The stock upload CRC: CRC-32/MPEG-2 over the data, padded with zeros to a whole number of words */
static inline uint32_t upload_crc(const uint8_t *data, uint32_t length) {
    uint32_t crc = 0xFFFFFFFFu;

    for (uint32_t i = 0; i < ((length + 3u) & ~3u); i++) {
        crc ^= (uint32_t)(i < length ? data[i] : 0) << 24;
        for (uint8_t bit = 0; bit < 8; bit++) {
            crc = crc & 0x80000000u ? (crc << 1) ^ 0x04C11DB7u : crc << 1;
        }
    }
    return crc;
}
