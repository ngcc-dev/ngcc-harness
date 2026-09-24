/* Phoenix AES-256 CTR DRBG interface
 *
 * Deterministic random bit generator for Phoenix signature scheme.
 */

#ifndef SPX_RNG_H
#define SPX_RNG_H

#include <stdio.h>

#define RNG_SUCCESS      0
#define RNG_BAD_MAXLEN  -1
#define RNG_BAD_OUTBUF  -2
#define RNG_BAD_REQ_LEN -3

/* AES-based XOF state for seed expansion */
typedef struct {
    unsigned char   buffer[16];
    unsigned long   buffer_pos;
    unsigned long   length_remaining;
    unsigned char   key[32];
    unsigned char   ctr[16];
} AES_XOF_struct;

/* AES-256 CTR DRBG internal state */
typedef struct {
    unsigned char   Key[32];
    unsigned char   V[16];
    int             reseed_counter;
} AES256_CTR_DRBG_struct;

/* Update DRBG key and counter from provided data */
void AES256_CTR_DRBG_Update(unsigned char *input_data,
                             unsigned char *drbg_key,
                             unsigned char *drbg_v);

/* Initialize seed expander with 32-byte seed and 8-byte diversifier */
int seedexpander_init(AES_XOF_struct *xof_state,
                      unsigned char *seed_bytes,
                      unsigned char *div_bytes,
                      unsigned long max_out_len);

/* Generate XOF output bytes from seed expander state */
int seedexpander(AES_XOF_struct *xof_state, unsigned char *out_buf, unsigned long out_len);

/* Initialize DRBG with entropy input and optional personalization string */
void randombytes_init(unsigned char *entropy_src,
                      unsigned char *pers_str);

/* Generate random bytes using AES-256 CTR DRBG */
int randombytes(unsigned char *out_buf, unsigned long long out_len);

#endif /* SPX_RNG_H */