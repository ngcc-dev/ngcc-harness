/********************************************************************************************
* MAMBA-Frost: unstructured LWQ-Z key encapsulation mechanism.
*
* Abstract: reference matrix arithmetic functions used by the KEM.
*
*********************************************************************************************/

#if defined(USE_AES128_FOR_A)
#if !defined(USE_OPENSSL)
    #include "../../common/aes/aes.h"
#else
    #include "../../common/aes/aes_openssl.h"
#endif
#define FROST_A_USES_AES
#define FROST_AES_A_SCHEDULE_BYTES (16*11)
#define FROST_AES_A_LOAD_SCHEDULE AES128_load_schedule
#define FROST_AES_A_ECB_ENC_SCH AES128_ECB_enc_sch
#define FROST_AES_A_FREE_SCHEDULE AES128_free_schedule
#define FROST_AES_A_EVP_CIPHER EVP_aes_128_ecb()
#elif defined (USE_SHAKE128_FOR_A)
    #include "../../common/sha3/fips202.h"
#endif
#ifdef FROST_USE_E8_CODE
#include "frost_e8.h"
#endif


#ifndef PARAMS_NBAR_R
#define PARAMS_NBAR_R PARAMS_NBAR
#endif
#ifndef PARAMS_NBAR_S
#define PARAMS_NBAR_S PARAMS_NBAR
#endif
const char *frost_message_codec_name(void)
{
#ifdef FROST_USE_E8_CODE
    return "e8";
#else
    return "scalar";
#endif
}

#ifdef FROST_CODEC_TRACE
volatile unsigned long frost_e8_encode_call_count = 0;
volatile unsigned long frost_e8_decode_call_count = 0;

void frost_codec_trace_reset(void)
{
    frost_e8_encode_call_count = 0;
    frost_e8_decode_call_count = 0;
}

unsigned long frost_codec_trace_e8_encode_calls(void)
{
    return frost_e8_encode_call_count;
}

unsigned long frost_codec_trace_e8_decode_calls(void)
{
    return frost_e8_decode_call_count;
}
#endif

