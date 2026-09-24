/*
 * uBlock-256/256 ECB wrapper
 */

#include "ublock.h"
#include "utils.h"
#include "x86_caps.h"
#include <string.h>

#if defined(PRG_ACCE) && (PRG_ACCE == 1) && (defined(__x86_64__) || defined(__i386__))
#define UBLOCK_ECB_HAS_X86_ACCE 1
#else
#define UBLOCK_ECB_HAS_X86_ACCE 0
#endif

#if UBLOCK_ECB_HAS_X86_ACCE
/*
 * AVX2 backend is optionally linked (ublockith PRG_ACCE=1 path).
 * Keep weak references so targets that do not link the C++ object
 * continue to build and transparently fall back to scalar C.
 */
extern void uBlock_256_KeySchedule(unsigned char* key) __attribute__((weak));
extern void uBlock_256_Encrypt(unsigned char* plain, unsigned char* cipher,
                               int round) __attribute__((weak));

static inline int ublock256_has_avx2_backend_runtime(void) {
    return ublockith_x86_has_avx2() && (uBlock_256_KeySchedule != NULL) &&
           (uBlock_256_Encrypt != NULL);
}

static void ublock256_recover_key_bytes(const ublock256_key_t* ks, uint8_t key[UBLOCK_KEY]) {
    for (size_t i = 0; i < 8; ++i) {
        store_u32_be(ks->rk[0][i], key + 4 * i);
    }
}

static void ublock256_ecb_encrypt_avx2(const ublock256_key_t* ks, uint8_t* out,
                                       const uint8_t* in, size_t blocks) {
    uint8_t key_bytes[UBLOCK_KEY];
    const size_t pair_blocks = blocks / 2;

    ublock256_recover_key_bytes(ks, key_bytes);
    uBlock_256_KeySchedule(key_bytes);

    for (size_t i = 0; i < pair_blocks; ++i) {
        const size_t off = i * 2 * UBLOCK_BLOCK;
        uBlock_256_Encrypt((unsigned char*)(in + off), (unsigned char*)(out + off),
                           UBLOCK_ROUNDS);
    }

    if ((blocks & 1u) != 0u) {
        const size_t off = pair_blocks * 2 * UBLOCK_BLOCK;
        ublock256_256_encrypt(in + off, out + off, ks);
    }

    explicit_bzero(key_bytes, sizeof(key_bytes));
}
#endif

int ublock256_ecb_encrypt(const ublock256_key_t *ks,
                          uint8_t *out, const uint8_t *in,
                          size_t blocks)
{
    if (!ks || !out || !in) {
        return -1;
    }

#if UBLOCK_ECB_HAS_X86_ACCE
    if (blocks >= 2 && ublock256_has_avx2_backend_runtime()) {
        ublock256_ecb_encrypt_avx2(ks, out, in, blocks);
        return 0;
    }
#endif

    for (size_t i = 0; i < blocks; i++) {
        ublock256_256_encrypt(in + i * 32, out + i * 32, ks);
    }
    return 0;
}

