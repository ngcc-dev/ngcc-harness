#ifndef WEAVER_BCH_H
#define WEAVER_BCH_H
#include <stdint.h>

//BCH parameter struct
struct bch_control {
	unsigned int    m;
	unsigned int    n;
	unsigned int    t;
	unsigned int    ecc_bits;
	unsigned int    ecc_bytes;
	unsigned int    ecc_words;
};
// ==========================================
// 高位 BCH 接口
// ==========================================
void encode_bch_high(const uint8_t *data, unsigned int len, uint8_t *ecc);
int  decode_bch_high(uint8_t *data, unsigned int len, const uint8_t *recv_ecc);
void encode_bch_high_nibbles(const unsigned char *data, unsigned int nibbles, uint8_t *ecc);
int decode_bch_high_nibbles(uint8_t *data, unsigned int nibbles, const uint8_t *recv_ecc);

// ==========================================
// 次高位 BCH 接口
// ==========================================
void encode_bch_low(const uint8_t *data, unsigned int len, uint8_t *ecc);
int  decode_bch_low(uint8_t *data, unsigned int len, const uint8_t *recv_ecc);
void encode_bch_low_nibbles(const unsigned char *data, unsigned int nibbles, uint8_t *ecc);
int decode_bch_low_nibbles(uint8_t *data, unsigned int nibbles, const uint8_t *recv_ecc);
#endif