int frost_mul_add_as_plus_e(uint16_t *out, const uint16_t *s, const uint16_t *e, const uint8_t *seed_A)
{ // Generate-and-multiply: generate matrix A (N x N) row-wise, multiply by s on the right.
  // Inputs: s, e (N x N_BAR)
  // Output: out = A*s + e (N x N_BAR)
    int i, j, k;
    int16_t *A = (int16_t *)calloc((size_t)PARAMS_N * PARAMS_N, sizeof(int16_t));
    if (A == NULL) return 0;
#ifdef PROFILE_ALL_LEVELS
    unsigned long long prof_t = prof_all_enabled() ? prof_now_cycles() : 0;
#endif

#if defined(FROST_A_USES_AES)    // Matrix A generation using AES128, done per 128-bit block
    size_t A_len = PARAMS_N * PARAMS_N * sizeof(int16_t);
    for (i = 0; i < PARAMS_N; i++) {
        for (j = 0; j < PARAMS_N; j += PARAMS_STRIPE_STEP) {
            A[i*PARAMS_N + j] = UINT16_TO_LE(i);                // Loading values in the little-endian order
            A[i*PARAMS_N + j + 1] = UINT16_TO_LE(j);
        }
    }

#if !defined(USE_OPENSSL)
    uint8_t aes_key_schedule[FROST_AES_A_SCHEDULE_BYTES];
    FROST_AES_A_LOAD_SCHEDULE(seed_A, aes_key_schedule);
    FROST_AES_A_ECB_ENC_SCH((uint8_t*)A, A_len, aes_key_schedule, (uint8_t*)A);
#else
    EVP_CIPHER_CTX *aes_key_schedule;
    int len;
    if (!(aes_key_schedule = EVP_CIPHER_CTX_new())) handleErrors();
    if (1 != EVP_EncryptInit_ex(aes_key_schedule, FROST_AES_A_EVP_CIPHER, NULL, seed_A, NULL)) handleErrors();
    if (1 != EVP_EncryptUpdate(aes_key_schedule, (uint8_t*)A, &len, (uint8_t*)A, A_len)) handleErrors();
#endif
#elif defined(USE_SHAKE128_FOR_A)  // Matrix A generation using SHAKE128, done per 16*N-bit row
    uint8_t seed_A_separated[2 + BYTES_SEED_A];
    uint16_t* seed_A_origin = (uint16_t*)&seed_A_separated;
    memcpy(&seed_A_separated[2], seed_A, BYTES_SEED_A);
    for (i = 0; i < PARAMS_N; i++) {
        seed_A_origin[0] = UINT16_TO_LE((uint16_t) i);
        shake128((unsigned char*)(A + i*PARAMS_N), (unsigned long long)(2*PARAMS_N), seed_A_separated, 2 + BYTES_SEED_A);
    }
#endif
    for (i = 0; i < PARAMS_N * PARAMS_N; i++) {
        A[i] = LE_TO_UINT16(A[i]);
    }
#ifdef PROFILE_ALL_LEVELS
    frost_prof_mat_add_expand(prof_all_enabled() ? prof_now_cycles() - prof_t : 0);
    prof_t = prof_all_enabled() ? prof_now_cycles() : 0;
#endif
    memcpy(out, e, PARAMS_NBAR_R * PARAMS_N * sizeof(uint16_t));

    for (i = 0; i < PARAMS_N; i++) {                            // Matrix multiplication-addition A*s + e
        for (k = 0; k < PARAMS_NBAR_R; k++) {
            uint16_t sum = 0;
            for (j = 0; j < PARAMS_N; j++) {
                sum = (uint16_t)((uint32_t)sum +
                                 (uint32_t)(uint16_t)A[i*PARAMS_N + j] * (uint32_t)s[k*PARAMS_N + j]);
            }
            out[i*PARAMS_NBAR_R + k] += sum;                      // Adding e. No need to reduce modulo 2^PARAMS_LOGQ, extra bits are taken care of during packing later on.
        }
    }
#ifdef PROFILE_ALL_LEVELS
    frost_prof_mat_add_mul(prof_all_enabled() ? prof_now_cycles() - prof_t : 0);
#endif

#if defined(FROST_A_USES_AES)
    FROST_AES_A_FREE_SCHEDULE(aes_key_schedule);
#endif
    clear_bytes((uint8_t *)A, (size_t)PARAMS_N * PARAMS_N * sizeof(int16_t));
    free(A);
    return 1;
}


