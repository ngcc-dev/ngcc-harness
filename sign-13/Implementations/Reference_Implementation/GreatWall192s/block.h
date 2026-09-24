#ifndef BLOCK_H
#define BLOCK_H

#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>
#include "config.h"

typedef struct
{
	uint64_t data[3];
} block137;

typedef struct
{
	uint64_t data[3];
} block192;

typedef struct
{
	uint64_t data[4];
} block197;

typedef struct
{
	uint64_t data[5];
} block263;

typedef struct
{
	uint64_t data[9];
} block521;

#define BLOCK521_WORDS 9
#define BLOCK521_BYTES 66

static inline void block521_canonicalize(block521* a) { a->data[8] &= 0x1ffULL; }
static inline bool block521_is_canonical(const block521* a) { return (a->data[8] & ~0x1ffULL) == 0; }
static inline void block521_load_to_block(block521* out, const uint8_t* in)
{
	assert((in[65] & 0xfe) == 0);
	memset(out, 0, sizeof(*out));
	memcpy(out, in, BLOCK521_BYTES);
	block521_canonicalize(out);
}
static inline void block521_load_words(block521* out, const uint64_t in[BLOCK521_WORDS])
{
	memcpy(out->data, in, sizeof(out->data));
	assert((in[8] & ~0x1ffULL) == 0);
	block521_canonicalize(out);
}
static inline void block521_store_from_block(uint8_t* out, const block521* in)
{
	assert(block521_is_canonical(in));
	memcpy(out, in->data, BLOCK521_BYTES);
}
static inline block521 block521_xor(block521 x, block521 y)
{
	block521 out;
	for (size_t i = 0; i < BLOCK521_WORDS; ++i) out.data[i] = x.data[i] ^ y.data[i];
	block521_canonicalize(&out);
	return out;
}

static inline void block137_canonicalize(block137* a) {
    a->data[2] &= 0x1FFULL;
}

static inline void block137_load_to_block(block137* out, const uint8_t* in)
{
	assert((in[17] & 0xFE) == 0);
    memset(out, 0, sizeof(*out));
    memcpy(out, in, 18);
    out->data[2] &= 0x1FFULL;
}

static inline void block137_load_words(block137* out, const uint64_t in[3])
{
    out->data[0] = in[0];
    out->data[1] = in[1];
    out->data[2] = in[2] & 0x1FFULL;

    assert((in[2] & ~0x1FFULL) == 0);
}

static inline bool block137_is_canonical(const block137* in)
{
    return (in->data[2] & ~0x1FFULL) == 0;
}

static inline void block137_store_from_block(uint8_t* out, const block137* in)
{
	assert(block137_is_canonical(in));
	memcpy(out, in, 18);
}
inline block137 block137_xor(block137 x, block137 y)
{
	block137 out;
	out.data[0] = x.data[0] ^ y.data[0];
	out.data[1] = x.data[1] ^ y.data[1];
	out.data[2] = x.data[2] ^ y.data[2];
	return out;
}


static inline void block197_canonicalize(block197* a) {
    a->data[3] &= 0x1FULL;
}

static inline bool block197_is_canonical(const block197* in)
{
    return (in->data[3] & ~0x1FULL) == 0;
}

static inline void block197_load_to_block(block197* out, const uint8_t* in)
{
    assert((in[24] & 0xE0) == 0);
    memset(out, 0, sizeof(*out));
    memcpy(out, in, 25);
    out->data[3] &= 0x1FULL;
}

static inline void block197_load_words(block197* out, const uint64_t in[4])
{
    out->data[0] = in[0];
    out->data[1] = in[1];
    out->data[2] = in[2];
    out->data[3] = in[3] & 0x1FULL;

    assert((in[3] & ~0x1FULL) == 0);
}

static inline void block197_store_from_block(uint8_t* out, const block197* in)
{
    assert(block197_is_canonical(in));
    memcpy(out, in, 25);
}

