#ifndef POLYNOMIALS_IMPL_HPP
#define POLYNOMIALS_IMPL_HPP

// NB:
// - code translated assuming POLY_VEC_LEN == 1
// - dropped `_vec` from the names
// - In the future, we can add a template argument for vector length.
// - load_dup and store1 do the same as load and store
// - clmul_block and poly128 are no longer the same type, switching is messy, maybe us
//   clmul_block everywhere?

#include "../block.hpp"
#include "../parameters.hpp"
#include "../polynomials_constants.hpp"
#include "../transpose.hpp"
#include "../util.hpp"
#include <cstdint>
#include <cstring>
#include <type_traits>

namespace faest
{

template <std::size_t size> struct poly;

using poly1 = poly<1>;
using poly64 = poly<64>;
using poly128 = poly<128>;
using poly160 = poly<160>;
using poly192 = poly<192>;
using poly224 = poly<224>;
using poly256 = poly<256>;
using poly320 = poly<320>;
using poly384 = poly<384>;
using poly448 = poly<448>;
using poly512 = poly<512>;
using poly576 = poly<576>;
using poly768 = poly<768>;
using poly1024 = poly<1024>;

template <std::size_t size> inline poly<size> operator+(poly<size> x, poly<size> y);

template <std::size_t size1, std::size_t size2>
poly<size1 + size2> operator*(poly<size1> x, poly<size2> y);

template <std::size_t size> poly<size> operator*(poly1 x, poly<size> y);

template <std::size_t size> inline bool operator==(poly<size> x, poly<size> y);

namespace detail
{
inline void karatsuba_mul_128_uninterpolated_other_sum(poly128 x, poly128 y, poly128 x_for_sum,
                                                       poly128 y_for_sum, poly128* out);
inline void karatsuba_mul_128_uninterpolated(poly128 x, poly128 y, poly128* out);
inline void karatsuba_mul_128_uncombined(poly128 x, poly128 y, poly128* out);
inline void combine_poly128s(poly128* out, const poly128* in, size_t n);
inline static clmul_block poly1_to_bit_mask(poly1 x);
inline void poly_shift_left_1(clmul_block* x, size_t chunks);
inline void poly_shift_left_8(clmul_block* out, const clmul_block* in, size_t chunks);
template <size_t Bytes>
inline void shift_left_bytes(uint8_t* out, const uint8_t* in, unsigned int bits);
inline bool get_bit(const uint8_t* data, size_t bit);
inline void xor_bit(uint8_t* data, size_t bit);
template <size_t InBits, size_t OutBits>
inline void reduce_double_size(const uint8_t* in, uint8_t* out, uint32_t modulus);
template <size_t ABits, size_t BBits>
inline void carryless_mul_bytes(const uint8_t* a, const uint8_t* b, uint8_t* out);
template <typename OutPoly, typename InPoly>
inline void xor_poly_at_offset(OutPoly& out, const InPoly& in, size_t byte_offset);
template <size_t size> poly<size> poly_exp(poly<size> base, size_t power);
} // namespace detail

// The single bit in each polynomial is duplicated for a whole byte.
template <> struct poly<1>
{
    uint8_t data;

    poly() = default;
    poly(unsigned long x) { *this = load(x, 0); }

    inline static poly1 set_zero() { return { 0 }; }
    inline static poly1 set_one() { return { 0xff }; }

    inline static poly1 set_all(uint8_t x) { return {x}; }

    inline static poly1 load(unsigned long x, unsigned int bit_offset)
    {
        poly1 poly;
        poly.data = ((x >> bit_offset) & 1) * 0xff;
        return poly;
    }

    inline static poly1 load_offset8(const void* s, unsigned int bit_offset)
    {
        poly1 poly;
        memcpy(&poly.data, s, sizeof(poly.data));
        poly.data >>= bit_offset;
        poly.data &= 1;
        poly.data *= 0xff;
        return poly;
    }

    template<typename T>
    poly& operator+=(const T& other)
    {
        *this = *this + other;
        return *this;
    }

    template<typename T>
    poly& operator*=(const T& other)
    {
        *this = *this * other;
        return *this;
    }
};

// This is twice as long as it needs to be. It is treated as a poly128, with the high half of
// each 128-bit polynomial ignored.
template <> struct poly<64>
{
    clmul_block data;

    inline static poly64 load(const void* s)
    {
        uint64_t in;
        memcpy(&in, s, sizeof(in));
        return {clmul_block::set_low64(in)};
        // XXX: old code used `_mm_cvtsi64_si128(in)`, is that better/different?
    }

    inline static poly64 load_dup(const void* s) { return load(s); }

    inline void store(void* d) const
    {
        uint64_t out = _mm_cvtsi128_si64(this->data.data);
        memcpy(d, &out, sizeof(out));
    }

    inline poly64 extract(size_t index) const
    {
        (void)index;
        return *this;
    }

    inline static poly64 set_zero() { return {clmul_block::set_zero()}; }

    inline static poly64 set_low32(uint32_t x) { return {_mm_cvtsi32_si128(x)}; }

    inline poly64 mul_a64_reduce64() const
    {
        poly64 modulus = set_low32(gf64_modulus);
        clmul_block high = clmul_block::clmul_ll(modulus.data, this->data); // Degree < 64 + 4
        clmul_block high_high = clmul_block::clmul_lh(modulus.data, high);  // Degree < 4 + 4

        // Only low 64 bits are correct, but the high 64 will be ignored anyway.
        return {high ^ high_high};
    }

    inline poly64 exp(size_t power) const { return detail::poly_exp(*this, power); }

    template<typename T>
    poly& operator+=(const T& other)
    {
        *this = *this + other;
        return *this;
    }

    template<typename T>
    poly& operator*=(const T& other)
    {
        *this = *this * other;
        return *this;
    }
};

template <> struct poly<128>
{
    clmul_block data;

    inline static poly128 load(const void* s)
    {
        poly128 out;
        memcpy(&out, s, sizeof(out));
        return out;
    }

    inline static poly128 load_dup(const void* s) { return load(s); }

    inline void store(void* d) const { memcpy(d, this, sizeof(*this)); }

    inline void store1(void* d) const { store(d); }

    inline poly128 extract(size_t index) const
    {
        (void)index;
        return *this;
    }

    template <std::size_t other_size> inline static poly128 from(poly<other_size> x)
    {
        if constexpr (other_size == 64)
        {
            // set the unused bits to zero
            return {_mm_insert_epi64(x.data.data, 0, 1)};
        }
        else
        {
            static_assert(dependent_false<other_size>);
        }
    }

    inline static poly128 set_zero() { return {clmul_block::set_zero()}; }

    inline static poly128 set_low32(uint32_t x) { return {_mm_cvtsi32_si128(x)}; }

    template <std::size_t out_size> inline poly<out_size> reduce_to() const
    {
        if constexpr (out_size == 64)
        {
            poly64 modulus = poly64::set_low32(gf64_modulus);
            clmul_block high = clmul_block::clmul_lh(modulus.data, this->data); // Degree < 64 + 4
            clmul_block high_high = clmul_block::clmul_lh(modulus.data, high);  // Degree < 4 + 4

            // Only low 64 bits are correct, but the high 64 will be ignored anyway.
            return {this->data ^ high ^ high_high};
        }
        else
        {
            static_assert(dependent_false<out_size>);
        }
    }

    inline static poly128 from_1(poly1 x) { return {_mm_cvtsi32_si128(x.data & 1)}; }

    inline static poly128 from_8_poly1(const poly1* bits);
    inline static poly128 from_8_self(const poly128* polys);

    inline static poly128 from_8_byte(uint8_t byte)
    {
        poly1 bits[8];
        for (size_t i = 0; i < 8; ++i)
            bits[i] = poly1::set_all(expand_bit_to_byte(byte, i));
        return from_8_poly1(bits);
    }

    inline poly128 exp(size_t power) const { return detail::poly_exp(*this, power); }

    template<typename T>
    poly& operator+=(const T& other)
    {
        *this = *this + other;
        return *this;
    }

    template<typename T>
    poly& operator*=(const T& other)
    {
        *this = *this * other;
        return *this;
    }
};

template <> struct poly<160>
{
    uint8_t data[20];

    inline static poly160 load(const void* s)
    {
        poly160 out;
        memcpy(out.data, s, sizeof(out.data));
        return out;
    }

    inline static poly160 load_dup(const void* s) { return load(s); }

    inline void store(void* d) const { memcpy(d, data, sizeof(data)); }
    inline void store1(void* d) const { store(d); }

    inline poly160 extract(size_t index) const
    {
        (void)index;
        return *this;
    }

    template <std::size_t other_size> inline static poly160 from(poly<other_size> x)
    {
        poly160 out = set_zero();
        if constexpr (other_size == 64)
        {
            x.store(out.data);
            return out;
        }
        else if constexpr (other_size == 128)
        {
            x.store(out.data);
            return out;
        }
        else
        {
            static_assert(dependent_false<other_size>);
        }
    }

    inline static poly160 set_zero()
    {
        poly160 out;
        memset(out.data, 0, sizeof(out.data));
        return out;
    }

    inline static poly160 set_low32(uint32_t x)
    {
        poly160 out = set_zero();
        memcpy(out.data, &x, sizeof(x));
        return out;
    }

    inline static poly160 from_1(poly1 x)
    {
        poly160 out = set_zero();
        out.data[0] = x.data & 1;
        return out;
    }

    inline static poly160 from_8_poly1(const poly1* bits);
    inline static poly160 from_8_self(const poly160* polys);

    inline static poly160 from_8_byte(uint8_t byte)
    {
        poly1 bits[8];
        for (size_t i = 0; i < 8; ++i)
            bits[i] = poly1::set_all(expand_bit_to_byte(byte, i));
        return from_8_poly1(bits);
    }

    inline poly160 exp(size_t power) const { return detail::poly_exp(*this, power); }

    template<typename T>
    poly& operator+=(const T& other)
    {
        *this = *this + other;
        return *this;
    }

    template<typename T>
    poly& operator*=(const T& other)
    {
        *this = *this * other;
        return *this;
    }
};

template <> struct poly<224>
{
    uint8_t data[28];

    inline static poly224 load(const void* s)
    {
        poly224 out;
        memcpy(out.data, s, sizeof(out.data));
        return out;
    }

    inline static poly224 load_dup(const void* s) { return load(s); }

    inline void store(void* d) const { memcpy(d, data, sizeof(data)); }

    inline poly224 extract(size_t index) const
    {
        (void)index;
        return *this;
    }

    inline static poly224 set_zero()
    {
        poly224 out;
        memset(out.data, 0, sizeof(out.data));
        return out;
    }

    template <std::size_t out_size> inline poly<out_size> reduce_to() const
    {
        if constexpr (out_size == 160)
        {
            poly160 out = poly160::load(data);
            poly64 high = poly64::load(data + 20);
            poly64 modulus = poly64::set_low32(gf160_modulus);
            poly128 folded = modulus * high;
            uint8_t folded_bytes[16];
            folded.store(folded_bytes);
            for (size_t i = 0; i < sizeof(folded_bytes); ++i)
                out.data[i] ^= folded_bytes[i];
            return out;
        }
        else
        {
            static_assert(dependent_false<out_size>);
        }
    }

    template<typename T>
    poly& operator+=(const T& other)
    {
        *this = *this + other;
        return *this;
    }
};

template <> struct poly<192>
{
    poly128 data[2]; // Striped in 128-bit chunks. Highest 64 bits of each poly are ignored.

    inline static poly192 load(const void* s)
    {
        poly192 out;
        memcpy(&out.data[0], s, sizeof(block128));
        out.data[1] = {_mm_loadu_si64(((char*)s) + sizeof(block128))};
        return out;
    }

    inline static poly192 load_dup(const void* s) { return load(s); }

    inline void store(void* d) const { memcpy(d, this, 24); }

    inline void store1(void* d) const { store(d); }

    inline poly192 extract(size_t index) const
    {
        (void)index;
        return *this;
    }

    template <std::size_t other_size> inline static poly192 from(poly<other_size> x)
    {
        poly192 out;
        if constexpr (other_size == 64)
        {
            return from(poly128::from(x));
        }
        else if constexpr (other_size == 128)
        {
            out.data[0] = x;
            out.data[1] = {clmul_block::set_zero()};
            return out;
        }
        else
        {
            static_assert(dependent_false<other_size>);
        }
    }

    inline static poly192 set_zero() { return {poly128::set_zero(), poly128::set_zero()}; }

    inline static poly192 set_low32(uint32_t x)
    {
        poly192 out;
        out.data[0] = poly128::set_low32(x);
        memset(&out.data[1], 0, sizeof(out.data[1]));
        return out;
    }

    template <std::size_t out_size> inline poly<out_size> reduce_to() const
    {
        if constexpr (out_size == 128)
        {
            poly64 modulus = poly64::set_low32(gf128_modulus);
            return this->data[0] + poly128{clmul_block::clmul_ll(modulus.data, this->data[1].data)};
        }
        else
        {
            static_assert(dependent_false<out_size>);
        }
    }

    inline static poly192 from_1(poly1 x)
    {
        poly192 out;
        out.data[0] = poly128::from_1(x);
        out.data[1] = {clmul_block::set_zero()};
        return out;
    }

    inline static poly192 from_8_poly1(const poly1* bits);
    inline static poly192 from_8_self(const poly192* polys);

    inline static poly192 from_8_byte(uint8_t byte)
    {
        poly1 bits[8];
        for (size_t i = 0; i < 8; ++i)
            bits[i] = poly1::set_all(expand_bit_to_byte(byte, i));
        return from_8_poly1(bits);
    }

    inline poly192 exp(size_t power) const { return detail::poly_exp(*this, power); }

    template<typename T>
    poly& operator+=(const T& other)
    {
        *this = *this + other;
        return *this;
    }

    template<typename T>
    poly& operator*=(const T& other)
    {
        *this = *this * other;
        return *this;
    }
};

template <> struct poly<256>
{
    poly128 data[2]; // Striped in 128-bit chunks.

    inline static poly256 load(const void* s)
    {
        poly256 out;
        memcpy(&out, s, sizeof(out));
        return out;
    }

    inline static poly256 load_dup(const void* s) { return load(s); }

    inline void store(void* d) const { memcpy(d, this, sizeof(*this)); }

    inline void store1(void* d) const { store(d); }

    inline poly256 extract(size_t index) const
    {
        (void)index;
        return *this;
    }

    template <std::size_t other_size> inline static poly256 from(poly<other_size> x)
    {
        poly256 out;
        if constexpr (other_size == 64)
        {
            return from(poly128::from(x));
        }
        else if constexpr (other_size == 128)
        {
            out.data[0] = x;
            out.data[1] = {clmul_block::set_zero()};
            return out;
        }
        else if constexpr (other_size == 192)
        {
            out.data[0] = x.data[0];
            out.data[1] = {_mm_insert_epi64(x.data[1].data.data, 0, 1)};
            return out;
        }
        else
        {
            static_assert(dependent_false<other_size>);
        }
    }

    inline static poly256 set_zero() { return {poly128::set_zero(), poly128::set_zero()}; }

    inline static poly256 set_low32(uint32_t x)
    {
        poly256 out;
        out.data[0] = poly128::set_low32(x);
        memset(&out.data[1], 0, sizeof(out.data[1]));
        return out;
    }

    inline poly128  low_half() const { return { data[0] }; }
    inline poly128 high_half() const { return { data[1] }; }

    template <std::size_t out_size> inline poly<out_size> reduce_to() const
    {
        if constexpr (out_size == 128)
        {
            poly256 x = *this;
            poly64 modulus = poly64::set_low32(gf128_modulus);
            clmul_block reduced_192 = clmul_block::clmul_lh(modulus.data, x.data[1].data);
            x.data[0] = {x.data[0].data ^ reduced_192.shift_left_64()};
            x.data[1] = {x.data[1].data ^ (reduced_192.shift_right_64())};
            x.data[0] = {x.data[0].data ^ clmul_block::clmul_ll(modulus.data, x.data[1].data)};
            return x.data[0];
        }
        else if constexpr (out_size == 192)
        {
            poly64 modulus = poly64::set_low32(gf192_modulus);
            poly192 out;
            out.data[1] = this->data[1];
            out.data[0] = {this->data[0].data ^
                           clmul_block::clmul_lh(modulus.data, out.data[1].data)};
            return out;
        }
        else
        {
            static_assert(dependent_false<out_size>);
        }
    }

    inline static poly256 from_1(poly1 x)
    {
        poly256 out;
        out.data[0] = poly128::from_1(x);
        out.data[1] = {clmul_block::set_zero()};
        return out;
    }

    inline static poly256 from_8_poly1(const poly1* bits);
    inline static poly256 from_8_self(const poly256* polys);

    inline static poly256 from_8_byte(uint8_t byte)
    {
        poly1 bits[8];
        for (size_t i = 0; i < 8; ++i)
            bits[i] = poly1::set_all(expand_bit_to_byte(byte, i));
        return from_8_poly1(bits);
    }

    inline poly256 shift_left_1() const;
    inline poly256 shift_left_8() const;

    inline poly256 exp(size_t power) const { return detail::poly_exp(*this, power); }

    template<typename T>
    poly& operator+=(const T& other)
    {
        *this = *this + other;
        return *this;
    }

    template<typename T>
    poly& operator*=(const T& other)
    {
        *this = *this * other;
        return *this;
    }
};

template <> struct poly<320>
{
    poly128 data[3]; // Striped in 128-bit chunks. Highest 64 bits of each poly are ignored.

    inline static poly320 load(const void* s)
    {
        poly320 out;
        memcpy(&out, s, 40);
        return out;
    }

    inline static poly320 load_dup(const void* s) { return load(s); }

    inline void store(void* d) const { memcpy(d, this, 40); }
    inline void store1(void* d) const { store(d); }

    inline poly320 extract(size_t index) const
    {
        (void)index;
        return *this;
    }

    template <std::size_t other_size> inline static poly320 from(poly<other_size> x)
    {
        poly320 out = set_zero();
        if constexpr (other_size == 160)
        {
            memcpy(&out, x.data, 20);
            return out;
        }
        else if constexpr (other_size == 224)
        {
            memcpy(&out, x.data, 28);
            return out;
        }
        else if constexpr (other_size == 256)
        {
            out.data[0] = x.data[0];
            out.data[1] = x.data[1];
            out.data[2] = {clmul_block::set_zero()};
            return out;
        }
        else
        {
            static_assert(dependent_false<other_size>);
        }
    }

    inline static poly320 set_zero() { return {poly128::set_zero(), poly128::set_zero(), poly128::set_zero()}; }

    inline static poly320 set_low32(uint32_t x)
    {
        poly320 out;
        out.data[0] = poly128::set_low32(x);
        memset(&out.data[1], 0, 2 * sizeof(out.data[1]));
        return out;
    }

    template <std::size_t out_size> inline poly<out_size> reduce_to() const
    {
        if constexpr (out_size == 160)
        {
            uint8_t tmp[40];
            store(tmp);
            poly160 out = poly160::load(tmp);
            poly160 high = poly160::load(tmp + 20);
            poly224 folded = poly64::set_low32(gf160_modulus) * high;
            return out + folded.template reduce_to<160>();
        }
        else if constexpr (out_size == 256)
        {
            poly64 modulus = poly64::set_low32(gf256_modulus);
            poly256 out;
            out.data[1] = this->data[1];
            out.data[0] =
                this->data[0] + poly128{clmul_block::clmul_ll(modulus.data, this->data[2].data)};
            return out;
        }
        else
        {
            static_assert(dependent_false<out_size>);
        }
    }

    inline static poly320 from_1(poly1 x)
    {
        poly320 out = set_zero();
        out.data[0] = poly128::from_1(x);
        return out;
    }

    inline poly320 shift_left_1() const;
    inline poly320 shift_left_8() const;

    template<typename T>
    poly& operator+=(const T& other)
    {
        *this = *this + other;
        return *this;
    }

    template<typename T>
    poly& operator*=(const T& other)
    {
        *this = *this * other;
        return *this;
    }
};

template <> struct poly<384>
{
    poly128 data[3]; // Striped in 128-bit chunks.

    inline static poly384 load(const void* s)
    {
        poly384 out;
        memcpy(&out, s, sizeof(out));
        return out;
    }

    inline static poly384 load_dup(const void* s) { return load(s); }

    inline void store(void* d) const { memcpy(d, this, sizeof(*this)); }
    inline void store1(void* d) const { store(d); }

    inline poly384 extract(size_t index) const
    {
        (void)index;
        return *this;
    }

    template <std::size_t other_size> inline static poly384 from(poly<other_size> x)
    {
        poly384 out;
        if constexpr (other_size == 192)
        {
            out.data[0] = x.data[0];
            out.data[1] = {_mm_insert_epi64(x.data[1].data.data, 0, 1)};
            out.data[2] = {clmul_block::set_zero()};
            return out;
        }
        else if constexpr (other_size == 64)
        {
            return from(poly128::from(x));
        }
        else if constexpr (other_size == 128)
        {
            out.data[0] = x;
            out.data[1] = {clmul_block::set_zero()};
            out.data[2] = {clmul_block::set_zero()};
            return out;
        }
        else if constexpr (other_size == 256)
        {
            return { x.data[0], x.data[1], clmul_block::set_zero() };
        }
        else
        {
            static_assert(dependent_false<other_size>);
        }
    }

    inline static poly384 set_zero() { return {poly128::set_zero(), poly128::set_zero(), poly128::set_zero()}; }

    inline static poly384 set_low32(uint32_t x)
    {
        poly384 out;
        out.data[0] = poly128::set_low32(x);
        memset(&out.data[1], 0, 2 * sizeof(out.data[1]));
        return out;
    }

    inline static poly384 from_1(poly1 x)
    {
        poly384 out = set_zero();
        out.data[0] = poly128::from_1(x);
        return out;
    }

    inline poly192 low_half() const { return { data[0], data[1] }; }

    inline poly192 high_half() const
    {
        unsigned char tmp[384 / 8];
        store(tmp);
        return poly192::load(&tmp[192 / 8]);
    }

    template <std::size_t out_size> inline poly<out_size> reduce_to() const
    {
        if constexpr (out_size == 192)
        {
            poly64 modulus = poly64::set_low32(gf192_modulus);
            clmul_block reduced_320 = clmul_block::clmul_lh(modulus.data, this->data[2].data);
            clmul_block reduced_256 = clmul_block::clmul_ll(modulus.data, this->data[2].data);
            poly192 out;
            out.data[1] = {this->data[1].data ^ reduced_320};
            out.data[0] = {this->data[0].data ^ reduced_256.shift_left_64()};
            out.data[0] = {out.data[0].data ^
                           clmul_block::clmul_lh(modulus.data, out.data[1].data)};
            out.data[1] = {out.data[1].data ^ reduced_256.shift_right_64()};
            return out;
        }
        else
        {
            static_assert(dependent_false<out_size>);
        }
    }

    inline poly384 shift_left_1() const;
    inline poly384 shift_left_8() const;

    template<typename T>
    poly& operator+=(const T& other)
    {
        *this = *this + other;
        return *this;
    }

    template<typename T>
    poly& operator*=(const T& other)
    {
        *this = *this * other;
        return *this;
    }
};

template <> struct poly<448>
{
    poly128 data[4]; // Highest 64 bits of the last chunk are unused.

    inline static poly448 set_zero()
    {
        return {poly128::set_zero(), poly128::set_zero(), poly128::set_zero(), poly128::set_zero()};
    }

    template <std::size_t out_size> inline poly<out_size> reduce_to() const
    {
        if constexpr (out_size == 384)
        {
            // Bits 384-447 are in the low 64 bits of data[3]. The high 64 bits are unused.
            // Multiply by modulus (degree 16): max result degree = 63 + 16 = 79, fits in 128 bits.
            // This lands at bit position 0..79 in the output, well within 384 bits.
            poly64 modulus = poly64::set_low32(gf384_modulus);
            poly128 reduced = {clmul_block::clmul_ll(modulus.data, this->data[3].data)};
            poly384 out;
            out.data[0] = this->data[0] + reduced;
            out.data[1] = this->data[1];
            out.data[2] = this->data[2];
            return out;
        }
        else
        {
            static_assert(dependent_false<out_size>);
        }
    }
};

template <> struct poly<512>
{
    poly128 data[4]; // Striped in 128-bit chunks.

    inline static poly512 load(const void* s)
    {
        poly512 out;
        memcpy(&out, s, sizeof(out));
        return out;
    }

    inline static poly512 load_dup(const void* s) { return load(s); }

    inline void store(void* d) const { memcpy(d, this, sizeof(*this)); }
    inline void store1(void* d) const { store(d); }

    inline poly512 extract(size_t index) const
    {
        (void)index;
        return *this;
    }

    template <std::size_t other_size> inline static poly512 from(poly<other_size> x)
    {
        poly512 out;
        if constexpr (other_size == 256)
        {
            out.data[0] = x.data[0];
            out.data[1] = x.data[1];
            out.data[2] = {clmul_block::set_zero()};
            out.data[3] = {clmul_block::set_zero()};
            return out;
        }
        else if constexpr (other_size == 64)
        {
            return from(poly128::from(x));
        }
        else if constexpr (other_size == 128)
        {
            out.data[0] = x;
            out.data[1] = {clmul_block::set_zero()};
            out.data[2] = {clmul_block::set_zero()};
            out.data[3] = {clmul_block::set_zero()};
            return out;
        }
        else if constexpr (other_size == 192)
        {
            out.data[0] = x.data[0];
            out.data[1] = {_mm_insert_epi64(x.data[1].data.data, 0, 1)};
            out.data[2] = {clmul_block::set_zero()};
            out.data[3] = {clmul_block::set_zero()};
            return out;
        }
        else if constexpr (other_size == 320)
        {
            out.data[0] = x.data[0];
            out.data[1] = x.data[1];
            out.data[2] = {_mm_insert_epi64(x.data[2].data.data, 0, 1)};
            out.data[3] = {clmul_block::set_zero()};
            return out;
        }
        else
        {
            static_assert(dependent_false<other_size>);
        }
    }

    inline static poly512 set_zero() { return {poly128::set_zero(), poly128::set_zero(), poly128::set_zero(), poly128::set_zero()}; }

    inline static poly512 set_low32(uint32_t x)
    {
        poly512 out;
        out.data[0] = poly128::set_low32(x);
        memset(&out.data[1], 0, 3 * sizeof(out.data[1]));
        return out;
    }

    inline static poly512 from_1(poly1 x)
    {
        poly512 out = set_zero();
        out.data[0] = poly128::from_1(x);
        return out;
    }

    inline poly256  low_half() const { return { data[0], data[1] }; }
    inline poly256 high_half() const { return { data[2], data[3] }; }

    template <std::size_t out_size> inline poly<out_size> reduce_to() const
    {
        if constexpr (out_size == 256)
        {
            poly64 modulus = poly64::set_low32(gf256_modulus);
            poly128 xmod[4];
            xmod[0] = poly128{clmul_block::set_zero()};
            xmod[1] = poly128{clmul_block::clmul_lh(modulus.data, this->data[2].data)};
            xmod[2] = poly128{clmul_block::clmul_ll(modulus.data, this->data[3].data)};
            xmod[3] = poly128{clmul_block::clmul_lh(modulus.data, this->data[3].data)};

            poly128 xmod_combined[3];
            detail::combine_poly128s(&xmod_combined[0], &xmod[0], 4);
            for (size_t i = 0; i < 3; ++i)
                xmod_combined[i] = xmod_combined[i] + this->data[i];
            xmod_combined[0] =
                xmod_combined[0] + poly128{clmul_block::clmul_ll(modulus.data, xmod_combined[2].data)};

            poly256 out;
            for (size_t i = 0; i < 2; ++i)
                out.data[i] = {xmod_combined[i]};
            return out;
        }
        else
        {
            static_assert(dependent_false<out_size>);
        }
    }

    inline poly512 shift_left_1() const;
    inline poly512 shift_left_8() const;

    template<typename T>
    poly& operator+=(const T& other)
    {
        *this = *this + other;
        return *this;
    }

    template<typename T>
    poly& operator*=(const T& other)
    {
        *this = *this * other;
        return *this;
    }
};

template <> struct poly<576>
{
    poly128 data[5]; // Highest 64 bits of the last chunk are unused.

    inline static poly576 set_zero()
    {
        return {poly128::set_zero(), poly128::set_zero(), poly128::set_zero(),
                poly128::set_zero(), poly128::set_zero()};
    }

    template <std::size_t out_size> inline poly<out_size> reduce_to() const
    {
        if constexpr (out_size == 512)
        {
            // Bits 512-575 are in the low 64 bits of data[4]. High 64 bits unused.
            // Multiply by modulus (degree 8): max result degree = 63 + 8 = 71, fits in 128 bits.
            poly64 modulus = poly64::set_low32(gf512_modulus);
            poly128 reduced = {clmul_block::clmul_ll(modulus.data, this->data[4].data)};
            poly512 out;
            out.data[0] = this->data[0] + reduced;
            out.data[1] = this->data[1];
            out.data[2] = this->data[2];
            out.data[3] = this->data[3];
            return out;
        }
        else
        {
            static_assert(dependent_false<out_size>);
        }
    }
};

template <> struct poly<768>
{
    poly128 data[6];

    inline static poly768 load(const void* s)
    {
        poly768 out;
        memcpy(&out, s, sizeof(out));
        return out;
    }

    inline static poly768 load_dup(const void* s) { return load(s); }

    inline void store(void* d) const { memcpy(d, this, sizeof(*this)); }

    inline void store1(void* d) const { store(d); }

    inline poly768 extract(size_t index) const
    {
        (void)index;
        return *this;
    }

    template <std::size_t other_size> inline static poly768 from(poly<other_size> x)
    {
        if constexpr (other_size == 384)
        {
            return {x.data[0], x.data[1], x.data[2], poly128::set_zero(),
                    poly128::set_zero(), poly128::set_zero()};
        }
        else
        {
            static_assert(dependent_false<other_size>);
        }
    }

    inline static poly768 set_zero()
    {
        return {poly128::set_zero(), poly128::set_zero(), poly128::set_zero(),
                poly128::set_zero(), poly128::set_zero(), poly128::set_zero()};
    }

    inline static poly768 set_low32(uint32_t x)
    {
        poly768 out = set_zero();
        out.data[0] = poly128::set_low32(x);
        return out;
    }

    template <std::size_t out_size> inline poly<out_size> reduce_to() const
    {
        if constexpr (out_size == 384)
        {
            // Fold data[3..5] (bits 384-767) back using modulus.
            // Modulus has degree 16, so product of a 128-bit chunk with modulus has degree <= 127+16 = 143.
            // Each chunk produces two overlapping 128-bit results (from clmul_ll and clmul_lh).
            poly64 modulus = poly64::set_low32(gf384_modulus);

            // Multiply each high chunk by modulus. Each 128-bit chunk has low64 and high64 parts.
            // clmul_ll: low64 × modulus → 128-bit result at chunk's base offset
            // clmul_lh: high64 × modulus → 128-bit result at chunk's base offset + 64 bits
            poly128 xmod[6];
            xmod[0] = poly128{clmul_block::clmul_ll(modulus.data, this->data[3].data)};
            xmod[1] = poly128{clmul_block::clmul_lh(modulus.data, this->data[3].data)};
            xmod[2] = poly128{clmul_block::clmul_ll(modulus.data, this->data[4].data)};
            xmod[3] = poly128{clmul_block::clmul_lh(modulus.data, this->data[4].data)};
            xmod[4] = poly128{clmul_block::clmul_ll(modulus.data, this->data[5].data)};
            xmod[5] = poly128{clmul_block::clmul_lh(modulus.data, this->data[5].data)};

            // Combine overlapping 128-bit pieces. 6 pieces → 4 combined 128-bit chunks
            // covering bit range [0, 128*4) = [0, 512) relative to bit 384.
            // But we only need to fold into 384 bits, so the combined result may overflow.
            poly128 combined[4];
            detail::combine_poly128s(&combined[0], &xmod[0], 6);

            // XOR into low half.
            // combined[0..2] go into data[0..2]. combined[3] may have overflow bits.
            poly128 out_data[3];
            for (size_t i = 0; i < 3; ++i)
                out_data[i] = combined[i] + this->data[i];

            // combined[3] contains bits at positions 384..511 (relative to output).
            // These are overflow beyond 384 bits. Fold again: multiply by modulus.
            // Max degree of combined[3] content: bits 384 + 127 = 511 relative to output,
            // but actually combined[3] has at most degree ~16 in the high part
            // (from 383+16=399, so positions 384..399, i.e. at most 16 bits).
            // Second fold: combined[3] × modulus → at most degree 127+16=143 bits, fits in 128 bits.
            out_data[0] = out_data[0] + poly128{clmul_block::clmul_ll(modulus.data, combined[3].data)};

            poly384 out;
            out.data[0] = out_data[0];
            out.data[1] = out_data[1];
            out.data[2] = out_data[2];
            return out;
        }
        else
        {
            static_assert(dependent_false<out_size>);
        }
    }

    inline poly768 shift_left_1() const;
    inline poly768 shift_left_8() const;

    template<typename T>
    poly& operator+=(const T& other)
    {
        *this = *this + other;
        return *this;
    }

    template<typename T>
    poly& operator*=(const T& other)
    {
        *this = *this * other;
        return *this;
    }
};

template <> struct poly<1024>
{
    poly128 data[8];

    inline static poly1024 load(const void* s)
    {
        poly1024 out;
        memcpy(&out, s, sizeof(out));
        return out;
    }

    inline static poly1024 load_dup(const void* s) { return load(s); }

    inline void store(void* d) const { memcpy(d, this, sizeof(*this)); }

    inline void store1(void* d) const { store(d); }

    inline poly1024 extract(size_t index) const
    {
        (void)index;
        return *this;
    }

    template <std::size_t other_size> inline static poly1024 from(poly<other_size> x)
    {
        if constexpr (other_size == 512)
        {
            return {x.data[0], x.data[1], x.data[2], x.data[3], poly128::set_zero(),
                    poly128::set_zero(), poly128::set_zero(), poly128::set_zero()};
        }
        else
        {
            static_assert(dependent_false<other_size>);
        }
    }

    inline static poly1024 set_zero()
    {
        return {poly128::set_zero(), poly128::set_zero(), poly128::set_zero(), poly128::set_zero(),
                poly128::set_zero(), poly128::set_zero(), poly128::set_zero(), poly128::set_zero()};
    }

    inline static poly1024 set_low32(uint32_t x)
    {
        poly1024 out = set_zero();
        out.data[0] = poly128::set_low32(x);
        return out;
    }

    template <std::size_t out_size> inline poly<out_size> reduce_to() const
    {
        if constexpr (out_size == 512)
        {
            // Fold data[4..7] (bits 512-1023) back using modulus.
            poly64 modulus = poly64::set_low32(gf512_modulus);

            poly128 xmod[8];
            xmod[0] = poly128{clmul_block::clmul_ll(modulus.data, this->data[4].data)};
            xmod[1] = poly128{clmul_block::clmul_lh(modulus.data, this->data[4].data)};
            xmod[2] = poly128{clmul_block::clmul_ll(modulus.data, this->data[5].data)};
            xmod[3] = poly128{clmul_block::clmul_lh(modulus.data, this->data[5].data)};
            xmod[4] = poly128{clmul_block::clmul_ll(modulus.data, this->data[6].data)};
            xmod[5] = poly128{clmul_block::clmul_lh(modulus.data, this->data[6].data)};
            xmod[6] = poly128{clmul_block::clmul_ll(modulus.data, this->data[7].data)};
            xmod[7] = poly128{clmul_block::clmul_lh(modulus.data, this->data[7].data)};

            // 8 inputs → 5 combined outputs at positions 0, 128, 256, 384, 512.
            poly128 combined[5];
            detail::combine_poly128s(&combined[0], &xmod[0], 8);

            poly128 out_data[4];
            for (size_t i = 0; i < 4; ++i)
                out_data[i] = combined[i] + this->data[i];

            // combined[4] has overflow past 512 bits. Fold again.
            // Max content: degree 8 (modulus) in the high-most product, so at most ~8 bits.
            out_data[0] = out_data[0] + poly128{clmul_block::clmul_ll(modulus.data, combined[4].data)};

            poly512 out;
            out.data[0] = out_data[0];
            out.data[1] = out_data[1];
            out.data[2] = out_data[2];
            out.data[3] = out_data[3];
            return out;
        }
        else
        {
            static_assert(dependent_false<out_size>);
        }
    }

    inline poly1024 shift_left_1() const;
    inline poly1024 shift_left_8() const;

    template<typename T>
    poly& operator+=(const T& other)
    {
        *this = *this + other;
        return *this;
    }

    template<typename T>
    poly& operator*=(const T& other)
    {
        *this = *this * other;
        return *this;
    }
};

template <> inline bool operator==(poly1 x, poly1 y) { return x.data == y.data; }

template <> inline bool operator==(poly64 x, poly64 y)
{
    return _mm_cvtsi128_si64(x.data.data) == _mm_cvtsi128_si64(y.data.data);
}

template <> inline bool operator==(poly128 x, poly128 y)
{
    __m128i tmp = _mm_xor_si128(x.data.data, y.data.data);
    return _mm_test_all_zeros(tmp, tmp);
}

template <> inline bool operator==(poly160 x, poly160 y)
{
    return memcmp(x.data, y.data, sizeof(x.data)) == 0;
}

template <> inline bool operator==(poly224 x, poly224 y)
{
    return memcmp(x.data, y.data, sizeof(x.data)) == 0;
}

template <> inline bool operator==(poly192 x, poly192 y)
{
    __m128i tmp0 = _mm_xor_si128(x.data[0].data.data, y.data[0].data.data);
    return _mm_test_all_zeros(tmp0, tmp0) &&
           (_mm_cvtsi128_si64(x.data[1].data.data) == _mm_cvtsi128_si64(y.data[1].data.data));
}

template <> inline bool operator==(poly256 x, poly256 y)
{
    __m128i tmp0 = _mm_xor_si128(x.data[0].data.data, y.data[0].data.data);
    __m128i tmp1 = _mm_xor_si128(x.data[1].data.data, y.data[1].data.data);
    return _mm_test_all_zeros(tmp0, tmp0) && _mm_test_all_zeros(tmp1, tmp1);
}

template <> inline bool operator==(poly320 x, poly320 y)
{
    __m128i tmp0 = _mm_xor_si128(x.data[0].data.data, y.data[0].data.data);
    __m128i tmp1 = _mm_xor_si128(x.data[1].data.data, y.data[1].data.data);
    return _mm_test_all_zeros(tmp0, tmp0) && _mm_test_all_zeros(tmp1, tmp1) &&
           (_mm_cvtsi128_si64(x.data[2].data.data) == _mm_cvtsi128_si64(y.data[2].data.data));
}

template <> inline bool operator==(poly384 x, poly384 y)
{
    __m128i tmp0 = _mm_xor_si128(x.data[0].data.data, y.data[0].data.data);
    __m128i tmp1 = _mm_xor_si128(x.data[1].data.data, y.data[1].data.data);
    __m128i tmp2 = _mm_xor_si128(x.data[2].data.data, y.data[2].data.data);
    return _mm_test_all_zeros(tmp0, tmp0) && _mm_test_all_zeros(tmp1, tmp1) &&
           _mm_test_all_zeros(tmp2, tmp2);
}

template <> inline bool operator==(poly512 x, poly512 y)
{
    __m128i tmp0 = _mm_xor_si128(x.data[0].data.data, y.data[0].data.data);
    __m128i tmp1 = _mm_xor_si128(x.data[1].data.data, y.data[1].data.data);
    __m128i tmp2 = _mm_xor_si128(x.data[2].data.data, y.data[2].data.data);
    __m128i tmp3 = _mm_xor_si128(x.data[3].data.data, y.data[3].data.data);
    return _mm_test_all_zeros(tmp0, tmp0) && _mm_test_all_zeros(tmp1, tmp1) &&
           _mm_test_all_zeros(tmp2, tmp2) && _mm_test_all_zeros(tmp3, tmp3);
}

template <> inline bool operator==(poly448 x, poly448 y)
{
    return memcmp(&x, &y, sizeof(x)) == 0;
}

template <> inline bool operator==(poly576 x, poly576 y)
{
    return memcmp(&x, &y, sizeof(x)) == 0;
}

template <> inline bool operator==(poly768 x, poly768 y)
{
    return memcmp(&x, &y, sizeof(x)) == 0;
}

template <> inline bool operator==(poly1024 x, poly1024 y)
{
    return memcmp(&x, &y, sizeof(x)) == 0;
}

template <> inline poly1 operator+(poly1 x, poly1 y)
{
    return {static_cast<uint8_t>(x.data ^ y.data)};
}

template <> inline poly64 operator+(poly64 x, poly64 y) { return {x.data ^ y.data}; }

template <> inline poly128 operator+(poly128 x, poly128 y) { return {x.data ^ y.data}; }

template <> inline poly160 operator+(poly160 x, poly160 y)
{
    poly160 out;
    for (size_t i = 0; i < sizeof(out.data); ++i)
        out.data[i] = x.data[i] ^ y.data[i];
    return out;
}

template <> inline poly224 operator+(poly224 x, poly224 y)
{
    poly224 out;
    for (size_t i = 0; i < sizeof(out.data); ++i)
        out.data[i] = x.data[i] ^ y.data[i];
    return out;
}

template <> inline poly192 operator+(poly192 x, poly192 y)
{
    return {{x.data[0] + y.data[0], x.data[1] + y.data[1]}};
}

template <> inline poly256 operator+(poly256 x, poly256 y)
{
    return {{x.data[0] + y.data[0], x.data[1] + y.data[1]}};
}

template <> inline poly320 operator+(poly320 x, poly320 y)
{
    return {{x.data[0] + y.data[0], x.data[1] + y.data[1], x.data[2] + y.data[2]}};
}

template <> inline poly384 operator+(poly384 x, poly384 y)
{
    return {{x.data[0] + y.data[0], x.data[1] + y.data[1], x.data[2] + y.data[2]}};
}

template <> inline poly512 operator+(poly512 x, poly512 y)
{
    return {{x.data[0] + y.data[0], x.data[1] + y.data[1], x.data[2] + y.data[2],
             x.data[3] + y.data[3]}};
}

template <> inline poly448 operator+(poly448 x, poly448 y)
{
    return {{x.data[0] + y.data[0], x.data[1] + y.data[1], x.data[2] + y.data[2],
             x.data[3] + y.data[3]}};
}

template <> inline poly576 operator+(poly576 x, poly576 y)
{
    return {{x.data[0] + y.data[0], x.data[1] + y.data[1], x.data[2] + y.data[2],
             x.data[3] + y.data[3], x.data[4] + y.data[4]}};
}

template <> inline poly768 operator+(poly768 x, poly768 y)
{
    return {{x.data[0] + y.data[0], x.data[1] + y.data[1], x.data[2] + y.data[2],
             x.data[3] + y.data[3], x.data[4] + y.data[4], x.data[5] + y.data[5]}};
}

template <> inline poly1024 operator+(poly1024 x, poly1024 y)
{
    return {{x.data[0] + y.data[0], x.data[1] + y.data[1], x.data[2] + y.data[2],
             x.data[3] + y.data[3], x.data[4] + y.data[4], x.data[5] + y.data[5],
             x.data[6] + y.data[6], x.data[7] + y.data[7]}};
}

template <> inline poly128 operator*(poly64 x, poly64 y)
{
    return {clmul_block::clmul_ll(x.data, y.data)};
}

template <> inline poly256 operator*(poly128 x, poly128 y)
{
    poly128 karatsuba_out[3];
    detail::karatsuba_mul_128_uncombined(x, y, &karatsuba_out[0]);

    poly256 out;
    detail::combine_poly128s(&out.data[0], &karatsuba_out[0], 3);
    return out;
}

template <> inline poly384 operator*(poly192 x, poly192 y)
{
    // Something like Toom-Cook.

    // Evaluate at 0.
    poly128 xlow_ylow = {clmul_block::clmul_ll(x.data[0].data, y.data[0].data)};

    // Evaluate at infinity.
    poly128 xhigh_yhigh = {clmul_block::clmul_ll(x.data[1].data, y.data[1].data)};

    // Evaluate at 1.
    clmul_block x1_cat_y0_plus_y2 =
        clmul_block::mix_64(y.data[0].data ^ y.data[1].data, x.data[0].data);
    clmul_block xsum = x.data[0].data ^ x.data[1].data ^ x1_cat_y0_plus_y2; // Result in low.
    clmul_block ysum = y.data[0].data ^ x1_cat_y0_plus_y2;                  // Result in high.
    poly128 xsum_ysum = {clmul_block::clmul_lh(xsum, ysum)};

    // Evaluate at the root of a^2 + a + 1.
    poly128 xa = x.data[0] + poly128{x.data[1].data.broadcast_low64()};
    poly128 ya = y.data[0] + poly128{y.data[1].data.broadcast_low64()};
    // Karatsuba multiplication of two degree 1 polynomials (with deg <64 polynomial coefficients).
    poly128 karatsuba_out[3];
    detail::karatsuba_mul_128_uninterpolated_other_sum(xa, ya, x.data[0], y.data[0],
                                                       &karatsuba_out[0]);
    poly128 xya0 = karatsuba_out[0] + karatsuba_out[2];
    poly128 xya1 = karatsuba_out[0] + karatsuba_out[1];

    // Interpolate through the 4 evaluation points.
    poly128 xya0_plus_xsum_ysum = xya0 + xsum_ysum;
    poly128 interp[5];
    interp[0] = xlow_ylow;
    interp[1] = xya0_plus_xsum_ysum + xhigh_yhigh;
    interp[2] = xya0_plus_xsum_ysum + xya1;
    interp[3] = xlow_ylow + xsum_ysum + xya1;
    interp[4] = xhigh_yhigh;

    // Combine overlapping 128-bit chunks.
    poly384 out;
    detail::combine_poly128s(&out.data[0], &interp[0], 5);
    return out;
}

template <> inline poly512 operator*(poly256 x, poly256 y)
{
    const poly256 z0 = x.data[0] * y.data[0];
    const poly256 z2 = x.data[1] * y.data[1];
    const poly128 xsum = x.data[0] + x.data[1];
    const poly128 ysum = y.data[0] + y.data[1];
    const poly256 z1 = (xsum * ysum) + z0 + z2;

    poly512 out = poly512::set_zero();
    detail::xor_poly_at_offset(out, z0, 0);
    detail::xor_poly_at_offset(out, z1, 16);
    detail::xor_poly_at_offset(out, z2, 32);
    return out;
}

template <> inline poly192 operator*(poly64 x, poly128 y)
{
    poly128 xy[2];
    xy[0] = poly128{clmul_block::clmul_ll(x.data, y.data)};
    xy[1] = poly128{clmul_block::clmul_lh(x.data, y.data)};

    poly192 out;
    detail::combine_poly128s(&out.data[0], &xy[0], 2);
    return out;
}

template <> inline poly224 operator*(poly64 x, poly160 y)
{
    poly64 y0 = poly64::load(y.data);
    poly64 y1 = poly64::load(y.data + 8);

    uint32_t y2_u32;
    memcpy(&y2_u32, y.data + 16, sizeof(y2_u32));
    poly64 y2 = poly64::set_low32(y2_u32);

    uint8_t out_bytes[28] = {};
    uint8_t prod[16];
    auto xor_prod = [&](poly64 b, size_t byte_offset, size_t bytes = sizeof(prod))
    {
        poly128 p = x * b;
        p.store(prod);
        for (size_t i = 0; i < bytes; ++i)
            out_bytes[byte_offset + i] ^= prod[i];
    };

    xor_prod(y0, 0);
    xor_prod(y1, 8);
    xor_prod(y2, 16, 12);

    poly224 out;
    memcpy(out.data, out_bytes, sizeof(out_bytes));
    return out;
}

template <> inline poly256 operator*(poly64 x, poly192 y)
{
    poly128 xy[3];
    xy[0] = poly128{clmul_block::clmul_ll(x.data, y.data[0].data)};
    xy[1] = poly128{clmul_block::clmul_lh(x.data, y.data[0].data)};
    xy[2] = poly128{clmul_block::clmul_ll(x.data, y.data[1].data)};

    poly256 out;
    detail::combine_poly128s(&out.data[0], &xy[0], 3);
    return out;
}

template <> inline poly320 operator*(poly64 x, poly256 y)
{
    poly128 xy[4];
    xy[0] = poly128{clmul_block::clmul_ll(x.data, y.data[0].data)};
    xy[1] = poly128{clmul_block::clmul_lh(x.data, y.data[0].data)};
    xy[2] = poly128{clmul_block::clmul_ll(x.data, y.data[1].data)};
    xy[3] = poly128{clmul_block::clmul_lh(x.data, y.data[1].data)};

    poly320 out;
    detail::combine_poly128s(&out.data[0], &xy[0], 4);
    return out;
}

template <> inline poly448 operator*(poly64 x, poly384 y)
{
    poly192 ylow = y.low_half();
    poly192 yhigh = y.high_half();
    poly256 z0 = x * ylow;
    poly256 z1 = x * yhigh;

    poly448 out = poly448::set_zero();
    detail::xor_poly_at_offset(out, z0, 0);
    detail::xor_poly_at_offset(out, z1, 192 / 8);
    return out;
}

template <> inline poly576 operator*(poly64 x, poly512 y)
{
    poly256 ylow = y.low_half();
    poly256 yhigh = y.high_half();
    poly320 z0 = x * ylow;
    poly320 z1 = x * yhigh;

    poly576 out = poly576::set_zero();
    detail::xor_poly_at_offset(out, z0, 0);
    detail::xor_poly_at_offset(out, z1, 256 / 8);
    return out;
}

template <> inline poly320 operator*(poly160 x, poly160 y)
{
    poly64 x0 = poly64::load(x.data);
    poly64 x1 = poly64::load(x.data + 8);
    poly64 y0 = poly64::load(y.data);
    poly64 y1 = poly64::load(y.data + 8);

    uint32_t x2_u32;
    uint32_t y2_u32;
    memcpy(&x2_u32, x.data + 16, sizeof(x2_u32));
    memcpy(&y2_u32, y.data + 16, sizeof(y2_u32));
    poly64 x2 = poly64::set_low32(x2_u32);
    poly64 y2 = poly64::set_low32(y2_u32);

    uint8_t out_bytes[40] = {};
    uint8_t prod[16];
    auto xor_prod = [&](poly64 a, poly64 b, size_t byte_offset, size_t bytes = sizeof(prod))
    {
        poly128 p = a * b;
        p.store(prod);
        for (size_t i = 0; i < bytes; ++i)
            out_bytes[byte_offset + i] ^= prod[i];
    };

    xor_prod(x0, y0, 0);
    xor_prod(x0, y1, 8);
    xor_prod(x1, y0, 8);
    xor_prod(x0, y2, 16);
    xor_prod(x1, y1, 16);
    xor_prod(x2, y0, 16);
    xor_prod(x1, y2, 24);
    xor_prod(x2, y1, 24);
    xor_prod(x2, y2, 32, 8);

    poly320 out = poly320::set_zero();
    memcpy(&out, out_bytes, sizeof(out_bytes));
    return out;
}

template <> inline poly128 operator*(poly1 x, poly128 y)
{
    return {detail::poly1_to_bit_mask(x) & y.data};
}

template <> inline poly160 operator*(poly1 x, poly160 y)
{
    const uint8_t mask = static_cast<uint8_t>(0u - static_cast<uint8_t>(x.data & 1u));
    poly160 out;
    for (size_t i = 0; i < sizeof(out.data); ++i)
        out.data[i] = y.data[i] & mask;
    return out;
}

template <> inline poly192 operator*(poly1 x, poly192 y)
{
    clmul_block mask = detail::poly1_to_bit_mask(x);
    poly192 out;
    out.data[0] = {mask & y.data[0].data};
    out.data[1] = {mask & y.data[1].data};
    return out;
}

template <> inline poly256 operator*(poly1 x, poly256 y)
{
    clmul_block mask = detail::poly1_to_bit_mask(x);
    poly256 out;
    out.data[0] = {mask & y.data[0].data};
    out.data[1] = {mask & y.data[1].data};
    return out;
}

template <> inline poly320 operator*(poly1 x, poly320 y)
{
    const uint8_t mask = static_cast<uint8_t>(0u - static_cast<uint8_t>(x.data & 1u));
    uint8_t in[40];
    uint8_t out_bytes[40];
    y.store(in);
    for (size_t i = 0; i < sizeof(in); ++i)
        out_bytes[i] = in[i] & mask;
    return poly320::load(out_bytes);
}

template <> inline poly384 operator*(poly1 x, poly384 y)
{
    clmul_block mask = detail::poly1_to_bit_mask(x);
    poly384 out;
    out.data[0] = {mask & y.data[0].data};
    out.data[1] = {mask & y.data[1].data};
    out.data[2] = {mask & y.data[2].data};
    return out;
}

template <> inline poly512 operator*(poly1 x, poly512 y)
{
    clmul_block mask = detail::poly1_to_bit_mask(x);
    poly512 out;
    out.data[0] = {mask & y.data[0].data};
    out.data[1] = {mask & y.data[1].data};
    out.data[2] = {mask & y.data[2].data};
    out.data[3] = {mask & y.data[3].data};
    return out;
}

template <> inline poly768 operator*(poly384 x, poly384 y)
{
    poly192 xlow = x.low_half();
    poly192 ylow = y.low_half();
    poly192 xhigh = x.high_half();
    poly192 yhigh = y.high_half();

    poly384 z0 = xlow * ylow;
    poly384 z2 = xhigh * yhigh;
    poly384 z1 = (xlow + xhigh) * (ylow + yhigh) + z0 + z2;

    poly768 out = poly768::set_zero();
    detail::xor_poly_at_offset(out, z0, 0);
    detail::xor_poly_at_offset(out, z1, 192 / 8);
    detail::xor_poly_at_offset(out, z2, 384 / 8);
    return out;
}

template <> inline poly1024 operator*(poly512 x, poly512 y)
{
    poly256 xlow = x.low_half();
    poly256 ylow = y.low_half();
    poly256 xhigh = x.high_half();
    poly256 yhigh = y.high_half();

    poly512 z0 = xlow * ylow;
    poly512 z2 = xhigh * yhigh;
    poly512 z1 = (xlow + xhigh) * (ylow + yhigh) + z0 + z2;

    poly1024 out = poly1024::set_zero();
    detail::xor_poly_at_offset(out, z0, 0);
    detail::xor_poly_at_offset(out, z1, 256 / 8);
    detail::xor_poly_at_offset(out, z2, 512 / 8);
    return out;
}

inline poly256 poly256::shift_left_1() const
{
    poly256 out = *this;
    detail::poly_shift_left_1(&out.data[0].data, 2);
    return out;
}
inline poly320 poly320::shift_left_1() const
{
    uint8_t in[40];
    uint8_t out_bytes[40];
    store(in);
    detail::shift_left_bytes<sizeof(out_bytes)>(out_bytes, in, 1);
    return poly320::load(out_bytes);
}
inline poly384 poly384::shift_left_1() const
{
    poly384 out = *this;
    detail::poly_shift_left_1(&out.data[0].data, 3);
    return out;
}
inline poly512 poly512::shift_left_1() const
{
    poly512 out = *this;
    detail::poly_shift_left_1(&out.data[0].data, 4);
    return out;
}
inline poly768 poly768::shift_left_1() const
{
    poly768 out = *this;
    detail::poly_shift_left_1(&out.data[0].data, 6);
    return out;
}
inline poly1024 poly1024::shift_left_1() const
{
    poly1024 out = *this;
    detail::poly_shift_left_1(&out.data[0].data, 8);
    return out;
}

inline poly256 poly256::shift_left_8() const
{
    poly256 out;
    detail::poly_shift_left_8(&out.data[0].data, &this->data[0].data, 2);
    return out;
}
inline poly320 poly320::shift_left_8() const
{
    uint8_t in[40];
    uint8_t out_bytes[40];
    store(in);
    detail::shift_left_bytes<sizeof(out_bytes)>(out_bytes, in, 8);
    return poly320::load(out_bytes);
}
inline poly384 poly384::shift_left_8() const
{
    poly384 out;
    detail::poly_shift_left_8(&out.data[0].data, &this->data[0].data, 3);
    return out;
}
inline poly512 poly512::shift_left_8() const
{
    poly512 out;
    detail::poly_shift_left_8(&out.data[0].data, &this->data[0].data, 4);
    return out;
}
inline poly768 poly768::shift_left_8() const
{
    poly768 out;
    detail::shift_left_bytes<sizeof(out)>(reinterpret_cast<uint8_t*>(&out),
                                          reinterpret_cast<const uint8_t*>(this), 8);
    return out;
}
inline poly1024 poly1024::shift_left_8() const
{
    poly1024 out;
    detail::shift_left_bytes<sizeof(out)>(reinterpret_cast<uint8_t*>(&out),
                                          reinterpret_cast<const uint8_t*>(this), 8);
    return out;
}

inline poly128 poly128::from_8_poly1(const poly1* bits)
{
    poly128 out = from_1(bits[0]);
    for (size_t i = 1; i < 8; ++i)
        out = out + bits[i] * poly128::load_dup(gf8_in_gf128[i - 1]);
    return out;
}

inline poly160 poly160::from_8_poly1(const poly1* bits)
{
    poly160 out = set_zero();
    for (size_t i = 0; i < 8; ++i)
        if (bits[i].data & 1u)
            out.data[0] ^= static_cast<uint8_t>(1u << i);
    return out;
}

inline poly192 poly192::from_8_poly1(const poly1* bits)
{
    poly192 out = from_1(bits[0]);
    for (size_t i = 1; i < 8; ++i)
        out = out + bits[i] * poly192::load_dup(gf8_in_gf192[i - 1]);
    return out;
}

inline poly256 poly256::from_8_poly1(const poly1* bits)
{
    poly256 out = from_1(bits[0]);
    for (size_t i = 1; i < 8; ++i)
        out = out + bits[i] * poly256::load_dup(gf8_in_gf256[i - 1]);
    return out;
}

inline poly128 poly128::from_8_self(const poly128* polys)
{
    poly256 out = poly256::from<128>(polys[0]);
    for (size_t i = 1; i < 8; ++i)
        out = out + polys[i] * poly128::load_dup(gf8_in_gf128[i - 1]);
    return out.reduce_to<128>();
}

inline poly160 poly160::from_8_self(const poly160* polys)
{
    poly160 out = polys[0];
    for (size_t i = 1; i < 8; ++i)
    {
        uint8_t wide_in[40] = {};
        uint8_t shifted[40];
        memcpy(wide_in, polys[i].data, sizeof(polys[i].data));
        detail::shift_left_bytes<sizeof(shifted)>(shifted, wide_in, static_cast<unsigned int>(i));
        out += poly320::load(shifted).template reduce_to<160>();
    }
    return out;
}

inline poly192 poly192::from_8_self(const poly192* polys)
{
    poly384 out = poly384::from<192>(polys[0]);
    for (size_t i = 1; i < 8; ++i)
        out = out + polys[i] * poly192::load_dup(gf8_in_gf192[i - 1]);
    return out.reduce_to<192>();
}

inline poly256 poly256::from_8_self(const poly256* polys)
{
    poly512 out = poly512::from<256>(polys[0]);
    for (size_t i = 1; i < 8; ++i)
        out = out + polys[i] * poly256::load_dup(gf8_in_gf256[i - 1]);
    return out.reduce_to<256>();
}

namespace detail
{
template <size_t Bytes>
inline void shift_left_bytes(uint8_t* out, const uint8_t* in, unsigned int bits)
{
    FAEST_ASSERT(bits <= 8);
    uint8_t carry = 0;
    for (size_t i = 0; i < Bytes; ++i)
    {
        out[i] = static_cast<uint8_t>((in[i] << bits) | carry);
        carry = bits == 8 ? in[i] : static_cast<uint8_t>(in[i] >> (8 - bits));
    }
}

inline bool get_bit(const uint8_t* data, size_t bit)
{
    return (data[bit / 8] >> (bit % 8)) & 1u;
}

inline void xor_bit(uint8_t* data, size_t bit)
{
    data[bit / 8] ^= static_cast<uint8_t>(1u << (bit % 8));
}

template <size_t ABits, size_t BBits>
inline void carryless_mul_bytes(const uint8_t* a, const uint8_t* b, uint8_t* out)
{
    static_assert(ABits % 8 == 0);
    static_assert(BBits % 8 == 0);
    constexpr size_t a_bytes = ABits / 8;
    constexpr size_t b_bytes = BBits / 8;
    constexpr size_t out_bytes = (ABits + BBits) / 8;

    memset(out, 0, out_bytes);
    for (size_t bit = 0; bit < ABits; ++bit)
    {
        if (!get_bit(a, bit))
            continue;

        const size_t byte_offset = bit / 8;
        const unsigned int bit_offset = static_cast<unsigned int>(bit % 8);
        uint8_t carry = 0;
        for (size_t j = 0; j < b_bytes; ++j)
        {
            const uint16_t shifted = static_cast<uint16_t>(b[j] << bit_offset) | carry;
            out[byte_offset + j] ^= static_cast<uint8_t>(shifted);
            carry = static_cast<uint8_t>(shifted >> 8);
        }
        if (carry != 0)
            out[byte_offset + b_bytes] ^= carry;
    }

    (void)a_bytes;
}

template <size_t InBits, size_t OutBits>
inline void reduce_double_size(const uint8_t* in, uint8_t* out, uint32_t modulus)
{
    constexpr size_t in_bytes = InBits / 8;
    constexpr size_t out_bytes = OutBits / 8;
    uint8_t tmp[in_bytes];
    memcpy(tmp, in, in_bytes);

    for (size_t bit = InBits; bit-- > OutBits;)
    {
        if (!get_bit(tmp, bit))
            continue;

        xor_bit(tmp, bit);
        const size_t base = bit - OutBits;
        xor_bit(tmp, base);
        for (unsigned int shift = 1; shift < 32; ++shift)
            if ((modulus >> shift) & 1u)
                xor_bit(tmp, base + shift);
    }

    memcpy(out, tmp, out_bytes);
}

template <typename OutPoly, typename InPoly>
inline void xor_poly_at_offset(OutPoly& out, const InPoly& in, size_t byte_offset)
{
    uint8_t out_bytes[sizeof(OutPoly)];
    uint8_t in_bytes[sizeof(InPoly)];
    memcpy(out_bytes, &out, sizeof(out_bytes));
    memcpy(in_bytes, &in, sizeof(in_bytes));

    for (size_t i = 0; i < sizeof(InPoly); ++i)
        out_bytes[byte_offset + i] ^= in_bytes[i];

    memcpy(&out, out_bytes, sizeof(out_bytes));
}

// Karatsuba multiplication, but end right after the multiplications, and use a different pair of
// vectors as the inputs for the sum of x and the sum of y.
inline void karatsuba_mul_128_uninterpolated_other_sum(poly128 x, poly128 y, poly128 x_for_sum,
                                                       poly128 y_for_sum, poly128* out)
{
    poly128 x0y0 = {clmul_block::clmul_ll(x.data, y.data)};
    poly128 x1y1 = {clmul_block::clmul_hh(x.data, y.data)};
    clmul_block x1_cat_y0 = clmul_block::mix_64(y_for_sum.data, x_for_sum.data);
    clmul_block xsum = x_for_sum.data ^ x1_cat_y0; // Result in low.
    clmul_block ysum = y_for_sum.data ^ x1_cat_y0; // Result in high.
    poly128 xsum_ysum = {clmul_block::clmul_lh(xsum, ysum)};

    out[0] = x0y0;
    out[1] = xsum_ysum;
    out[2] = x1y1;
}

// Karatsuba multiplication, but end right after the multiplications.
inline void karatsuba_mul_128_uninterpolated(poly128 x, poly128 y, poly128* out)
{
    karatsuba_mul_128_uninterpolated_other_sum(x, y, x, y, out);
}

// Karatsuba multiplication, but don't combine the 3 128-bit polynomials into a 256-bit polynomial.
inline void karatsuba_mul_128_uncombined(poly128 x, poly128 y, poly128* out)
{
    karatsuba_mul_128_uninterpolated(x, y, out);
    out[1] = out[0] + out[2] + out[1];
}

// Given a sequence of n (for n >= 2) 128-bit polynomials p_i, compute the sum of x^(64*i) p_i as a
// sequence of n / 2 + 1 128-bit polynomials.
inline void combine_poly128s(poly128* out, const poly128* in, size_t n)
{
    out[0] = in[0] + poly128{in[1].data.shift_left_64()};
    for (size_t i = 1; i < n / 2; ++i)
        out[i] = in[2 * i] + poly128{clmul_block::mix_64(in[2 * i + 1].data, in[2 * i - 1].data)};
    if (n % 2)
        out[n / 2] = in[n - 1] + poly128{in[n - 2].data.shift_right_64()};
    else
        out[n / 2] = {in[n - 1].data.shift_right_64()};
}

inline static clmul_block poly1_to_bit_mask(poly1 x) { return {_mm_set1_epi8(x.data)}; }

inline void poly_shift_left_1(clmul_block* x, size_t chunks)
{
    clmul_block low[8], high[8], high_shifted[8];
    for (size_t i = 0; i < chunks; ++i)
    {
        low[i] = {_mm_slli_epi32(x[i].data, 1)};
        high[i] = {_mm_srli_epi32(x[i].data, 31)};
        high_shifted[i] = {i > 0 ? _mm_alignr_epi8(high[i].data, high[i - 1].data, 12)
                                 : _mm_slli_si128(high[i].data, 4)};
    }

    for (size_t i = 0; i < chunks; ++i)
        x[i] = low[i] ^ high_shifted[i];
}

inline void poly_shift_left_8(clmul_block* out, const clmul_block* in, size_t chunks)
{
    for (size_t i = 0; i < chunks; ++i)
        out[i] = {i > 0 ? _mm_alignr_epi8(in[i].data, in[i - 1].data, 15)
                        : _mm_slli_si128(in[i].data, 1)};
}

template <size_t size> poly<size> poly_exp(poly<size> base, size_t power)
{
    poly<size> base_exp_pow2 = base;
    poly<size> base_exp = poly<size>::set_low32(1);
    bool first = true;
    for (size_t i = 1; i <= power; i <<= 1)
    {
        if (power & i)
        {
            if (first)
            {
                base_exp = base_exp_pow2;
                first = false;
            }
            else
            {
                base_exp = (base_exp * base_exp_pow2).template reduce_to<size>();
            }
        }
        if ((i << 1) <= power)
        {
            base_exp_pow2 = (base_exp_pow2 * base_exp_pow2).template reduce_to<size>();
        }
    }
    return base_exp;
}

} // namespace detail

} // namespace faest

#endif
