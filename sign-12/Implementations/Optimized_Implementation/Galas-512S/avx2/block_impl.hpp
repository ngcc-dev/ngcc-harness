#ifndef BLOCK_IMPL_AVX2_HPP
#define BLOCK_IMPL_AVX2_HPP

#include <cstdint>
#include <cstring>
#include <immintrin.h>
#include <wmmintrin.h>

namespace faest
{

// NB: We assume that bits is divisible by 8 and each block is exactly of size bits / 8 byte.

template <std::size_t bits> struct block;

using block128 = block<128>;
using block192 = block<192>;
using block256 = block<256>;
using block384 = block<384>;
using block512 = block<512>;

template <> struct block<128>
{
    __m128i data;

    inline block128 operator^(const block128& y) const
    {
        return {_mm_xor_si128(this->data, y.data)};
    }
    inline block128 operator&(const block128& y) const
    {
        return {_mm_and_si128(this->data, y.data)};
    }
    inline block128 add32(const block128& y) const
    {
        return {_mm_add_epi32(this->data, y.data)};
    }
    inline static block128 set_zero() { return {_mm_setzero_si128()}; }
    inline static block128 set_all_8(uint8_t x) { return {_mm_set1_epi8(x)}; }
    inline static block128 set_low32(uint32_t x) { return {_mm_setr_epi32(x, 0, 0, 0)}; }
    inline static block128 set_low32(uint32_t x, uint32_t y) { return {_mm_setr_epi32(x, y, 0, 0)}; }
    inline static block128 set_low_high32(uint32_t x, uint32_t y) { return {_mm_setr_epi32(x, 0, 0, y)}; }
    inline static block128 set_low64(uint64_t x) { return {_mm_set_epi64x(0, x)}; }
    inline bool any_zeros() const
    {
        return _mm_movemask_epi8(_mm_cmpeq_epi8(this->data, _mm_setzero_si128()));
    }
    inline block128 byte_reverse() const
    {
        __m128i shuffle = _mm_setr_epi8(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);
        return {_mm_shuffle_epi8(this->data, shuffle)};
    }

    inline static block128 clmul_ll(block128 x, block128 y)
    {
        return {_mm_clmulepi64_si128(x.data, y.data, 0x00)};
    }
    inline static block128 clmul_lh(block128 x, block128 y)
    {
        return {_mm_clmulepi64_si128(x.data, y.data, 0x10)};
    }
    inline static block128 clmul_hl(block128 x, block128 y)
    {
        return {_mm_clmulepi64_si128(x.data, y.data, 0x01)};
    }
    inline static block128 clmul_hh(block128 x, block128 y)
    {
        return {_mm_clmulepi64_si128(x.data, y.data, 0x11)};
    }

    inline block128 shift_left_64() const { return {_mm_slli_si128(this->data, 8)}; }
    inline block128 shift_right_64() const { return {_mm_srli_si128(this->data, 8)}; }
    inline static block128 mix_64(block128 x, block128 y) // output = y high, x low.
    {
        return {_mm_alignr_epi8(x.data, y.data, 8)};
    }
    inline block128 broadcast_low64() const { return {_mm_broadcastq_epi64(this->data)}; }
};

template <> struct block<192>
{
    uint64_t data[3];

    inline block192 operator^(const block192& y) const
    {
        // Plain c version for now at least. Hopefully it will be autovectorized.
        block192 out;
        out.data[0] = this->data[0] ^ y.data[0];
        out.data[1] = this->data[1] ^ y.data[1];
        out.data[2] = this->data[2] ^ y.data[2];
        return out;
    }

    inline block192 operator&(const block192& y) const
    {
        block192 out;
        out.data[0] = this->data[0] & y.data[0];
        out.data[1] = this->data[1] & y.data[1];
        out.data[2] = this->data[2] & y.data[2];
        return out;
    }

    inline block192 add32(const block192& y) const
    {
        uint32_t x_data[6];
        uint32_t y_data[6];
        memcpy(x_data, data, sizeof(data));
        memcpy(y_data, y.data, sizeof(y.data));
        for (int i = 0; i < 6; ++i) x_data[i] += y_data[i];

        block192 out;
        memcpy(out.data, x_data, sizeof(out.data));
        return out;
    }

    inline static block192 set_all_8(uint8_t x)
    {
        uint64_t x64 = x;
        x64 *= UINT64_MAX / 0xff;

        block192 out = {{x64, x64, x64}};
        return out;
    }

    inline static block192 set_low64(uint64_t x)
    {
        block192 out = {{x, 0, 0}};
        return out;
    }

    inline static block192 set_low32(uint32_t x) { return set_low64(x); }

    inline static block192 set_low32(uint32_t x, uint32_t y)
    {
        return set_low64(((uint64_t) x << 32) + y);
    }

    inline static block192 set_low_high32(uint32_t x, uint32_t y)
    {
        block192 out = {{x, 0, (uint64_t) y << 32}};
        return out;
    }

    inline static block192 set_zero() { return set_low64(0); }

    inline bool any_zeros() const
    {
        __m256i b = _mm256_setzero_si256();
        memcpy(&b, &this->data, sizeof(this->data));
        return _mm256_movemask_epi8(_mm256_cmpeq_epi8(b, _mm256_setzero_si256())) & 0x00ffffff;
    }
};

template <> struct block<256>
{
    __m256i data;

    inline block256 operator^(const block256& y) const
    {
        return {_mm256_xor_si256(this->data, y.data)};
    }
    inline block256 operator&(const block256& y) const
    {
        return {_mm256_and_si256(this->data, y.data)};
    }
    inline block256 add32(const block256& y) const
    {
        return {_mm256_add_epi32(this->data, y.data)};
    }
    inline static block256 set_zero() { return {_mm256_setzero_si256()}; }
    inline static block256 set_all_8(uint8_t x) { return {_mm256_set1_epi8(x)}; }
    inline static block256 set_low32(uint32_t x)
    {
        return {_mm256_setr_epi32(x, 0, 0, 0, 0, 0, 0, 0)};
    }
    inline static block256 set_low32(uint32_t x, uint32_t y)
    {
        return {_mm256_setr_epi32(x, y, 0, 0, 0, 0, 0, 0)};
    }
    inline static block256 set_low_high32(uint32_t x, uint32_t y)
    {
        return {_mm256_setr_epi32(x, 0, 0, 0, 0, 0, 0, y)};
    }
    inline static block256 set_low64(uint64_t x) { return {_mm256_setr_epi64x(x, 0, 0, 0)}; }
    inline static block256 set_128(block128 x0, block128 x1)
    {
        return {_mm256_setr_m128i(x0.data, x1.data)};
    }
    inline static block256 block256_set_low128(block128 x)
    {
        return {_mm256_inserti128_si256(_mm256_setzero_si256(), x.data, 0)};
    }
    inline bool any_zeros() const
    {
        return _mm256_movemask_epi8(_mm256_cmpeq_epi8(this->data, _mm256_setzero_si256()));
    }
    inline static block256 from_2_block128(block128 x, block128 y)
    {
        return {_mm256_setr_m128i(x.data, y.data)};
    }

    inline static block256 clmul_ll(block256 x, block256 y)
    {
        return {_mm256_clmulepi64_epi128(x.data, y.data, 0x00)};
    }
    inline static block256 clmul_lh(block256 x, block256 y)
    {
        return {_mm256_clmulepi64_epi128(x.data, y.data, 0x10)};
    }
    inline static block256 clmul_hl(block256 x, block256 y)
    {
        return {_mm256_clmulepi64_epi128(x.data, y.data, 0x01)};
    }
    inline static block256 clmul_hh(block256 x, block256 y)
    {
        return {_mm256_clmulepi64_epi128(x.data, y.data, 0x11)};
    }

    inline block256 shift_left_64() const { return {_mm256_slli_si256(this->data, 8)}; }
    inline block256 shift_right_64() const { return {_mm256_srli_si256(this->data, 8)}; }
    inline static block256 mix_64(block256 x, block256 y) // output = y high, x low.
    {
        return {_mm256_alignr_epi8(x.data, y.data, 8)};
    }
    inline block256 broadcast_low64() const { return {_mm256_shuffle_epi32(this->data, 0x44)}; }
};

template <> struct block<384>
{
    block128 data[3];

    inline block384 operator^(const block384& y) const
    {
        block384 out;
        out.data[0] = this->data[0] ^ y.data[0];
        out.data[1] = this->data[1] ^ y.data[1];
        out.data[2] = this->data[2] ^ y.data[2];
        return out;
    }
    inline block384 operator&(const block384& y) const
    {
        block384 out;
        out.data[0] = this->data[0] & y.data[0];
        out.data[1] = this->data[1] & y.data[1];
        out.data[2] = this->data[2] & y.data[2];
        return out;
    }
    inline block384 add32(const block384& y) const
    {
        block384 out;
        out.data[0] = this->data[0].add32(y.data[0]);
        out.data[1] = this->data[1].add32(y.data[1]);
        out.data[2] = this->data[2].add32(y.data[2]);
        return out;
    }
    inline static block384 set_zero()
    {
        block384 out;
        out.data[0] = block128::set_zero();
        out.data[1] = block128::set_zero();
        out.data[2] = block128::set_zero();
        return out;
    }
    inline static block384 set_all_8(uint8_t x)
    {
        block384 out;
        out.data[0] = block128::set_all_8(x);
        out.data[1] = block128::set_all_8(x);
        out.data[2] = block128::set_all_8(x);
        return out;
    }
    inline bool any_zeros() const
    {
        unsigned char tmp[48];
        memcpy(tmp, this, 48);
        for (int i = 0; i < 48; i++) if (tmp[i] == 0) return true;
        return false;
    }
    inline static block384 set_low32(uint32_t x)
    {
        block384 out = block384::set_zero();
        out.data[0] = block128::set_low32(x);
        return out;
    }
    inline static block384 set_low32(uint32_t x, uint32_t y)
    {
        block384 out = block384::set_zero();
        out.data[0] = block128::set_low32(x, y);
        return out;
    }
    inline static block384 set_low_high32(uint32_t x, uint32_t y)
    {
        block384 out = block384::set_zero();
        out.data[0] = block128::set_low32(x);
        out.data[2] = block128::set_low_high32(0, y);
        return out;
    }
    inline static block384 set_low64(uint64_t x)
    {
        block384 out = block384::set_zero();
        out.data[0] = block128::set_low64(x);
        return out;
    }
};

template <> struct block<512>
{
    block256 data[2];

    inline block512 operator^(const block512& y) const
    {
        block512 out;
        out.data[0] = this->data[0] ^ y.data[0];
        out.data[1] = this->data[1] ^ y.data[1];
        return out;
    }

    inline block512 operator&(const block512& y) const
    {
        block512 out;
        out.data[0] = this->data[0] & y.data[0];
        out.data[1] = this->data[1] & y.data[1];
        return out;
    }
    inline block512 add32(const block512& y) const
    {
        block512 out;
        out.data[0] = this->data[0].add32(y.data[0]);
        out.data[1] = this->data[1].add32(y.data[1]);
        return out;
    }

    inline static block512 set_zero()
    {
        block512 out;
        out.data[0] = block256::set_zero();
        out.data[1] = block256::set_zero();
        return out;
    }

    inline static block512 block512_set_all_8(uint8_t x)
    {
        block512 out;
        out.data[0] = block256::set_all_8(x);
        out.data[1] = block256::set_all_8(x);
        return out;
    }
    inline static block512 set_all_8(uint8_t x) { return block512_set_all_8(x); }
    inline bool any_zeros() const
    {
        unsigned char tmp[64];
        memcpy(tmp, this, 64);
        for (int i = 0; i < 64; i++) if (tmp[i] == 0) return true;
        return false;
    }

    inline static block512 block512_set_low32(uint32_t x)
    {
        block512 out;
        out.data[0] = block256::set_low32(x);
        out.data[1] = block256::set_zero();
        return out;
    }
    inline static block512 set_low32(uint32_t x) { return block512_set_low32(x); }
    inline static block512 block512_set_low32(uint32_t x, uint32_t y)
    {
        block512 out;
        out.data[0] = block256::set_low32(x, y);
        out.data[1] = block256::set_zero();
        return out;
    }
    inline static block512 set_low32(uint32_t x, uint32_t y) { return block512_set_low32(x, y); }
    inline static block512 set_low_high32(uint32_t x, uint32_t y)
    {
        block512 out = block512::set_zero();
        out.data[0] = block256::set_low32(x);
        out.data[1] = block256::set_low_high32(0, y);
        return out;
    }
    inline static block512 block512_set_low64(uint64_t x)
    {
        block512 out;
        out.data[0] = block256::set_low64(x);
        out.data[1] = block256::set_zero();
        return out;
    }
    inline static block512 set_low64(uint64_t x) { return block512_set_low64(x); }
};

template <std::size_t bits> bool operator==(const block<bits>& x, const block<bits>& y)
{
    return memcmp(&x, &y, sizeof(x)) == 0;
}

// Unfortunately, there's no alternative version of these that works on integers.
#define shuffle_2x4xepi32(x, y, i)                                                                 \
    _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(x), _mm_castsi128_ps(y), i))
