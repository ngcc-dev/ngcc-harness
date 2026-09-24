#include <stddef.h>
#include <stdint.h>
#include <quaternion_data.h>
const short QUAT_small_primes_where_neg_q_square[101] = {2, 5, 13, 17, 29, 37, 41, 53, 61, 73, 89, 97, 101, 109, 113, 137, 149, 157, 173, 181, 193, 197, 229, 233, 241, 0x101, 0x10d, 0x115, 0x119, 0x125, 0x139, 0x13d, 0x151, 0x15d, 0x161, 0x175, 0x185, 0x18d, 0x191, 0x199, 0x1a5, 0x1b1, 0x1c1, 0x1c9, 0x1cd, 0x1fd, 0x209, 0x21d, 0x22d, 0x239, 0x241, 0x251, 0x259, 0x265, 0x269, 0x281, 0x28d, 0x295, 0x2a1, 0x2a5, 0x2bd, 0x2c5, 0x2dd, 0x2f5, 0x2f9, 0x301, 0x305, 0x31d, 0x329, 0x335, 0x33d, 0x355, 0x359, 0x36d, 0x371, 0x3a1, 0x3a9, 0x3ad, 0x3b9, 0x3d1, 0x3e5, 0x3f1, 0x3f5, 0x3fd, 0x409, 0x419, 0x425, 0x42d, 0x445, 0x449, 0x455, 0x45d, 0x469, 0x481, 0x49d, 0x4a9, 0x4b1, 0x4bd, 0x4c1, 0x4cd, 0x4d5};
const ibz_t QUAT_prods_of_bad_primes[1] = {
#if 0
#elif GMP_LIMB_BITS == 16
{{._mp_alloc = 0, ._mp_size = 55, ._mp_d = (mp_limb_t[]) {0xca85,0xc3eb,0xeae8,0x4c6c,0x40c5,0x133e,0xe3da,0xad5f,0xdf5d,0x44c9,0x4331,0x8e,0xcaaf,0xd4d6,0xef47,0x3409,0xb9d1,0x1003,0x566,0x2832,0xbc41,0x23ee,0x88fa,0xf005,0xf459,0xc5b0,0x1e3a,0x1357,0xee1b,0x50e5,0x9788,0x68d7,0x84e3,0xa1e0,0xce7c,0xfc90,0x91b2,0x73b,0xece3,0x3826,0xca91,0x2fd1,0x2ada,0x142c,0x574d,0x13b8,0xc568,0xc99a,0x37db,0xae00,0xa077,0x7e44,0x1e80,0xf5c5,0x5}}}
#elif GMP_LIMB_BITS == 32
{{._mp_alloc = 0, ._mp_size = 28, ._mp_d = (mp_limb_t[]) {0xc3ebca85,0x4c6ceae8,0x133e40c5,0xad5fe3da,0x44c9df5d,0x8e4331,0xd4d6caaf,0x3409ef47,0x1003b9d1,0x28320566,0x23eebc41,0xf00588fa,0xc5b0f459,0x13571e3a,0x50e5ee1b,0x68d79788,0xa1e084e3,0xfc90ce7c,0x73b91b2,0x3826ece3,0x2fd1ca91,0x142c2ada,0x13b8574d,0xc99ac568,0xae0037db,0x7e44a077,0xf5c51e80,0x5}}}
#elif GMP_LIMB_BITS == 64
{{._mp_alloc = 0, ._mp_size = 14, ._mp_d = (mp_limb_t[]) {0x4c6ceae8c3ebca85,0xad5fe3da133e40c5,0x8e433144c9df5d,0x3409ef47d4d6caaf,0x283205661003b9d1,0xf00588fa23eebc41,0x13571e3ac5b0f459,0x68d7978850e5ee1b,0xfc90ce7ca1e084e3,0x3826ece3073b91b2,0x142c2ada2fd1ca91,0xc99ac56813b8574d,0x7e44a077ae0037db,0x5f5c51e80}}}
#endif
};
const ibz_t QUAT_prime_cofactor = 
#if 0
#elif GMP_LIMB_BITS == 16
{{._mp_alloc = 0, ._mp_size = 32, ._mp_d = (mp_limb_t[]) {0x33,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x200}}}
#elif GMP_LIMB_BITS == 32
{{._mp_alloc = 0, ._mp_size = 16, ._mp_d = (mp_limb_t[]) {0x33,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x2000000}}}
#elif GMP_LIMB_BITS == 64
{{._mp_alloc = 0, ._mp_size = 8, ._mp_d = (mp_limb_t[]) {0x33,0x0,0x0,0x0,0x0,0x0,0x0,0x200000000000000}}}
#endif
;
const quat_alg_t QUATALG_PINFTY = {
#if 0
#elif GMP_LIMB_BITS == 16
{{._mp_alloc = 0, ._mp_size = 32, ._mp_d = (mp_limb_t[]) {0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0x1af}}}
#elif GMP_LIMB_BITS == 32
{{._mp_alloc = 0, ._mp_size = 16, ._mp_d = (mp_limb_t[]) {0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0x1afffff}}}
#elif GMP_LIMB_BITS == 64
{{._mp_alloc = 0, ._mp_size = 8, ._mp_d = (mp_limb_t[]) {0xffffffffffffffff,0xffffffffffffffff,0xffffffffffffffff,0xffffffffffffffff,0xffffffffffffffff,0xffffffffffffffff,0xffffffffffffffff,0x1afffffffffffff}}}
#endif
};
const ibz_t QUATALG_PINFTY_ISQRT = 
#if 0
#elif GMP_LIMB_BITS == 16
{{._mp_alloc = 0, ._mp_size = 16, ._mp_d = (mp_limb_t[]) {0x43d3, 0x652b, 0x87cb, 0x2955, 0x3b5b, 0x93ee, 0x2279, 0x6dc6, 0xa2ca, 0x45a8, 0x7221, 0xc5c1, 0x7fd6, 0x4239, 0xdc2e, 0x14c8} }}
#elif GMP_LIMB_BITS == 32
{{._mp_alloc = 0, ._mp_size = 8, ._mp_d = (mp_limb_t[]) {0x652b43d3, 0x295587cb, 0x93ee3b5b, 0x6dc62279, 0x45a8a2ca, 0xc5c17221, 0x42397fd6, 0x14c8dc2e} }}
#elif GMP_LIMB_BITS == 64
{{._mp_alloc = 0, ._mp_size = 4, ._mp_d = (mp_limb_t[]) {0x295587cb652b43d3, 0x6dc6227993ee3b5b, 0xc5c1722145a8a2ca, 0x14c8dc2e42397fd6} }}
#endif
;
const quat_p_extremal_maximal_order_t EXTREMAL_ORDERS[1] = {{{
#if 0
#elif GMP_LIMB_BITS == 16
{{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x2}}}
#elif GMP_LIMB_BITS == 32
{{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x2}}}
#elif GMP_LIMB_BITS == 64
{{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x2}}}
#endif
, {{
#if 0
#elif GMP_LIMB_BITS == 16
{{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x2}}}
#elif GMP_LIMB_BITS == 32
{{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x2}}}
#elif GMP_LIMB_BITS == 64
{{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x2}}}
#endif
, 
#if 0
#elif GMP_LIMB_BITS == 16
{{._mp_alloc = 0, ._mp_size = 0, ._mp_d = (mp_limb_t[]) {0x0}}}
#elif GMP_LIMB_BITS == 32
{{._mp_alloc = 0, ._mp_size = 0, ._mp_d = (mp_limb_t[]) {0x0}}}
#elif GMP_LIMB_BITS == 64
{{._mp_alloc = 0, ._mp_size = 0, ._mp_d = (mp_limb_t[]) {0x0}}}
#endif
, 
#if 0
#elif GMP_LIMB_BITS == 16
{{._mp_alloc = 0, ._mp_size = 0, ._mp_d = (mp_limb_t[]) {0x0}}}
#elif GMP_LIMB_BITS == 32
{{._mp_alloc = 0, ._mp_size = 0, ._mp_d = (mp_limb_t[]) {0x0}}}
#elif GMP_LIMB_BITS == 64
{{._mp_alloc = 0, ._mp_size = 0, ._mp_d = (mp_limb_t[]) {0x0}}}
#endif
, 
#if 0
#elif GMP_LIMB_BITS == 16
{{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x1}}}
#elif GMP_LIMB_BITS == 32
{{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x1}}}
#elif GMP_LIMB_BITS == 64
{{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x1}}}
#endif
}, {
#if 0
#elif GMP_LIMB_BITS == 16
{{._mp_alloc = 0, ._mp_size = 0, ._mp_d = (mp_limb_t[]) {0x0}}}
#elif GMP_LIMB_BITS == 32
{{._mp_alloc = 0, ._mp_size = 0, ._mp_d = (mp_limb_t[]) {0x0}}}
#elif GMP_LIMB_BITS == 64
{{._mp_alloc = 0, ._mp_size = 0, ._mp_d = (mp_limb_t[]) {0x0}}}
#endif
, 
#if 0
#elif GMP_LIMB_BITS == 16
{{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x2}}}
#elif GMP_LIMB_BITS == 32
{{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x2}}}
#elif GMP_LIMB_BITS == 64
{{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x2}}}
#endif
, 
#if 0
#elif GMP_LIMB_BITS == 16
{{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x1}}}
#elif GMP_LIMB_BITS == 32
{{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x1}}}
#elif GMP_LIMB_BITS == 64
{{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x1}}}
#endif
, 
#if 0
#elif GMP_LIMB_BITS == 16
{{._mp_alloc = 0, ._mp_size = 0, ._mp_d = (mp_limb_t[]) {0x0}}}
#elif GMP_LIMB_BITS == 32
{{._mp_alloc = 0, ._mp_size = 0, ._mp_d = (mp_limb_t[]) {0x0}}}
#elif GMP_LIMB_BITS == 64
{{._mp_alloc = 0, ._mp_size = 0, ._mp_d = (mp_limb_t[]) {0x0}}}
#endif
}, {
#if 0
#elif GMP_LIMB_BITS == 16
{{._mp_alloc = 0, ._mp_size = 0, ._mp_d = (mp_limb_t[]) {0x0}}}
#elif GMP_LIMB_BITS == 32
{{._mp_alloc = 0, ._mp_size = 0, ._mp_d = (mp_limb_t[]) {0x0}}}
#elif GMP_LIMB_BITS == 64
{{._mp_alloc = 0, ._mp_size = 0, ._mp_d = (mp_limb_t[]) {0x0}}}
#endif
, 
#if 0
#elif GMP_LIMB_BITS == 16
{{._mp_alloc = 0, ._mp_size = 0, ._mp_d = (mp_limb_t[]) {0x0}}}
#elif GMP_LIMB_BITS == 32
{{._mp_alloc = 0, ._mp_size = 0, ._mp_d = (mp_limb_t[]) {0x0}}}
#elif GMP_LIMB_BITS == 64
{{._mp_alloc = 0, ._mp_size = 0, ._mp_d = (mp_limb_t[]) {0x0}}}
#endif
, 
#if 0
#elif GMP_LIMB_BITS == 16
{{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x1}}}
#elif GMP_LIMB_BITS == 32
{{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x1}}}
#elif GMP_LIMB_BITS == 64
{{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x1}}}
#endif
, 
#if 0
#elif GMP_LIMB_BITS == 16
{{._mp_alloc = 0, ._mp_size = 0, ._mp_d = (mp_limb_t[]) {0x0}}}
#elif GMP_LIMB_BITS == 32
{{._mp_alloc = 0, ._mp_size = 0, ._mp_d = (mp_limb_t[]) {0x0}}}
#elif GMP_LIMB_BITS == 64
{{._mp_alloc = 0, ._mp_size = 0, ._mp_d = (mp_limb_t[]) {0x0}}}
#endif
}, {
#if 0
#elif GMP_LIMB_BITS == 16
{{._mp_alloc = 0, ._mp_size = 0, ._mp_d = (mp_limb_t[]) {0x0}}}
#elif GMP_LIMB_BITS == 32
{{._mp_alloc = 0, ._mp_size = 0, ._mp_d = (mp_limb_t[]) {0x0}}}
#elif GMP_LIMB_BITS == 64
{{._mp_alloc = 0, ._mp_size = 0, ._mp_d = (mp_limb_t[]) {0x0}}}
#endif
, 
#if 0
#elif GMP_LIMB_BITS == 16
{{._mp_alloc = 0, ._mp_size = 0, ._mp_d = (mp_limb_t[]) {0x0}}}
#elif GMP_LIMB_BITS == 32
{{._mp_alloc = 0, ._mp_size = 0, ._mp_d = (mp_limb_t[]) {0x0}}}
#elif GMP_LIMB_BITS == 64
{{._mp_alloc = 0, ._mp_size = 0, ._mp_d = (mp_limb_t[]) {0x0}}}
#endif
, 
#if 0
#elif GMP_LIMB_BITS == 16
{{._mp_alloc = 0, ._mp_size = 0, ._mp_d = (mp_limb_t[]) {0x0}}}
#elif GMP_LIMB_BITS == 32
{{._mp_alloc = 0, ._mp_size = 0, ._mp_d = (mp_limb_t[]) {0x0}}}
#elif GMP_LIMB_BITS == 64
{{._mp_alloc = 0, ._mp_size = 0, ._mp_d = (mp_limb_t[]) {0x0}}}
#endif
, 
#if 0
#elif GMP_LIMB_BITS == 16
{{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x1}}}
#elif GMP_LIMB_BITS == 32
{{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x1}}}
#elif GMP_LIMB_BITS == 64
{{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x1}}}
#endif
}}}, {
#if 0
#elif GMP_LIMB_BITS == 16
{{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x2}}}
#elif GMP_LIMB_BITS == 32
{{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x2}}}
#elif GMP_LIMB_BITS == 64
{{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x2}}}
#endif
, {
#if 0
#elif GMP_LIMB_BITS == 16
{{._mp_alloc = 0, ._mp_size = 0, ._mp_d = (mp_limb_t[]) {0x0}}}
#elif GMP_LIMB_BITS == 32
{{._mp_alloc = 0, ._mp_size = 0, ._mp_d = (mp_limb_t[]) {0x0}}}
#elif GMP_LIMB_BITS == 64
{{._mp_alloc = 0, ._mp_size = 0, ._mp_d = (mp_limb_t[]) {0x0}}}
#endif
, 
#if 0
#elif GMP_LIMB_BITS == 16
{{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x2}}}
#elif GMP_LIMB_BITS == 32
{{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x2}}}
#elif GMP_LIMB_BITS == 64
{{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x2}}}
#endif
, 
#if 0
#elif GMP_LIMB_BITS == 16
{{._mp_alloc = 0, ._mp_size = 0, ._mp_d = (mp_limb_t[]) {0x0}}}
#elif GMP_LIMB_BITS == 32
{{._mp_alloc = 0, ._mp_size = 0, ._mp_d = (mp_limb_t[]) {0x0}}}
#elif GMP_LIMB_BITS == 64
{{._mp_alloc = 0, ._mp_size = 0, ._mp_d = (mp_limb_t[]) {0x0}}}
#endif
, 
#if 0
#elif GMP_LIMB_BITS == 16
{{._mp_alloc = 0, ._mp_size = 0, ._mp_d = (mp_limb_t[]) {0x0}}}
#elif GMP_LIMB_BITS == 32
{{._mp_alloc = 0, ._mp_size = 0, ._mp_d = (mp_limb_t[]) {0x0}}}
#elif GMP_LIMB_BITS == 64
{{._mp_alloc = 0, ._mp_size = 0, ._mp_d = (mp_limb_t[]) {0x0}}}
#endif
}}, {
#if 0
#elif GMP_LIMB_BITS == 16
{{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x1}}}
#elif GMP_LIMB_BITS == 32
{{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x1}}}
#elif GMP_LIMB_BITS == 64
{{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x1}}}
#endif
, {
#if 0
#elif GMP_LIMB_BITS == 16
{{._mp_alloc = 0, ._mp_size = 0, ._mp_d = (mp_limb_t[]) {0x0}}}
#elif GMP_LIMB_BITS == 32
{{._mp_alloc = 0, ._mp_size = 0, ._mp_d = (mp_limb_t[]) {0x0}}}
#elif GMP_LIMB_BITS == 64
{{._mp_alloc = 0, ._mp_size = 0, ._mp_d = (mp_limb_t[]) {0x0}}}
#endif
, 
#if 0
#elif GMP_LIMB_BITS == 16
{{._mp_alloc = 0, ._mp_size = 0, ._mp_d = (mp_limb_t[]) {0x0}}}
#elif GMP_LIMB_BITS == 32
{{._mp_alloc = 0, ._mp_size = 0, ._mp_d = (mp_limb_t[]) {0x0}}}
#elif GMP_LIMB_BITS == 64
{{._mp_alloc = 0, ._mp_size = 0, ._mp_d = (mp_limb_t[]) {0x0}}}
#endif
, 
#if 0
#elif GMP_LIMB_BITS == 16
{{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x1}}}
#elif GMP_LIMB_BITS == 32
{{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x1}}}
#elif GMP_LIMB_BITS == 64
{{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x1}}}
#endif
, 
#if 0
#elif GMP_LIMB_BITS == 16
{{._mp_alloc = 0, ._mp_size = 0, ._mp_d = (mp_limb_t[]) {0x0}}}
#elif GMP_LIMB_BITS == 32
{{._mp_alloc = 0, ._mp_size = 0, ._mp_d = (mp_limb_t[]) {0x0}}}
#elif GMP_LIMB_BITS == 64
{{._mp_alloc = 0, ._mp_size = 0, ._mp_d = (mp_limb_t[]) {0x0}}}
#endif
}}, 1}};