static inline block197 block197_xor(block197 x, block197 y)
{
    block197 out;
    out.data[0] = x.data[0] ^ y.data[0];
    out.data[1] = x.data[1] ^ y.data[1];
    out.data[2] = x.data[2] ^ y.data[2];
    out.data[3] = (x.data[3] ^ y.data[3]) & 0x1FULL;
    return out;
}

static inline void block263_canonicalize(block263* a) {
    a->data[4] &= 0x7FULL;
}

static inline bool block263_is_canonical(const block263* in)
{
    return (in->data[4] & ~0x7FULL) == 0;
}

static inline void block263_load_to_block(block263* out, const uint8_t* in)
{
    assert((in[32] & 0x80) == 0);

    memset(out, 0, sizeof(*out));
    memcpy(out, in, 33);

    out->data[4] &= 0x7FULL;
}

static inline void block263_load_words(block263* out, const uint64_t in[5])
{
    out->data[0] = in[0];
    out->data[1] = in[1];
    out->data[2] = in[2];
    out->data[3] = in[3];
    out->data[4] = in[4] & 0x7FULL;

    assert((in[4] & ~0x7FULL) == 0);
}

static inline void block263_store_from_block(uint8_t* out, const block263* in)
{
    assert(block263_is_canonical(in));

    memcpy(out, in, 33);

    out[32] &= 0x7F;
}

static inline block263 block263_xor(block263 x, block263 y)
{
    block263 out;

    out.data[0] = x.data[0] ^ y.data[0];
    out.data[1] = x.data[1] ^ y.data[1];
    out.data[2] = x.data[2] ^ y.data[2];
    out.data[3] = x.data[3] ^ y.data[3];
    out.data[4] = (x.data[4] ^ y.data[4]) & 0x7FULL;

    return out;
}

inline block192 block192_xor(block192 x, block192 y)
{
	block192 out;
	out.data[0] = x.data[0] ^ y.data[0];
	out.data[1] = x.data[1] ^ y.data[1];
	out.data[2] = x.data[2] ^ y.data[2];
	return out;
}

inline block192 block192_and(block192 x, block192 y)
{
	block192 out;
	out.data[0] = x.data[0] & y.data[0];
	out.data[1] = x.data[1] & y.data[1];
	out.data[2] = x.data[2] & y.data[2];
	return out;
}

inline block192 block192_set_all_8(uint8_t x)
{
	uint64_t x64 = x;
	x64 *= UINT64_MAX / 0xff;

	block192 out = {{x64, x64, x64}};
	return out;
}

inline block192 block192_set_low64(uint64_t x)
{
	block192 out = {{x, 0, 0}};
	return out;
}

inline block192 block192_set_low32(uint32_t x)
{
	return block192_set_low64(x);
}

inline block192 block192_set_zero()
{
	return block192_set_low64(0);
}

#include "block_impl.h"



#define VOLE_BLOCK (1 << VOLE_BLOCK_SHIFT)

static_assert(sizeof(block128) == 16, "Padding in block128.");
static_assert(sizeof(block192) == 24, "Padding in block192.");
static_assert(sizeof(block256) == 32, "Padding in block256.");
static_assert(sizeof(block384) == 48, "Padding in block384.");
static_assert(sizeof(block512) == 64, "Padding in block512.");
static_assert(sizeof(block1024) == 128, "Padding in block1024.");

inline block128 block128_xor(block128 x, block128 y);
inline block256 block256_xor(block256 x, block256 y);
inline block384 block384_xor(block384 x, block384 y);
inline block512 block512_xor(block512 x, block512 y);
inline block1024 block1024_xor(block1024 x, block1024 y);
inline vole_block vole_block_xor(vole_block x, vole_block y);

inline block128 block128_and(block128 x, block128 y);
inline block256 block256_and(block256 x, block256 y);
inline block384 block384_and(block384 x, block384 y);
inline block512 block512_and(block512 x, block512 y);
inline block1024 block1024_and(block1024 x, block1024 y);
inline vole_block vole_block_and(vole_block x, vole_block y);

