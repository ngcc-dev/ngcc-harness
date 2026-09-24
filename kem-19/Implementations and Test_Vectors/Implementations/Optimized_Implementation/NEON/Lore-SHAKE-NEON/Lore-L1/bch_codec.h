#ifndef LORE_BCH_CODEC_H
#define LORE_BCH_CODEC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct bch_control {
    unsigned int m;
    unsigned int n;
    unsigned int t;
    unsigned int ecc_bits;
    unsigned int ecc_bytes;
    unsigned int msg_bits;
    unsigned int roots;
    unsigned int code_bits;
    unsigned int code_nibbles;
    unsigned int rows;
    unsigned int words;
    uint16_t *alpha_to;
    int16_t *index_of;
    uint64_t *parity_matrix;
    uint64_t *encode_matrix;
    uint16_t *syndrome_nibble_table;
};

struct bch_control *init_bch(int m, int t, unsigned int prim_poly);
void free_bch(struct bch_control *bch);

void encode_bch(struct bch_control *bch, const uint8_t *data,
                unsigned int len, uint8_t *ecc);
void encodebits_bch(struct bch_control *bch, const uint8_t *data,
                    uint8_t *ecc);

int decode_bch(struct bch_control *bch, const uint8_t *data, unsigned int len,
               const uint8_t *recv_ecc, const uint8_t *calc_ecc,
               const unsigned int *syn, unsigned int *errloc);
int decodebits_bch(struct bch_control *bch, const uint8_t *data,
                   const uint8_t *recv_ecc, unsigned int *errloc);

void correct_bch(struct bch_control *bch, uint8_t *data, unsigned int len,
                 unsigned int *errloc, int nerr);
void correctbits_bch(struct bch_control *bch, uint8_t *databits,
                     unsigned int *errloc, int nerr);

#ifdef __cplusplus
}
#endif

#endif