#define permute_8xepi32(x, i) _mm256_castps_si256(_mm256_permute_ps(_mm256_castsi256_ps(x), i))
#define shuffle_2x4xepi64(x, y, i)                                                                 \
    _mm256_castpd_si256(_mm256_shuffle_pd(_mm256_castsi256_pd(x), _mm256_castsi256_pd(y), i))

// Generic multi-byte fallback blocks for sizes not covered by SIMD (160, 320, 768, 1024)
template <> struct block<160>
{
    uint8_t data[20];
    inline block<160> operator^(const block<160>& y) const { block<160> o; for(int i=0;i<20;i++) o.data[i]=data[i]^y.data[i]; return o; }
    inline block<160> operator&(const block<160>& y) const { block<160> o; for(int i=0;i<20;i++) o.data[i]=data[i]&y.data[i]; return o; }
    inline static block<160> set_zero() { block<160> o; memset(o.data,0,20); return o; }
    inline static block<160> set_all_8(uint8_t x) { block<160> o; memset(o.data,x,20); return o; }
    inline bool is_zero() const { for(int i=0;i<20;i++) if(data[i]) return false; return true; }
    inline bool any_zeros() const { for(int i=0;i<20;i++) if(data[i] == 0) return true; return false; }
};
template <> struct block<320>
{
    uint8_t data[40];
    inline block<320> operator^(const block<320>& y) const { block<320> o; for(int i=0;i<40;i++) o.data[i]=data[i]^y.data[i]; return o; }
    inline block<320> operator&(const block<320>& y) const { block<320> o; for(int i=0;i<40;i++) o.data[i]=data[i]&y.data[i]; return o; }
    inline static block<320> set_zero() { block<320> o; memset(o.data,0,40); return o; }
    inline static block<320> set_all_8(uint8_t x) { block<320> o; memset(o.data,x,40); return o; }
    inline bool is_zero() const { for(int i=0;i<40;i++) if(data[i]) return false; return true; }
    inline bool any_zeros() const { for(int i=0;i<40;i++) if(data[i] == 0) return true; return false; }
};
template <> struct block<768>
{
    block256 data[3];
    inline block<768> operator^(const block<768>& y) const { block<768> o; for(int i=0;i<3;i++) o.data[i]=data[i]^y.data[i]; return o; }
    inline block<768> operator&(const block<768>& y) const { block<768> o; for(int i=0;i<3;i++) o.data[i]=data[i]&y.data[i]; return o; }
    inline static block<768> set_zero() { block<768> o; for(int i=0;i<3;i++) o.data[i]=block256::set_zero(); return o; }
};
template <> struct block<1024>
{
    block512 data[2];
    inline block<1024> operator^(const block<1024>& y) const { block<1024> o; o.data[0]=data[0]^y.data[0]; o.data[1]=data[1]^y.data[1]; return o; }
    inline block<1024> operator&(const block<1024>& y) const { block<1024> o; o.data[0]=data[0]&y.data[0]; o.data[1]=data[1]&y.data[1]; return o; }
    inline static block<1024> set_zero() { block<1024> o; o.data[0]=block512::set_zero(); o.data[1]=block512::set_zero(); return o; }
};

// #define VOLE_BLOCK_SHIFT 0
using vole_block = block128;
using clmul_block = block128;

} // namespace faest

#endif
