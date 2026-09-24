#include "afs_sbox64.h"

#if (defined(__x86_64__) || defined(__i386__)) && \
    (defined(__GNUC__) || defined(__clang__)) && \
    defined(AFS_TREDM_USE_X86_SBOX_ASM)
#define AFS_TREDM_HAVE_X86_SBOX_ASM 1
#endif

#if !defined(AFS_TREDM_HAVE_X86_SBOX_ASM)
/* Function rotr32: rotates a 32-bit word right by a constant-count modulo 32 amount. */
static uint32_t rotr32(uint32_t x, unsigned n)
{
    n &= 31U;
    return (uint32_t)((x >> n) | (x << ((32U - n) & 31U)));
}
#endif

#if defined(AFS_TREDM_HAVE_X86_SBOX_ASM)
/* Function afs64_t5_k2_x86_asm: computes the AFS64_t5_k2 S-box using the optional x86 assembly sequence. */
static uint64_t afs64_t5_k2_x86_asm(uint64_t in, uint32_t c)
{
    uint32_t x = (uint32_t)(in >> 32);
    uint32_t y = (uint32_t)in;
    uint32_t t;

    __asm__ __volatile__(
        "movl %[y], %[t]\n\t"
        "rorl $17, %[t]\n\t"
        "addl %[t], %[x]\n\t"
        "xorl %[c], %[x]\n\t"
        "movl %[x], %[t]\n\t"
        "rorl $24, %[t]\n\t"
        "addl %[t], %[y]\n\t"
        "xorl %[c], %[y]\n\t"
        "movl %[y], %[t]\n\t"
        "rorl $1, %[t]\n\t"
        "xorl %[t], %[x]\n\t"
        "movl %[x], %[t]\n\t"
        "rorl $1, %[t]\n\t"
        "xorl %[t], %[y]\n\t"
        "movl %[y], %[t]\n\t"
        "rorl $16, %[t]\n\t"
        "xorl %[t], %[x]\n\t"
        "movl %[x], %[t]\n\t"
        "rorl $31, %[t]\n\t"
        "xorl %[t], %[y]\n\t"
        "movl %[y], %[t]\n\t"
        "rorl $24, %[t]\n\t"
        "addl %[t], %[x]\n\t"
        "xorl %[c], %[x]\n\t"
        "addl %[x], %[y]\n\t"
        "xorl %[c], %[y]\n\t"
        : [x] "+&r"(x), [y] "+&r"(y), [t] "=&r"(t)
        : [c] "r"(c)
        : "cc");

    return (((uint64_t)x) << 32) | (uint64_t)y;
}
#endif

/* Function afs64_t5_k2: computes the AFS64_t5_k2 ARX S-box for one 64-bit lane. */
uint64_t afs64_t5_k2(uint64_t in, uint32_t c)
{
#if defined(AFS_TREDM_HAVE_X86_SBOX_ASM)
    return afs64_t5_k2_x86_asm(in, c);
#else
    uint32_t x = (uint32_t)(in >> 32);
    uint32_t y = (uint32_t)in;

    /* AFS-64, A8=11000011, K8=[17,24,1,1,16,31,24,0]. */
    x = (uint32_t)(x + rotr32(y, 17)); x ^= c;
    y = (uint32_t)(y + rotr32(x, 24)); y ^= c;
    x ^= rotr32(y, 1);
    y ^= rotr32(x, 1);
    x ^= rotr32(y, 16);
    y ^= rotr32(x, 31);
    x = (uint32_t)(x + rotr32(y, 24)); x ^= c;
    y = (uint32_t)(y + rotr32(x, 0));  y ^= c;

    return (((uint64_t)x) << 32) | (uint64_t)y;
#endif
}
