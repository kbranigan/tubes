
#ifndef BLOCK_VARINT_H
#define BLOCK_VARINT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint64_t decode_unsigned_varint(const uint8_t * const data, int * decoded_bytes);
int64_t decode_signed_varint(const uint8_t * const data, int * decoded_bytes);
int encode_unsigned_varint(uint8_t * const buffer, uint64_t value);
int encode_signed_varint(uint8_t * const buffer, int64_t value);

#ifdef __cplusplus
}
#endif

#endif
