#ifndef POLYNOMIALS_H
#define POLYNOMIALS_H

#include <stdbool.h>
#include <inttypes.h>

#define POLY_VEC_LEN (1 << POLY_VEC_LEN_SHIFT)

extern const uint32_t gf64_modulus;
extern const uint32_t gf128_modulus;
extern const uint32_t gf192_modulus;
extern const uint32_t gf256_modulus;

#include "polynomials_impl.h"



inline poly1_vec poly1_set_all(uint8_t x);

inline poly64_vec poly64_load(const void* s);
inline poly128_vec poly128_load(const void* s);
inline poly192_vec poly192_load(const void* s);
inline poly256_vec poly256_load(const void* s);
inline poly320_vec poly320_load(const void* s);
inline poly384_vec poly384_load(const void* s);
inline poly512_vec poly512_load(const void* s);

inline poly1_vec poly1_load(unsigned long x, unsigned int bit_offset);

inline poly1_vec poly1_load_offset8(const void* s, unsigned int bit_offset);

inline poly64_vec poly64_load_dup(const void* s);
inline poly128_vec poly128_load_dup(const void* s);
inline poly192_vec poly192_load_dup(const void* s);
inline poly256_vec poly256_load_dup(const void* s);

inline poly64_vec poly64_set_zero();
inline poly128_vec poly128_set_zero();
inline poly192_vec poly192_set_zero();
inline poly256_vec poly256_set_zero();

inline poly64_vec poly64_set_low32(uint32_t x);
inline poly128_vec poly128_set_low32(uint32_t x);
inline poly192_vec poly192_set_low32(uint32_t x);
inline poly256_vec poly256_set_low32(uint32_t x);
inline poly320_vec poly320_set_low32(uint32_t x);
inline poly384_vec poly384_set_low32(uint32_t x);
inline poly512_vec poly512_set_low32(uint32_t x);

inline void poly64_store(void* d, poly64_vec s);
inline void poly128_store(void* d, poly128_vec s);
inline void poly192_store(void* d, poly192_vec s);
inline void poly256_store(void* d, poly256_vec s);
inline void poly320_store(void* d, poly320_vec s);
inline void poly384_store(void* d, poly384_vec s);
inline void poly512_store(void* d, poly512_vec s);

inline void poly128_store1(void* d, poly128_vec s);
inline void poly192_store1(void* d, poly192_vec s);
inline void poly256_store1(void* d, poly256_vec s);

inline poly128_vec poly128_from_64(poly64_vec x);
inline poly192_vec poly192_from_128(poly128_vec x);
inline poly256_vec poly256_from_128(poly128_vec x);
inline poly256_vec poly256_from_192(poly192_vec x);
inline poly384_vec poly384_from_192(poly192_vec x);
inline poly320_vec poly320_from_256(poly256_vec x);
inline poly512_vec poly512_from_256(poly256_vec x);
inline poly128_vec poly128_from_1(poly1_vec x);
inline poly192_vec poly192_from_1(poly1_vec x);
inline poly256_vec poly256_from_1(poly1_vec x);

inline poly64_vec poly64_add(poly64_vec x, poly64_vec y);
inline poly128_vec poly128_add(poly128_vec x, poly128_vec y);
inline poly192_vec poly192_add(poly192_vec x, poly192_vec y);
inline poly256_vec poly256_add(poly256_vec x, poly256_vec y);
inline poly320_vec poly320_add(poly320_vec x, poly320_vec y);
inline poly384_vec poly384_add(poly384_vec x, poly384_vec y);
inline poly512_vec poly512_add(poly512_vec x, poly512_vec y);

inline poly128_vec poly64_mul(poly64_vec x, poly64_vec y);
inline poly256_vec poly128_mul(poly128_vec x, poly128_vec y);
inline poly384_vec poly192_mul(poly192_vec x, poly192_vec y);
inline poly512_vec poly256_mul(poly256_vec x, poly256_vec y);

inline poly192_vec poly64x128_mul(poly64_vec x, poly128_vec y);
inline poly256_vec poly64x192_mul(poly64_vec x, poly192_vec y);
inline poly320_vec poly64x256_mul(poly64_vec x, poly256_vec y);

