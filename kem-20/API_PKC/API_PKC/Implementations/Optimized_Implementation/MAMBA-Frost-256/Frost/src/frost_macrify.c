/********************************************************************************************
* MAMBA-Frost: unstructured LWQ-Z key encapsulation mechanism.
*
* Abstract: matrix arithmetic functions used by the KEM.
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
#if !defined(USE_AVX2)
    #include "../../common/sha3/fips202.h"
#else
    #include "../../common/sha3/fips202x4.h"
#endif
#endif
#ifdef FROST_USE_E8_CODE
#include "frost_e8.h"
#endif
#if defined(USE_AVX2)
    #include <immintrin.h>
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
    ALIGN_HEADER(32) int16_t a_row[4*PARAMS_N] ALIGN_FOOTER(32) = {0};
#ifdef PROFILE_ALL_LEVELS
    unsigned long long prof_expand = 0, prof_mul = 0, prof_t = 0;
#endif

    for (i = 0; i < (PARAMS_N*PARAMS_NBAR_R); i += 2) {
        *((uint32_t*)&out[i]) = *((uint32_t*)&e[i]);
    }

#if defined(FROST_A_USES_AES)
    int16_t a_row_temp[4*PARAMS_N] = {0};                       // Take four lines of A at once
#if !defined(USE_OPENSSL)
    uint8_t aes_key_schedule[FROST_AES_A_SCHEDULE_BYTES];
    FROST_AES_A_LOAD_SCHEDULE(seed_A, aes_key_schedule);
#else
    EVP_CIPHER_CTX *aes_key_schedule;
    int len;
    if (!(aes_key_schedule = EVP_CIPHER_CTX_new())) handleErrors();
    if (1 != EVP_EncryptInit_ex(aes_key_schedule, FROST_AES_A_EVP_CIPHER, NULL, seed_A, NULL)) handleErrors();
#endif

    for (j = 0; j < PARAMS_N; j += PARAMS_STRIPE_STEP) {
        a_row_temp[j + 1 + 0*PARAMS_N] = UINT16_TO_LE(j);       // Loading values in the little-endian order
        a_row_temp[j + 1 + 1*PARAMS_N] = UINT16_TO_LE(j);
        a_row_temp[j + 1 + 2*PARAMS_N] = UINT16_TO_LE(j);
        a_row_temp[j + 1 + 3*PARAMS_N] = UINT16_TO_LE(j);
    }

    for (i = 0; i < PARAMS_N; i += 4) {
#ifdef PROFILE_ALL_LEVELS
        prof_t = prof_all_enabled() ? prof_now_cycles() : 0;
#endif
        for (j = 0; j < PARAMS_N; j += PARAMS_STRIPE_STEP) {    // Go through A, four rows at a time
            a_row_temp[j + 0*PARAMS_N] = UINT16_TO_LE(i+0);     // Loading values in the little-endian order
            a_row_temp[j + 1*PARAMS_N] = UINT16_TO_LE(i+1);
            a_row_temp[j + 2*PARAMS_N] = UINT16_TO_LE(i+2);
            a_row_temp[j + 3*PARAMS_N] = UINT16_TO_LE(i+3);
        }

#if !defined(USE_OPENSSL)
        FROST_AES_A_ECB_ENC_SCH((uint8_t*)a_row_temp, 4*PARAMS_N*sizeof(int16_t), aes_key_schedule, (uint8_t*)a_row);
#else
        if (1 != EVP_EncryptUpdate(aes_key_schedule, (uint8_t*)a_row, &len, (uint8_t*)a_row_temp, 4*PARAMS_N*sizeof(int16_t))) handleErrors();
#endif
#elif defined (USE_SHAKE128_FOR_A)
#if !defined(USE_AVX2)
    uint8_t seed_A_separated[2 + BYTES_SEED_A];
    uint16_t* seed_A_origin = (uint16_t*)&seed_A_separated;
    memcpy(&seed_A_separated[2], seed_A, BYTES_SEED_A);
    for (i = 0; i < PARAMS_N; i += 4) {
#ifdef PROFILE_ALL_LEVELS
        prof_t = prof_all_enabled() ? prof_now_cycles() : 0;
#endif
        seed_A_origin[0] = UINT16_TO_LE(i + 0);
        shake128((unsigned char*)(a_row + 0*PARAMS_N), (unsigned long long)(2*PARAMS_N), seed_A_separated, 2 + BYTES_SEED_A);
        seed_A_origin[0] = UINT16_TO_LE(i + 1);
        shake128((unsigned char*)(a_row + 1*PARAMS_N), (unsigned long long)(2*PARAMS_N), seed_A_separated, 2 + BYTES_SEED_A);
        seed_A_origin[0] = UINT16_TO_LE(i + 2);
        shake128((unsigned char*)(a_row + 2*PARAMS_N), (unsigned long long)(2*PARAMS_N), seed_A_separated, 2 + BYTES_SEED_A);
        seed_A_origin[0] = UINT16_TO_LE(i + 3);
        shake128((unsigned char*)(a_row + 3*PARAMS_N), (unsigned long long)(2*PARAMS_N), seed_A_separated, 2 + BYTES_SEED_A);
#else
    uint8_t seed_A_separated_0[2 + BYTES_SEED_A];
    uint8_t seed_A_separated_1[2 + BYTES_SEED_A];
    uint8_t seed_A_separated_2[2 + BYTES_SEED_A];
    uint8_t seed_A_separated_3[2 + BYTES_SEED_A];
    uint16_t* seed_A_origin_0 = (uint16_t*)&seed_A_separated_0;
    uint16_t* seed_A_origin_1 = (uint16_t*)&seed_A_separated_1;
    uint16_t* seed_A_origin_2 = (uint16_t*)&seed_A_separated_2;
    uint16_t* seed_A_origin_3 = (uint16_t*)&seed_A_separated_3;
    memcpy(&seed_A_separated_0[2], seed_A, BYTES_SEED_A);
    memcpy(&seed_A_separated_1[2], seed_A, BYTES_SEED_A);
    memcpy(&seed_A_separated_2[2], seed_A, BYTES_SEED_A);
    memcpy(&seed_A_separated_3[2], seed_A, BYTES_SEED_A);
    for (i = 0; i < PARAMS_N; i += 4) {
#ifdef PROFILE_ALL_LEVELS
        prof_t = prof_all_enabled() ? prof_now_cycles() : 0;
#endif
        seed_A_origin_0[0] = UINT16_TO_LE(i + 0);
        seed_A_origin_1[0] = UINT16_TO_LE(i + 1);
        seed_A_origin_2[0] = UINT16_TO_LE(i + 2);
        seed_A_origin_3[0] = UINT16_TO_LE(i + 3);
        shake128_4x((unsigned char*)(a_row), (unsigned char*)(a_row + PARAMS_N), (unsigned char*)(a_row + 2*PARAMS_N), (unsigned char*)(a_row + 3*PARAMS_N),
                    (unsigned long long)(2*PARAMS_N), seed_A_separated_0, seed_A_separated_1, seed_A_separated_2, seed_A_separated_3, 2 + BYTES_SEED_A);
#endif
#endif
        for (k = 0; k < 4 * PARAMS_N; k++) {
            a_row[k] = LE_TO_UINT16(a_row[k]);
        }
#ifdef PROFILE_ALL_LEVELS
        prof_expand += prof_all_enabled() ? prof_now_cycles() - prof_t : 0;
        prof_t = prof_all_enabled() ? prof_now_cycles() : 0;
#endif
        for (k = 0; k < PARAMS_NBAR_R; k++) {
            uint16_t sum[4] = {0};
            for (j = 0; j < PARAMS_N; j++) {                    // Matrix-vector multiplication
                uint16_t sp = s[k*PARAMS_N + j];
                sum[0] += a_row[0*PARAMS_N + j] * sp;           // Go through four lines with same s
                sum[1] += a_row[1*PARAMS_N + j] * sp;
                sum[2] += a_row[2*PARAMS_N + j] * sp;
                sum[3] += a_row[3*PARAMS_N + j] * sp;
            }
            out[(i+0)*PARAMS_NBAR_R + k] += sum[0];
            out[(i+2)*PARAMS_NBAR_R + k] += sum[2];
            out[(i+1)*PARAMS_NBAR_R + k] += sum[1];
            out[(i+3)*PARAMS_NBAR_R + k] += sum[3];
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
}


int frost_mul_add_sa_plus_e(uint16_t *out, const uint16_t *s, uint16_t *e, const uint8_t *seed_A)
{ // Generate-and-multiply: generate matrix A (N x N) column-wise, multiply by s' on the left.
  // Inputs: s', e' (N_BAR x N)
  // Output: out = s'*A + e' (N_BAR x N)
  // The matrix multiplication uses the row-wise blocking and packing (RWCF) approach described in: J.W. Bos, M. Ofner, J. Renes,
    int i, j, q, p;
    ALIGN_HEADER(32) uint16_t A[PARAMS_N*8] ALIGN_FOOTER(32) = {0};
#ifdef PROFILE_ALL_LEVELS
    unsigned long long prof_expand = 0, prof_mul = 0, prof_t = 0;
#endif

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
    // Initialize matrix used for encryption
    ALIGN_HEADER(32) uint16_t Ainit[PARAMS_N*8] ALIGN_FOOTER(32) = {0};

    for(j = 0; j < PARAMS_N; j+=8) {
        Ainit[0*PARAMS_N + j + 1] = UINT16_TO_LE(j);
        Ainit[1*PARAMS_N + j + 1] = UINT16_TO_LE(j);
        Ainit[2*PARAMS_N + j + 1] = UINT16_TO_LE(j);
        Ainit[3*PARAMS_N + j + 1] = UINT16_TO_LE(j);
        Ainit[4*PARAMS_N + j + 1] = UINT16_TO_LE(j);
        Ainit[5*PARAMS_N + j + 1] = UINT16_TO_LE(j);
        Ainit[6*PARAMS_N + j + 1] = UINT16_TO_LE(j);
        Ainit[7*PARAMS_N + j + 1] = UINT16_TO_LE(j);
    }

    // Start matrix multiplication
    for (i = 0; i < PARAMS_N; i+=8) {
#ifdef PROFILE_ALL_LEVELS
        prof_t = prof_all_enabled() ? prof_now_cycles() : 0;
#endif
        // Generate 8 rows of A on-the-fly using AES
        for (q = 0; q < 8; q++) {
            for (p = 0; p < PARAMS_N; p+=8) {
                Ainit[q*PARAMS_N + p] = UINT16_TO_LE(i+q);
            }
        }

        size_t A_len = 8 * PARAMS_N * sizeof(uint16_t);
#if !defined(USE_OPENSSL)
        FROST_AES_A_ECB_ENC_SCH((uint8_t*)Ainit, A_len, aes_key_schedule, (uint8_t*)A);
#else
        if (1 != EVP_EncryptUpdate(aes_key_schedule, (uint8_t*)A, &len, (uint8_t*)Ainit, A_len)) handleErrors();
#endif
#elif defined (USE_SHAKE128_FOR_A)  // SHAKE128
#if !defined(USE_AVX2)
    uint8_t seed_A_separated[2 + BYTES_SEED_A];
    uint16_t* seed_A_origin = (uint16_t*)&seed_A_separated;
    memcpy(&seed_A_separated[2], seed_A, BYTES_SEED_A);

    // Start matrix multiplication
    for (i = 0; i < PARAMS_N; i+=8) {
#ifdef PROFILE_ALL_LEVELS
        prof_t = prof_all_enabled() ? prof_now_cycles() : 0;
#endif
        seed_A_origin[0] = UINT16_TO_LE(i + 0);
        shake128((unsigned char*)(A + 0*PARAMS_N), (unsigned long long)(2*PARAMS_N), seed_A_separated, 2 + BYTES_SEED_A);
        seed_A_origin[0] = UINT16_TO_LE(i + 1);
        shake128((unsigned char*)(A + 1*PARAMS_N), (unsigned long long)(2*PARAMS_N), seed_A_separated, 2 + BYTES_SEED_A);
        seed_A_origin[0] = UINT16_TO_LE(i + 2);
        shake128((unsigned char*)(A + 2*PARAMS_N), (unsigned long long)(2*PARAMS_N), seed_A_separated, 2 + BYTES_SEED_A);
        seed_A_origin[0] = UINT16_TO_LE(i + 3);
        shake128((unsigned char*)(A + 3*PARAMS_N), (unsigned long long)(2*PARAMS_N), seed_A_separated, 2 + BYTES_SEED_A);
        seed_A_origin[0] = UINT16_TO_LE(i + 4);
        shake128((unsigned char*)(A + 4*PARAMS_N), (unsigned long long)(2*PARAMS_N), seed_A_separated, 2 + BYTES_SEED_A);
        seed_A_origin[0] = UINT16_TO_LE(i + 5);
        shake128((unsigned char*)(A + 5*PARAMS_N), (unsigned long long)(2*PARAMS_N), seed_A_separated, 2 + BYTES_SEED_A);
        seed_A_origin[0] = UINT16_TO_LE(i + 6);
        shake128((unsigned char*)(A + 6*PARAMS_N), (unsigned long long)(2*PARAMS_N), seed_A_separated, 2 + BYTES_SEED_A);
        seed_A_origin[0] = UINT16_TO_LE(i + 7);
        shake128((unsigned char*)(A + 7*PARAMS_N), (unsigned long long)(2*PARAMS_N), seed_A_separated, 2 + BYTES_SEED_A);
#else  // Using vector intrinsics
    uint8_t seed_A_separated_0[2 + BYTES_SEED_A];
    uint8_t seed_A_separated_1[2 + BYTES_SEED_A];
    uint8_t seed_A_separated_2[2 + BYTES_SEED_A];
    uint8_t seed_A_separated_3[2 + BYTES_SEED_A];
    uint16_t *seed_A_origin_0 = (uint16_t*)&seed_A_separated_0;
    uint16_t *seed_A_origin_1 = (uint16_t*)&seed_A_separated_1;
    uint16_t *seed_A_origin_2 = (uint16_t*)&seed_A_separated_2;
    uint16_t *seed_A_origin_3 = (uint16_t*)&seed_A_separated_3;
    memcpy(&seed_A_separated_0[2], seed_A, BYTES_SEED_A);
    memcpy(&seed_A_separated_1[2], seed_A, BYTES_SEED_A);
    memcpy(&seed_A_separated_2[2], seed_A, BYTES_SEED_A);
    memcpy(&seed_A_separated_3[2], seed_A, BYTES_SEED_A);

    // Start matrix multiplication
    for (i = 0; i < PARAMS_N; i+=8) {
#ifdef PROFILE_ALL_LEVELS
        prof_t = prof_all_enabled() ? prof_now_cycles() : 0;
#endif
        // Generate hash output
        // First 4 rows
        seed_A_origin_0[0] = UINT16_TO_LE(i + 0);
        seed_A_origin_1[0] = UINT16_TO_LE(i + 1);
        seed_A_origin_2[0] = UINT16_TO_LE(i + 2);
        seed_A_origin_3[0] = UINT16_TO_LE(i + 3);
        shake128_4x((unsigned char*)(A + 0*PARAMS_N), (unsigned char*)(A + 1*PARAMS_N), (unsigned char*)(A + 2*PARAMS_N), (unsigned char*)(A + 3*PARAMS_N),
                    (unsigned long long)(2*PARAMS_N), seed_A_separated_0, seed_A_separated_1, seed_A_separated_2, seed_A_separated_3, 2 + BYTES_SEED_A);
        // Second 4 rows
        seed_A_origin_0[0] = UINT16_TO_LE(i + 4);
        seed_A_origin_1[0] = UINT16_TO_LE(i + 5);
        seed_A_origin_2[0] = UINT16_TO_LE(i + 6);
        seed_A_origin_3[0] = UINT16_TO_LE(i + 7);
        shake128_4x((unsigned char*)(A + 4*PARAMS_N), (unsigned char*)(A + 5*PARAMS_N), (unsigned char*)(A + 6*PARAMS_N), (unsigned char*)(A + 7*PARAMS_N),
                    (unsigned long long)(2*PARAMS_N), seed_A_separated_0, seed_A_separated_1, seed_A_separated_2, seed_A_separated_3, 2 + BYTES_SEED_A);
#endif
#endif

#ifdef PROFILE_ALL_LEVELS
        prof_expand += prof_all_enabled() ? prof_now_cycles() - prof_t : 0;
        prof_t = prof_all_enabled() ? prof_now_cycles() : 0;
#endif
#if !defined(USE_AVX2)
        for (j = 0; j < PARAMS_NBAR_S; j++) {
            uint16_t sum = 0;
            int16_t sp[8];
            for (p = 0; p < 8; p++) {
                sp[p] = s[j*PARAMS_N + i + p];
            }
            for (q = 0; q < PARAMS_N; q++) {
                sum = e[j*PARAMS_N + q];
                for (p = 0; p < 8; p++) {
                    sum += sp[p] * A[p*PARAMS_N + q];
                }
                e[j*PARAMS_N + q] = sum;
            }
        }
#ifdef PROFILE_ALL_LEVELS
        prof_mul += prof_all_enabled() ? prof_now_cycles() - prof_t : 0;
#endif
    }
#else  // Using vector intrinsics
        for (j = 0; j < PARAMS_NBAR_S; j++) {
            __m256i b, sp[8], acc;
            for (p = 0; p < 8; p++) {
                sp[p] = _mm256_set1_epi16(s[j*PARAMS_N + i + p]);
            }
            for (q = 0; q + 15 < PARAMS_N; q+=16) {
                // e may not be 32-byte aligned at every call site, so use
                // unaligned vector loads/stores. The q+15 bound keeps the
                // vectorized path safe for profiles such as Frost-192 whose N
                // is not a multiple of 16.
                acc = _mm256_loadu_si256((const __m256i*)&e[j*PARAMS_N + q]);
                for (p = 0; p < 8; p++) {
                    b = _mm256_loadu_si256((const __m256i*)&A[p*PARAMS_N + q]);
                    b = _mm256_mullo_epi16(b, sp[p]);
                    acc = _mm256_add_epi16(b, acc);
                }
                _mm256_storeu_si256((__m256i*)&e[j*PARAMS_N + q], acc);
            }
#if ((PARAMS_N % 16) != 0)
            for (; q < PARAMS_N; q++) {
                uint16_t sum = e[j*PARAMS_N + q];
                for (p = 0; p < 8; p++) {
                    sum = (uint16_t)((uint32_t)sum +
                                     (uint32_t)s[j*PARAMS_N + i + p] * (uint32_t)A[p*PARAMS_N + q]);
                }
                e[j*PARAMS_N + q] = sum;
            }
#endif
        }
#ifdef PROFILE_ALL_LEVELS
        prof_mul += prof_all_enabled() ? prof_now_cycles() - prof_t : 0;
#endif
    }
#endif
#ifdef PROFILE_ALL_LEVELS
    frost_prof_mat_add_expand(prof_expand);
    frost_prof_mat_add_mul(prof_mul);
#endif
    memcpy((unsigned char*)out, (unsigned char*)e, 2*PARAMS_N*PARAMS_NBAR_S);

#if defined(FROST_A_USES_AES)
    FROST_AES_A_FREE_SCHEDULE(aes_key_schedule);
#endif
    return 1;
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
                out[k*PARAMS_NBAR_R + i] += (int16_t)s[k*PARAMS_N + j] * b[j*PARAMS_NBAR_R + i];
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
