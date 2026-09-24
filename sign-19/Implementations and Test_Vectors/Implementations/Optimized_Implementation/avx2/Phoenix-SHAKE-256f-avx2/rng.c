/* Phoenix AES-256 CTR DRBG implementation
 *
 * Deterministic random bit generator for Phoenix signature scheme.
 */

#include <string.h>
#include "rng.h"
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>

AES256_CTR_DRBG_struct DRBG_ctx;

void AES256_ECB(unsigned char *aes_key, unsigned char *plaintext, unsigned char *ciphertext);

/* Initialize seed expander with 32-byte seed and 8-byte diversifier */
int seedexpander_init(AES_XOF_struct *xof_state,
                      unsigned char *seed_bytes,
                      unsigned char *div_bytes,
                      unsigned long max_out_len)
{
    if (max_out_len >= 0x100000000)
        return RNG_BAD_MAXLEN;

    xof_state->length_remaining = max_out_len;

    memcpy(xof_state->key, seed_bytes, 32);

    memcpy(xof_state->ctr, div_bytes, 8);
    xof_state->ctr[11] = (unsigned char)(max_out_len % 256);
    max_out_len >>= 8;
    xof_state->ctr[10] = (unsigned char)(max_out_len % 256);
    max_out_len >>= 8;
    xof_state->ctr[9] = (unsigned char)(max_out_len % 256);
    max_out_len >>= 8;
    xof_state->ctr[8] = (unsigned char)(max_out_len % 256);
    memset(xof_state->ctr+12, 0x00, 4);

    xof_state->buffer_pos = 16;
    memset(xof_state->buffer, 0x00, 16);

    return RNG_SUCCESS;
}

/* Generate XOF output bytes from seed expander state */
int seedexpander(AES_XOF_struct *xof_state, unsigned char *out_buf, unsigned long out_len)
{
    unsigned long buf_offset;

    if (out_buf == NULL)
        return RNG_BAD_OUTBUF;
    if (out_len >= xof_state->length_remaining)
        return RNG_BAD_REQ_LEN;

    xof_state->length_remaining -= out_len;

    buf_offset = 0;
    while (out_len > 0) {
        if (out_len <= (16 - xof_state->buffer_pos)) {
            memcpy(out_buf + buf_offset, xof_state->buffer + xof_state->buffer_pos, out_len);
            xof_state->buffer_pos += out_len;
            return RNG_SUCCESS;
        }

        /* Copy remaining buffer content */
        memcpy(out_buf + buf_offset, xof_state->buffer + xof_state->buffer_pos, 16 - xof_state->buffer_pos);
        out_len -= 16 - xof_state->buffer_pos;
        buf_offset += 16 - xof_state->buffer_pos;

        AES256_ECB(xof_state->key, xof_state->ctr, xof_state->buffer);
        xof_state->buffer_pos = 0;

        /* Increment counter bytes 12-15 */
        for (int pos_idx = 15; pos_idx >= 12; pos_idx--) {
            if (xof_state->ctr[pos_idx] == 0xff)
                xof_state->ctr[pos_idx] = 0x00;
            else {
                xof_state->ctr[pos_idx]++;
                break;
            }
        }
    }

    return RNG_SUCCESS;
}

static void handleErrors(void)
{
    ERR_print_errors_fp(stderr);
    abort();
}

/* AES-256 ECB encryption using OpenSSL EVP interface */
void AES256_ECB(unsigned char *aes_key, unsigned char *plaintext, unsigned char *ciphertext)
{
    EVP_CIPHER_CTX *enc_ctx;
    int out_len;

    if (!(enc_ctx = EVP_CIPHER_CTX_new())) handleErrors();

    if (1 != EVP_EncryptInit_ex(enc_ctx, EVP_aes_256_ecb(), NULL, aes_key, NULL))
        handleErrors();

    if (1 != EVP_EncryptUpdate(enc_ctx, ciphertext, &out_len, plaintext, 16))
        handleErrors();

    EVP_CIPHER_CTX_free(enc_ctx);
}

/* Initialize DRBG with entropy input and optional personalization string */
void randombytes_init(unsigned char *entropy_src,
                      unsigned char *pers_str)
{
    unsigned char seed_mat[48];

    memcpy(seed_mat, entropy_src, 48);
    if (pers_str) {
        for (int byte_idx = 0; byte_idx < 48; byte_idx++)
            seed_mat[byte_idx] ^= pers_str[byte_idx];
    }
    memset(DRBG_ctx.Key, 0x00, 32);
    memset(DRBG_ctx.V, 0x00, 16);
    AES256_CTR_DRBG_Update(seed_mat, DRBG_ctx.Key, DRBG_ctx.V);
    DRBG_ctx.reseed_counter = 1;
}

/* Generate random bytes using AES-256 CTR DRBG */
int randombytes(unsigned char *out_buf, unsigned long long out_len)
{
    unsigned char aes_block[16];
    int block_idx = 0;

    while (out_len > 0) {
        /* Increment counter V */
        for (int pos_idx = 15; pos_idx >= 0; pos_idx--) {
            if (DRBG_ctx.V[pos_idx] == 0xff)
                DRBG_ctx.V[pos_idx] = 0x00;
            else {
                DRBG_ctx.V[pos_idx]++;
                break;
            }
        }
        AES256_ECB(DRBG_ctx.Key, DRBG_ctx.V, aes_block);
        if (out_len > 15) {
            memcpy(out_buf + block_idx, aes_block, 16);
            block_idx += 16;
            out_len -= 16;
        } else {
            memcpy(out_buf + block_idx, aes_block, out_len);
            out_len = 0;
        }
    }
    AES256_CTR_DRBG_Update(NULL, DRBG_ctx.Key, DRBG_ctx.V);
    DRBG_ctx.reseed_counter++;

    return RNG_SUCCESS;
}

/* Update DRBG key and counter from provided data */
void AES256_CTR_DRBG_Update(unsigned char *input_data,
                             unsigned char *drbg_key,
                             unsigned char *drbg_v)
{
    unsigned char tmp_buf[48];

    for (int iter_idx = 0; iter_idx < 3; iter_idx++) {
        /* Increment counter V */
        for (int pos_idx = 15; pos_idx >= 0; pos_idx--) {
            if (drbg_v[pos_idx] == 0xff)
                drbg_v[pos_idx] = 0x00;
            else {
                drbg_v[pos_idx]++;
                break;
            }
        }

        AES256_ECB(drbg_key, drbg_v, tmp_buf + 16 * iter_idx);
    }
    if (input_data != NULL) {
        for (int byte_idx = 0; byte_idx < 48; byte_idx++)
            tmp_buf[byte_idx] ^= input_data[byte_idx];
    }
    memcpy(drbg_key, tmp_buf, 32);
    memcpy(drbg_v, tmp_buf + 32, 16);
}