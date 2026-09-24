/*
 * QingLuan Digital Signature Scheme
 * utils.c - Utility functions with SM3-DRBG (GM/T 0005)
 *
 * Implements a Hash-DRBG based on SM3 per GM/T 0005 standard.
 * Uses the system CSPRNG as the entropy source for seeding.
 *
 * Security fixes:
 *   - DRBG state is thread-local to prevent cross-thread corruption
 *   - Entropy failure in reseed is treated as fatal (returns error)
 *   - secure_zero uses compiler barrier to prevent dead-store elimination
 */

#include "utils.h"
#include "hash.h"
#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#include <bcrypt.h>
#pragma comment(lib, "bcrypt.lib")
#else
#include <fcntl.h>
#include <unistd.h>
#endif

#define DRBG_SEED_SIZE   32
#define DRBG_RESEED_MAX  1024

/* Thread-local DRBG state to prevent cross-thread corruption.
 * Without this, concurrent calls to crypto_sign_keypair() or
 * crypto_sign_signature() from different threads would corrupt
 * the shared DRBG state, potentially producing predictable output. */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_THREADS__)
  #define THREAD_LOCAL _Thread_local
#elif defined(_MSC_VER)
  #define THREAD_LOCAL __declspec(thread)
#elif defined(__GNUC__) || defined(__clang__)
  #define THREAD_LOCAL __thread
#else
  #define THREAD_LOCAL /* fallback: no thread-local support */
#endif

static THREAD_LOCAL uint8_t  drbg_V[DRBG_SEED_SIZE];
static THREAD_LOCAL uint8_t  drbg_C[DRBG_SEED_SIZE];
static THREAD_LOCAL uint32_t drbg_reseed_counter;
static THREAD_LOCAL int      drbg_initialized;

static int sys_randombytes(uint8_t *out, size_t len)
{
#ifdef _WIN32
    NTSTATUS status = BCryptGenRandom(NULL, out, (ULONG)len,
                                       BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    if (status != 0) return -1;
#else
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0) return -1;

    size_t pos = 0;
    while (pos < len) {
        ssize_t ret = read(fd, out + pos, len - pos);
        if (ret < 0) {
            close(fd);
            return -1;
        }
        if (ret > 0) pos += (size_t)ret;
    }
    close(fd);
#endif
    return 0;
}

int drbg_init(void)
{
    uint8_t seed_material[DRBG_SEED_SIZE * 2];
    if (sys_randombytes(seed_material, sizeof(seed_material)) != 0)
        return -1;

    uint8_t seed_buf[1 + DRBG_SEED_SIZE * 2];
    seed_buf[0] = 0x00;
    memcpy(seed_buf + 1, seed_material, DRBG_SEED_SIZE * 2);
    sm3(seed_buf, 1 + DRBG_SEED_SIZE * 2, drbg_V);

    uint8_t c_buf[1 + DRBG_SEED_SIZE];
    c_buf[0] = 0x00;
    memcpy(c_buf + 1, drbg_V, DRBG_SEED_SIZE);
    sm3(c_buf, 1 + DRBG_SEED_SIZE, drbg_C);

    secure_zero(seed_material, sizeof(seed_material));
    secure_zero(seed_buf, sizeof(seed_buf));
    secure_zero(c_buf, sizeof(c_buf));

    drbg_reseed_counter = 1;
    drbg_initialized = 1;
    return 0;
}

/*
 * Deterministically (re)initialize the DRBG from a caller-supplied seed.
 * Used for KAT reproducibility. Subsequent randombytes() output is a
 * deterministic function of `seed`.
 */
int drbg_seed(const uint8_t *seed, size_t len)
{
    uint8_t buf[1 + 256];
    if (len > 256) len = 256;
    buf[0] = 0x00;
    memcpy(buf + 1, seed, len);
    sm3(buf, 1 + len, drbg_V);

    uint8_t c_buf[1 + DRBG_SEED_SIZE];
    c_buf[0] = 0x00;
    memcpy(c_buf + 1, drbg_V, DRBG_SEED_SIZE);
    sm3(c_buf, 1 + DRBG_SEED_SIZE, drbg_C);

    secure_zero(buf, sizeof(buf));
    secure_zero(c_buf, sizeof(c_buf));
    drbg_reseed_counter = 1;
    drbg_initialized = 1;
    return 0;
}

/*
 * Reseed the DRBG from the system entropy source.
 * Returns 0 on success, -1 on entropy failure.
 * Entropy failure is treated as fatal — the caller must not
 * continue generating random output with stale DRBG state.
 */
static int drbg_reseed(void)
{
    uint8_t entropy[DRBG_SEED_SIZE];
    if (sys_randombytes(entropy, sizeof(entropy)) != 0)
        return -1;  /* Entropy failure: propagate error to caller */

    uint8_t seed_buf[1 + DRBG_SEED_SIZE + DRBG_SEED_SIZE];
    seed_buf[0] = 0x01;
    memcpy(seed_buf + 1, entropy, DRBG_SEED_SIZE);
    memcpy(seed_buf + 1 + DRBG_SEED_SIZE, drbg_V, DRBG_SEED_SIZE);
    sm3(seed_buf, sizeof(seed_buf), drbg_V);

    uint8_t c_buf[1 + DRBG_SEED_SIZE];
    c_buf[0] = 0x00;
    memcpy(c_buf + 1, drbg_V, DRBG_SEED_SIZE);
    sm3(c_buf, 1 + DRBG_SEED_SIZE, drbg_C);

    secure_zero(entropy, sizeof(entropy));
    secure_zero(seed_buf, sizeof(seed_buf));
    secure_zero(c_buf, sizeof(c_buf));

    drbg_reseed_counter = 1;
    return 0;
}

