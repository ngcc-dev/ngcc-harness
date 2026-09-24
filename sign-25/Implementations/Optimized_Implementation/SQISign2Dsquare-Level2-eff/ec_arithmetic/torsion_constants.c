#include <torsion_constants.h>
#if 0
#elif 8*DIGIT_LEN == 32
const uint64_t TORSION_PLUS_EVEN_POWER = 0xa1;
const uint64_t TORSION_PLUS_THREE_POWER = 0x60;
const ibz_t TORSION_PLUS_2POWER = {{._mp_alloc = 0, ._mp_size = 6, ._mp_d = (mp_limb_t[]) {0x0, 0x0, 0x0, 0x0, 0x0, 0x2, }}};
const ibz_t TORSION_PLUS_3POWER = {{._mp_alloc = 0, ._mp_size = 5, ._mp_d = (mp_limb_t[]) {0xf2c17b81, 0x8db5fb39, 0x4498179, 0xfaf40ac5, 0x11d500b, }}};
#elif 8*DIGIT_LEN == 64
const uint64_t TORSION_PLUS_EVEN_POWER = 0xa1;
const uint64_t TORSION_PLUS_THREE_POWER = 0x60;
const ibz_t TORSION_PLUS_2POWER = {{._mp_alloc = 0, ._mp_size = 3, ._mp_d = (mp_limb_t[]) {0x0, 0x0, 0x200000000, }}};
const ibz_t TORSION_PLUS_3POWER = {{._mp_alloc = 0, ._mp_size = 3, ._mp_d = (mp_limb_t[]) {0x8db5fb39f2c17b81, 0xfaf40ac504498179, 0x11d500b, }}};
#endif