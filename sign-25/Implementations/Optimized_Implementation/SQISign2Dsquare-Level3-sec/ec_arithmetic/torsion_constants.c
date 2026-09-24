#include <torsion_constants.h>
#if 0
#elif 8*DIGIT_LEN == 32
const uint64_t TORSION_PLUS_EVEN_POWER = 0x108;
const uint64_t TORSION_PLUS_THREE_POWER = 0xa3;
const ibz_t TORSION_PLUS_2POWER = {{._mp_alloc = 0, ._mp_size = 9, ._mp_d = (mp_limb_t[]) {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x100, }}};
const ibz_t TORSION_PLUS_3POWER = {{._mp_alloc = 0, ._mp_size = 9, ._mp_d = (mp_limb_t[]) {0xea92759b, 0x69440131, 0xc1711941, 0x16455bc4, 0xf6c9762f, 0x84f88487, 0x76a1e71d, 0x1824e66e, 0x5, }}};
#elif 8*DIGIT_LEN == 64
const uint64_t TORSION_PLUS_EVEN_POWER = 0x108;
const uint64_t TORSION_PLUS_THREE_POWER = 0xa3;
const ibz_t TORSION_PLUS_2POWER = {{._mp_alloc = 0, ._mp_size = 5, ._mp_d = (mp_limb_t[]) {0x0, 0x0, 0x0, 0x0, 0x100, }}};
const ibz_t TORSION_PLUS_3POWER = {{._mp_alloc = 0, ._mp_size = 5, ._mp_d = (mp_limb_t[]) {0x69440131ea92759b, 0x16455bc4c1711941, 0x84f88487f6c9762f, 0x1824e66e76a1e71d, 0x5, }}};
#endif