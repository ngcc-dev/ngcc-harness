#include <stddef.h>
#include <stdint.h>
#include <torsion_constants.h>
#if 0
#elif 8*DIGIT_LEN == 16
const uint64_t TORSION_PLUS_EVEN_POWER = 0xbf;
const uint64_t TORSION_ODD_PRIMES[5] = {0x3, 0x5, 0x7, 0x3b, 0x3a9};
const uint64_t TORSION_ODD_POWERS[5] = {0x76, 0x1, 0x1, 0x1, 0x1};
const uint64_t TORSION_PLUS_ODD_PRIMES[1] = {0x3};
const size_t TORSION_PLUS_ODD_POWERS[1] = {0x76};
const uint64_t TORSION_MINUS_ODD_PRIMES[4] = {0x5, 0x7, 0x3b, 0x3a9};
const size_t TORSION_MINUS_ODD_POWERS[4] = {0x1, 0x1, 0x1, 0x1};
const size_t DEGREE_COMMITMENT_POWERS[5] = {0x0, 0x1, 0x1, 0x1, 0x1};
const ibz_t CHARACTERISTIC = {{._mp_alloc = 0, ._mp_size = 24, ._mp_d = (mp_limb_t[]) {0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0x7fff,0x3cc,0x347f,0x7fd4,0x21d4,0x76df,0x9a6b,0x653c,0xf96d,0x64c3,0x24fc,0x508a,0x412}}};
const ibz_t TORSION_ODD = {{._mp_alloc = 0, ._mp_size = 13, ._mp_d = (mp_limb_t[]) {0xc711,0x34de,0xf83b,0x7c86,0x30ec,0x32da,0x82fe,0x886d,0x8b2c,0x8648,0xdd6b,0x3bc4,0xf06b}}};
const ibz_t TORSION_ODD_PRIMEPOWERS[5] = {{{._mp_alloc = 0, ._mp_size = 12, ._mp_d = (mp_limb_t[]) {0x799,0x68fe,0xffa8,0x43a8,0xedbe,0x34d6,0xca79,0xf2da,0xc987,0x49f8,0xa114,0x824}}}, {{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x5}}}, {{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x7}}}, {{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x3b}}}, {{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x3a9}}}};
const ibz_t TORSION_ODD_PLUS = {{._mp_alloc = 0, ._mp_size = 12, ._mp_d = (mp_limb_t[]) {0x799,0x68fe,0xffa8,0x43a8,0xedbe,0x34d6,0xca79,0xf2da,0xc987,0x49f8,0xa114,0x824}}};
const ibz_t TORSION_ODD_MINUS = {{._mp_alloc = 0, ._mp_size = 2, ._mp_d = (mp_limb_t[]) {0x8639,0x1d}}};
const ibz_t TORSION_PLUS_2POWER = {{._mp_alloc = 0, ._mp_size = 12, ._mp_d = (mp_limb_t[]) {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x8000}}};
const ibz_t TORSION_PLUS_3POWER = {{._mp_alloc = 0, ._mp_size = 12, ._mp_d = (mp_limb_t[]) {0x799,0x68fe,0xffa8,0x43a8,0xedbe,0x34d6,0xca79,0xf2da,0xc987,0x49f8,0xa114,0x824}}};
const ibz_t TORSION_PLUS_23POWER = {{._mp_alloc = 0, ._mp_size = 24, ._mp_d = (mp_limb_t[]) {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x8000,0x3cc,0x347f,0x7fd4,0x21d4,0x76df,0x9a6b,0x653c,0xf96d,0x64c3,0x24fc,0x508a,0x412}}};
const ibz_t DEGREE_COMMITMENT = {{._mp_alloc = 0, ._mp_size = 2, ._mp_d = (mp_limb_t[]) {0x8639,0x1d}}};
const ibz_t DEGREE_COMMITMENT_PLUS = {{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x1}}};
const ibz_t DEGREE_COMMITMENT_MINUS = {{._mp_alloc = 0, ._mp_size = 2, ._mp_d = (mp_limb_t[]) {0x8639,0x1d}}};
const ibz_t DEGREE_CHALLENGE = {{._mp_alloc = 0, ._mp_size = 24, ._mp_d = (mp_limb_t[]) {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x8000,0x3cc,0x347f,0x7fd4,0x21d4,0x76df,0x9a6b,0x653c,0xf96d,0x64c3,0x24fc,0x508a,0x412}}};
#elif 8*DIGIT_LEN == 32
const uint64_t TORSION_PLUS_EVEN_POWER = 0xbf;
const uint64_t TORSION_ODD_PRIMES[5] = {0x3, 0x5, 0x7, 0x3b, 0x3a9};
const uint64_t TORSION_ODD_POWERS[5] = {0x76, 0x1, 0x1, 0x1, 0x1};
const uint64_t TORSION_PLUS_ODD_PRIMES[1] = {0x3};
const size_t TORSION_PLUS_ODD_POWERS[1] = {0x76};
const uint64_t TORSION_MINUS_ODD_PRIMES[4] = {0x5, 0x7, 0x3b, 0x3a9};
const size_t TORSION_MINUS_ODD_POWERS[4] = {0x1, 0x1, 0x1, 0x1};
const size_t DEGREE_COMMITMENT_POWERS[5] = {0x0, 0x1, 0x1, 0x1, 0x1};
const ibz_t CHARACTERISTIC = {{._mp_alloc = 0, ._mp_size = 12, ._mp_d = (mp_limb_t[]) {0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0x7fffffff,0x347f03cc,0x21d47fd4,0x9a6b76df,0xf96d653c,0x24fc64c3,0x412508a}}};
const ibz_t TORSION_ODD = {{._mp_alloc = 0, ._mp_size = 7, ._mp_d = (mp_limb_t[]) {0x34dec711,0x7c86f83b,0x32da30ec,0x886d82fe,0x86488b2c,0x3bc4dd6b,0xf06b}}};
const ibz_t TORSION_ODD_PRIMEPOWERS[5] = {{{._mp_alloc = 0, ._mp_size = 6, ._mp_d = (mp_limb_t[]) {0x68fe0799,0x43a8ffa8,0x34d6edbe,0xf2daca79,0x49f8c987,0x824a114}}}, {{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x5}}}, {{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x7}}}, {{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x3b}}}, {{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x3a9}}}};
const ibz_t TORSION_ODD_PLUS = {{._mp_alloc = 0, ._mp_size = 6, ._mp_d = (mp_limb_t[]) {0x68fe0799,0x43a8ffa8,0x34d6edbe,0xf2daca79,0x49f8c987,0x824a114}}};
const ibz_t TORSION_ODD_MINUS = {{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x1d8639}}};
const ibz_t TORSION_PLUS_2POWER = {{._mp_alloc = 0, ._mp_size = 6, ._mp_d = (mp_limb_t[]) {0x0,0x0,0x0,0x0,0x0,0x80000000}}};
const ibz_t TORSION_PLUS_3POWER = {{._mp_alloc = 0, ._mp_size = 6, ._mp_d = (mp_limb_t[]) {0x68fe0799,0x43a8ffa8,0x34d6edbe,0xf2daca79,0x49f8c987,0x824a114}}};
const ibz_t TORSION_PLUS_23POWER = {{._mp_alloc = 0, ._mp_size = 12, ._mp_d = (mp_limb_t[]) {0x0,0x0,0x0,0x0,0x0,0x80000000,0x347f03cc,0x21d47fd4,0x9a6b76df,0xf96d653c,0x24fc64c3,0x412508a}}};
const ibz_t DEGREE_COMMITMENT = {{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x1d8639}}};
const ibz_t DEGREE_COMMITMENT_PLUS = {{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x1}}};
const ibz_t DEGREE_COMMITMENT_MINUS = {{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x1d8639}}};
const ibz_t DEGREE_CHALLENGE = {{._mp_alloc = 0, ._mp_size = 12, ._mp_d = (mp_limb_t[]) {0x0,0x0,0x0,0x0,0x0,0x80000000,0x347f03cc,0x21d47fd4,0x9a6b76df,0xf96d653c,0x24fc64c3,0x412508a}}};
#elif 8*DIGIT_LEN == 64
const uint64_t TORSION_PLUS_EVEN_POWER = 0xbf;
const uint64_t TORSION_ODD_PRIMES[5] = {0x3, 0x5, 0x7, 0x3b, 0x3a9};
const uint64_t TORSION_ODD_POWERS[5] = {0x76, 0x1, 0x1, 0x1, 0x1};
const uint64_t TORSION_PLUS_ODD_PRIMES[1] = {0x3};
const size_t TORSION_PLUS_ODD_POWERS[1] = {0x76};
const uint64_t TORSION_MINUS_ODD_PRIMES[4] = {0x5, 0x7, 0x3b, 0x3a9};
const size_t TORSION_MINUS_ODD_POWERS[4] = {0x1, 0x1, 0x1, 0x1};
const size_t DEGREE_COMMITMENT_POWERS[5] = {0x0, 0x1, 0x1, 0x1, 0x1};
const ibz_t CHARACTERISTIC = {{._mp_alloc = 0, ._mp_size = 6, ._mp_d = (mp_limb_t[]) {0xffffffffffffffff,0xffffffffffffffff,0x7fffffffffffffff,0x21d47fd4347f03cc,0xf96d653c9a6b76df,0x412508a24fc64c3}}};
const ibz_t TORSION_ODD = {{._mp_alloc = 0, ._mp_size = 4, ._mp_d = (mp_limb_t[]) {0x7c86f83b34dec711,0x886d82fe32da30ec,0x3bc4dd6b86488b2c,0xf06b}}};
const ibz_t TORSION_ODD_PRIMEPOWERS[5] = {{{._mp_alloc = 0, ._mp_size = 3, ._mp_d = (mp_limb_t[]) {0x43a8ffa868fe0799,0xf2daca7934d6edbe,0x824a11449f8c987}}}, {{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x5}}}, {{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x7}}}, {{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x3b}}}, {{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x3a9}}}};
const ibz_t TORSION_ODD_PLUS = {{._mp_alloc = 0, ._mp_size = 3, ._mp_d = (mp_limb_t[]) {0x43a8ffa868fe0799,0xf2daca7934d6edbe,0x824a11449f8c987}}};
const ibz_t TORSION_ODD_MINUS = {{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x1d8639}}};
const ibz_t TORSION_PLUS_2POWER = {{._mp_alloc = 0, ._mp_size = 3, ._mp_d = (mp_limb_t[]) {0x0,0x0,0x8000000000000000}}};
const ibz_t TORSION_PLUS_3POWER = {{._mp_alloc = 0, ._mp_size = 3, ._mp_d = (mp_limb_t[]) {0x43a8ffa868fe0799,0xf2daca7934d6edbe,0x824a11449f8c987}}};
const ibz_t TORSION_PLUS_23POWER = {{._mp_alloc = 0, ._mp_size = 6, ._mp_d = (mp_limb_t[]) {0x0,0x0,0x8000000000000000,0x21d47fd4347f03cc,0xf96d653c9a6b76df,0x412508a24fc64c3}}};
const ibz_t DEGREE_COMMITMENT = {{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x1d8639}}};
const ibz_t DEGREE_COMMITMENT_PLUS = {{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x1}}};
const ibz_t DEGREE_COMMITMENT_MINUS = {{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x1d8639}}};
const ibz_t DEGREE_CHALLENGE = {{._mp_alloc = 0, ._mp_size = 6, ._mp_d = (mp_limb_t[]) {0x0,0x0,0x8000000000000000,0x21d47fd4347f03cc,0xf96d653c9a6b76df,0x412508a24fc64c3}}};
#endif