int frost_mul_add_sa_plus_e(uint16_t *out, const uint16_t *s, uint16_t *e, const uint8_t *seed_A)
{ // Generate-and-multiply: generate matrix A (N x N) column-wise, multiply by s' on the left.
  // Inputs: s', e' (N_BAR x N)
  // Output: out = s'*A + e' (N_BAR x N)
    int i, j, k;
#if defined(FROST_U16_STREAMING_MATMUL) && !defined(FROST_U16_MATERIALIZED_A_MATMUL)
    int16_t A_row[PARAMS_N] = {0};
#ifdef PROFILE_ALL_LEVELS
    unsigned long long prof_expand = 0, prof_mul = 0, prof_t = 0;
#endif

    memcpy(out, e, PARAMS_NBAR_S * PARAMS_N * sizeof(uint16_t));

#if defined(FROST_A_USES_AES)
#if !defined(USE_OPENSSL)
    uint8_t aes_key_schedule[FROST_AES_A_SCHEDULE_BYTES];
    FROST_AES_A_LOAD_SCHEDULE(seed_A, aes_key_schedule);
#else
    EVP_CIPHER_CTX *aes_key_schedule;
    int len;
    if (!(aes_key_schedule = EVP_CIPHER_CTX_new())) handleErrors();
    if (1 != EVP_EncryptInit_ex(aes_key_schedule, FROST_AES_A_EVP_CIPHER, NULL, seed_A, NULL)) handleErrors();
#endif
#elif defined(USE_SHAKE128_FOR_A)
    uint8_t seed_A_separated[2 + BYTES_SEED_A];
    uint16_t* seed_A_origin = (uint16_t*)&seed_A_separated;
    memcpy(&seed_A_separated[2], seed_A, BYTES_SEED_A);
#else
#error FROST_U16_STREAMING_MATMUL requires AES128 or SHAKE128 A generation
#endif

    for (j = 0; j < PARAMS_N; j++) {
#ifdef PROFILE_ALL_LEVELS
        prof_t = prof_all_enabled() ? prof_now_cycles() : 0;
#endif
#if defined(FROST_A_USES_AES)
        memset(A_row, 0, PARAMS_N * sizeof(int16_t));
        for (i = 0; i < PARAMS_N; i += PARAMS_STRIPE_STEP) {
            A_row[i] = UINT16_TO_LE(j);
            A_row[i + 1] = UINT16_TO_LE(i);
        }
#if !defined(USE_OPENSSL)
        FROST_AES_A_ECB_ENC_SCH((uint8_t*)A_row, PARAMS_N * sizeof(int16_t), aes_key_schedule, (uint8_t*)A_row);
#else
        if (1 != EVP_EncryptUpdate(aes_key_schedule, (uint8_t*)A_row, &len, (uint8_t*)A_row, PARAMS_N * sizeof(int16_t))) handleErrors();
#endif
#elif defined(USE_SHAKE128_FOR_A)
        seed_A_origin[0] = UINT16_TO_LE((uint16_t) j);
        shake128((unsigned char*)A_row, (unsigned long long)(2*PARAMS_N), seed_A_separated, 2 + BYTES_SEED_A);
#endif
        for (i = 0; i < PARAMS_N; i++) {
            A_row[i] = LE_TO_UINT16(A_row[i]);
        }
#ifdef PROFILE_ALL_LEVELS
        prof_expand += prof_all_enabled() ? prof_now_cycles() - prof_t : 0;
        prof_t = prof_all_enabled() ? prof_now_cycles() : 0;
#endif
        for (k = 0; k < PARAMS_NBAR_S; k++) {
            const uint16_t sp = s[k*PARAMS_N + j];
            for (i = 0; i < PARAMS_N; i++) {
                out[k*PARAMS_N + i] = (uint16_t)((uint32_t)out[k*PARAMS_N + i] +
                                                  (uint32_t)(uint16_t)A_row[i] * (uint32_t)sp);
            }
        }
#ifdef PROFILE_ALL_LEVELS
        prof_mul += prof_all_enabled() ? prof_now_cycles() - prof_t : 0;
#endif
    }
#ifdef PROFILE_ALL_LEVELS
    frost_prof_mat_add_expand(prof_expand);
    frost_prof_mat_add_mul(prof_mul);
#endif

#if defined(FROST_A_USES_AES)
    FROST_AES_A_FREE_SCHEDULE(aes_key_schedule);
#endif
    return 1;
#else
    int16_t A[PARAMS_N * PARAMS_N] = {0};
#ifdef PROFILE_ALL_LEVELS
    unsigned long long prof_t = prof_all_enabled() ? prof_now_cycles() : 0;
#endif

#if defined(FROST_A_USES_AES)    // Matrix A generation using AES128, done per 128-bit block
    size_t A_len = PARAMS_N * PARAMS_N * sizeof(int16_t);
    for (i = 0; i < PARAMS_N; i++) {
        for (j = 0; j < PARAMS_N; j += PARAMS_STRIPE_STEP) {
            A[i*PARAMS_N + j] = UINT16_TO_LE(i);                // Loading values in the little-endian order
            A[i*PARAMS_N + j + 1] = UINT16_TO_LE(j);
        }
    }

#if !defined(USE_OPENSSL)
    uint8_t aes_key_schedule[FROST_AES_A_SCHEDULE_BYTES];
    FROST_AES_A_LOAD_SCHEDULE(seed_A, aes_key_schedule);
    FROST_AES_A_ECB_ENC_SCH((uint8_t*)A, A_len, aes_key_schedule, (uint8_t*)A);
#else
    EVP_CIPHER_CTX *aes_key_schedule;
    int len;
    if (!(aes_key_schedule = EVP_CIPHER_CTX_new())) handleErrors();
    if (1 != EVP_EncryptInit_ex(aes_key_schedule, FROST_AES_A_EVP_CIPHER, NULL, seed_A, NULL)) handleErrors();
    if (1 != EVP_EncryptUpdate(aes_key_schedule, (uint8_t*)A, &len, (uint8_t*)A, A_len)) handleErrors();
#endif
#elif defined (USE_SHAKE128_FOR_A)  // Matrix A generation using SHAKE128, done per 16*N-bit row
    uint8_t seed_A_separated[2 + BYTES_SEED_A];
    uint16_t* seed_A_origin = (uint16_t*)&seed_A_separated;
    memcpy(&seed_A_separated[2], seed_A, BYTES_SEED_A);
    for (i = 0; i < PARAMS_N; i++) {
        seed_A_origin[0] = UINT16_TO_LE((uint16_t) i);
        shake128((unsigned char*)(A + i*PARAMS_N), (unsigned long long)(2*PARAMS_N), seed_A_separated, 2 + BYTES_SEED_A);
    }
#endif
    for (i = 0; i < PARAMS_N * PARAMS_N; i++) {
        A[i] = LE_TO_UINT16(A[i]);
    }
#ifdef PROFILE_ALL_LEVELS
    frost_prof_mat_add_expand(prof_all_enabled() ? prof_now_cycles() - prof_t : 0);
    prof_t = prof_all_enabled() ? prof_now_cycles() : 0;
#endif
    memcpy(out, e, PARAMS_NBAR_S * PARAMS_N * sizeof(uint16_t));

    for (i = 0; i < PARAMS_N; i++) {                            // Matrix multiplication-addition A*s + e
        for (k = 0; k < PARAMS_NBAR_S; k++) {
            uint16_t sum = 0;
            for (j = 0; j < PARAMS_N; j++) {
                sum = (uint16_t)((uint32_t)sum +
                                 (uint32_t)(uint16_t)A[j*PARAMS_N + i] * (uint32_t)s[k*PARAMS_N + j]);
            }
            out[k*PARAMS_N + i] += sum;                         // Adding e. No need to reduce modulo 2^PARAMS_LOGQ, extra bits are taken care of during packing later on.
        }
    }
#ifdef PROFILE_ALL_LEVELS
    frost_prof_mat_add_mul(prof_all_enabled() ? prof_now_cycles() - prof_t : 0);
#endif

#if defined(FROST_A_USES_AES)
    FROST_AES_A_FREE_SCHEDULE(aes_key_schedule);
#endif
    return 1;
#endif
}

