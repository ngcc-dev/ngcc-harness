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
//bch encode
void encode_bch_low(const uint8_t *data, unsigned int len, uint8_t *ecc);
//bch decode and correct
int  decode_bch_low(uint8_t *data, unsigned int len, const uint8_t *recv_ecc);