inline poly128_vec poly1x128_mul(poly1_vec x, poly128_vec y);
inline poly192_vec poly1x192_mul(poly1_vec x, poly192_vec y);
inline poly256_vec poly1x256_mul(poly1_vec x, poly256_vec y);
inline poly384_vec poly1x384_mul(poly1_vec x, poly384_vec y);
inline poly512_vec poly1x512_mul(poly1_vec x, poly512_vec y);

inline poly256_vec poly256_shift_left_1(poly256_vec x);
inline poly384_vec poly384_shift_left_1(poly384_vec x);
inline poly512_vec poly512_shift_left_1(poly512_vec x);
inline poly256_vec poly256_shift_left_8(poly256_vec x);
inline poly384_vec poly384_shift_left_8(poly384_vec x);
inline poly512_vec poly512_shift_left_8(poly512_vec x);

inline poly64_vec poly128_reduce64(poly128_vec x);
inline poly128_vec poly192_reduce128(poly192_vec x);
inline poly128_vec poly256_reduce128(poly256_vec x);
inline poly192_vec poly256_reduce192(poly256_vec x);
inline poly192_vec poly384_reduce192(poly384_vec x);
inline poly256_vec poly320_reduce256(poly320_vec x);
inline poly256_vec poly512_reduce256(poly512_vec x);

inline poly64_vec poly64_mul_a64_reduce64(poly64_vec x);

inline poly128_vec poly128_from_byte(uint8_t byte);
inline poly192_vec poly192_from_byte(uint8_t byte);
inline poly256_vec poly256_from_byte(uint8_t byte);

inline poly128_vec poly128_from_8_poly1(const poly1_vec* bits);
inline poly192_vec poly192_from_8_poly1(const poly1_vec* bits);
inline poly256_vec poly256_from_8_poly1(const poly1_vec* bits);

inline poly128_vec poly128_from_128_poly1(const poly1_vec* polys);
inline poly192_vec poly192_from_192_poly1(const poly1_vec* polys);
inline poly256_vec poly256_from_256_poly1(const poly1_vec* polys);

inline poly128_vec poly128_from_16_poly1(const poly1_vec* bits);

inline poly128_vec poly128_from_8_poly128(const poly128_vec* polys);
inline poly192_vec poly192_from_8_poly192(const poly192_vec* polys);
inline poly256_vec poly256_from_8_poly256(const poly256_vec* polys);

inline poly128_vec poly128_from_8_poly64(const poly64_vec* polys);
inline poly192_vec poly192_from_8_poly64(const poly64_vec* polys);
inline poly256_vec poly256_from_8_poly64(const poly64_vec* polys);

inline poly128_vec poly128_from_128_poly128(const poly128_vec* polys);
inline poly192_vec poly192_from_192_poly192(const poly192_vec* polys);
inline poly256_vec poly256_from_256_poly256(const poly256_vec* polys);

inline poly128_vec poly128_from_16_poly128(const poly128_vec* polys);

poly64_vec poly64_exp(poly64_vec base, size_t power);
poly128_vec poly128_exp(poly128_vec base, size_t power);
poly192_vec poly192_exp(poly192_vec base, size_t power);
poly256_vec poly256_exp(poly256_vec base, size_t power);

inline bool poly64_eq(poly64_vec x, poly64_vec y);
inline bool poly128_eq(poly128_vec x, poly128_vec y);
inline bool poly192_eq(poly192_vec x, poly192_vec y);
inline bool poly256_eq(poly256_vec x, poly256_vec y);
inline bool poly320_eq(poly320_vec x, poly320_vec y);
inline bool poly384_eq(poly384_vec x, poly384_vec y);
inline bool poly512_eq(poly512_vec x, poly512_vec y);

inline poly64_vec poly64_extract(poly64_vec x, size_t index);
inline poly128_vec poly128_extract(poly128_vec x, size_t index);
inline poly192_vec poly192_extract(poly192_vec x, size_t index);
inline poly256_vec poly256_extract(poly256_vec x, size_t index);
inline poly384_vec poly384_extract(poly384_vec x, size_t index);
inline poly512_vec poly512_extract(poly512_vec x, size_t index);

#if SECURITY_PARAM == 512
typedef poly512_vec poly_secpar_vec;
typedef poly576_vec poly_secpar_plus_64_vec;
typedef poly1024_vec poly_2secpar_vec;

