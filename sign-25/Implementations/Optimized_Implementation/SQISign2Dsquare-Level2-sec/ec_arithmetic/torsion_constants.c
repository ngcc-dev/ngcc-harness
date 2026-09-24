#include <torsion_constants.h>
#if 0
#elif 8*DIGIT_LEN == 32
const uint64_t TORSION_PLUS_EVEN_POWER = 0xa8;
const uint64_t TORSION_PLUS_THREE_POWER = 0x65;
const ibz_t TORSION_PLUS_2POWER = {{._mp_alloc = 0, ._mp_size = 6, ._mp_d = (mp_limb_t[]) {0x0, 0x0, 0x0, 0x0, 0x0, 0x100, }}};
const ibz_t TORSION_PLUS_3POWER = {{._mp_alloc = 0, ._mp_size = 6, ._mp_d = (mp_limb_t[]) {0x6da83b73, 0x83bd7801, 0x11c5e661, 0x35a63903, 0xed2fb5f, 0x1, }}};
#elif 8*DIGIT_LEN == 64
const uint64_t TORSION_PLUS_EVEN_POWER = 0xa8;
const uint64_t TORSION_PLUS_THREE_POWER = 0x65;
const ibz_t TORSION_PLUS_2POWER = {{._mp_alloc = 0, ._mp_size = 3, ._mp_d = (mp_limb_t[]) {0x0, 0x0, 0x10000000000, }}};
const ibz_t TORSION_PLUS_3POWER = {{._mp_alloc = 0, ._mp_size = 3, ._mp_d = (mp_limb_t[]) {0x83bd78016da83b73, 0x35a6390311c5e661, 0x10ed2fb5f, }}};
#endif