inline block128 block128_set_zero();
inline block256 block256_set_zero();
inline block384 block384_set_zero();
inline block512 block512_set_zero();
inline block1024 block1024_set_zero();

inline block128 block128_set_all_8(uint8_t x);
inline block256 block256_set_all_8(uint8_t x);
inline block384 block384_set_all_8(uint8_t x);
inline block512 block512_set_all_8(uint8_t x);
inline block1024 block1024_set_all_8(uint8_t x);
inline vole_block vole_block_set_all_8(uint8_t x);

inline block128 block128_set_low32(uint32_t x);
inline block256 block256_set_low32(uint32_t x);
inline block384 block384_set_low32(uint32_t x);
inline block512 block512_set_low32(uint32_t x);
inline block1024 block1024_set_low32(uint32_t x);
inline vole_block vole_block_set_low32(uint32_t x);

inline block128 block128_set_low64(uint64_t x);
inline block256 block256_set_low64(uint64_t x);
inline block384 block384_set_low64(uint64_t x);
inline block512 block512_set_low64(uint64_t x);
inline block1024 block1024_set_low64(uint64_t x);
inline vole_block vole_block_set_low64(uint64_t x);

inline block256 block256_set_128(block128 x0, block128 x1);
inline block256 block256_set_low128(block128 x);

inline bool block128_any_zeros(block128 x);
inline bool block192_any_zeros(block192 x);
inline bool block256_any_zeros(block256 x);
inline bool block512_any_zeros(block512 x);

inline block128 block128_byte_reverse(block128 x);

inline block256 block256_from_2_block128(block128 x, block128 y);