inline poly_secpar_vec poly_secpar_load(const void* s) { return poly512_load(s); }
inline poly_secpar_vec poly_secpar_load_dup(const void* s) { return poly512_load(s); }
inline poly_secpar_vec poly_secpar_set_zero() { poly_secpar_vec z={{{0}}}; return z; }
inline poly_secpar_vec poly_secpar_set_low32(uint32_t x) { poly_secpar_vec z={{{0}}}; memcpy(&z,&x,4); return z; }
inline void poly_secpar_store(void* d, poly_secpar_vec s) { poly512_store(d,s); }
inline void poly_secpar_store1(void* d, poly_secpar_vec s) { poly512_store(d,s); }
inline poly_secpar_vec poly_secpar_add(poly_secpar_vec x, poly_secpar_vec y) { return poly512_add(x,y); }
inline poly_secpar_plus_64_vec poly_secpar_plus_64_add(poly_secpar_plus_64_vec x, poly_secpar_plus_64_vec y) { return poly576_add(x,y); }
inline poly_2secpar_vec poly_2secpar_add(poly_2secpar_vec x, poly_2secpar_vec y) { return poly1024_add(x,y); }
inline poly_2secpar_vec poly_secpar_mul(poly_secpar_vec x, poly_secpar_vec y) { return poly512_mul(x,y); }
inline poly_secpar_vec poly1xsecpar_mul(poly1_vec x, poly_secpar_vec y) { return poly1x512_security_mul(x,y); }
inline poly_2secpar_vec poly1x2secpar_mul(poly1_vec x, poly_2secpar_vec y) { return poly1x1024_mul(x,y); }
inline poly_secpar_plus_64_vec poly64xsecpar_mul(poly64_vec x, poly_secpar_vec y) { return poly64x512_mul(x,y); }
inline poly_2secpar_vec poly_2secpar_shift_left_1(poly_2secpar_vec x) { return poly1024_shift_left_1(x); }
inline poly_2secpar_vec poly_2secpar_shift_left_8(poly_2secpar_vec x) { return poly1024_shift_left_8(x); }
inline poly_secpar_vec poly_2secpar_reduce_secpar(poly_2secpar_vec x) { return poly1024_reduce512(x); }
inline poly_secpar_vec poly_secpar_plus_64_reduce_secpar(poly_secpar_plus_64_vec x) { return poly576_reduce512(x); }
inline poly_secpar_plus_64_vec poly_secpar_plus_64_from_secpar(poly_secpar_vec x) { return poly576_from_512(x); }
inline poly_2secpar_vec poly_2secpar_from_secpar(poly_secpar_vec x) { return poly1024_from_512(x); }
inline poly_secpar_vec poly_secpar_from_64(poly64_vec x) { poly_secpar_vec z={{{0}}}; memcpy(&z,&x,8); return z; }
inline bool poly_secpar_eq(poly_secpar_vec x, poly_secpar_vec y) { return poly512_eq(x,y); }
inline poly_secpar_vec poly_secpar_extract(poly_secpar_vec x, size_t index) { return poly512_extract(x,index); }
inline poly_2secpar_vec poly_2secpar_extract(poly_2secpar_vec x, size_t index) { return poly1024_extract(x,index); }
inline poly_secpar_vec poly_secpar_from_1(poly1_vec p) { return poly1x512_security_mul(p, poly_secpar_set_low32(1)); }

