#include <torsion_constants.h>
#if 0
#elif 8*DIGIT_LEN == 32
const uint64_t TORSION_PLUS_EVEN_POWER = 0x107;
const uint64_t TORSION_PLUS_THREE_POWER = 0x9c;
const ibz_t TORSION_PLUS_2POWER = {{._mp_alloc = 0, ._mp_size = 9, ._mp_d = (mp_limb_t[]) {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x80, }}};
const ibz_t TORSION_PLUS_3POWER = {{._mp_alloc = 0, ._mp_size = 8, ._mp_d = (mp_limb_t[]) {0x2159d931, 0x36988ddf, 0x838aa52d, 0x4171b143, 0x8a96dc00, 0xbe4694b8, 0x626176cc, 0x98a832, }}};
#elif 8*DIGIT_LEN == 64
const uint64_t TORSION_PLUS_EVEN_POWER = 0x107;
const uint64_t TORSION_PLUS_THREE_POWER = 0x9c;
const ibz_t TORSION_PLUS_2POWER = {{._mp_alloc = 0, ._mp_size = 5, ._mp_d = (mp_limb_t[]) {0x0, 0x0, 0x0, 0x0, 0x80, }}};
const ibz_t TORSION_PLUS_3POWER = {{._mp_alloc = 0, ._mp_size = 4, ._mp_d = (mp_limb_t[]) {0x36988ddf2159d931, 0x4171b143838aa52d, 0xbe4694b88a96dc00, 0x98a832626176cc, }}};
#endif