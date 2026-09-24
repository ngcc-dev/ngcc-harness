/*
* SM4-ECB PRG wrapper implementation, obtained from sm4th
*/

#include "sm4.h"
#include "compat.h"
#include "x86_caps.h"
//#include <time.h>
#include <string.h>

#ifndef SM4TH_ENABLE_HYGON_CIS
#define SM4TH_ENABLE_HYGON_CIS 0
#endif

#if SM4TH_ENABLE_HYGON_CIS && defined(__x86_64__)
#include "hygon_cis_sm4.h"
#endif

#ifndef PRG_BLOCK_SIZE
#define PRG_BLOCK_SIZE SM4_BLOCK_SIZE
#endif

#if defined(__x86_64__) || defined(__i386__)
#include <x86intrin.h>
#endif

/* Design note:
 * - Keep the historical fast path lightweight: SM4NI4 + yuchen key schedule.
 * - Do not add round-key conversion or byte-swapping on the hot small-batch path.
 * - Only use wider kernels for clearly large batches (>= 16 blocks) where setup cost amortizes.
 * - Keep 4-block and 2/3-tail requests on sm4_encrypt4; single-block requests stay scalar. */

#if SM4TH_ENABLE_SM4_ACCE && (defined(__x86_64__) || defined(__i386__))
#pragma GCC push_options
#pragma GCC target("ssse3,aes")

#undef SM4_ROUNDS

#define sm4_key_schedule yuchen_sm4_key_schedule_impl
#define sm4_encrypt      yuchen_sm4_encrypt_impl
#define SM4_REF_NO_DECRYPT 1
/* Avoid static helper name clash with utils.h */
#define load_u32_be      yuchen_sm4_ref_load_u32_be
#define store_u32_be     yuchen_sm4_ref_store_u32_be
#include "sm4ni/sm4_ref.c"
#undef store_u32_be
#undef load_u32_be
#undef SM4_REF_NO_DECRYPT

#include "sm4ni/sm4ni.c"

#undef sm4_encrypt
#undef sm4_key_schedule

#pragma GCC pop_options

static int sm4_set_key_yuchen(const uint8_t* key, SM4_KEY* ks) {
    uint32_t rk[SM4_KEY_SCHEDULE];
    yuchen_sm4_key_schedule_impl(key, rk);
    memcpy(ks->rk, rk, sizeof(rk));
    return 1;
}

//static void sm4_encrypt4_x86_sm4ni_yuchen(const SM4_KEY* ks, const uint8_t* src, uint8_t* dst) {
//    yuchen_sm4_encrypt4_impl(ks->rk, (void*)src, (const void*)dst);
//}

extern void sm4_aesni_avx2_asm_ecb_enc_blk_16(unsigned int* rk, unsigned char* dst,
                                              unsigned char* src) __attribute__((weak));
extern void sm4_encrypt4_opt(const uint32_t rk[32], const void* src, const void* dst);

static int sm4_has_avx2_ecb_kernel(void) {
    return sm4_aesni_avx2_asm_ecb_enc_blk_16 && sm4th_x86_has_aes_ssse3() &&
           sm4th_x86_has_avx2();
}
#endif

static unsigned int sm4_detect_acce_impl(void) {
#if !SM4TH_ENABLE_SM4_ACCE
#if SM4TH_ENABLE_HYGON_CIS && defined(__x86_64__)
    if (hygon_cis_sm4_is_supported()) {
        return SM4_ACCE_IMPL_X86_HYGON_CIS;
    }
#endif
    return SM4_ACCE_IMPL_SCALAR;
#else
#if SM4TH_ENABLE_HYGON_CIS && defined(__x86_64__)
    if (hygon_cis_sm4_is_supported()) {
        return SM4_ACCE_IMPL_X86_HYGON_CIS;
    }
#endif
#if defined(__x86_64__) || defined(__i386__)
    if (sm4th_x86_has_aes_ssse3()) {
        return SM4_ACCE_IMPL_X86_SM4NI4;
    }
#endif
    return SM4_ACCE_IMPL_SCALAR;
#endif
}