void frost_mul_bs(uint16_t *out, const uint16_t *b, const uint16_t *s)
{ // Multiply by s on the right
  // Inputs: b (PARAMS_NBAR_S x N), s (PARAMS_NBAR_R x N)
  // Output: out = b*s^T (PARAMS_NBAR_S x PARAMS_NBAR_R)
    int i, j, k;

    for (j = 0; j < PARAMS_NBAR_S; j++) {
        for (i = 0; i < PARAMS_NBAR_R; i++) {
            out[j*PARAMS_NBAR_R + i] = 0;
            for (k = 0; k < PARAMS_N; k++) {
                out[j*PARAMS_NBAR_R + i] += b[j*PARAMS_N + k] * (int16_t)s[i*PARAMS_N + k];
            }
            out[j*PARAMS_NBAR_R + i] = (uint32_t)(out[j*PARAMS_NBAR_R + i]) & ((1<<PARAMS_LOGQ)-1);
        }
    }
}


void frost_mul_add_sb_plus_e(uint16_t *out, const uint16_t *b, const uint16_t *s, const uint16_t *e)
{ // Multiply by s on the left
  // Inputs: b (N x N_BAR), s (N_BAR x N), e (N_BAR x N_BAR)
  // Output: out = s*b + e (N_BAR x N_BAR)
    int i, j, k;

    for (k = 0; k < PARAMS_NBAR_S; k++) {
        for (i = 0; i < PARAMS_NBAR_R; i++) {
            out[k*PARAMS_NBAR_R + i] = e[k*PARAMS_NBAR_R + i];
            for (j = 0; j < PARAMS_N; j++) {
                out[k*PARAMS_NBAR_R + i] = (uint16_t)((uint32_t)out[k*PARAMS_NBAR_R + i] +
                                                      (uint32_t)s[k*PARAMS_N + j] * (uint32_t)b[j*PARAMS_NBAR_R + i]);
            }
            out[k*PARAMS_NBAR_R + i] = (uint32_t)(out[k*PARAMS_NBAR_R + i]) & ((1<<PARAMS_LOGQ)-1);
        }
    }
}