inline poly_secpar_vec poly_secpar_from_secpar_poly1(const poly1_vec* bits) { poly_secpar_vec z=poly_secpar_set_zero(); uint8_t* d=(uint8_t*)&z; for(size_t i=0;i<512;i++) d[i/8]|=(bits[i]&1)<<(i%8); return z; }
inline poly_secpar_vec poly_secpar_from_8_poly1(const poly1_vec* bits) { poly1_vec all[512]={0}; memcpy(all,bits,8*sizeof(*bits)); return poly_secpar_from_secpar_poly1(all); }
inline poly_secpar_vec poly_secpar_from_16_poly1(const poly1_vec* bits) { poly1_vec all[512]={0}; memcpy(all,bits,16*sizeof(*bits)); return poly_secpar_from_secpar_poly1(all); }
inline poly_secpar_vec poly_secpar_from_byte(uint8_t b) { poly_secpar_vec z=poly_secpar_set_zero(); memcpy(&z,&b,1); return z; }
inline poly_secpar_vec poly_secpar_from_secpar_poly_secpar(
    const poly_secpar_vec* p)
{
    uint64_t acc[16] = {0};

    for (size_t i = 0; i < 512; ++i) {
        uint64_t words[8];
        poly512_words(words, p[i]);

        size_t word_shift = i >> 6;
        unsigned bit_shift = i & 63;

        for (size_t j = 0; j < 8; ++j) {
            acc[word_shift + j] ^= words[j] << bit_shift;

            if (bit_shift && word_shift + j + 1 < 16) {
                acc[word_shift + j + 1] ^= words[j] >> (64 - bit_shift);
            }
        }
    }

    return poly1024_reduce512(poly1024_from_words(acc));
}
inline poly_secpar_vec poly_secpar_from_8_poly_secpar(const poly_secpar_vec* p) { poly_secpar_vec all[512]; memset(all,0,sizeof(all)); memcpy(all,p,8*sizeof(*p)); return poly_secpar_from_secpar_poly_secpar(all); }
inline poly_secpar_vec poly_secpar_from_16_poly_secpar(const poly_secpar_vec* p) { poly_secpar_vec all[512]; memset(all,0,sizeof(all)); memcpy(all,p,16*sizeof(*p)); return poly_secpar_from_secpar_poly_secpar(all); }
inline poly_secpar_vec poly_secpar_from_8_poly64(const poly64_vec* p) { poly_secpar_vec z=poly_secpar_from_64(p[0]); poly_secpar_vec x=poly_secpar_set_low32(2),power=x; for(size_t i=1;i<8;i++){z=poly_secpar_add(z,poly_2secpar_reduce_secpar(poly_secpar_mul(poly_secpar_from_64(p[i]),power)));for(int j=0;j<64;j++)power=poly_2secpar_reduce_secpar(poly_secpar_mul(power,x));}return z; }
inline poly_secpar_vec poly_secpar_exp(poly_secpar_vec base, size_t power) { poly_secpar_vec out=poly_secpar_set_low32(1);while(power){if(power&1)out=poly_2secpar_reduce_secpar(poly_secpar_mul(out,base));base=poly_2secpar_reduce_secpar(poly_secpar_mul(base,base));power>>=1;}return out; }
#else

#if SECURITY_PARAM == 128
typedef poly128_vec poly_secpar_vec;
typedef poly192_vec poly_secpar_plus_64_vec;
typedef poly256_vec poly_2secpar_vec;