int SM4_set_key_acce(const uint8_t* key, SM4_KEY* ks, unsigned int* impl) {
    unsigned int chosen_impl = SM4_ACCE_IMPL_SCALAR;

    if (!key || !ks) {
        return -1;
    }

    chosen_impl = sm4_detect_acce_impl();

#if SM4TH_ENABLE_HYGON_CIS && defined(__x86_64__)
    if (chosen_impl == SM4_ACCE_IMPL_X86_HYGON_CIS) {
        hygon_cis_sm4_set_key(key, ks->rk);
    } else
#endif
#if SM4TH_ENABLE_SM4_ACCE && (defined(__x86_64__) || defined(__i386__))
    if (chosen_impl == SM4_ACCE_IMPL_X86_SM4NI4) {
        if (sm4_set_key_yuchen(key, ks) == 0) {
            return -1;
        }
    } else
#endif
    {
        if (SM4_set_key(key, ks) == 0) {
            return -1;
        }
    }

    if (impl) {
        *impl = chosen_impl;
    }

    return 0;
}

int SM4_encrypt_acce(const uint8_t* in, uint8_t* out, const SM4_KEY* ks, size_t blocks) {
    if (!in || !out || !ks) {
        return -1;
    }

#if SM4TH_ENABLE_HYGON_CIS && defined(__x86_64__)
    if (hygon_cis_sm4_is_supported()) {
        hygon_cis_sm4_encrypt_blocks(ks->rk, in, out, blocks);
        return 0;
    }
#endif

#if SM4TH_ENABLE_SM4_ACCE && (defined(__x86_64__) || defined(__i386__))
    const int has_sm4ni4 = sm4th_x86_has_aes_ssse3();
    const int has_avx2_kernel = sm4_has_avx2_ecb_kernel();

    if (has_sm4ni4 && (blocks == 4)) {
        sm4_encrypt4_opt(ks->rk, in, out);
        return 0;
    }

    if (has_avx2_kernel && (blocks == 18)) {
        sm4_aesni_avx2_asm_ecb_enc_blk_16((unsigned int*)ks->rk, out, (unsigned char*)in);
        SM4_encrypt(in + 16 * PRG_BLOCK_SIZE, out + 16 * PRG_BLOCK_SIZE, ks);
        SM4_encrypt(in + 17 * PRG_BLOCK_SIZE, out + 17 * PRG_BLOCK_SIZE, ks);
        return 0;
    }

    if (has_avx2_kernel) {
        while (blocks >= 16) {
            sm4_aesni_avx2_asm_ecb_enc_blk_16((unsigned int*)ks->rk, out, (unsigned char*)in);
            blocks -= 16;
            in += 16 * PRG_BLOCK_SIZE;
            out += 16 * PRG_BLOCK_SIZE;
        }
    }

    if (has_sm4ni4) {
        for (; blocks >= 4; blocks -= 4, in += 4 * PRG_BLOCK_SIZE, out += 4 * PRG_BLOCK_SIZE) {
            sm4_encrypt4_opt(ks->rk, in, out);
        }
    }
#endif

    for (; blocks; --blocks, in += PRG_BLOCK_SIZE, out += PRG_BLOCK_SIZE) {
        SM4_encrypt(in, out, ks);
    }
    return 0;
}

/* ================================================================== *
 *  Generic SM4-ECB PRG wrapper (public)
 * ================================================================== */

int generic_sm4_ecb_new(generic_sm4_ecb_t* ctx, const uint8_t* key, unsigned int seclvl) {
    (void) seclvl; /* SM4 supports 128-bit keys only in this code */
    if (!ctx || !key) return -1;
    if (SM4_set_key_acce(key, &ctx->ks, &ctx->acce_impl) != 0) return -1;
    ctx->seclvl = seclvl;
    return 0;
}

int generic_sm4_ecb_encrypt(generic_sm4_ecb_t* ctx, uint8_t* ciphertext, const uint8_t* plaintext,
                            size_t blocks) {
    if (!ctx) return -1;
    return SM4_encrypt_acce(plaintext, ciphertext, &ctx->ks, blocks);
}

void generic_sm4_ecb_free(generic_sm4_ecb_t* ctx) {
    if (!ctx) return;
    explicit_bzero(&ctx->ks, sizeof(ctx->ks));
    ctx->acce_impl = SM4_ACCE_IMPL_SCALAR;
}