#if SECURITY_PARAM == 128
#define BLOCK_SECPAR_LEN_SHIFT 0
#define BLOCK_2SECPAR_LEN 2
typedef block128 block_secpar;
typedef block256 block_2secpar;
inline block_secpar block_secpar_xor(block_secpar x, block_secpar y) { return block128_xor(x, y); }
inline block_secpar block_secpar_and(block_secpar x, block_secpar y) { return block128_and(x, y); }
inline block_secpar block_secpar_set_all_8(uint8_t x) { return block128_set_all_8(x); }
inline block_secpar block_secpar_set_low32(uint32_t x) { return block128_set_low32(x); }
inline block_secpar block_secpar_set_low64(uint64_t x) { return block128_set_low64(x); }
inline block_secpar block_secpar_set_zero() { return block128_set_zero(); }
inline bool block_secpar_any_zeros(block_secpar x) { return block128_any_zeros(x); }
inline block_2secpar block_2secpar_xor(block_2secpar x, block_2secpar y) { return block256_xor(x, y); }
inline block_2secpar block_2secpar_and(block_2secpar x, block_2secpar y) { return block256_and(x, y); }
inline block_2secpar block_2secpar_set_all_8(uint8_t x) { return block256_set_all_8(x); }
inline block_2secpar block_2secpar_set_low32(uint32_t x) { return block256_set_low32(x); }
inline block_2secpar block_2secpar_set_low64(uint64_t x) { return block256_set_low64(x); }
inline block_2secpar block_2secpar_set_zero() { return block256_set_zero(); }
#elif SECURITY_PARAM == 192
#define BLOCK_2SECPAR_LEN 3
typedef block192 block_secpar;
typedef block384 block_2secpar;
inline block_secpar block_secpar_xor(block_secpar x, block_secpar y) { return block192_xor(x, y); }
inline block_secpar block_secpar_and(block_secpar x, block_secpar y) { return block192_and(x, y); }
inline block_secpar block_secpar_set_all_8(uint8_t x) { return block192_set_all_8(x); }
inline block_secpar block_secpar_set_low32(uint32_t x) { return block192_set_low32(x); }
inline block_secpar block_secpar_set_low64(uint64_t x) { return block192_set_low64(x); }
inline block_secpar block_secpar_set_zero() { return block192_set_zero(); }
inline bool block_secpar_any_zeros(block_secpar x) { return block192_any_zeros(x); }
inline block_2secpar block_2secpar_xor(block_2secpar x, block_2secpar y) { return block384_xor(x, y); }
inline block_2secpar block_2secpar_and(block_2secpar x, block_2secpar y) { return block384_and(x, y); }
inline block_2secpar block_2secpar_set_all_8(uint8_t x) { return block384_set_all_8(x); }
inline block_2secpar block_2secpar_set_low32(uint32_t x) { return block384_set_low32(x); }
inline block_2secpar block_2secpar_set_low64(uint64_t x) { return block384_set_low64(x); }
inline block_2secpar block_2secpar_set_zero() { return block384_set_zero(); }
#elif SECURITY_PARAM == 256
#define BLOCK_SECPAR_LEN_SHIFT 1
#define BLOCK_2SECPAR_LEN 4
typedef block256 block_secpar;
typedef block512 block_2secpar;
inline block_secpar block_secpar_xor(block_secpar x, block_secpar y) { return block256_xor(x, y); }
inline block_secpar block_secpar_and(block_secpar x, block_secpar y) { return block256_and(x, y); }
inline block_secpar block_secpar_set_all_8(uint8_t x) { return block256_set_all_8(x); }
inline block_secpar block_secpar_set_low32(uint32_t x) { return block256_set_low32(x); }
inline block_secpar block_secpar_set_low64(uint64_t x) { return block256_set_low64(x); }
inline block_secpar block_secpar_set_zero() { return block256_set_zero(); }
inline bool block_secpar_any_zeros(block_secpar x) { return block256_any_zeros(x); }
inline block_2secpar block_2secpar_xor(block_2secpar x, block_2secpar y) { return block512_xor(x, y); }
inline block_2secpar block_2secpar_and(block_2secpar x, block_2secpar y) { return block512_and(x, y); }
inline block_2secpar block_2secpar_set_all_8(uint8_t x) { return block512_set_all_8(x); }
inline block_2secpar block_2secpar_set_low32(uint32_t x) { return block512_set_low32(x); }
inline block_2secpar block_2secpar_set_low64(uint64_t x) { return block512_set_low64(x); }
inline block_2secpar block_2secpar_set_zero() { return block512_set_zero(); }
#elif SECURITY_PARAM == 512
#define BLOCK_SECPAR_LEN_SHIFT 2
#define BLOCK_2SECPAR_LEN 8
typedef block512 block_secpar;
typedef block1024 block_2secpar;
inline block_secpar block_secpar_xor(block_secpar x, block_secpar y) { return block512_xor(x,y); }
inline block_secpar block_secpar_and(block_secpar x, block_secpar y) { return block512_and(x,y); }
inline block_secpar block_secpar_set_all_8(uint8_t x) { return block512_set_all_8(x); }
inline block_secpar block_secpar_set_low32(uint32_t x) { return block512_set_low32(x); }
inline block_secpar block_secpar_set_low64(uint64_t x) { return block512_set_low64(x); }
inline block_secpar block_secpar_set_zero() { return block512_set_zero(); }
inline bool block_secpar_any_zeros(block_secpar x) { return block512_any_zeros(x); }
inline block_2secpar block_2secpar_xor(block_2secpar x, block_2secpar y) { return block1024_xor(x,y); }
inline block_2secpar block_2secpar_and(block_2secpar x, block_2secpar y) { return block1024_and(x,y); }
inline block_2secpar block_2secpar_set_all_8(uint8_t x) { return block1024_set_all_8(x); }
inline block_2secpar block_2secpar_set_low32(uint32_t x) { return block1024_set_low32(x); }
inline block_2secpar block_2secpar_set_low64(uint64_t x) { return block1024_set_low64(x); }
inline block_2secpar block_2secpar_set_zero() { return block1024_set_zero(); }
#endif

#define BLOCK_SECPAR_LEN (1 << BLOCK_SECPAR_LEN_SHIFT)

#endif