inline poly_secpar_vec poly_secpar_load(const void* s)
{
	return poly128_load(s);
}
inline poly_secpar_vec poly_secpar_load_dup(const void* s)
{
	return poly128_load_dup(s);
}
inline poly_secpar_vec poly_secpar_set_zero()
{
	return poly128_set_zero();
}
inline poly_secpar_vec poly_secpar_set_low32(uint32_t x)
{
	return poly128_set_low32(x);
}
inline void poly_secpar_store(void* d, poly_secpar_vec s)
{
	poly128_store(d, s);
}
inline void poly_secpar_store1(void* d, poly_secpar_vec s)
{
	poly128_store1(d, s);
}
inline poly_secpar_vec poly_secpar_add(poly_secpar_vec x, poly_secpar_vec y)
{
	return poly128_add(x, y);
}
inline poly_secpar_plus_64_vec poly_secpar_plus_64_add(poly_secpar_plus_64_vec x, poly_secpar_plus_64_vec y)
{
	return poly192_add(x, y);
}
inline poly_2secpar_vec poly_2secpar_add(poly_2secpar_vec x, poly_2secpar_vec y)
{
	return poly256_add(x, y);
}
inline poly_2secpar_vec poly_secpar_mul(poly_secpar_vec x, poly_secpar_vec y)
{
	return poly128_mul(x, y);
}
inline poly_secpar_vec poly1xsecpar_mul(poly1_vec x, poly_secpar_vec y)
{
	return poly1x128_mul(x, y);
}
inline poly_2secpar_vec poly1x2secpar_mul(poly1_vec x, poly_2secpar_vec y)
{
	return poly1x256_mul(x, y);
}
inline poly_secpar_plus_64_vec poly64xsecpar_mul(poly64_vec x, poly_secpar_vec y)
{
	return poly64x128_mul(x, y);
}
inline poly_2secpar_vec poly_2secpar_shift_left_1(poly_2secpar_vec x)
{
	return poly256_shift_left_1(x);
}
inline poly_2secpar_vec poly_2secpar_shift_left_8(poly_2secpar_vec x)
{
	return poly256_shift_left_8(x);
}
inline poly_secpar_vec poly_2secpar_reduce_secpar(poly_2secpar_vec x)
{
	return poly256_reduce128(x);
}
inline poly_secpar_vec poly_secpar_plus_64_reduce_secpar(poly_secpar_plus_64_vec x)
{
	return poly192_reduce128(x);
}
inline poly_secpar_plus_64_vec poly_secpar_plus_64_from_secpar(poly_secpar_vec x)
{
    return poly192_from_128(x);
}
inline poly_2secpar_vec poly_2secpar_from_secpar(poly_secpar_vec x)
{
    return poly256_from_128(x);
}
inline poly_secpar_vec poly_secpar_from_64(poly64_vec x)
{
    return poly128_from_64(x);
}
inline bool poly_secpar_eq(poly_secpar_vec x, poly_secpar_vec y)
{
	return poly128_eq(x, y);
}
inline poly_secpar_vec poly_secpar_extract(poly_secpar_vec x, size_t index)
{
    return poly128_extract(x, index);
}
inline poly_2secpar_vec poly_2secpar_extract(poly_2secpar_vec x, size_t index)
{
    return poly256_extract(x, index);
}
inline poly_secpar_vec poly_secpar_from_1(poly1_vec p)
{
    return poly128_from_1(p);
}
inline poly_secpar_vec poly_secpar_from_byte(uint8_t byte)
{
    return poly128_from_byte(byte);
}
inline poly_secpar_vec poly_secpar_from_8_poly1(const poly1_vec* bits)
{
	return poly128_from_8_poly1(bits);
}
inline poly_secpar_vec poly_secpar_from_secpar_poly1(const poly1_vec* bits)
{
	return poly128_from_128_poly1(bits);
}
inline poly_secpar_vec poly_secpar_from_16_poly1(const poly1_vec* bits)
{
	return poly128_from_16_poly1(bits);
}
inline poly_secpar_vec poly_secpar_from_8_poly_secpar(const poly_secpar_vec* polys)
{
	return poly128_from_8_poly128(polys);
}
inline poly_secpar_vec poly_secpar_from_8_poly64(const poly64_vec* polys)
{
	return poly128_from_8_poly64(polys);
}
inline poly_secpar_vec poly_secpar_from_secpar_poly_secpar(const poly_secpar_vec* polys) 
{
	return poly128_from_128_poly128(polys);
}
inline poly_secpar_vec poly_secpar_from_16_poly_secpar(const poly_secpar_vec* polys)
{
	return poly128_from_16_poly128(polys);
}
inline poly_secpar_vec poly_secpar_exp(poly_secpar_vec base, size_t power)
{
	return poly128_exp(base, power);
}

#elif SECURITY_PARAM == 192
typedef poly192_vec poly_secpar_vec;
typedef poly256_vec poly_secpar_plus_64_vec;
typedef poly384_vec poly_2secpar_vec;

