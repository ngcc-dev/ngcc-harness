#include <torsion_constants.h>
#if 0
#elif 8*DIGIT_LEN == 32
const uint64_t TORSION_PLUS_EVEN_POWER = 0x83;
const uint64_t TORSION_PLUS_THREE_POWER = 0x4e;
const ibz_t TORSION_PLUS_2POWER = {{._mp_alloc = 0, ._mp_size = 5, ._mp_d = (mp_limb_t[]) {0x0, 0x0, 0x0, 0x0, 0x8, }}};
const ibz_t TORSION_PLUS_3POWER = {{._mp_alloc = 0, ._mp_size = 4, ._mp_d = (mp_limb_t[]) {0xd87f5079, 0x94fd9829, 0xf302bcbf, 0xc5afe6f, }}};
#elif 8*DIGIT_LEN == 64
const uint64_t TORSION_PLUS_EVEN_POWER = 0x83;
const uint64_t TORSION_PLUS_THREE_POWER = 0x4e;
const ibz_t TORSION_PLUS_2POWER = {{._mp_alloc = 0, ._mp_size = 3, ._mp_d = (mp_limb_t[]) {0x0, 0x0, 0x8, }}};
const ibz_t TORSION_PLUS_3POWER = {{._mp_alloc = 0, ._mp_size = 2, ._mp_d = (mp_limb_t[]) {0x94fd9829d87f5079, 0xc5afe6ff302bcbf, }}};
#endif