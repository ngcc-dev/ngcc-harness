#include <torsion_constants.h>
#if 0
#elif 8*DIGIT_LEN == 32
const uint64_t TORSION_PLUS_EVEN_POWER = 0x89;
const uint64_t TORSION_PLUS_THREE_POWER = 0x54;
const ibz_t TORSION_PLUS_2POWER = {{._mp_alloc = 0, ._mp_size = 5, ._mp_d = (mp_limb_t[]) {0x0, 0x0, 0x0, 0x0, 0x200, }}};
const ibz_t TORSION_PLUS_3POWER = {{._mp_alloc = 0, ._mp_size = 5, ._mp_d = (mp_limb_t[]) {0x828c2891, 0x46264f29, 0x2cb7d8f, 0x2f1e8ccb, 0x23, }}};
#elif 8*DIGIT_LEN == 64
const uint64_t TORSION_PLUS_EVEN_POWER = 0x89;
const uint64_t TORSION_PLUS_THREE_POWER = 0x54;
const ibz_t TORSION_PLUS_2POWER = {{._mp_alloc = 0, ._mp_size = 3, ._mp_d = (mp_limb_t[]) {0x0, 0x0, 0x200, }}};
const ibz_t TORSION_PLUS_3POWER = {{._mp_alloc = 0, ._mp_size = 3, ._mp_d = (mp_limb_t[]) {0x46264f29828c2891, 0x2f1e8ccb02cb7d8f, 0x23, }}};
#endif