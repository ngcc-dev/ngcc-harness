#ifndef CSPRNG_H
#define CSPRNG_H

#include <stdint.h>
#include "macros.h"

/**
 * 使用密码学安全的随机数填充缓冲区。
 *
 * 在 Windows 下会使用 [`BCryptGenRandom`](https://learn.microsoft.com/zh-cn/windows/win32/api/bcrypt/nf-bcrypt-bcryptgenrandom)，在 Linux 下会使用 [`getrandom`](https://man7.org/linux/man-pages/man2/getrandom.2.html)。
 *
 * @param out 缓冲区
 * @param length 缓冲区的大小
 */
static inline void csprng_read(void *out, size_t length);

/**
 * 生成一个特定类型的随机数。
 */
#define IMPL_CSPRNG_GET(T) \
static inline T csprng_get_##T() { \
    T r; \
    csprng_read(&r, sizeof(r)); \
    return r; \
}

IMPL_CSPRNG_GET(uint8_t)
IMPL_CSPRNG_GET(uint16_t)
IMPL_CSPRNG_GET(uint32_t)
IMPL_CSPRNG_GET(uint64_t)
IMPL_CSPRNG_GET(size_t)

#undef IMPL_CSPRNG_GET

/**
 * 使用 Fisher–Yates Shuffle 将特定类型的数组打乱。
 *
 * 算法进行多少步，数组的前几个值就是被等概率打乱的。
 *
 * @param arr 需要打乱的数组
 * @param len 数组长度
 * @param steps 打乱部分的长度
 */
#define IMPL_CSPRNG_SHUFFLE_PARTIAL(T) \
static inline void csprng_shuffle_partial_##T(T *arr, size_t len, size_t steps) { \
    for (size_t i = 0; i < steps; i++) { \
        const size_t j = i + csprng_get_size_t() % (len - i); \
        SWAP(arr[i], arr[j], T); \
    } \
}

IMPL_CSPRNG_SHUFFLE_PARTIAL(uint8_t)
IMPL_CSPRNG_SHUFFLE_PARTIAL(uint16_t)
IMPL_CSPRNG_SHUFFLE_PARTIAL(uint32_t)
IMPL_CSPRNG_SHUFFLE_PARTIAL(uint64_t)
IMPL_CSPRNG_SHUFFLE_PARTIAL(size_t)

#undef IMPL_CSPRNG_SHUFFLE_PARTIAL

/**
 * 使用 Fisher–Yates Shuffle 将特定类型的数组打乱。
 *
 * @param arr 需要打乱的数组
 * @param len 数组长度
 */
#define IMPL_CSPRNG_SHUFFLE(T) \
static inline void csprng_shuffle_##T(T *arr, size_t len) { \
    csprng_shuffle_partial_##T(arr, len, len); \
}

IMPL_CSPRNG_SHUFFLE(uint8_t)
IMPL_CSPRNG_SHUFFLE(uint16_t)
IMPL_CSPRNG_SHUFFLE(uint32_t)
IMPL_CSPRNG_SHUFFLE(uint64_t)
IMPL_CSPRNG_SHUFFLE(size_t)

#undef IMPL_CSPRNG_SHUFFLE

/**
 * 定义这个值作为种子来使用固定的随机数序列。
 *
 * （使用 SHAKE256 的输出作为随机数）
 */
// #define FIXED_RNG_SEED "Mountain of Faith"

#ifdef FIXED_RNG_SEED
    #include <stdbool.h>
    #include "keccak/shake256.h"

    static shake256_ctx csprng_ctx;
    static bool csprng_inited = false;

    static inline void csprng_reset(void *seed, size_t length) {
        shake256_init(&csprng_ctx);
        shake256_update(&csprng_ctx, seed, length);
        csprng_inited = true;
    }

    static inline void csprng_read(void *out, size_t length) {
        if (!csprng_inited) csprng_reset(FIXED_RNG_SEED, sizeof(FIXED_RNG_SEED));
        shake256_read(&csprng_ctx, out, length);
    }
#else
    #ifdef _WIN32
        #include <windows.h>
        #include <bcrypt.h>

        static inline void csprng_read(void *out, size_t length) {
            BCryptGenRandom(NULL, out, length, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
        }
    #else
        #include <sys/random.h>

        static inline void csprng_read(void *out, size_t length) {
            getrandom(out, length, 0);
        }
    #endif
#endif

#endif