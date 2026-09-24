#include <stddef.h>
#include <stdint.h>
#include <torsion_constants.h>
#if 0
#elif 8*DIGIT_LEN == 16
const uint64_t TORSION_PLUS_EVEN_POWER = 0x107;
const uint64_t TORSION_ODD_PRIMES[5] = {0x3, 0xb, 0x11, 0x3b, 0x3b3};
const uint64_t TORSION_ODD_POWERS[5] = {0x9c, 0x1, 0x1, 0x1, 0x1};
const uint64_t TORSION_PLUS_ODD_PRIMES[1] = {0x3};
const size_t TORSION_PLUS_ODD_POWERS[1] = {0x9c};
const uint64_t TORSION_MINUS_ODD_PRIMES[4] = {0xb, 0x11, 0x3b, 0x3b3};
const size_t TORSION_MINUS_ODD_POWERS[4] = {0x1, 0x1, 0x1, 0x1};
const size_t DEGREE_COMMITMENT_POWERS[5] = {0x0, 0x1, 0x1, 0x1, 0x1};
const ibz_t CHARACTERISTIC = {{._mp_alloc = 0, ._mp_size = 32, ._mp_d = (mp_limb_t[]) {0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0x987f,0xacec,0xef90,0x4c46,0x969b,0xc552,0xa1c1,0xb8d8,0x20,0x4b6e,0x5c45,0x234a,0x665f,0x30bb,0x1931,0x4c54}}};
const ibz_t TORSION_ODD = {{._mp_alloc = 0, ._mp_size = 17, ._mp_d = (mp_limb_t[]) {0x378b,0x2d14,0xe236,0x4d49,0x85b8,0xd306,0x9d8e,0xf308,0x7599,0x7650,0x1b5f,0xd7da,0x6f39,0x8d93,0x5ea1,0xc041,0x5f11}}};
const ibz_t TORSION_ODD_PRIMEPOWERS[5] = {{{._mp_alloc = 0, ._mp_size = 16, ._mp_d = (mp_limb_t[]) {0xd931,0x2159,0x8ddf,0x3698,0xa52d,0x838a,0xb143,0x4171,0xdc00,0x8a96,0x94b8,0xbe46,0x76cc,0x6261,0xa832,0x98}}}, {{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0xb}}}, {{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x11}}}, {{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x3b}}}, {{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x3b3}}}};
const ibz_t TORSION_ODD_PLUS = {{._mp_alloc = 0, ._mp_size = 16, ._mp_d = (mp_limb_t[]) {0xd931,0x2159,0x8ddf,0x3698,0xa52d,0x838a,0xb143,0x4171,0xdc00,0x8a96,0x94b8,0xbe46,0x76cc,0x6261,0xa832,0x98}}};
const ibz_t TORSION_ODD_MINUS = {{._mp_alloc = 0, ._mp_size = 2, ._mp_d = (mp_limb_t[]) {0x6d7b,0x9f}}};
const ibz_t TORSION_PLUS_2POWER = {{._mp_alloc = 0, ._mp_size = 17, ._mp_d = (mp_limb_t[]) {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x80}}};
const ibz_t TORSION_PLUS_3POWER = {{._mp_alloc = 0, ._mp_size = 16, ._mp_d = (mp_limb_t[]) {0xd931,0x2159,0x8ddf,0x3698,0xa52d,0x838a,0xb143,0x4171,0xdc00,0x8a96,0x94b8,0xbe46,0x76cc,0x6261,0xa832,0x98}}};
const ibz_t TORSION_PLUS_23POWER = {{._mp_alloc = 0, ._mp_size = 32, ._mp_d = (mp_limb_t[]) {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x9880,0xacec,0xef90,0x4c46,0x969b,0xc552,0xa1c1,0xb8d8,0x20,0x4b6e,0x5c45,0x234a,0x665f,0x30bb,0x1931,0x4c54}}};
const ibz_t DEGREE_COMMITMENT = {{._mp_alloc = 0, ._mp_size = 2, ._mp_d = (mp_limb_t[]) {0x6d7b,0x9f}}};
const ibz_t DEGREE_COMMITMENT_PLUS = {{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x1}}};
const ibz_t DEGREE_COMMITMENT_MINUS = {{._mp_alloc = 0, ._mp_size = 2, ._mp_d = (mp_limb_t[]) {0x6d7b,0x9f}}};
const ibz_t DEGREE_CHALLENGE = {{._mp_alloc = 0, ._mp_size = 32, ._mp_d = (mp_limb_t[]) {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x9880,0xacec,0xef90,0x4c46,0x969b,0xc552,0xa1c1,0xb8d8,0x20,0x4b6e,0x5c45,0x234a,0x665f,0x30bb,0x1931,0x4c54}}};
#elif 8*DIGIT_LEN == 32
const uint64_t TORSION_PLUS_EVEN_POWER = 0x107;
const uint64_t TORSION_ODD_PRIMES[5] = {0x3, 0xb, 0x11, 0x3b, 0x3b3};
const uint64_t TORSION_ODD_POWERS[5] = {0x9c, 0x1, 0x1, 0x1, 0x1};
const uint64_t TORSION_PLUS_ODD_PRIMES[1] = {0x3};
const size_t TORSION_PLUS_ODD_POWERS[1] = {0x9c};
const uint64_t TORSION_MINUS_ODD_PRIMES[4] = {0xb, 0x11, 0x3b, 0x3b3};
const size_t TORSION_MINUS_ODD_POWERS[4] = {0x1, 0x1, 0x1, 0x1};
const size_t DEGREE_COMMITMENT_POWERS[5] = {0x0, 0x1, 0x1, 0x1, 0x1};
const ibz_t CHARACTERISTIC = {{._mp_alloc = 0, ._mp_size = 16, ._mp_d = (mp_limb_t[]) {0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xacec987f,0x4c46ef90,0xc552969b,0xb8d8a1c1,0x4b6e0020,0x234a5c45,0x30bb665f,0x4c541931}}};
const ibz_t TORSION_ODD = {{._mp_alloc = 0, ._mp_size = 9, ._mp_d = (mp_limb_t[]) {0x2d14378b,0x4d49e236,0xd30685b8,0xf3089d8e,0x76507599,0xd7da1b5f,0x8d936f39,0xc0415ea1,0x5f11}}};
const ibz_t TORSION_ODD_PRIMEPOWERS[5] = {{{._mp_alloc = 0, ._mp_size = 8, ._mp_d = (mp_limb_t[]) {0x2159d931,0x36988ddf,0x838aa52d,0x4171b143,0x8a96dc00,0xbe4694b8,0x626176cc,0x98a832}}}, {{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0xb}}}, {{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x11}}}, {{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x3b}}}, {{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x3b3}}}};
const ibz_t TORSION_ODD_PLUS = {{._mp_alloc = 0, ._mp_size = 8, ._mp_d = (mp_limb_t[]) {0x2159d931,0x36988ddf,0x838aa52d,0x4171b143,0x8a96dc00,0xbe4694b8,0x626176cc,0x98a832}}};
const ibz_t TORSION_ODD_MINUS = {{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x9f6d7b}}};
const ibz_t TORSION_PLUS_2POWER = {{._mp_alloc = 0, ._mp_size = 9, ._mp_d = (mp_limb_t[]) {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x80}}};
const ibz_t TORSION_PLUS_3POWER = {{._mp_alloc = 0, ._mp_size = 8, ._mp_d = (mp_limb_t[]) {0x2159d931,0x36988ddf,0x838aa52d,0x4171b143,0x8a96dc00,0xbe4694b8,0x626176cc,0x98a832}}};
const ibz_t TORSION_PLUS_23POWER = {{._mp_alloc = 0, ._mp_size = 16, ._mp_d = (mp_limb_t[]) {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0xacec9880,0x4c46ef90,0xc552969b,0xb8d8a1c1,0x4b6e0020,0x234a5c45,0x30bb665f,0x4c541931}}};
const ibz_t DEGREE_COMMITMENT = {{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x9f6d7b}}};
const ibz_t DEGREE_COMMITMENT_PLUS = {{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x1}}};
const ibz_t DEGREE_COMMITMENT_MINUS = {{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x9f6d7b}}};
const ibz_t DEGREE_CHALLENGE = {{._mp_alloc = 0, ._mp_size = 16, ._mp_d = (mp_limb_t[]) {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0xacec9880,0x4c46ef90,0xc552969b,0xb8d8a1c1,0x4b6e0020,0x234a5c45,0x30bb665f,0x4c541931}}};
#elif 8*DIGIT_LEN == 64
const uint64_t TORSION_PLUS_EVEN_POWER = 0x107;
const uint64_t TORSION_ODD_PRIMES[5] = {0x3, 0xb, 0x11, 0x3b, 0x3b3};
const uint64_t TORSION_ODD_POWERS[5] = {0x9c, 0x1, 0x1, 0x1, 0x1};
const uint64_t TORSION_PLUS_ODD_PRIMES[1] = {0x3};
const size_t TORSION_PLUS_ODD_POWERS[1] = {0x9c};
const uint64_t TORSION_MINUS_ODD_PRIMES[4] = {0xb, 0x11, 0x3b, 0x3b3};
const size_t TORSION_MINUS_ODD_POWERS[4] = {0x1, 0x1, 0x1, 0x1};
const size_t DEGREE_COMMITMENT_POWERS[5] = {0x0, 0x1, 0x1, 0x1, 0x1};
const ibz_t CHARACTERISTIC = {{._mp_alloc = 0, ._mp_size = 8, ._mp_d = (mp_limb_t[]) {0xffffffffffffffff,0xffffffffffffffff,0xffffffffffffffff,0xffffffffffffffff,0x4c46ef90acec987f,0xb8d8a1c1c552969b,0x234a5c454b6e0020,0x4c54193130bb665f}}};
const ibz_t TORSION_ODD = {{._mp_alloc = 0, ._mp_size = 5, ._mp_d = (mp_limb_t[]) {0x4d49e2362d14378b,0xf3089d8ed30685b8,0xd7da1b5f76507599,0xc0415ea18d936f39,0x5f11}}};
const ibz_t TORSION_ODD_PRIMEPOWERS[5] = {{{._mp_alloc = 0, ._mp_size = 4, ._mp_d = (mp_limb_t[]) {0x36988ddf2159d931,0x4171b143838aa52d,0xbe4694b88a96dc00,0x98a832626176cc}}}, {{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0xb}}}, {{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x11}}}, {{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x3b}}}, {{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x3b3}}}};
const ibz_t TORSION_ODD_PLUS = {{._mp_alloc = 0, ._mp_size = 4, ._mp_d = (mp_limb_t[]) {0x36988ddf2159d931,0x4171b143838aa52d,0xbe4694b88a96dc00,0x98a832626176cc}}};
const ibz_t TORSION_ODD_MINUS = {{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x9f6d7b}}};
const ibz_t TORSION_PLUS_2POWER = {{._mp_alloc = 0, ._mp_size = 5, ._mp_d = (mp_limb_t[]) {0x0,0x0,0x0,0x0,0x80}}};
const ibz_t TORSION_PLUS_3POWER = {{._mp_alloc = 0, ._mp_size = 4, ._mp_d = (mp_limb_t[]) {0x36988ddf2159d931,0x4171b143838aa52d,0xbe4694b88a96dc00,0x98a832626176cc}}};
const ibz_t TORSION_PLUS_23POWER = {{._mp_alloc = 0, ._mp_size = 8, ._mp_d = (mp_limb_t[]) {0x0,0x0,0x0,0x0,0x4c46ef90acec9880,0xb8d8a1c1c552969b,0x234a5c454b6e0020,0x4c54193130bb665f}}};
const ibz_t DEGREE_COMMITMENT = {{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x9f6d7b}}};
const ibz_t DEGREE_COMMITMENT_PLUS = {{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x1}}};
const ibz_t DEGREE_COMMITMENT_MINUS = {{._mp_alloc = 0, ._mp_size = 1, ._mp_d = (mp_limb_t[]) {0x9f6d7b}}};
const ibz_t DEGREE_CHALLENGE = {{._mp_alloc = 0, ._mp_size = 8, ._mp_d = (mp_limb_t[]) {0x0,0x0,0x0,0x0,0x4c46ef90acec9880,0xb8d8a1c1c552969b,0x234a5c454b6e0020,0x4c54193130bb665f}}};
#endif