int randombytes(uint8_t *out, size_t len)
{
    if (!drbg_initialized) {
        if (drbg_init() != 0) return -1;
    }

    if (drbg_reseed_counter > DRBG_RESEED_MAX) {
        if (drbg_reseed() != 0) return -1;
    }

    /* Hashgen (SP 800-90A 10.1.1.4 / GM/T 0005 Hash_DRBG generate):
     * use a WORK COPY of V so the per-block increment doesn't mutate the
     * persistent state — the persistent V is mixed once at the end via
     * the standard (V + H + C + reseed_counter) update below. */
    uint8_t work_V[DRBG_SEED_SIZE];
    memcpy(work_V, drbg_V, DRBG_SEED_SIZE);

    size_t offset = 0;
    while (offset < len) {
        uint8_t h[SM3_DIGEST_SIZE];
        uint8_t tmp[1 + DRBG_SEED_SIZE];
        tmp[0] = 0x02;
        memcpy(tmp + 1, work_V, DRBG_SEED_SIZE);
        sm3(tmp, 1 + DRBG_SEED_SIZE, h);

        size_t take = SM3_DIGEST_SIZE < (len - offset) ? SM3_DIGEST_SIZE : (len - offset);
        memcpy(out + offset, h, take);
        offset += take;

        int carry = 1;
        for (int i = DRBG_SEED_SIZE - 1; i >= 0; i--) {
            int sum = (int)work_V[i] + carry;
            work_V[i] = (uint8_t)sum;
            carry = sum >> 8;
        }

        secure_zero(h, sizeof(h));
        secure_zero(tmp, sizeof(tmp));
    }
    secure_zero(work_V, sizeof(work_V));

    /* Standard Hash_DRBG state update for backtracking resistance:
     *   H = SHA(0x03 || V)
     *   V = (V + H + C + reseed_counter) mod 2^seedlen
     *   reseed_counter += 1
     * Previously only V was mixed back, dropping C and the counter — this
     * weakened forward security relative to the SP 800-90A / GM/T 0005 spec. */
    {
        uint8_t h_in[1 + DRBG_SEED_SIZE];
        h_in[0] = 0x03;
        memcpy(h_in + 1, drbg_V, DRBG_SEED_SIZE);
        uint8_t H[SM3_DIGEST_SIZE];
        sm3(h_in, sizeof(h_in), H);

        int carry = 0;
        for (int i = DRBG_SEED_SIZE - 1; i >= 0; i--) {
            int counter_byte_idx = DRBG_SEED_SIZE - 1 - i;
            uint32_t c_byte = 0;
            if (counter_byte_idx < 4) {
                c_byte = (drbg_reseed_counter >> (counter_byte_idx * 8)) & 0xFFu;
            }
            int sum = (int)drbg_V[i] + (int)H[i] + (int)drbg_C[i] + (int)c_byte + carry;
            drbg_V[i] = (uint8_t)sum;
            carry = sum >> 8;
        }

        secure_zero(h_in, sizeof(h_in));
        secure_zero(H, sizeof(H));
    }
    drbg_reseed_counter++;

    return 0;
}

int ct_memcmp(const void *a, const void *b, size_t len)
{
    const uint8_t *pa = (const uint8_t *)a;
    const uint8_t *pb = (const uint8_t *)b;
    uint8_t diff = 0;
    for (size_t i = 0; i < len; i++) {
        diff |= pa[i] ^ pb[i];
    }
    return (int)diff;
}

void ct_cswap(uint8_t *a, uint8_t *b, size_t len, int condition)
{
    uint8_t mask = (uint8_t)(-(condition & 1));
    for (size_t i = 0; i < len; i++) {
        uint8_t t = mask & (a[i] ^ b[i]);
        a[i] ^= t;
        b[i] ^= t;
    }
}

void u32_to_be(uint8_t *out, uint32_t v)
{
    out[0] = (uint8_t)(v >> 24);
    out[1] = (uint8_t)(v >> 16);
    out[2] = (uint8_t)(v >> 8);
    out[3] = (uint8_t)(v);
}

uint32_t be_to_u32(const uint8_t *in)
{
    return ((uint32_t)in[0] << 24) |
           ((uint32_t)in[1] << 16) |
           ((uint32_t)in[2] << 8) |
           (uint32_t)in[3];
}

/*
 * Secure memory zeroing that resists compiler dead-store elimination.
 *
 * Uses platform-specific secure functions where available, with a
 * compiler-barrier fallback for GCC/Clang. The volatile fallback
 * is kept as a last resort.
 */
void secure_zero(void *ptr, size_t len)
{
    if (ptr == NULL || len == 0) return;
#ifdef _WIN32
    SecureZeroMemory(ptr, len);
#elif defined(__GNUC__) || defined(__clang__)
    memset(ptr, 0, len);
    /* Compiler barrier: tells the compiler that ptr's memory is
     * externally observable, preventing dead-store elimination */
    __asm__ __volatile__("" : : "r"(ptr) : "memory");
#else
    volatile uint8_t *p = (volatile uint8_t *)ptr;
    size_t i;
    for (i = 0; i < len; i++)
        p[i] = 0;
#endif
}