inline poly_secpar_vec poly_secpar_load(const void* s)
{
	return poly192_load(s);
}
inline poly_secpar_vec poly_secpar_load_dup(const void* s)
{
	return poly192_load_dup(s);
}
inline poly_secpar_vec poly_secpar_set_zero()
{
	return poly192_set_zero();
}
inline poly_secpar_vec poly_secpar_set_low32(uint32_t x)
{
	return poly192_set_low32(x);
}
inline void poly_secpar_store(void* d, poly_secpar_vec s)
{
	poly192_store(d, s);
}
inline void poly_secpar_store1(void* d, poly_secpar_vec s)
{
	poly192_store1(d, s);
}
inline poly_secpar_vec poly_secpar_add(poly_secpar_vec x, poly_secpar_vec y)
{
	return poly192_add(x, y);
}
inline poly_secpar_plus_64_vec poly_secpar_plus_64_add(poly_secpar_plus_64_vec x, poly_secpar_plus_64_vec y)
{
	return poly256_add(x, y);
}
inline poly_2secpar_vec poly_2secpar_add(poly_2secpar_vec x, poly_2secpar_vec y)
{
	return poly384_add(x, y);
}
inline poly_2secpar_vec poly_secpar_mul(poly_secpar_vec x, poly_secpar_vec y)
{
	return poly192_mul(x, y);
}
inline poly_secpar_vec poly1xsecpar_mul(poly1_vec x, poly_secpar_vec y)
{
	return poly1x192_mul(x, y);
}
inline poly_2secpar_vec poly1x2secpar_mul(poly1_vec x, poly_2secpar_vec y)
{
	return poly1x384_mul(x, y);
}
inline poly_secpar_plus_64_vec poly64xsecpar_mul(poly64_vec x, poly_secpar_vec y)
{
	return poly64x192_mul(x, y);
}
inline poly_2secpar_vec poly_2secpar_shift_left_1(poly_2secpar_vec x)
{
	return poly384_shift_left_1(x);
}
inline poly_2secpar_vec poly_2secpar_shift_left_8(poly_2secpar_vec x)
{
	return poly384_shift_left_8(x);
}
inline poly_secpar_vec poly_2secpar_reduce_secpar(poly_2secpar_vec x)
{
	return poly384_reduce192(x);
}
inline poly_secpar_vec poly_secpar_plus_64_reduce_secpar(poly_secpar_plus_64_vec x)
{
	return poly256_reduce192(x);
}
inline poly_secpar_plus_64_vec poly_secpar_plus_64_from_secpar(poly_secpar_vec x)
{
    return poly256_from_192(x);
}
inline poly_2secpar_vec poly_2secpar_from_secpar(poly_secpar_vec x)
{
    return poly384_from_192(x);
}
inline poly_secpar_vec poly_secpar_from_64(poly64_vec x)
{
    return poly192_from_128(poly128_from_64(x));
}
inline bool poly_secpar_eq(poly_secpar_vec x, poly_secpar_vec y)
{
	return poly192_eq(x, y);
}
inline poly_secpar_vec poly_secpar_extract(poly_secpar_vec x, size_t index)
{
    return poly192_extract(x, index);
}
inline poly_2secpar_vec poly_2secpar_extract(poly_2secpar_vec x, size_t index)
{
    return poly384_extract(x, index);
}
inline poly_secpar_vec poly_secpar_from_1(poly1_vec p)
{
    return poly192_from_1(p);
}
inline poly_secpar_vec poly_secpar_from_byte(uint8_t byte)
{
    return poly192_from_byte(byte);
}
inline poly_secpar_vec poly_secpar_from_8_poly1(const poly1_vec* bits)
{
	return poly192_from_8_poly1(bits);
}
inline poly_secpar_vec poly_secpar_from_secpar_poly1(const poly1_vec* bits)
{
	return poly192_from_192_poly1(bits);
}
inline poly_secpar_vec poly_secpar_from_16_poly1(const poly1_vec* bits)
{
	return poly192_from_16_poly1(bits);
}
inline poly_secpar_vec poly_secpar_from_8_poly_secpar(const poly_secpar_vec* polys)
{
	return poly192_from_8_poly192(polys);
}
inline poly_secpar_vec poly_secpar_from_8_poly64(const poly64_vec* polys)
{
	return poly192_from_8_poly64(polys);
}
inline poly_secpar_vec poly_secpar_from_secpar_poly_secpar(const poly_secpar_vec* polys) {
	return poly192_from_192_poly192(polys);
}
inline poly_secpar_vec poly_secpar_from_16_poly_secpar(const poly_secpar_vec* polys)
{
	return poly192_from_16_poly192(polys);
}
inline poly_secpar_vec poly_secpar_exp(poly_secpar_vec base, size_t power)
{
	return poly192_exp(base, power);
}

#elif SECURITY_PARAM == 256
typedef poly256_vec poly_secpar_vec;
typedef poly320_vec poly_secpar_plus_64_vec;
typedef poly512_vec poly_2secpar_vec;