void frost_add(uint16_t *out, const uint16_t *a, const uint16_t *b)
{ // Add a and b
  // Inputs: a, b (N_BAR x N_BAR)
  // Output: c = a + b

    for (int i = 0; i < (PARAMS_NBAR_R*PARAMS_NBAR_S); i++) {
        out[i] = (a[i] + b[i]) & ((1<<PARAMS_LOGQ)-1);
    }
}


void frost_sub(uint16_t *out, const uint16_t *a, const uint16_t *b)
{ // Subtract a and b
  // Inputs: a, b (N_BAR x N_BAR)
  // Output: c = a - b

    for (int i = 0; i < (PARAMS_NBAR_R*PARAMS_NBAR_S); i++) {
        out[i] = (a[i] - b[i]) & ((1<<PARAMS_LOGQ)-1);
    }
}


void frost_key_encode(uint16_t *out, const uint16_t *in)
{ // Encoding
#ifdef FROST_USE_E8_CODE
    frost_e8_encode_u16(out, (const uint8_t*)in);
#else
    unsigned int i, j, npieces_word = 8;
    unsigned int nwords = (PARAMS_NBAR_R*PARAMS_NBAR_S)/8;
    uint64_t temp, mask = ((uint64_t)1 << PARAMS_EXTRACTED_BITS) - 1;
    uint16_t* pos = out;

    for (i = 0; i < nwords; i++) {
        temp = 0;
        for(j = 0; j < PARAMS_EXTRACTED_BITS; j++)
            temp |= ((uint64_t)((uint8_t*)in)[i*PARAMS_EXTRACTED_BITS + j]) << (8*j);
        for (j = 0; j < npieces_word; j++) {
            *pos = (uint16_t)((temp & mask) << (PARAMS_LOGQ - PARAMS_EXTRACTED_BITS));
            temp >>= PARAMS_EXTRACTED_BITS;
            pos++;
        }
    }
#endif
}


void frost_key_decode(uint16_t *out, const uint16_t *in)
{ // Decoding
#ifdef FROST_USE_E8_CODE
    frost_e8_decode_u16((uint8_t*)out, in);
#else
    unsigned int i, j, index = 0, npieces_word = 8;
    unsigned int nwords = (PARAMS_NBAR_R * PARAMS_NBAR_S) / 8;
    uint16_t temp, maskex=((uint16_t)1 << PARAMS_EXTRACTED_BITS) -1, maskq =((uint16_t)1 << PARAMS_LOGQ) -1;
    uint8_t  *pos = (uint8_t*)out;
    uint64_t templong;

    for (i = 0; i < nwords; i++) {
        templong = 0;
        for (j = 0; j < npieces_word; j++) {  // temp = floor(in*2^{-11}+0.5)
            temp = ((in[index] & maskq) + (1 << (PARAMS_LOGQ - PARAMS_EXTRACTED_BITS - 1))) >> (PARAMS_LOGQ - PARAMS_EXTRACTED_BITS);
            templong |= ((uint64_t)(temp & maskex)) << (PARAMS_EXTRACTED_BITS * j);
            index++;
        }
	for(j = 0; j < PARAMS_EXTRACTED_BITS; j++)
	    pos[i*PARAMS_EXTRACTED_BITS + j] = (templong >> (8*j)) & 0xFF;
    }
#endif
}