inline poly_secpar_vec poly_secpar_load(const void* s)
{
	return poly256_load(s);
}
inline poly_secpar_vec poly_secpar_load_dup(const void* s)
{
	return poly256_load_dup(s);
}
inline poly_secpar_vec poly_secpar_set_zero()
{
	return poly256_set_zero();
}
inline poly_secpar_vec poly_secpar_set_low32(uint32_t x)
{
	return poly256_set_low32(x);
}
inline void poly_secpar_store(void* d, poly_secpar_vec s)
{
	poly256_store(d, s);
}
inline void poly_secpar_store1(void* d, poly_secpar_vec s)
{
	poly256_store1(d, s);
}
inline poly_secpar_vec poly_secpar_add(poly_secpar_vec x, poly_secpar_vec y)
{
	return poly256_add(x, y);
}
inline poly_secpar_plus_64_vec poly_secpar_plus_64_add(poly_secpar_plus_64_vec x, poly_secpar_plus_64_vec y)
{
	return poly320_add(x, y);
}
inline poly_2secpar_vec poly_2secpar_add(poly_2secpar_vec x, poly_2secpar_vec y)
{
	return poly512_add(x, y);
}
inline poly_2secpar_vec poly_secpar_mul(poly_secpar_vec x, poly_secpar_vec y)
{
	return poly256_mul(x, y);
}
inline poly_secpar_vec poly1xsecpar_mul(poly1_vec x, poly_secpar_vec y)
{
	return poly1x256_mul(x, y);
}
inline poly_2secpar_vec poly1x2secpar_mul(poly1_vec x, poly_2secpar_vec y)
{
	return poly1x512_mul(x, y);
}
inline poly_secpar_plus_64_vec poly64xsecpar_mul(poly64_vec x, poly_secpar_vec y)
{
	return poly64x256_mul(x, y);
}
inline poly_2secpar_vec poly_2secpar_shift_left_1(poly_2secpar_vec x)
{
	return poly512_shift_left_1(x);
}
inline poly_2secpar_vec poly_2secpar_shift_left_8(poly_2secpar_vec x)
{
	return poly512_shift_left_8(x);
}
inline poly_secpar_vec poly_2secpar_reduce_secpar(poly_2secpar_vec x)
{
	return poly512_reduce256(x);
}
inline poly_secpar_vec poly_secpar_plus_64_reduce_secpar(poly_secpar_plus_64_vec x)
{
	return poly320_reduce256(x);
}
inline poly_secpar_plus_64_vec poly_secpar_plus_64_from_secpar(poly_secpar_vec x)
{
    return poly320_from_256(x);
}
inline poly_2secpar_vec poly_2secpar_from_secpar(poly_secpar_vec x)
{
    return poly512_from_256(x);
}
inline poly_secpar_vec poly_secpar_from_64(poly64_vec x)
{
    return poly256_from_128(poly128_from_64(x));
}
inline bool poly_secpar_eq(poly_secpar_vec x, poly_secpar_vec y)
{
	return poly256_eq(x, y);
}
inline poly_secpar_vec poly_secpar_extract(poly_secpar_vec x, size_t index)
{
    return poly256_extract(x, index);
}
inline poly_2secpar_vec poly_2secpar_extract(poly_2secpar_vec x, size_t index)
{
    return poly512_extract(x, index);
}
inline poly_secpar_vec poly_secpar_from_1(poly1_vec p)
{
    return poly256_from_1(p);
}
inline poly_secpar_vec poly_secpar_from_byte(uint8_t byte)
{
    return poly256_from_byte(byte);
}
inline poly_secpar_vec poly_secpar_from_8_poly1(const poly1_vec* bits)
{
	return poly256_from_8_poly1(bits);
}
inline poly_secpar_vec poly_secpar_from_secpar_poly1(const poly1_vec* bits) 
{
	return poly256_from_256_poly1(bits);
}
inline poly_secpar_vec poly_secpar_from_16_poly1(const poly1_vec* bits)
{
	return poly256_from_16_poly1(bits);
}
inline poly_secpar_vec poly_secpar_from_8_poly_secpar(const poly_secpar_vec* polys)
{
	return poly256_from_8_poly256(polys);
}
inline poly_secpar_vec poly_secpar_from_8_poly64(const poly64_vec* polys)
{
	return poly256_from_8_poly64(polys);
}
inline poly_secpar_vec poly_secpar_from_secpar_poly_secpar(const poly_secpar_vec* polys) 
{
	return poly256_from_256_poly256(polys);
}
inline poly_secpar_vec poly_secpar_from_16_poly_secpar(const poly_secpar_vec* polys)
{
	return poly256_from_16_poly256(polys);
}
inline poly_secpar_vec poly_secpar_exp(poly_secpar_vec base, size_t power)
{
	return poly256_exp(base, power);
}

#endif

#endif

inline poly_2secpar_vec poly_2secpar_set_zero()
{
	return poly_2secpar_from_secpar(poly_secpar_set_zero());
}

#endif
