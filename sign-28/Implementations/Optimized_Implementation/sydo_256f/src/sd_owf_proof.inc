#ifndef SYDO_SD_OWF_PROOF_INC
#define SYDO_SD_OWF_PROOF_INC

#include <array>
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <tuple>
#include <utility>
#include <vector>

#include "quicksilver.hpp"
#include "sd_owf_proof.hpp"
#include "profile.hpp"

namespace sydo
{

inline poly1 mul_poly1(poly1 x, poly1 y)
{
    return poly1::set_all(static_cast<uint8_t>(x.data & y.data));
}

template <secpar S> inline poly_secpar<S> mul_poly1_polysecpar(poly1 x, const poly_secpar<S>& y)
{
    return x * y;
}

template <secpar S>
ALWAYS_INLINE poly_secpar<S> mul_poly1_mask_polysecpar(clmul_block mask, const poly_secpar<S>& y)
{
    if constexpr (S == secpar::s128)
    {
        return {mask & y.data};
    }
    else
    {
        poly_secpar<S> out;
        constexpr size_t limbs = sizeof(out.data) / sizeof(out.data[0]);
        for (size_t i = 0; i < limbs; ++i)
            out.data[i] = {mask & y.data[i].data};
        if constexpr (S == secpar::s160)
            out.clear_unused_bits();
        return out;
    }
}

template <secpar S>
ALWAYS_INLINE void add_mul_poly1_mask_polysecpar(poly_secpar<S>& out, clmul_block mask,
                                                 const poly_secpar<S>& y)
{
    if constexpr (S == secpar::s128)
    {
        out.data = out.data ^ (mask & y.data);
    }
    else
    {
        constexpr size_t limbs = sizeof(out.data) / sizeof(out.data[0]);
        for (size_t i = 0; i < limbs; ++i)
            out.data[i] = {out.data[i].data ^ (mask & y.data[i].data)};
        if constexpr (S == secpar::s160)
            out.clear_unused_bits();
    }
}

template <secpar S>
inline poly_secpar<S> mul_polysecpar_reduce(const poly_secpar<S>& x, const poly_secpar<S>& y)
{
    return (x * y).template reduce_to<secpar_to_bits(S)>();
}

template <secpar S>
inline poly_2secpar<S> mul_polysecpar(const poly_secpar<S>& x, const poly_secpar<S>& y)
{
    return x * y;
}

template <secpar S> inline poly_secpar<S> reduce_poly2secpar(const poly_2secpar<S>& x)
{
    return x.template reduce_to<secpar_to_bits(S)>();
}

template <secpar S> struct polysecpar_rhs_sum_type
{
    using type = poly128;
};

template <> struct polysecpar_rhs_sum_type<secpar::s512>
{
    using type = poly256;
};

template <secpar S> using polysecpar_rhs_sum_t = typename polysecpar_rhs_sum_type<S>::type;

template <secpar S> ALWAYS_INLINE polysecpar_rhs_sum_t<S> polysecpar_rhs_sum(const poly_secpar<S>&)
{
    return poly128::set_zero();
}

template <> ALWAYS_INLINE poly128 polysecpar_rhs_sum<secpar::s256>(const poly_secpar<secpar::s256>& x)
{
    return x.data[0] + x.data[1];
}

template <> ALWAYS_INLINE poly256 polysecpar_rhs_sum<secpar::s512>(const poly_secpar<secpar::s512>& x)
{
    return x.low_half() + x.high_half();
}

template <secpar S> struct polysecpar_dot_accumulator
{
    poly_2secpar<S> acc;

    polysecpar_dot_accumulator() : acc(poly_2secpar<S>::set_zero()) {}

    ALWAYS_INLINE void add(const poly_secpar<S>& x, const poly_secpar<S>& y)
    {
        acc += mul_polysecpar<S>(x, y);
    }

    ALWAYS_INLINE void add_with_rhs_sum(const poly_secpar<S>& x, const poly_secpar<S>& y,
                                        poly128 ysum)
    {
        (void)ysum;
        add(x, y);
    }

    ALWAYS_INLINE poly_2secpar<S> value() const { return acc; }
    ALWAYS_INLINE poly_secpar<S> reduced() const { return reduce_poly2secpar<S>(acc); }
};

template <> struct polysecpar_dot_accumulator<secpar::s128>
{
    std::array<poly128, 3> chunks;

    polysecpar_dot_accumulator()
    {
        for (auto& chunk : chunks)
            chunk = poly128::set_zero();
    }

    ALWAYS_INLINE void add(poly_secpar<secpar::s128> x, poly_secpar<secpar::s128> y)
    {
        poly128 xy[3];
        detail::karatsuba_mul_128_uncombined(x, y, &xy[0]);
        chunks[0] += xy[0];
        chunks[1] += xy[1];
        chunks[2] += xy[2];
    }

    ALWAYS_INLINE void add_with_rhs_sum(poly_secpar<secpar::s128> x,
                                        poly_secpar<secpar::s128> y, poly128)
    {
        add(x, y);
    }

    ALWAYS_INLINE poly_2secpar<secpar::s128> value() const
    {
        poly_2secpar<secpar::s128> out;
        detail::combine_poly128s(&out.data[0], chunks.data(), chunks.size());
        return out;
    }

    ALWAYS_INLINE poly_secpar<secpar::s128> reduced() const
    {
        poly128 d0 = chunks[0] + poly128{chunks[1].data.shift_left_64()};
        poly128 d1 = chunks[2] + poly128{chunks[1].data.shift_right_64()};
        const poly64 modulus = poly64::set_low32(gf128_modulus);
        const clmul_block reduced_192 = clmul_block::clmul_lh(modulus.data, d1.data);
        d0 = {d0.data ^ reduced_192.shift_left_64()};
        d1 = {d1.data ^ reduced_192.shift_right_64()};
        d0 = {d0.data ^ clmul_block::clmul_ll(modulus.data, d1.data)};
        return d0;
    }
};

template <> struct polysecpar_dot_accumulator<secpar::s160>
{
    std::array<poly128, 5> chunks;

    polysecpar_dot_accumulator()
    {
        for (auto& chunk : chunks)
            chunk = poly128::set_zero();
    }

    ALWAYS_INLINE void add(poly_secpar<secpar::s160> x, poly_secpar<secpar::s160> y)
    {
        const poly128 xlow_ylow = {clmul_block::clmul_ll(x.data[0].data, y.data[0].data)};
        const poly128 xhigh_yhigh = {clmul_block::clmul_ll(x.data[1].data, y.data[1].data)};

        const clmul_block x1_cat_y0_plus_y2 =
            clmul_block::mix_64(y.data[0].data ^ y.data[1].data, x.data[0].data);
        const clmul_block xsum = x.data[0].data ^ x.data[1].data ^ x1_cat_y0_plus_y2;
        const clmul_block ysum = y.data[0].data ^ x1_cat_y0_plus_y2;
        const poly128 xsum_ysum = {clmul_block::clmul_lh(xsum, ysum)};

        const poly128 xa = x.data[0] + poly128{x.data[1].data.broadcast_low64()};
        const poly128 ya = y.data[0] + poly128{y.data[1].data.broadcast_low64()};
        poly128 karatsuba_out[3];
        detail::karatsuba_mul_128_uninterpolated_other_sum(xa, ya, x.data[0], y.data[0],
                                                           &karatsuba_out[0]);
        const poly128 xya0 = karatsuba_out[0] + karatsuba_out[2];
        const poly128 xya1 = karatsuba_out[0] + karatsuba_out[1];
        const poly128 xya0_plus_xsum_ysum = xya0 + xsum_ysum;

        chunks[0] += xlow_ylow;
        chunks[1] += xya0_plus_xsum_ysum + xhigh_yhigh;
        chunks[2] += xya0_plus_xsum_ysum + xya1;
        chunks[3] += xlow_ylow + xsum_ysum + xya1;
        chunks[4] += xhigh_yhigh;
    }

    ALWAYS_INLINE void add_with_rhs_sum(poly_secpar<secpar::s160> x,
                                        poly_secpar<secpar::s160> y, poly128)
    {
        add(x, y);
    }

    ALWAYS_INLINE poly_2secpar<secpar::s160> value() const
    {
        poly_2secpar<secpar::s160> out;
        detail::combine_poly128s(&out.data[0], chunks.data(), chunks.size());
        return out;
    }

    ALWAYS_INLINE poly_secpar<secpar::s160> reduced() const
    {
        const poly128 d0 = chunks[0] + poly128{chunks[1].data.shift_left_64()};
        const poly128 d1 =
            chunks[2] + poly128{clmul_block::mix_64(chunks[3].data, chunks[1].data)};
        const poly128 d2 = chunks[4] + poly128{chunks[3].data.shift_right_64()};

        poly160 low;
        low.data[0] = d0;
        low.data[1] = {_mm_cvtsi32_si128(_mm_cvtsi128_si32(d1.data.data))};

        poly160 high;
        high.data[0] = {_mm_alignr_epi8(d2.data.data, d1.data.data, 4)};
        high.data[1] = {_mm_srli_si128(d2.data.data, 4)};

        return low + (poly64::set_low32(gf160_modulus) * high).template reduce_to<160>();
    }
};

template <> struct polysecpar_dot_accumulator<secpar::s192>
{
    std::array<poly128, 5> chunks;

    polysecpar_dot_accumulator()
    {
        for (auto& chunk : chunks)
            chunk = poly128::set_zero();
    }

    ALWAYS_INLINE void add(poly_secpar<secpar::s192> x, poly_secpar<secpar::s192> y)
    {
        const poly128 xlow_ylow = {clmul_block::clmul_ll(x.data[0].data, y.data[0].data)};
        const poly128 xhigh_yhigh = {clmul_block::clmul_ll(x.data[1].data, y.data[1].data)};

        const clmul_block x1_cat_y0_plus_y2 =
            clmul_block::mix_64(y.data[0].data ^ y.data[1].data, x.data[0].data);
        const clmul_block xsum = x.data[0].data ^ x.data[1].data ^ x1_cat_y0_plus_y2;
        const clmul_block ysum = y.data[0].data ^ x1_cat_y0_plus_y2;
        const poly128 xsum_ysum = {clmul_block::clmul_lh(xsum, ysum)};

        const poly128 xa = x.data[0] + poly128{x.data[1].data.broadcast_low64()};
        const poly128 ya = y.data[0] + poly128{y.data[1].data.broadcast_low64()};
        poly128 karatsuba_out[3];
        detail::karatsuba_mul_128_uninterpolated_other_sum(xa, ya, x.data[0], y.data[0],
                                                           &karatsuba_out[0]);
        const poly128 xya0 = karatsuba_out[0] + karatsuba_out[2];
        const poly128 xya1 = karatsuba_out[0] + karatsuba_out[1];
        const poly128 xya0_plus_xsum_ysum = xya0 + xsum_ysum;

        chunks[0] += xlow_ylow;
        chunks[1] += xya0_plus_xsum_ysum + xhigh_yhigh;
        chunks[2] += xya0_plus_xsum_ysum + xya1;
        chunks[3] += xlow_ylow + xsum_ysum + xya1;
        chunks[4] += xhigh_yhigh;
    }

    ALWAYS_INLINE void add_with_rhs_sum(poly_secpar<secpar::s192> x,
                                        poly_secpar<secpar::s192> y, poly128)
    {
        add(x, y);
    }

    ALWAYS_INLINE poly_2secpar<secpar::s192> value() const
    {
        poly_2secpar<secpar::s192> out;
        detail::combine_poly128s(&out.data[0], chunks.data(), chunks.size());
        return out;
    }

    ALWAYS_INLINE poly_secpar<secpar::s192> reduced() const
    {
        const poly128 d0 = chunks[0] + poly128{chunks[1].data.shift_left_64()};
        const poly128 d1 =
            chunks[2] + poly128{clmul_block::mix_64(chunks[3].data, chunks[1].data)};
        const poly128 d2 = chunks[4] + poly128{chunks[3].data.shift_right_64()};

        const poly64 modulus = poly64::set_low32(gf192_modulus);
        const clmul_block reduced_320 = clmul_block::clmul_lh(modulus.data, d2.data);
        const clmul_block reduced_256 = clmul_block::clmul_ll(modulus.data, d2.data);
        poly192 out;
        out.data[1] = {d1.data ^ reduced_320};
        out.data[0] = {d0.data ^ reduced_256.shift_left_64()};
        out.data[0] = {out.data[0].data ^ clmul_block::clmul_lh(modulus.data, out.data[1].data)};
        out.data[1] = {out.data[1].data ^ reduced_256.shift_right_64()};
        return out;
    }
};

template <> struct polysecpar_dot_accumulator<secpar::s256>
{
    std::array<poly128, 7> chunks;

    polysecpar_dot_accumulator()
    {
        for (auto& chunk : chunks)
            chunk = poly128::set_zero();
    }

    ALWAYS_INLINE void add(poly_secpar<secpar::s256> x, poly_secpar<secpar::s256> y)
    {
        poly128 x0y0[3], x1y1[3], xsum_ysum[3];
        detail::karatsuba_mul_128_uncombined(x.data[0], y.data[0], &x0y0[0]);
        detail::karatsuba_mul_128_uncombined(x.data[1], y.data[1], &x1y1[0]);

        const poly128 xsum = x.data[0] + x.data[1];
        const poly128 ysum = y.data[0] + y.data[1];
        detail::karatsuba_mul_128_uncombined(xsum, ysum, &xsum_ysum[0]);

        const poly128 cross = x0y0[2] + x1y1[0];
        chunks[0] += x0y0[0];
        chunks[1] += x0y0[1];
        chunks[2] += xsum_ysum[0] + x0y0[0] + cross;
        chunks[3] += xsum_ysum[1] + x0y0[1] + x1y1[1];
        chunks[4] += xsum_ysum[2] + x1y1[2] + cross;
        chunks[5] += x1y1[1];
        chunks[6] += x1y1[2];
    }

    ALWAYS_INLINE void add_with_rhs_sum(poly_secpar<secpar::s256> x,
                                        poly_secpar<secpar::s256> y, poly128 ysum)
    {
        poly128 x0y0[3], x1y1[3], xsum_ysum[3];
        detail::karatsuba_mul_128_uncombined(x.data[0], y.data[0], &x0y0[0]);
        detail::karatsuba_mul_128_uncombined(x.data[1], y.data[1], &x1y1[0]);

        const poly128 xsum = x.data[0] + x.data[1];
        detail::karatsuba_mul_128_uncombined(xsum, ysum, &xsum_ysum[0]);

        const poly128 cross = x0y0[2] + x1y1[0];
        chunks[0] += x0y0[0];
        chunks[1] += x0y0[1];
        chunks[2] += xsum_ysum[0] + x0y0[0] + cross;
        chunks[3] += xsum_ysum[1] + x0y0[1] + x1y1[1];
        chunks[4] += xsum_ysum[2] + x1y1[2] + cross;
        chunks[5] += x1y1[1];
        chunks[6] += x1y1[2];
    }

    ALWAYS_INLINE poly_2secpar<secpar::s256> value() const
    {
        poly_2secpar<secpar::s256> out;
        detail::combine_poly128s(&out.data[0], chunks.data(), chunks.size());
        return out;
    }

    ALWAYS_INLINE poly_secpar<secpar::s256> reduced() const
    {
        const poly128 d0 = chunks[0] + poly128{chunks[1].data.shift_left_64()};
        const poly128 d1 =
            chunks[2] + poly128{clmul_block::mix_64(chunks[3].data, chunks[1].data)};
        const poly128 d2 =
            chunks[4] + poly128{clmul_block::mix_64(chunks[5].data, chunks[3].data)};
        const poly128 d3 = chunks[6] + poly128{chunks[5].data.shift_right_64()};

        const poly64 modulus = poly64::set_low32(gf256_modulus);
        const poly128 xmod1 = {clmul_block::clmul_lh(modulus.data, d2.data)};
        const poly128 xmod2 = {clmul_block::clmul_ll(modulus.data, d3.data)};
        const poly128 xmod3 = {clmul_block::clmul_lh(modulus.data, d3.data)};

        poly128 c0 = d0 + poly128{xmod1.data.shift_left_64()};
        const poly128 c1 = d1 + xmod2 + poly128{clmul_block::mix_64(xmod3.data, xmod1.data)};
        const poly128 c2 = d2 + poly128{xmod3.data.shift_right_64()};
        c0 += poly128{clmul_block::clmul_ll(modulus.data, c2.data)};
        return {c0, c1};
    }
};

template <> struct polysecpar_dot_accumulator<secpar::s512>
{
    std::array<poly128, 15> chunks;

    polysecpar_dot_accumulator()
    {
        for (auto& chunk : chunks)
            chunk = poly128::set_zero();
    }
    explicit polysecpar_dot_accumulator(int) {}

    ALWAYS_INLINE void add(poly_secpar<secpar::s512> x, poly_secpar<secpar::s512> y)
    {
        add_product(x, y, true);
    }

    ALWAYS_INLINE void set(poly_secpar<secpar::s512> x, poly_secpar<secpar::s512> y)
    {
        add_product(x, y, false);
    }

    ALWAYS_INLINE void add_product(poly_secpar<secpar::s512> x, poly_secpar<secpar::s512> y,
                                   bool accumulate)
    {
        const poly256 x0 = x.low_half();
        const poly256 x1 = x.high_half();
        const poly256 xsum = x0 + x1;
        const poly256 y0 = y.low_half();
        const poly256 y1 = y.high_half();
        const poly256 ysum = y0 + y1;

        add_product_parts(x0, x1, xsum, y0, y1, ysum, accumulate);
    }

    ALWAYS_INLINE void add_product_parts(poly256 x0, poly256 x1, poly256 xsum, poly256 y0,
                                         poly256 y1, poly256 ysum, bool accumulate)
    {
        poly128 z0[7], z2[7], zsum[7];
        detail::karatsuba_mul_256_uncombined(x0, y0, &z0[0]);
        detail::karatsuba_mul_256_uncombined(x1, y1, &z2[0]);
        detail::karatsuba_mul_256_uncombined(xsum, ysum, &zsum[0]);

        const poly128 c0 = z0[0];
        const poly128 c1 = z0[1];
        const poly128 c2 = z0[2];
        const poly128 c3 = z0[3];
        const poly128 c4 = z0[4] + zsum[0] + z0[0] + z2[0];
        const poly128 c5 = z0[5] + zsum[1] + z0[1] + z2[1];
        const poly128 c6 = z0[6] + zsum[2] + z0[2] + z2[2];
        const poly128 c7 = zsum[3] + z0[3] + z2[3];
        const poly128 c8 = zsum[4] + z0[4] + z2[4] + z2[0];
        const poly128 c9 = zsum[5] + z0[5] + z2[5] + z2[1];
        const poly128 c10 = zsum[6] + z0[6] + z2[6] + z2[2];
        const poly128 c11 = z2[3];
        const poly128 c12 = z2[4];
        const poly128 c13 = z2[5];
        const poly128 c14 = z2[6];

        if (accumulate)
        {
            chunks[0] += c0;
            chunks[1] += c1;
            chunks[2] += c2;
            chunks[3] += c3;
            chunks[4] += c4;
            chunks[5] += c5;
            chunks[6] += c6;
            chunks[7] += c7;
            chunks[8] += c8;
            chunks[9] += c9;
            chunks[10] += c10;
            chunks[11] += c11;
            chunks[12] += c12;
            chunks[13] += c13;
            chunks[14] += c14;
        }
        else
        {
            chunks[0] = c0;
            chunks[1] = c1;
            chunks[2] = c2;
            chunks[3] = c3;
            chunks[4] = c4;
            chunks[5] = c5;
            chunks[6] = c6;
            chunks[7] = c7;
            chunks[8] = c8;
            chunks[9] = c9;
            chunks[10] = c10;
            chunks[11] = c11;
            chunks[12] = c12;
            chunks[13] = c13;
            chunks[14] = c14;
        }
    }

    ALWAYS_INLINE void add_with_rhs_sum(poly_secpar<secpar::s512> x,
                                        poly_secpar<secpar::s512> y, poly256 ysum)
    {
        const poly256 x0 = x.low_half();
        const poly256 x1 = x.high_half();
        const poly256 y0 = y.low_half();
        const poly256 y1 = y.high_half();

        poly128 z0[7], z2[7], zsum[7];
        detail::karatsuba_mul_256_uncombined(x0, y0, &z0[0]);
        detail::karatsuba_mul_256_uncombined(x1, y1, &z2[0]);
        detail::karatsuba_mul_256_uncombined(x0 + x1, ysum, &zsum[0]);

        chunks[0] += z0[0];
        chunks[1] += z0[1];
        chunks[2] += z0[2];
        chunks[3] += z0[3];
        chunks[4] += z0[4] + zsum[0] + z0[0] + z2[0];
        chunks[5] += z0[5] + zsum[1] + z0[1] + z2[1];
        chunks[6] += z0[6] + zsum[2] + z0[2] + z2[2];
        chunks[7] += zsum[3] + z0[3] + z2[3];
        chunks[8] += zsum[4] + z0[4] + z2[4] + z2[0];
        chunks[9] += zsum[5] + z0[5] + z2[5] + z2[1];
        chunks[10] += zsum[6] + z0[6] + z2[6] + z2[2];
        chunks[11] += z2[3];
        chunks[12] += z2[4];
        chunks[13] += z2[5];
        chunks[14] += z2[6];
    }

    ALWAYS_INLINE poly_2secpar<secpar::s512> value() const
    {
        poly_2secpar<secpar::s512> out;
        detail::combine_poly128s(&out.data[0], chunks.data(), chunks.size());
        return out;
    }

    ALWAYS_INLINE poly_secpar<secpar::s512> reduced() const
    {
        return detail::reduce_512_uncombined(chunks.data());
    }
};

template <>
ALWAYS_INLINE poly_secpar<secpar::s512>
mul_polysecpar_reduce<secpar::s512>(const poly_secpar<secpar::s512>& x,
                                    const poly_secpar<secpar::s512>& y)
{
    return detail::mul_reduce_512(x, y);
}

namespace sydo_detail
{

template <typename P>
inline bool public_vector_row_is_full(size_t row_idx)
{
    constexpr size_t bits_per_row = secpar_to_bits(P::secpar_v);
    constexpr size_t used_bits = P::rsd_codim;

    const size_t row_bit_start = row_idx * bits_per_row;
    if (row_bit_start >= used_bits)
        return false;

    const size_t row_used_bits = used_bits - row_bit_start;
    return row_used_bits >= bits_per_row;
}

template <typename P>
inline block_secpar<P::secpar_v> public_vector_row_mask(size_t row_idx)
{
    constexpr size_t bits_per_row = secpar_to_bits(P::secpar_v);
    constexpr size_t bytes_per_row = secpar_to_bytes(P::secpar_v);
    constexpr size_t used_bits = P::rsd_codim;

    const size_t row_bit_start = row_idx * bits_per_row;
    if (row_bit_start >= used_bits)
        return block_secpar<P::secpar_v>::set_zero();

    const size_t row_used_bits = used_bits - row_bit_start;
    if (row_used_bits >= bits_per_row)
        return block_secpar<P::secpar_v>::set_all_8(0xff);

    block_secpar<P::secpar_v> mask = block_secpar<P::secpar_v>::set_zero();
    uint8_t* bytes = reinterpret_cast<uint8_t*>(&mask);
    const size_t used_bytes = (row_used_bits + 7) / 8;
    memset(bytes, 0xff, used_bytes);
    if ((row_used_bits & 7u) != 0u)
    {
        const uint8_t keep_mask = static_cast<uint8_t>((1u << (row_used_bits & 7u)) - 1u);
        bytes[used_bytes - 1] &= keep_mask;
    }
    (void)bytes_per_row;
    return mask;
}

template <typename P>
inline block_secpar<P::secpar_v> mask_public_vector_row_bits(size_t row_idx,
                                                             block_secpar<P::secpar_v> v)
{
    if (public_vector_row_is_full<P>(row_idx))
        return v;
    return v & public_vector_row_mask<P>(row_idx);
}

template <typename P, size_t parallel_blocks = 8>
inline void sydo_expand_pubkey_prg_blocks_range(const typename P::prg_t::key_t& prg_key,
                                                  size_t start_block, size_t block_count,
                                                  typename P::prg_t::block_t* prg_blocks_out,
                                                  size_t prg_blocks_out_len)
{
    using prg_t = typename P::prg_t;
    using count_t = typename prg_t::count_t;
    constexpr size_t num_keys = 1;

    if (block_count > prg_blocks_out_len)
        abort();
    if (block_count == 0)
        return;

    typename prg_t::key_t key[num_keys] = {prg_key};
    typename prg_t::expanded_key_t expanded[num_keys];
    typename prg_t::tweak_t tweaks[num_keys] = {0};
    typename prg_t::iv_t iv = prg_t::iv_t::set_zero();
    count_t counters[num_keys] = {static_cast<count_t>(start_block)};

    const size_t full_blocks = (block_count / parallel_blocks) * parallel_blocks;
    if (full_blocks != 0)
    {
        prg_t::template init<num_keys, parallel_blocks>(key, expanded, iv, tweaks, counters,
                                                        prg_blocks_out);
    }
    else
    {
        typename prg_t::block_t tail[parallel_blocks];
        prg_t::template init<num_keys, parallel_blocks>(key, expanded, iv, tweaks, counters,
                                                        tail);
        memcpy(prg_blocks_out, tail, block_count * sizeof(typename prg_t::block_t));
        return;
    }

    for (size_t i = parallel_blocks; i < full_blocks; i += parallel_blocks)
    {
        counters[0] = static_cast<count_t>(start_block + i);
        prg_t::template gen<num_keys, parallel_blocks>(expanded, iv, tweaks, counters,
                                                       prg_blocks_out + i);
    }

    if (full_blocks != block_count)
    {
        typename prg_t::block_t tail[parallel_blocks];
        counters[0] = static_cast<count_t>(start_block + full_blocks);
        prg_t::template gen<num_keys, parallel_blocks>(expanded, iv, tweaks, counters, tail);
        memcpy(prg_blocks_out + full_blocks, tail,
               (block_count - full_blocks) * sizeof(typename prg_t::block_t));
    }
}

template <typename P, size_t parallel_blocks = 8>
inline void sydo_expand_pubkey_prg_blocks_prefix(const typename P::prg_t::key_t& prg_key,
                                                   size_t rows_to_expand,
                                                   typename P::prg_t::block_t* prg_blocks_out,
                                                   size_t prg_blocks_out_len)
{
    constexpr size_t blocks_per_elem = h_prg_blocks_per_elem<P>;
    const size_t total_blocks_sz = rows_to_expand * P::rsd_n * blocks_per_elem;
    sydo_expand_pubkey_prg_blocks_range<P, parallel_blocks>(prg_key, 0, total_blocks_sz,
                                                              prg_blocks_out,
                                                              prg_blocks_out_len);
}

template <typename P, bool verifier>
void sydo_fold_expanded_h_rows(
    block_secpar<P::secpar_v>* folded_row_out, size_t folded_row_len,
    const typename P::prg_t::key_t& seed_pk,
    const quicksilver_state<P::secpar_v, verifier, P::OWF_CONSTS::QS_DEGREE,
                            P::quicksilver_term_v>* state)
{
    constexpr secpar S = P::secpar_v;
    using field_t = poly_secpar<S>;
    using h_block_t = typename P::prg_t::block_t;

    constexpr size_t rows = SYDO_PUBLIC_KEY_VECTOR_ELEMENTS<P>;
    constexpr size_t cols = P::rsd_n;
    constexpr size_t blocks_per_elem = h_prg_blocks_per_elem<P>;
    constexpr size_t base_qs_challenge_bytes = (3 * secpar_to_bits(S) + 64) / 8;
    constexpr size_t total_blocks_sz = rows * cols * blocks_per_elem;
    field_t alpha[rows];
    for (size_t row = 0; row < rows; ++row)
    {
        alpha[row] = field_t::load_dup(state->challenge_bytes + base_qs_challenge_bytes +
                                       row * secpar_to_bytes(S));
    }

    if (folded_row_len < cols)
        abort();
    static thread_local h_block_t prg_blocks[total_blocks_sz];
    sydo_expand_pubkey_prg_blocks_prefix<P, 8>(seed_pk, rows, prg_blocks, total_blocks_sz);

    for (size_t col = 0; col < cols; ++col)
    {
        if constexpr (S == secpar::s512)
        {
            polysecpar_dot_accumulator<S> acc(0);
            for (size_t row = 0; row < rows; ++row)
            {
                const size_t elem_idx = row * cols + col;
                const size_t base = elem_idx * blocks_per_elem;
                block_secpar<P::secpar_v> h_elem =
                    compose_secpar_block_from_parts<P>(prg_blocks + base);
                h_elem = mask_public_vector_row_bits<P>(row, h_elem);
                const field_t h = field_t::load_dup(&h_elem);
                if (row == 0)
                    acc.set(alpha[row], h);
                else
                    acc.add(alpha[row], h);
            }
            const field_t folded = acc.reduced();
            folded.store1(folded_row_out + col);
        }
        else
        {
            polysecpar_dot_accumulator<S> acc;
            for (size_t row = 0; row < rows; ++row)
            {
                const size_t elem_idx = row * cols + col;
                const size_t base = elem_idx * blocks_per_elem;
                block_secpar<P::secpar_v> h_elem =
                    compose_secpar_block_from_parts<P>(prg_blocks + base);
                h_elem = mask_public_vector_row_bits<P>(row, h_elem);
                const field_t h = field_t::load_dup(&h_elem);
                acc.add(alpha[row], h);
            }
            const field_t folded = acc.reduced();
            folded.store1(folded_row_out + col);
        }
    }
}

template <typename P, bool verifier>
void sydo_fold_expanded_h_rows_field(
    poly_secpar<P::secpar_v>* folded_row_out, size_t folded_row_len,
    const typename P::prg_t::key_t& seed_pk,
    const quicksilver_state<P::secpar_v, verifier, P::OWF_CONSTS::QS_DEGREE,
                            P::quicksilver_term_v>* state)
{
    constexpr secpar S = P::secpar_v;
    using field_t = poly_secpar<S>;
    using h_block_t = typename P::prg_t::block_t;

    constexpr size_t rows = SYDO_PUBLIC_KEY_VECTOR_ELEMENTS<P>;
    constexpr size_t cols = P::rsd_n;
    constexpr size_t blocks_per_elem = h_prg_blocks_per_elem<P>;
    constexpr size_t base_qs_challenge_bytes = (3 * secpar_to_bits(S) + 64) / 8;
    field_t alpha[rows];
    for (size_t row = 0; row < rows; ++row)
    {
        alpha[row] = field_t::load_dup(state->challenge_bytes + base_qs_challenge_bytes +
                                       row * secpar_to_bytes(S));
    }

    if (folded_row_len < cols)
        abort();

    constexpr size_t total_blocks_sz = rows * cols * blocks_per_elem;
    static thread_local h_block_t prg_blocks[total_blocks_sz];
    sydo_expand_pubkey_prg_blocks_prefix<P, 8>(seed_pk, rows, prg_blocks, total_blocks_sz);

    for (size_t col = 0; col < cols; ++col)
    {
        if constexpr (S == secpar::s512)
        {
            polysecpar_dot_accumulator<S> acc(0);
            for (size_t row = 0; row < rows; ++row)
            {
                const size_t elem_idx = row * cols + col;
                const size_t base = elem_idx * blocks_per_elem;
                block_secpar<P::secpar_v> h_elem =
                    compose_secpar_block_from_parts<P>(prg_blocks + base);
                h_elem = mask_public_vector_row_bits<P>(row, h_elem);
                const field_t h = field_t::load_dup(&h_elem);
                if (row == 0)
                    acc.set(alpha[row], h);
                else
                    acc.add(alpha[row], h);
            }
            folded_row_out[col] = acc.reduced();
        }
        else
        {
            polysecpar_dot_accumulator<S> acc;
            for (size_t row = 0; row < rows; ++row)
            {
                const size_t elem_idx = row * cols + col;
                const size_t base = elem_idx * blocks_per_elem;
                block_secpar<P::secpar_v> h_elem =
                    compose_secpar_block_from_parts<P>(prg_blocks + base);
                h_elem = mask_public_vector_row_bits<P>(row, h_elem);
                const field_t h = field_t::load_dup(&h_elem);
                acc.add(alpha[row], h);
            }
            folded_row_out[col] = acc.reduced();
        }
    }
}

template <typename P, bool verifier>
inline poly_secpar<P::secpar_v> sydo_fold_public_y(
    const sydo_public_key<P>* pk,
    const quicksilver_state<P::secpar_v, verifier, P::OWF_CONSTS::QS_DEGREE,
                            P::quicksilver_term_v>* state)
{
    constexpr secpar S = P::secpar_v;
    using field_t = poly_secpar<S>;
    using wide_field_t = poly_2secpar<S>;

    constexpr size_t rows = SYDO_PUBLIC_KEY_VECTOR_ELEMENTS<P>;
    static_assert(rows >= 1, "Public y vector must be non-empty.");

    constexpr size_t base_qs_challenge_bytes = (3 * secpar_to_bits(S) + 64) / 8;
    field_t alpha[rows];
    for (size_t row = 0; row < rows; ++row)
    {
        alpha[row] = field_t::load_dup(state->challenge_bytes + base_qs_challenge_bytes +
                                       row * secpar_to_bytes(S));
    }
    field_t y_rows[rows];
    for (size_t row = 0; row < rows; ++row)
        y_rows[row] = field_t::load_dup(&pk->y[row]);

    wide_field_t acc = wide_field_t::set_zero();
    for (size_t row = 0; row < rows; ++row)
        acc += mul_polysecpar<S>(alpha[row], y_rows[row]);
    return reduce_poly2secpar<S>(acc);
}

} // namespace sydo_detail

constexpr size_t sydo_owf_choose_runtime(size_t n, size_t r)
{
    if (r > n)
        return 0;
    if (r == 0 || r == n)
        return 1;
    const size_t k = (r < (n - r)) ? r : (n - r);
    size_t result = 1;
    for (size_t i = 1; i <= k; ++i)
        result = (result * (n - k + i)) / i;
    return result;
}

constexpr size_t sydo_owf_ball_count_runtime(size_t n, size_t r)
{
    size_t total = 0;
    for (size_t w = 0; w <= r; ++w)
        total += sydo_owf_choose_runtime(n, w);
    return total;
}

template <size_t n, size_t k> constexpr auto sydo_owf_make_ranked_subsets()
{
    constexpr size_t count = sydo_owf_choose_runtime(n, k);
    std::array<std::array<size_t, k>, count> out{};

    for (size_t rank = 0; rank < count; ++rank)
    {
        size_t left = k;
        size_t rem_rank = rank;
        size_t out_pos = 0;
        for (size_t pos = 0; pos < n; ++pos)
        {
            if (left == 0)
                break;
            const size_t skip_count = sydo_owf_choose_runtime(n - pos - 1, left);
            const bool take = (rem_rank >= skip_count);
            if (take)
            {
                rem_rank -= skip_count;
                out[rank][out_pos] = pos;
                ++out_pos;
                --left;
            }
        }
    }
    return out;
}

template <size_t n, size_t d> constexpr auto sydo_owf_make_ball_masks()
{
    constexpr size_t count = sydo_owf_ball_count_runtime(n, d);
    std::array<size_t, count> out{};
    size_t idx = 0;

    out[idx++] = 0;
    if constexpr (d >= 1)
    {
        constexpr auto choose1 = sydo_owf_make_ranked_subsets<n, 1>();
        for (size_t r = 0; r < choose1.size(); ++r)
            out[idx++] = size_t{1} << choose1[r][0];
    }
    if constexpr (d >= 2)
    {
        constexpr auto choose2 = sydo_owf_make_ranked_subsets<n, 2>();
        for (size_t r = 0; r < choose2.size(); ++r)
            out[idx++] = (size_t{1} << choose2[r][0]) | (size_t{1} << choose2[r][1]);
    }
    if constexpr (d >= 3)
    {
        constexpr auto choose3 = sydo_owf_make_ranked_subsets<n, 3>();
        for (size_t r = 0; r < choose3.size(); ++r)
            out[idx++] = (size_t{1} << choose3[r][0]) | (size_t{1} << choose3[r][1]) |
                         (size_t{1} << choose3[r][2]);
    }
    return out;
}

template <size_t n, size_t d> constexpr auto sydo_owf_make_ball_rank_by_mask()
{
    constexpr size_t table_size = size_t{1} << n;
    constexpr auto masks = sydo_owf_make_ball_masks<n, d>();
    std::array<size_t, table_size> out{};
    for (size_t i = 0; i < table_size; ++i)
        out[i] = static_cast<size_t>(-1);
    for (size_t rank = 0; rank < masks.size(); ++rank)
        out[masks[rank]] = rank;
    return out;
}

template <size_t n, size_t d, size_t k> constexpr auto sydo_owf_make_ball_ranks_by_size()
{
    constexpr size_t count = sydo_owf_choose_runtime(n, k);
    constexpr auto subsets = sydo_owf_make_ranked_subsets<n, k>();
    constexpr auto rank_by_mask = sydo_owf_make_ball_rank_by_mask<n, d>();
    std::array<size_t, count> out{};
    for (size_t idx = 0; idx < count; ++idx)
    {
        size_t mask = 0;
        for (size_t i = 0; i < k; ++i)
            mask |= size_t{1} << subsets[idx][i];
        out[idx] = rank_by_mask[mask];
    }
    return out;
}

template <size_t n, size_t d> constexpr auto sydo_owf_make_single_ball_rank_by_index()
{
    constexpr auto rank_by_mask = sydo_owf_make_ball_rank_by_mask<n, d>();
    std::array<size_t, n> out{};
    for (size_t i = 0; i < n; ++i)
        out[i] = rank_by_mask[size_t{1} << i];
    return out;
}

constexpr size_t sydo_owf_mask_weight(size_t mask)
{
    size_t weight = 0;
    while (mask != 0)
    {
        weight += mask & 1u;
        mask >>= 1;
    }
    return weight;
}

template <size_t n> constexpr auto sydo_owf_make_pair_rank()
{
    std::array<std::array<size_t, n>, n> pair_rank{};
    for (size_t i = 0; i < n; ++i)
        for (size_t j = 0; j < n; ++j)
            pair_rank[i][j] = static_cast<size_t>(-1);

    constexpr auto choose2 = sydo_owf_make_ranked_subsets<n, 2>();
    for (size_t idx = 0; idx < choose2.size(); ++idx)
    {
        const size_t i = choose2[idx][0];
        const size_t j = choose2[idx][1];
        pair_rank[i][j] = idx;
        pair_rank[j][i] = idx;
    }
    return pair_rank;
}

template <size_t n> constexpr auto sydo_owf_make_triple_rank()
{
    std::array<std::array<std::array<size_t, n>, n>, n> triple_rank{};
    for (size_t i = 0; i < n; ++i)
        for (size_t j = 0; j < n; ++j)
            for (size_t k = 0; k < n; ++k)
                triple_rank[i][j][k] = static_cast<size_t>(-1);

    constexpr auto choose3 = sydo_owf_make_ranked_subsets<n, 3>();
    for (size_t idx = 0; idx < choose3.size(); ++idx)
    {
        const size_t a = choose3[idx][0];
        const size_t b = choose3[idx][1];
        const size_t c = choose3[idx][2];
        triple_rank[a][b][c] = idx;
        triple_rank[a][c][b] = idx;
        triple_rank[b][a][c] = idx;
        triple_rank[b][c][a] = idx;
        triple_rank[c][a][b] = idx;
        triple_rank[c][b][a] = idx;
    }
    return triple_rank;
}

template <size_t n> constexpr auto sydo_owf_make_choose3_pair_ranks()
{
    constexpr size_t count = sydo_owf_choose_runtime(n, 3);
    constexpr auto choose3 = sydo_owf_make_ranked_subsets<n, 3>();
    constexpr auto pair_rank = sydo_owf_make_pair_rank<n>();

    std::array<std::array<size_t, 3>, count> out{};
    for (size_t idx = 0; idx < count; ++idx)
    {
        const size_t i = choose3[idx][0];
        const size_t j = choose3[idx][1];
        const size_t k = choose3[idx][2];
        out[idx][0] = pair_rank[i][j];
        out[idx][1] = pair_rank[i][k];
        out[idx][2] = pair_rank[j][k];
    }
    return out;
}

template <size_t n> constexpr auto sydo_owf_make_choose4_pair_ranks()
{
    constexpr size_t count = sydo_owf_choose_runtime(n, 4);
    constexpr auto choose4 = sydo_owf_make_ranked_subsets<n, 4>();
    constexpr auto pair_rank = sydo_owf_make_pair_rank<n>();

    std::array<std::array<size_t, 6>, count> out{};
    for (size_t idx = 0; idx < count; ++idx)
    {
        const size_t i = choose4[idx][0];
        const size_t j = choose4[idx][1];
        const size_t k = choose4[idx][2];
        const size_t l = choose4[idx][3];
        out[idx][0] = pair_rank[i][j];
        out[idx][1] = pair_rank[i][k];
        out[idx][2] = pair_rank[i][l];
        out[idx][3] = pair_rank[j][k];
        out[idx][4] = pair_rank[j][l];
        out[idx][5] = pair_rank[k][l];
    }
    return out;
}

template <size_t n> constexpr auto sydo_owf_make_choose4_triple_ranks()
{
    constexpr size_t count = sydo_owf_choose_runtime(n, 4);
    constexpr auto choose4 = sydo_owf_make_ranked_subsets<n, 4>();
    constexpr auto triple_rank = sydo_owf_make_triple_rank<n>();

    std::array<std::array<size_t, 4>, count> out{};
    for (size_t idx = 0; idx < count; ++idx)
    {
        const size_t i = choose4[idx][0];
        const size_t j = choose4[idx][1];
        const size_t k = choose4[idx][2];
        const size_t l = choose4[idx][3];
        out[idx][0] = triple_rank[i][j][k];
        out[idx][1] = triple_rank[i][j][l];
        out[idx][2] = triple_rank[i][k][l];
        out[idx][3] = triple_rank[j][k][l];
    }
    return out;
}

template <typename P> constexpr auto sydo_owf_block_offsets()
{
    std::array<size_t, P::num_var_blocks> offsets{};
    size_t off = 0;
    for (size_t b = 0; b < P::num_var_blocks; ++b)
    {
        offsets[b] = off;
        off += P::block_sizes[b];
    }
    return offsets;
}

template <typename P> constexpr size_t sydo_owf_sum_block_sizes()
{
    size_t total = 0;
    for (size_t b = 0; b < P::num_var_blocks; ++b)
        total += P::block_sizes[b];
    return total;
}

template <typename P> constexpr size_t sydo_owf_total_weight()
{
    size_t total = 0;
    for (size_t b = 0; b < P::num_var_blocks; ++b)
        total += P::block_wts[b];
    return total;
}

template <typename P> constexpr size_t sydo_owf_block_domain_count(size_t block_idx)
{
    if constexpr (P::elementary_encoding_v == elementary_encoding::hamming_sphere)
        return sydo_owf_choose_runtime(P::block_sizes[block_idx], P::block_wts[block_idx]);
    else
        return sydo_owf_ball_count_runtime(P::block_sizes[block_idx], P::block_wts[block_idx]);
}

template <typename P> constexpr size_t sydo_owf_terms_per_subvector()
{
    size_t total = 1;
    for (size_t b = 0; b < P::num_var_blocks; ++b)
        total *= sydo_owf_block_domain_count<P>(b);
    return total;
}

template <typename P, typename field_t>
inline const field_t* sydo_owf_padded_h_subvector(const field_t* folded_h_row,
                                                    size_t folded_h_row_len, size_t subvec_i,
                                                    field_t* padded)
{
    constexpr size_t active_terms = P::elementary_vector_len;
    constexpr size_t domain_terms = sydo_owf_terms_per_subvector<P>();
    static_assert(active_terms <= domain_terms,
                  "Actual elementary-vector segment length must fit in the block domain.");

    const size_t h_base = subvec_i * active_terms;
    SYDO_ASSERT(h_base + active_terms <= folded_h_row_len);
    if constexpr (active_terms == domain_terms)
    {
        (void)padded;
        return folded_h_row + h_base;
    }
    else
    {
        for (size_t i = 0; i < active_terms; ++i)
            padded[i] = folded_h_row[h_base + i];
        for (size_t i = active_terms; i < domain_terms; ++i)
            padded[i] = field_t::set_zero();
        return padded;
    }
}

template <typename P> constexpr bool sydo_owf_params_in_supported_range()
{
    if constexpr (P::num_var_blocks < 2 || P::num_var_blocks > 4)
        return false;
    for (size_t b = 0; b < P::num_var_blocks; ++b)
    {
        const size_t n_b = P::block_sizes[b];
        const size_t w_b = P::block_wts[b];
        if (w_b < 1 || w_b > 3)
            return false;
        if (n_b < 2 * w_b)
            return false;
    }
    return true;
}

template <typename P, size_t block_idx> struct sydo_owf_block_meta
{
    constexpr static size_t block_size = P::block_sizes[block_idx];
    constexpr static size_t weight = P::block_wts[block_idx];
    static_assert(weight >= 1 && weight <= 3, "Supported weight range is [1,3].");
    static_assert(block_size >= 2 * weight, "Require n_b >= 2 * w_b.");

    constexpr static size_t choose_w_count = sydo_owf_choose_runtime(block_size, weight);
    constexpr static size_t choose_w1_count = sydo_owf_choose_runtime(block_size, weight + 1);
    constexpr static size_t choose2_count = sydo_owf_choose_runtime(block_size, 2);
    constexpr static size_t choose3_count = sydo_owf_choose_runtime(block_size, 3);
    constexpr static size_t domain_count =
        (P::elementary_encoding_v == elementary_encoding::hamming_sphere)
            ? choose_w_count
            : sydo_owf_ball_count_runtime(block_size, weight);

    constexpr static auto choose_w_subsets = sydo_owf_make_ranked_subsets<block_size, weight>();
    constexpr static auto choose_w1_subsets =
        sydo_owf_make_ranked_subsets<block_size, weight + 1>();
    constexpr static auto choose2_subsets = sydo_owf_make_ranked_subsets<block_size, 2>();
    constexpr static auto choose3_subsets = sydo_owf_make_ranked_subsets<block_size, 3>();
    constexpr static auto ball_masks = sydo_owf_make_ball_masks<block_size, weight>();
    constexpr static auto ball_rank_by_mask =
        sydo_owf_make_ball_rank_by_mask<block_size, weight>();
    constexpr static auto single_ball_rank_by_index =
        sydo_owf_make_single_ball_rank_by_index<block_size, weight>();
    constexpr static auto pair_ball_ranks =
        sydo_owf_make_ball_ranks_by_size<block_size, weight, 2>();
    constexpr static auto triple_ball_ranks =
        sydo_owf_make_ball_ranks_by_size<block_size, weight, 3>();
    constexpr static auto pair_rank = sydo_owf_make_pair_rank<block_size>();
    constexpr static auto triple_rank = sydo_owf_make_triple_rank<block_size>();
    constexpr static auto choose3_pair_ranks = sydo_owf_make_choose3_pair_ranks<block_size>();
    constexpr static auto choose4_pair_ranks = sydo_owf_make_choose4_pair_ranks<block_size>();
    constexpr static auto choose4_triple_ranks = sydo_owf_make_choose4_triple_ranks<block_size>();
};

template <typename P, typename QS, size_t block_idx, bool is_verifier = QS::is_verifier>
struct sydo_owf_block_cache_impl;

template <typename P, typename QS, size_t block_idx>
struct sydo_owf_block_cache_impl<P, QS, block_idx, false>
{
    using meta = sydo_owf_block_meta<P, block_idx>;
    std::array<poly_secpar<P::secpar_v>, meta::block_size> c_values;
    std::array<poly1, meta::block_size> a_values;
    std::array<poly_secpar<P::secpar_v>, meta::choose2_count> t2_values;
    std::array<poly1, meta::choose2_count> u2_values;
    std::array<poly_secpar<P::secpar_v>, (meta::weight == 3) ? meta::choose3_count : 0> t3_values;
    std::array<poly1, (meta::weight == 3) ? meta::choose3_count : 0> u3_values;
};

template <typename P, typename QS, size_t block_idx>
struct sydo_owf_block_cache_impl<P, QS, block_idx, true>
{
    using meta = sydo_owf_block_meta<P, block_idx>;
    std::array<quicksilver_gf2<QS>, meta::block_size> bits;
    std::array<quicksilver_gf2<QS, 2>, meta::choose2_count> pair_products;
    std::array<quicksilver_gf2<QS, 3>, (meta::weight == 3) ? meta::choose3_count : 0>
        triple_products;
};

template <typename P, typename QS, size_t block_idx>
using sydo_owf_block_cache = sydo_owf_block_cache_impl<P, QS, block_idx, QS::is_verifier>;

template <typename P, typename QS, typename Seq> struct sydo_owf_block_cache_tuple_impl;

template <typename P, typename QS, size_t... block_idx>
struct sydo_owf_block_cache_tuple_impl<P, QS, std::index_sequence<block_idx...>>
{
    using type = std::tuple<sydo_owf_block_cache<P, QS, block_idx>...>;
};

template <typename P, typename QS>
using sydo_owf_block_cache_tuple =
    typename sydo_owf_block_cache_tuple_impl<P, QS,
                                               std::make_index_sequence<P::num_var_blocks>>::type;

template <typename P, typename QS, size_t block_idx>
inline void sydo_owf_load_block_cache(QS* state, size_t subvector_offset_bits,
                                        sydo_owf_block_cache<P, QS, block_idx>* out)
{
    using meta = sydo_owf_block_meta<P, block_idx>;
    constexpr auto block_offsets = sydo_owf_block_offsets<P>();
    constexpr size_t load_stage =
        block_idx < 4 ? profile::OWF_LOAD_BLOCK0 + block_idx : profile::STAGE_COUNT;
    profile::scope timer(load_stage);

    const size_t block_base = subvector_offset_bits + block_offsets[block_idx];
    for (size_t local_idx = 0; local_idx < meta::block_size; ++local_idx)
    {
        if constexpr (!QS::is_verifier)
        {
            const auto bit = state->get_witness_bit(block_base + local_idx);
            out->c_values[local_idx] = bit.mac.coeffs[0];
            out->a_values[local_idx] = bit.value();
        }
        else
        {
            out->bits[local_idx] = state->get_witness_bit(block_base + local_idx);
        }
    }

    for (size_t idx = 0; idx < meta::choose2_count; ++idx)
    {
        const size_t i = meta::choose2_subsets[idx][0];
        const size_t j = meta::choose2_subsets[idx][1];
        if constexpr (!QS::is_verifier)
        {
            const auto t2 = mul_polysecpar_reduce<P::secpar_v>(out->c_values[i], out->c_values[j]);
            const auto u2 = mul_poly1(out->a_values[i], out->a_values[j]);
            out->t2_values[idx] = t2;
            out->u2_values[idx] = u2;
        }
        else
        {
            quicksilver_gf2<QS, 2> pair(0, out->bits[i]);
            pair.mac = mul_polysecpar_reduce<P::secpar_v>(out->bits[i].mac, out->bits[j].mac);
            pair.delta_powers = out->bits[i].delta_powers;
            out->pair_products[idx] = pair;
        }
    }

    if constexpr (meta::weight == 3)
    {
        for (size_t idx = 0; idx < meta::choose3_count; ++idx)
        {
            const size_t i = meta::choose3_subsets[idx][0];
            const size_t k = meta::choose3_subsets[idx][2];
            const size_t ij = meta::choose3_pair_ranks[idx][0];
            if constexpr (!QS::is_verifier)
            {
                const auto t3 =
                    mul_polysecpar_reduce<P::secpar_v>(out->t2_values[ij], out->c_values[k]);
                const auto u3 = mul_poly1(out->u2_values[ij], out->a_values[k]);
                out->t3_values[idx] = t3;
                out->u3_values[idx] = u3;
            }
            else
            {
                quicksilver_gf2<QS, 3> triple(0, out->bits[i]);
                triple.mac = mul_polysecpar_reduce<P::secpar_v>(out->pair_products[ij].mac,
                                                                out->bits[k].mac);
                triple.delta_powers = out->bits[i].delta_powers;
                out->triple_products[idx] = triple;
            }
        }
    }
}

template <typename P, typename QS, size_t block_idx>
inline quicksilver_gf2<QS, sydo_owf_block_meta<P, block_idx>::weight>
sydo_owf_choose_w_product(const sydo_owf_block_cache<P, QS, block_idx>& cache,
                            size_t subset_idx)
{
    using meta = sydo_owf_block_meta<P, block_idx>;
    if constexpr (meta::weight == 1)
    {
        const size_t i = meta::choose_w_subsets[subset_idx][0];
        return cache.bits[i];
    }
    else if constexpr (meta::weight == 2)
    {
        return cache.pair_products[subset_idx];
    }
    else
    {
        return cache.triple_products[subset_idx];
    }
}

template <typename P, typename QS, size_t block_idx>
inline const poly_secpar<P::secpar_v>&
sydo_owf_choose_w_eval_verifier(const sydo_owf_block_cache<P, QS, block_idx>& cache,
                                  size_t subset_idx)
{
    static_assert(QS::is_verifier, "Verifier-only helper.");
    using meta = sydo_owf_block_meta<P, block_idx>;
    if constexpr (meta::weight == 1)
    {
        const size_t i = meta::choose_w_subsets[subset_idx][0];
        return cache.bits[i].mac;
    }
    else if constexpr (meta::weight == 2)
    {
        return cache.pair_products[subset_idx].mac;
    }
    else
    {
        return cache.triple_products[subset_idx].mac;
    }
}

template <typename P, typename QS, size_t block_idx>
inline quicksilver_gf2<QS, sydo_owf_block_meta<P, block_idx>::weight + 1>
sydo_owf_choose_w1_product(const sydo_owf_block_cache<P, QS, block_idx>& cache,
                             size_t subset_idx)
{
    static_assert(QS::is_verifier, "Verifier-only helper.");
    using meta = sydo_owf_block_meta<P, block_idx>;
    if constexpr (meta::weight == 1)
    {
        return cache.pair_products[subset_idx];
    }
    else if constexpr (meta::weight == 2)
    {
        const auto subset = meta::choose_w1_subsets[subset_idx];
        const size_t ij = meta::choose3_pair_ranks[subset_idx][0];
        quicksilver_gf2<QS, 3> triple(0, cache.bits[subset[0]]);
        triple.mac = mul_polysecpar_reduce<P::secpar_v>(cache.pair_products[ij].mac,
                                                        cache.bits[subset[2]].mac);
        triple.delta_powers = cache.bits[subset[0]].delta_powers;
        return triple;
    }
    else
    {
        const auto subset = meta::choose_w1_subsets[subset_idx];
        const auto pair_ranks = meta::choose4_pair_ranks[subset_idx];
        const size_t ij = pair_ranks[0];
        const size_t kl = pair_ranks[5];
        quicksilver_gf2<QS, 4> quad(0, cache.bits[subset[0]]);
        quad.mac = mul_polysecpar_reduce<P::secpar_v>(cache.pair_products[ij].mac,
                                                      cache.pair_products[kl].mac);
        quad.delta_powers = cache.bits[subset[0]].delta_powers;
        return quad;
    }
}

template <typename QS, size_t deg>
inline void sydo_owf_set_prover_polynomial(
    quicksilver_gfsecpar<QS, deg>* out,
    const std::array<poly_secpar<QS::secpar_v>, deg>& leading_coeffs,
    poly_secpar<QS::secpar_v> leading_value)
{
    if constexpr (QS::term_v == quicksilver_term::constant)
    {
        out->value() = leading_value;
        for (size_t i = 0; i < deg; ++i)
            out->mac.coeffs[i + 1] = leading_coeffs[deg - 1 - i];
    }
    else
    {
        for (size_t i = 0; i < deg; ++i)
            out->mac.coeffs[i] = leading_coeffs[i];
        out->value() = leading_value;
    }
}

template <typename P, typename QS, size_t block_idx>
inline void sydo_owf_fill_choose_w1_constraint_prover(
    const sydo_owf_block_cache<P, QS, block_idx>& cache, size_t subset_idx,
    quicksilver_gfsecpar<QS, sydo_owf_block_meta<P, block_idx>::weight + 1>* out)
{
    static_assert(!QS::is_verifier, "Prover-only helper.");
    using meta = sydo_owf_block_meta<P, block_idx>;
    using field_t = poly_secpar<P::secpar_v>;

    if constexpr (meta::weight == 1)
    {
        const auto& subset = meta::choose_w1_subsets[subset_idx];
        const size_t i = subset[0];
        const size_t j = subset[1];
        const size_t ij = subset_idx;
        field_t c1 = field_t::set_zero();
        add_mul_poly1_mask_polysecpar<P::secpar_v>(
            c1, detail::poly1_to_bit_mask(cache.a_values[i]), cache.c_values[j]);
        add_mul_poly1_mask_polysecpar<P::secpar_v>(
            c1, detail::poly1_to_bit_mask(cache.a_values[j]), cache.c_values[i]);
        sydo_owf_set_prover_polynomial<QS, 2>(
            out, {cache.t2_values[ij], c1}, field_t::from_1(cache.u2_values[ij]));
    }
    else if constexpr (meta::weight == 2)
    {
        const auto& subset = meta::choose_w1_subsets[subset_idx];
        const size_t i = subset[0];
        const size_t j = subset[1];
        const size_t k = subset[2];
        const size_t ij = meta::pair_rank[i][j];
        const size_t ik = meta::pair_rank[i][k];
        const size_t jk = meta::pair_rank[j][k];

        field_t c0 = mul_polysecpar_reduce<P::secpar_v>(cache.t2_values[ij], cache.c_values[k]);

        field_t c1 = field_t::set_zero();
        add_mul_poly1_mask_polysecpar<P::secpar_v>(
            c1, detail::poly1_to_bit_mask(cache.a_values[i]), cache.t2_values[jk]);
        add_mul_poly1_mask_polysecpar<P::secpar_v>(
            c1, detail::poly1_to_bit_mask(cache.a_values[j]), cache.t2_values[ik]);
        add_mul_poly1_mask_polysecpar<P::secpar_v>(
            c1, detail::poly1_to_bit_mask(cache.a_values[k]), cache.t2_values[ij]);

        field_t c2 = field_t::set_zero();
        add_mul_poly1_mask_polysecpar<P::secpar_v>(
            c2, detail::poly1_to_bit_mask(cache.u2_values[ij]), cache.c_values[k]);
        add_mul_poly1_mask_polysecpar<P::secpar_v>(
            c2, detail::poly1_to_bit_mask(cache.u2_values[ik]), cache.c_values[j]);
        add_mul_poly1_mask_polysecpar<P::secpar_v>(
            c2, detail::poly1_to_bit_mask(cache.u2_values[jk]), cache.c_values[i]);

        sydo_owf_set_prover_polynomial<QS, 3>(
            out, {c0, c1, c2},
            field_t::from_1(mul_poly1(cache.u2_values[ij], cache.a_values[k])));
    }
    else
    {
        static_assert(meta::weight == 3, "Supported weight range is [1,3].");
        const auto& subset = meta::choose_w1_subsets[subset_idx];
        const size_t i = subset[0];
        const size_t j = subset[1];
        const size_t k = subset[2];
        const size_t l = subset[3];
        const auto& pair_ranks = meta::choose4_pair_ranks[subset_idx];
        const auto& triple_ranks = meta::choose4_triple_ranks[subset_idx];
        const size_t ij = pair_ranks[0];
        const size_t ik = pair_ranks[1];
        const size_t il = pair_ranks[2];
        const size_t jk = pair_ranks[3];
        const size_t jl = pair_ranks[4];
        const size_t kl = pair_ranks[5];
        const size_t ijk_rank = triple_ranks[0];
        const size_t ijl_rank = triple_ranks[1];
        const size_t ikl_rank = triple_ranks[2];
        const size_t jkl_rank = triple_ranks[3];

        field_t c0 = mul_polysecpar_reduce<P::secpar_v>(cache.t2_values[ij],
                                                        cache.t2_values[kl]);

        field_t c1 = field_t::set_zero();
        add_mul_poly1_mask_polysecpar<P::secpar_v>(
            c1, detail::poly1_to_bit_mask(cache.a_values[i]), cache.t3_values[jkl_rank]);
        add_mul_poly1_mask_polysecpar<P::secpar_v>(
            c1, detail::poly1_to_bit_mask(cache.a_values[j]), cache.t3_values[ikl_rank]);
        add_mul_poly1_mask_polysecpar<P::secpar_v>(
            c1, detail::poly1_to_bit_mask(cache.a_values[k]), cache.t3_values[ijl_rank]);
        add_mul_poly1_mask_polysecpar<P::secpar_v>(
            c1, detail::poly1_to_bit_mask(cache.a_values[l]), cache.t3_values[ijk_rank]);

        field_t c2 = field_t::set_zero();
        add_mul_poly1_mask_polysecpar<P::secpar_v>(
            c2, detail::poly1_to_bit_mask(cache.u2_values[ij]), cache.t2_values[kl]);
        add_mul_poly1_mask_polysecpar<P::secpar_v>(
            c2, detail::poly1_to_bit_mask(cache.u2_values[ik]), cache.t2_values[jl]);
        add_mul_poly1_mask_polysecpar<P::secpar_v>(
            c2, detail::poly1_to_bit_mask(cache.u2_values[il]), cache.t2_values[jk]);
        add_mul_poly1_mask_polysecpar<P::secpar_v>(
            c2, detail::poly1_to_bit_mask(cache.u2_values[jk]), cache.t2_values[il]);
        add_mul_poly1_mask_polysecpar<P::secpar_v>(
            c2, detail::poly1_to_bit_mask(cache.u2_values[jl]), cache.t2_values[ik]);
        add_mul_poly1_mask_polysecpar<P::secpar_v>(
            c2, detail::poly1_to_bit_mask(cache.u2_values[kl]), cache.t2_values[ij]);

        field_t c3 = field_t::set_zero();
        add_mul_poly1_mask_polysecpar<P::secpar_v>(
            c3, detail::poly1_to_bit_mask(cache.u3_values[ijk_rank]), cache.c_values[l]);
        add_mul_poly1_mask_polysecpar<P::secpar_v>(
            c3, detail::poly1_to_bit_mask(cache.u3_values[ijl_rank]), cache.c_values[k]);
        add_mul_poly1_mask_polysecpar<P::secpar_v>(
            c3, detail::poly1_to_bit_mask(cache.u3_values[ikl_rank]), cache.c_values[j]);
        add_mul_poly1_mask_polysecpar<P::secpar_v>(
            c3, detail::poly1_to_bit_mask(cache.u3_values[jkl_rank]), cache.c_values[i]);

        sydo_owf_set_prover_polynomial<QS, 4>(
            out, {c0, c1, c2, c3},
            field_t::from_1(mul_poly1(cache.u2_values[ij], cache.u2_values[kl])));
    }
}

template <typename P, typename QS, size_t block_idx>
inline std::array<poly_secpar<P::secpar_v>, sydo_owf_block_meta<P, block_idx>::weight + 1>
sydo_owf_choose_w1_constraint_coeffs_prover(
    const sydo_owf_block_cache<P, QS, block_idx>& cache, size_t subset_idx)
{
    static_assert(!QS::is_verifier, "Prover-only helper.");
    using meta = sydo_owf_block_meta<P, block_idx>;
    using field_t = poly_secpar<P::secpar_v>;
    std::array<field_t, meta::weight + 1> coeffs{};
    for (auto& coeff : coeffs)
        coeff = field_t::set_zero();

    if constexpr (meta::weight == 1)
    {
        const auto& subset = meta::choose_w1_subsets[subset_idx];
        const size_t i = subset[0];
        const size_t j = subset[1];
        const size_t ij = subset_idx;
        field_t c1 = field_t::set_zero();
        add_mul_poly1_mask_polysecpar<P::secpar_v>(
            c1, detail::poly1_to_bit_mask(cache.a_values[i]), cache.c_values[j]);
        add_mul_poly1_mask_polysecpar<P::secpar_v>(
            c1, detail::poly1_to_bit_mask(cache.a_values[j]), cache.c_values[i]);
        coeffs[0] = cache.t2_values[ij];
        coeffs[1] = c1;
    }
    else if constexpr (meta::weight == 2)
    {
        const auto& subset = meta::choose_w1_subsets[subset_idx];
        const size_t i = subset[0];
        const size_t j = subset[1];
        const size_t k = subset[2];
        const size_t ij = meta::pair_rank[i][j];
        const size_t ik = meta::pair_rank[i][k];
        const size_t jk = meta::pair_rank[j][k];

        coeffs[0] = mul_polysecpar_reduce<P::secpar_v>(cache.t2_values[ij],
                                                       cache.c_values[k]);

        add_mul_poly1_mask_polysecpar<P::secpar_v>(
            coeffs[1], detail::poly1_to_bit_mask(cache.a_values[i]), cache.t2_values[jk]);
        add_mul_poly1_mask_polysecpar<P::secpar_v>(
            coeffs[1], detail::poly1_to_bit_mask(cache.a_values[j]), cache.t2_values[ik]);
        add_mul_poly1_mask_polysecpar<P::secpar_v>(
            coeffs[1], detail::poly1_to_bit_mask(cache.a_values[k]), cache.t2_values[ij]);

        add_mul_poly1_mask_polysecpar<P::secpar_v>(
            coeffs[2], detail::poly1_to_bit_mask(cache.u2_values[ij]), cache.c_values[k]);
        add_mul_poly1_mask_polysecpar<P::secpar_v>(
            coeffs[2], detail::poly1_to_bit_mask(cache.u2_values[ik]), cache.c_values[j]);
        add_mul_poly1_mask_polysecpar<P::secpar_v>(
            coeffs[2], detail::poly1_to_bit_mask(cache.u2_values[jk]), cache.c_values[i]);
    }
    else
    {
        static_assert(meta::weight == 3, "Supported weight range is [1,3].");
        const auto& subset = meta::choose_w1_subsets[subset_idx];
        const size_t i = subset[0];
        const size_t j = subset[1];
        const size_t k = subset[2];
        const size_t l = subset[3];
        const auto& pair_ranks = meta::choose4_pair_ranks[subset_idx];
        const auto& triple_ranks = meta::choose4_triple_ranks[subset_idx];
        const size_t ij = pair_ranks[0];
        const size_t ik = pair_ranks[1];
        const size_t il = pair_ranks[2];
        const size_t jk = pair_ranks[3];
        const size_t jl = pair_ranks[4];
        const size_t kl = pair_ranks[5];
        const size_t ijk_rank = triple_ranks[0];
        const size_t ijl_rank = triple_ranks[1];
        const size_t ikl_rank = triple_ranks[2];
        const size_t jkl_rank = triple_ranks[3];

        coeffs[0] = mul_polysecpar_reduce<P::secpar_v>(cache.t2_values[ij],
                                                       cache.t2_values[kl]);

        add_mul_poly1_mask_polysecpar<P::secpar_v>(
            coeffs[1], detail::poly1_to_bit_mask(cache.a_values[i]),
            cache.t3_values[jkl_rank]);
        add_mul_poly1_mask_polysecpar<P::secpar_v>(
            coeffs[1], detail::poly1_to_bit_mask(cache.a_values[j]),
            cache.t3_values[ikl_rank]);
        add_mul_poly1_mask_polysecpar<P::secpar_v>(
            coeffs[1], detail::poly1_to_bit_mask(cache.a_values[k]),
            cache.t3_values[ijl_rank]);
        add_mul_poly1_mask_polysecpar<P::secpar_v>(
            coeffs[1], detail::poly1_to_bit_mask(cache.a_values[l]),
            cache.t3_values[ijk_rank]);

        add_mul_poly1_mask_polysecpar<P::secpar_v>(
            coeffs[2], detail::poly1_to_bit_mask(cache.u2_values[ij]),
            cache.t2_values[kl]);
        add_mul_poly1_mask_polysecpar<P::secpar_v>(
            coeffs[2], detail::poly1_to_bit_mask(cache.u2_values[ik]),
            cache.t2_values[jl]);
        add_mul_poly1_mask_polysecpar<P::secpar_v>(
            coeffs[2], detail::poly1_to_bit_mask(cache.u2_values[il]),
            cache.t2_values[jk]);
        add_mul_poly1_mask_polysecpar<P::secpar_v>(
            coeffs[2], detail::poly1_to_bit_mask(cache.u2_values[jk]),
            cache.t2_values[il]);
        add_mul_poly1_mask_polysecpar<P::secpar_v>(
            coeffs[2], detail::poly1_to_bit_mask(cache.u2_values[jl]),
            cache.t2_values[ik]);
        add_mul_poly1_mask_polysecpar<P::secpar_v>(
            coeffs[2], detail::poly1_to_bit_mask(cache.u2_values[kl]),
            cache.t2_values[ij]);

        add_mul_poly1_mask_polysecpar<P::secpar_v>(
            coeffs[3], detail::poly1_to_bit_mask(cache.u3_values[ijk_rank]),
            cache.c_values[l]);
        add_mul_poly1_mask_polysecpar<P::secpar_v>(
            coeffs[3], detail::poly1_to_bit_mask(cache.u3_values[ijl_rank]),
            cache.c_values[k]);
        add_mul_poly1_mask_polysecpar<P::secpar_v>(
            coeffs[3], detail::poly1_to_bit_mask(cache.u3_values[ikl_rank]),
            cache.c_values[j]);
        add_mul_poly1_mask_polysecpar<P::secpar_v>(
            coeffs[3], detail::poly1_to_bit_mask(cache.u3_values[jkl_rank]),
            cache.c_values[i]);
    }
    return coeffs;
}

template <typename P, typename QS, size_t block_idx>
inline quicksilver_gfsecpar<QS, sydo_owf_block_meta<P, block_idx>::weight>
sydo_owf_build_exact_weight_prover(QS* state,
                                     const sydo_owf_block_cache<P, QS, block_idx>& cache)
{
    static_assert(!QS::is_verifier, "Prover-only exact weight builder.");
    using meta = sydo_owf_block_meta<P, block_idx>;
    using field_t = poly_secpar<P::secpar_v>;

    auto exact_weight = quicksilver_gfsecpar<QS, meta::weight>(0, state);
    std::array<field_t, meta::weight> coeffs{};
    for (auto& coeff : coeffs)
        coeff = field_t::set_zero();
    field_t leading_value = field_t::from_1(poly1::set_one());
    if constexpr (meta::weight == 1)
    {
        for (size_t subset_idx = 0; subset_idx < meta::choose_w_count; ++subset_idx)
        {
            const size_t i = meta::choose_w_subsets[subset_idx][0];
            coeffs[0] += cache.c_values[i];
            leading_value += field_t::from_1(cache.a_values[i]);
        }
    }
    else if constexpr (meta::weight == 2)
    {
        poly1 all_a = poly1::set_zero();
        for (size_t i = 0; i < meta::block_size; ++i)
            all_a += cache.a_values[i];

        for (size_t idx = 0; idx < meta::choose2_count; ++idx)
        {
            coeffs[0] += cache.t2_values[idx];
            leading_value += field_t::from_1(cache.u2_values[idx]);
        }

        for (size_t i = 0; i < meta::block_size; ++i)
        {
            const poly1 sum_except_i = all_a + cache.a_values[i];
            coeffs[1] += mul_poly1_polysecpar<P::secpar_v>(sum_except_i, cache.c_values[i]);
        }
    }
    else
    {
        static_assert(meta::weight == 3, "Supported weight range is [1,3].");
        poly1 all_a = poly1::set_zero();
        for (size_t i = 0; i < meta::block_size; ++i)
            all_a += cache.a_values[i];

        poly1 total_u2 = poly1::set_zero();
        for (size_t p = 0; p < meta::choose2_count; ++p)
            total_u2 += cache.u2_values[p];

        for (size_t idx = 0; idx < meta::choose3_count; ++idx)
        {
            coeffs[0] += cache.t3_values[idx];
            leading_value += field_t::from_1(cache.u3_values[idx]);
        }

        for (size_t p = 0; p < meta::choose2_count; ++p)
        {
            const size_t i = meta::choose2_subsets[p][0];
            const size_t j = meta::choose2_subsets[p][1];
            const poly1 sum_except_pair = all_a + cache.a_values[i] + cache.a_values[j];
            coeffs[1] += mul_poly1_polysecpar<P::secpar_v>(sum_except_pair,
                                                            cache.t2_values[p]);
        }

        for (size_t i = 0; i < meta::block_size; ++i)
        {
            poly1 sum_incident_u2 = poly1::set_zero();
            for (size_t j = 0; j < meta::block_size; ++j)
            {
                if (j == i)
                    continue;
                const size_t p = meta::pair_rank[i][j];
                sum_incident_u2 += cache.u2_values[p];
            }
            const poly1 sum_except_i = total_u2 + sum_incident_u2;
            coeffs[2] += mul_poly1_polysecpar<P::secpar_v>(sum_except_i, cache.c_values[i]);
        }
    }
    sydo_owf_set_prover_polynomial<QS, meta::weight>(&exact_weight, coeffs, leading_value);
    return exact_weight;
}

template <typename P, typename QS, size_t block_idx>
inline void sydo_owf_emit_block_constraints(QS* state,
                                              const sydo_owf_block_cache<P, QS, block_idx>& cache)
{
    using meta = sydo_owf_block_meta<P, block_idx>;
    static_assert(meta::weight + 1 <= QS::max_degree,
                  "Block overweight constraint degree exceeds QS degree.");
    constexpr size_t emit_stage =
        block_idx < 4 ? profile::OWF_EMIT_BLOCK0 + block_idx : profile::STAGE_COUNT;
    profile::scope timer(emit_stage);

    if constexpr (!QS::is_verifier)
    {
        for (size_t subset_idx = 0; subset_idx < meta::choose_w1_count; ++subset_idx)
        {
            std::array<poly_secpar<P::secpar_v>, meta::weight + 1> coeffs{};
            {
                profile::scope coeff_timer(profile::OWF_EMIT_PROVER_COEFF_BUILD);
                coeffs =
                    sydo_owf_choose_w1_constraint_coeffs_prover<P, QS, block_idx>(cache,
                                                                                    subset_idx);
            }
            {
                profile::scope add_timer(profile::OWF_EMIT_PROVER_ADD_CONSTRAINT);
                state->template add_prover_constraint_coeffs<meta::weight + 1>(coeffs);
            }
        }
    }
    else
    {
        for (size_t subset_idx = 0; subset_idx < meta::choose_w1_count; ++subset_idx)
        {
            const auto overweight =
                sydo_owf_choose_w1_product<P, QS, block_idx>(cache, subset_idx);
            state->add_constraint(quicksilver_gfsecpar<QS, meta::weight + 1>(overweight));
        }
    }
    if constexpr (P::elementary_encoding_v == elementary_encoding::hamming_sphere)
    {
        if constexpr (!QS::is_verifier)
        {
            auto exact_weight =
                sydo_owf_build_exact_weight_prover<P, QS, block_idx>(state, cache);
            state->add_constraint(exact_weight);
        }
        else
        {
            auto exact_weight = quicksilver_gf2<QS, meta::weight>(1, state);
            for (size_t subset_idx = 0; subset_idx < meta::choose_w_count; ++subset_idx)
                exact_weight += sydo_owf_choose_w_product<P, QS, block_idx>(cache, subset_idx);
            state->add_constraint(quicksilver_gfsecpar<QS, meta::weight>(exact_weight));
        }
    }
}

template <typename P, typename QS, typename CacheTuple, size_t... block_idx>
inline void sydo_owf_load_all_block_caches(QS* state, size_t subvector_offset_bits,
                                             CacheTuple* block_caches,
                                             std::index_sequence<block_idx...>)
{
    (sydo_owf_load_block_cache<P, QS, block_idx>(state, subvector_offset_bits,
                                                   &std::get<block_idx>(*block_caches)),
     ...);
}

template <typename P, typename QS, typename CacheTuple, size_t... block_idx>
inline void sydo_owf_emit_all_block_constraints(QS* state, const CacheTuple& block_caches,
                                                  std::index_sequence<block_idx...>)
{
    (sydo_owf_emit_block_constraints<P, QS, block_idx>(state,
                                                         std::get<block_idx>(block_caches)),
     ...);
}

template <typename P> constexpr auto sydo_owf_choose_w_counts()
{
    std::array<size_t, P::num_var_blocks> counts{};
    for (size_t b = 0; b < P::num_var_blocks; ++b)
        counts[b] = sydo_owf_block_domain_count<P>(b);
    return counts;
}

template <typename P> constexpr auto sydo_owf_tuple_strides()
{
    constexpr auto counts = sydo_owf_choose_w_counts<P>();
    std::array<size_t, P::num_var_blocks> strides{};
    size_t prod = 1;
    for (size_t b = P::num_var_blocks; b-- > 0;)
    {
        strides[b] = prod;
        prod *= counts[b];
    }
    return strides;
}

template <typename P> constexpr auto sydo_owf_order_choose_w_counts()
{
    constexpr auto counts = sydo_owf_choose_w_counts<P>();
    std::array<size_t, P::num_var_blocks> ordered{};
    for (size_t stage = 0; stage < P::num_var_blocks; ++stage)
        ordered[stage] = counts[P::FOLD_ORDER[stage]];
    return ordered;
}

template <typename P, size_t stage> constexpr size_t sydo_owf_stage_input_size()
{
    constexpr auto ordered_counts = sydo_owf_order_choose_w_counts<P>();
    size_t prod = 1;
    for (size_t s = stage; s < P::num_var_blocks; ++s)
        prod *= ordered_counts[s];
    return prod;
}

template <typename P, size_t stage> constexpr size_t sydo_owf_stage_output_size()
{
    constexpr auto ordered_counts = sydo_owf_order_choose_w_counts<P>();
    size_t prod = 1;
    for (size_t s = stage + 1; s < P::num_var_blocks; ++s)
        prod *= ordered_counts[s];
    return prod;
}

template <typename P, size_t stage> constexpr size_t sydo_owf_stage_prefix_degree()
{
    size_t sum = 0;
    for (size_t s = 0; s < stage; ++s)
        sum += P::block_wts[P::FOLD_ORDER[s]];
    return sum;
}

template <typename P, size_t start_stage> constexpr auto sydo_owf_suffix_base_offsets()
{
    constexpr auto ordered_counts = sydo_owf_order_choose_w_counts<P>();
    constexpr auto natural_strides = sydo_owf_tuple_strides<P>();
    constexpr size_t next_size = []
    {
        constexpr auto counts = sydo_owf_order_choose_w_counts<P>();
        size_t prod = 1;
        for (size_t s = start_stage; s < P::num_var_blocks; ++s)
            prod *= counts[s];
        return prod;
    }();
    std::array<size_t, next_size> offsets{};
    for (size_t suffix_idx = 0; suffix_idx < next_size; ++suffix_idx)
    {
        size_t rem = suffix_idx;
        size_t natural_base = 0;
        for (size_t stage = start_stage; stage < P::num_var_blocks; ++stage)
        {
            size_t suffix_prod = 1;
            for (size_t s = stage + 1; s < P::num_var_blocks; ++s)
                suffix_prod *= ordered_counts[s];
            const size_t digit = rem / suffix_prod;
            rem %= suffix_prod;
            const size_t block_idx = P::FOLD_ORDER[stage];
            natural_base += digit * natural_strides[block_idx];
        }
        offsets[suffix_idx] = natural_base;
    }
    return offsets;
}

template <typename P> constexpr auto sydo_owf_stage0_suffix_base_offsets()
{
    return sydo_owf_suffix_base_offsets<P, 1>();
}

template <typename P, typename QS, size_t block_idx, size_t cur_degree, size_t coeff_stride,
          size_t row_stride>
inline void sydo_owf_contract_block_weight_sum_batch_add(
    const sydo_owf_block_cache<P, QS, block_idx>& cache,
    const poly_secpar<P::secpar_v>* coeff_base, poly_secpar<P::secpar_v>* out)
{
    static_assert(!QS::is_verifier, "Prover-only contraction helper.");
    using meta = sydo_owf_block_meta<P, block_idx>;
    using field_t = poly_secpar<P::secpar_v>;

    if constexpr (meta::weight == 1)
    {
        for (size_t subset_idx = 0; subset_idx < meta::choose_w_count; ++subset_idx)
        {
            const size_t i = meta::choose_w_subsets[subset_idx][0];
            for (size_t t = 0; t <= cur_degree; ++t)
            {
                const auto& h_coeff = coeff_base[t * row_stride + subset_idx * coeff_stride];
                out[t] += mul_polysecpar_reduce<P::secpar_v>(h_coeff, cache.c_values[i]);
                out[t + 1] += mul_poly1_polysecpar<P::secpar_v>(cache.a_values[i], h_coeff);
            }
        }
    }
    else if constexpr (meta::weight == 2)
    {
        std::array<field_t, (cur_degree + 1) * meta::block_size> single_mix{};
        for (size_t idx = 0; idx < single_mix.size(); ++idx)
            single_mix[idx] = field_t::set_zero();

        for (size_t subset_idx = 0; subset_idx < meta::choose_w_count; ++subset_idx)
        {
            const auto& subset = meta::choose_w_subsets[subset_idx];
            const size_t i = subset[0];
            const size_t j = subset[1];
            const size_t ij = subset_idx;
            for (size_t t = 0; t <= cur_degree; ++t)
            {
                const auto& h_coeff = coeff_base[t * row_stride + subset_idx * coeff_stride];
                out[t] += mul_polysecpar_reduce<P::secpar_v>(h_coeff, cache.t2_values[ij]);
                single_mix[t * meta::block_size + i] +=
                    mul_poly1_polysecpar<P::secpar_v>(cache.a_values[j], h_coeff);
                single_mix[t * meta::block_size + j] +=
                    mul_poly1_polysecpar<P::secpar_v>(cache.a_values[i], h_coeff);
                out[t + 2] += mul_poly1_polysecpar<P::secpar_v>(cache.u2_values[ij], h_coeff);
            }
        }

        for (size_t t = 0; t <= cur_degree; ++t)
            for (size_t i = 0; i < meta::block_size; ++i)
                out[t + 1] += mul_polysecpar_reduce<P::secpar_v>(
                    cache.c_values[i], single_mix[t * meta::block_size + i]);
    }
    else
    {
        static_assert(meta::weight == 3, "Supported weight range is [1,3].");
        std::array<field_t, (cur_degree + 1) * meta::choose2_count> pair_mix{};
        std::array<field_t, (cur_degree + 1) * meta::block_size> single_mix{};
        for (size_t idx = 0; idx < pair_mix.size(); ++idx)
            pair_mix[idx] = field_t::set_zero();
        for (size_t idx = 0; idx < single_mix.size(); ++idx)
            single_mix[idx] = field_t::set_zero();

        for (size_t subset_idx = 0; subset_idx < meta::choose_w_count; ++subset_idx)
        {
            const auto& subset = meta::choose_w_subsets[subset_idx];
            const size_t i = subset[0];
            const size_t j = subset[1];
            const size_t k = subset[2];
            const auto& pair_ranks = meta::choose3_pair_ranks[subset_idx];
            const size_t ij = pair_ranks[0];
            const size_t ik = pair_ranks[1];
            const size_t jk = pair_ranks[2];
            const size_t ijk = subset_idx;

            for (size_t t = 0; t <= cur_degree; ++t)
            {
                const auto& h_coeff = coeff_base[t * row_stride + subset_idx * coeff_stride];
                out[t] += mul_polysecpar_reduce<P::secpar_v>(h_coeff, cache.t3_values[ijk]);

                pair_mix[t * meta::choose2_count + jk] +=
                    mul_poly1_polysecpar<P::secpar_v>(cache.a_values[i], h_coeff);
                pair_mix[t * meta::choose2_count + ik] +=
                    mul_poly1_polysecpar<P::secpar_v>(cache.a_values[j], h_coeff);
                pair_mix[t * meta::choose2_count + ij] +=
                    mul_poly1_polysecpar<P::secpar_v>(cache.a_values[k], h_coeff);

                single_mix[t * meta::block_size + i] +=
                    mul_poly1_polysecpar<P::secpar_v>(cache.u2_values[jk], h_coeff);
                single_mix[t * meta::block_size + j] +=
                    mul_poly1_polysecpar<P::secpar_v>(cache.u2_values[ik], h_coeff);
                single_mix[t * meta::block_size + k] +=
                    mul_poly1_polysecpar<P::secpar_v>(cache.u2_values[ij], h_coeff);

                out[t + 3] += mul_poly1_polysecpar<P::secpar_v>(cache.u3_values[ijk], h_coeff);
            }
        }

        for (size_t t = 0; t <= cur_degree; ++t)
        {
            for (size_t p = 0; p < meta::choose2_count; ++p)
                out[t + 1] += mul_polysecpar_reduce<P::secpar_v>(
                    cache.t2_values[p], pair_mix[t * meta::choose2_count + p]);
            for (size_t i = 0; i < meta::block_size; ++i)
                out[t + 2] += mul_polysecpar_reduce<P::secpar_v>(
                    cache.c_values[i], single_mix[t * meta::block_size + i]);
        }
    }
}

template <typename P, typename QS, size_t block_idx, size_t cur_degree>
inline void sydo_owf_add_ball_monomial_prover(
    const sydo_owf_block_cache<P, QS, block_idx>& cache, size_t mask,
    const std::array<poly_secpar<P::secpar_v>, cur_degree + 1>& coeffs,
    poly_secpar<P::secpar_v>* out)
{
    static_assert(!QS::is_verifier, "Prover-only ball monomial helper.");
    using meta = sydo_owf_block_meta<P, block_idx>;
    using field_t = poly_secpar<P::secpar_v>;
    const size_t weight = sydo_owf_mask_weight(mask);

    if (weight == 0)
    {
        for (size_t t = 0; t <= cur_degree; ++t)
            out[t] += coeffs[t];
        return;
    }

    std::array<size_t, 3> idx{};
    size_t pos = 0;
    for (size_t i = 0; i < meta::block_size; ++i)
    {
        if (((mask >> i) & 1u) != 0)
            idx[pos++] = i;
    }

    if (weight == 1)
    {
        const size_t i = idx[0];
        for (size_t t = 0; t <= cur_degree; ++t)
        {
            out[t] += mul_polysecpar_reduce<P::secpar_v>(coeffs[t], cache.c_values[i]);
            out[t + 1] += mul_poly1_polysecpar<P::secpar_v>(cache.a_values[i], coeffs[t]);
        }
    }
    else if (weight == 2)
    {
        const size_t i = idx[0];
        const size_t j = idx[1];
        const size_t ij = meta::pair_rank[i][j];
        field_t x1 = mul_poly1_polysecpar<P::secpar_v>(cache.a_values[i], cache.c_values[j]);
        x1 += mul_poly1_polysecpar<P::secpar_v>(cache.a_values[j], cache.c_values[i]);
        for (size_t t = 0; t <= cur_degree; ++t)
        {
            out[t] += mul_polysecpar_reduce<P::secpar_v>(coeffs[t], cache.t2_values[ij]);
            out[t + 1] += mul_polysecpar_reduce<P::secpar_v>(coeffs[t], x1);
            out[t + 2] += mul_poly1_polysecpar<P::secpar_v>(cache.u2_values[ij], coeffs[t]);
        }
    }
    else
    {
        const size_t i = idx[0];
        const size_t j = idx[1];
        const size_t k = idx[2];
        const size_t ij = meta::pair_rank[i][j];
        const size_t ik = meta::pair_rank[i][k];
        const size_t jk = meta::pair_rank[j][k];
        const size_t ijk = meta::triple_rank[i][j][k];

        field_t x1 = mul_poly1_polysecpar<P::secpar_v>(cache.a_values[i], cache.t2_values[jk]);
        x1 += mul_poly1_polysecpar<P::secpar_v>(cache.a_values[j], cache.t2_values[ik]);
        x1 += mul_poly1_polysecpar<P::secpar_v>(cache.a_values[k], cache.t2_values[ij]);

        field_t x2 = mul_poly1_polysecpar<P::secpar_v>(cache.u2_values[ij], cache.c_values[k]);
        x2 += mul_poly1_polysecpar<P::secpar_v>(cache.u2_values[ik], cache.c_values[j]);
        x2 += mul_poly1_polysecpar<P::secpar_v>(cache.u2_values[jk], cache.c_values[i]);

        const poly1 x3 = mul_poly1(cache.u2_values[ij], cache.a_values[k]);
        for (size_t t = 0; t <= cur_degree; ++t)
        {
            out[t] += mul_polysecpar_reduce<P::secpar_v>(coeffs[t], cache.t3_values[ijk]);
            out[t + 1] += mul_polysecpar_reduce<P::secpar_v>(coeffs[t], x1);
            out[t + 2] += mul_polysecpar_reduce<P::secpar_v>(coeffs[t], x2);
            out[t + 3] += mul_poly1_polysecpar<P::secpar_v>(x3, coeffs[t]);
        }
    }
}

template <typename P, typename QS, typename CacheTuple>
inline poly_secpar<P::secpar_v>
sydo_owf_selected_ball_h_value_prover(const CacheTuple& block_caches,
                                        const poly_secpar<P::secpar_v>* h_subvector);

template <typename P, typename QS, size_t block_idx>
inline void sydo_owf_ball_monomial_coeffs_prover(
    const sydo_owf_block_cache<P, QS, block_idx>& cache, size_t mask,
    std::array<poly_secpar<P::secpar_v>, sydo_owf_block_meta<P, block_idx>::weight + 1>* out)
{
    static_assert(!QS::is_verifier, "Prover-only ball monomial expansion helper.");
    using meta = sydo_owf_block_meta<P, block_idx>;
    using field_t = poly_secpar<P::secpar_v>;
    const size_t weight = sydo_owf_mask_weight(mask);

    for (auto& coeff : *out)
        coeff = field_t::set_zero();
    if (weight == 0)
    {
        (*out)[0] = field_t::from_1(poly1::set_one());
        return;
    }

    std::array<size_t, 3> idx{};
    size_t pos = 0;
    for (size_t i = 0; i < meta::block_size; ++i)
        if (((mask >> i) & 1u) != 0)
            idx[pos++] = i;

    if (weight == 1)
    {
        const size_t i = idx[0];
        (*out)[0] = cache.c_values[i];
        (*out)[1] = field_t::from_1(cache.a_values[i]);
    }
    else if (weight == 2)
    {
        const size_t i = idx[0];
        const size_t j = idx[1];
        const size_t ij = meta::pair_rank[i][j];
        (*out)[0] = cache.t2_values[ij];
        (*out)[1] = mul_poly1_polysecpar<P::secpar_v>(cache.a_values[i], cache.c_values[j]);
        (*out)[1] += mul_poly1_polysecpar<P::secpar_v>(cache.a_values[j], cache.c_values[i]);
        (*out)[2] = field_t::from_1(cache.u2_values[ij]);
    }
    else
    {
        const size_t i = idx[0];
        const size_t j = idx[1];
        const size_t k = idx[2];
        const size_t ij = meta::pair_rank[i][j];
        const size_t ik = meta::pair_rank[i][k];
        const size_t jk = meta::pair_rank[j][k];
        const size_t ijk = meta::triple_rank[i][j][k];

        (*out)[0] = cache.t3_values[ijk];
        (*out)[1] = mul_poly1_polysecpar<P::secpar_v>(cache.a_values[i],
                                                       cache.t2_values[jk]);
        (*out)[1] += mul_poly1_polysecpar<P::secpar_v>(cache.a_values[j],
                                                        cache.t2_values[ik]);
        (*out)[1] += mul_poly1_polysecpar<P::secpar_v>(cache.a_values[k],
                                                        cache.t2_values[ij]);
        (*out)[2] = mul_poly1_polysecpar<P::secpar_v>(cache.u2_values[ij],
                                                       cache.c_values[k]);
        (*out)[2] += mul_poly1_polysecpar<P::secpar_v>(cache.u2_values[ik],
                                                        cache.c_values[j]);
        (*out)[2] += mul_poly1_polysecpar<P::secpar_v>(cache.u2_values[jk],
                                                        cache.c_values[i]);
        (*out)[3] = field_t::from_1(cache.u3_values[ijk]);
    }
}

template <typename P>
inline void sydo_owf_ball_build_indicator_coeffs_2block(
    const poly_secpar<P::secpar_v>* h_subvector,
    std::array<poly_secpar<P::secpar_v>, sydo_owf_terms_per_subvector<P>()>* coeff_by_rank);

template <typename P, size_t block_idx, typename Fn>
inline void sydo_owf_for_ball_superset_ranks(size_t point_mask, Fn&& fn)
{
    using meta = sydo_owf_block_meta<P, block_idx>;
    const size_t point_weight = sydo_owf_mask_weight(point_mask);
    SYDO_ASSERT(point_weight <= meta::weight);

    fn(meta::ball_rank_by_mask[point_mask]);
    if (point_weight == meta::weight)
        return;

    for (size_t i = 0; i < meta::block_size; ++i)
    {
        const size_t bi = size_t{1} << i;
        if ((point_mask & bi) != 0)
            continue;
        const size_t mi = point_mask | bi;
        fn(meta::ball_rank_by_mask[mi]);
    }
    if (point_weight + 1 == meta::weight)
        return;

    for (size_t i = 0; i < meta::block_size; ++i)
    {
        const size_t bi = size_t{1} << i;
        if ((point_mask & bi) != 0)
            continue;
        for (size_t j = i + 1; j < meta::block_size; ++j)
        {
            const size_t bj = size_t{1} << j;
            if ((point_mask & bj) != 0)
                continue;
            const size_t mij = point_mask | bi | bj;
            fn(meta::ball_rank_by_mask[mij]);
        }
    }
    if (point_weight + 2 == meta::weight)
        return;

    for (size_t i = 0; i < meta::block_size; ++i)
    {
        const size_t bi = size_t{1} << i;
        if ((point_mask & bi) != 0)
            continue;
        for (size_t j = i + 1; j < meta::block_size; ++j)
        {
            const size_t bj = size_t{1} << j;
            if ((point_mask & bj) != 0)
                continue;
            for (size_t k = j + 1; k < meta::block_size; ++k)
            {
                const size_t bk = size_t{1} << k;
                if ((point_mask & bk) != 0)
                    continue;
                const size_t mijk = point_mask | bi | bj | bk;
                fn(meta::ball_rank_by_mask[mijk]);
            }
        }
    }
}

template <typename P> constexpr size_t sydo_owf_ball_2block_superset_update_count()
{
    static_assert(P::elementary_encoding_v == elementary_encoding::hamming_ball);
    static_assert(P::num_var_blocks == 2);
    using meta0 = sydo_owf_block_meta<P, 0>;
    using meta1 = sydo_owf_block_meta<P, 1>;

    size_t total = 0;
    for (size_t p0_rank = 0; p0_rank < meta0::domain_count; ++p0_rank)
    {
        const size_t p0 = meta0::ball_masks[p0_rank];
        for (size_t p1_rank = 0; p1_rank < meta1::domain_count; ++p1_rank)
        {
            const size_t p1 = meta1::ball_masks[p1_rank];
            for (size_t m0_rank = 0; m0_rank < meta0::domain_count; ++m0_rank)
            {
                const size_t m0 = meta0::ball_masks[m0_rank];
                if ((p0 & ~m0) != 0)
                    continue;
                for (size_t m1_rank = 0; m1_rank < meta1::domain_count; ++m1_rank)
                {
                    const size_t m1 = meta1::ball_masks[m1_rank];
                    if ((p1 & ~m1) == 0)
                        ++total;
                }
            }
        }
    }
    return total;
}

template <typename P> constexpr auto sydo_owf_make_ball_2block_superset_offsets()
{
    using meta0 = sydo_owf_block_meta<P, 0>;
    using meta1 = sydo_owf_block_meta<P, 1>;
    constexpr auto natural_strides = sydo_owf_tuple_strides<P>();
    std::array<size_t, sydo_owf_terms_per_subvector<P>() + 1> offsets{};

    size_t pos = 0;
    for (size_t p0_rank = 0; p0_rank < meta0::domain_count; ++p0_rank)
    {
        const size_t p0 = meta0::ball_masks[p0_rank];
        for (size_t p1_rank = 0; p1_rank < meta1::domain_count; ++p1_rank)
        {
            const size_t p1 = meta1::ball_masks[p1_rank];
            const size_t point_rank = p0_rank * natural_strides[0] + p1_rank * natural_strides[1];
            offsets[point_rank] = pos;
            for (size_t m0_rank = 0; m0_rank < meta0::domain_count; ++m0_rank)
            {
                const size_t m0 = meta0::ball_masks[m0_rank];
                if ((p0 & ~m0) != 0)
                    continue;
                for (size_t m1_rank = 0; m1_rank < meta1::domain_count; ++m1_rank)
                {
                    const size_t m1 = meta1::ball_masks[m1_rank];
                    if ((p1 & ~m1) == 0)
                        ++pos;
                }
            }
        }
    }
    offsets[sydo_owf_terms_per_subvector<P>()] = pos;
    return offsets;
}

template <typename P> constexpr auto sydo_owf_make_ball_2block_superset_ranks()
{
    using meta0 = sydo_owf_block_meta<P, 0>;
    using meta1 = sydo_owf_block_meta<P, 1>;
    constexpr auto natural_strides = sydo_owf_tuple_strides<P>();
    constexpr size_t update_count = sydo_owf_ball_2block_superset_update_count<P>();
    std::array<size_t, update_count> ranks{};

    size_t pos = 0;
    for (size_t p0_rank = 0; p0_rank < meta0::domain_count; ++p0_rank)
    {
        const size_t p0 = meta0::ball_masks[p0_rank];
        for (size_t p1_rank = 0; p1_rank < meta1::domain_count; ++p1_rank)
        {
            const size_t p1 = meta1::ball_masks[p1_rank];
            for (size_t m0_rank = 0; m0_rank < meta0::domain_count; ++m0_rank)
            {
                const size_t m0 = meta0::ball_masks[m0_rank];
                if ((p0 & ~m0) != 0)
                    continue;
                for (size_t m1_rank = 0; m1_rank < meta1::domain_count; ++m1_rank)
                {
                    const size_t m1 = meta1::ball_masks[m1_rank];
                    if ((p1 & ~m1) == 0)
                        ranks[pos++] = m0_rank * natural_strides[0] + m1_rank * natural_strides[1];
                }
            }
        }
    }
    return ranks;
}

template <typename P>
inline void sydo_owf_ball_build_indicator_coeffs_2block(
    const poly_secpar<P::secpar_v>* h_subvector,
    std::array<poly_secpar<P::secpar_v>, sydo_owf_terms_per_subvector<P>()>* coeff_by_rank)
{
    static_assert(P::elementary_encoding_v == elementary_encoding::hamming_ball);
    static_assert(P::num_var_blocks == 2);
    using field_t = poly_secpar<P::secpar_v>;
    constexpr auto offsets = sydo_owf_make_ball_2block_superset_offsets<P>();
    constexpr auto ranks = sydo_owf_make_ball_2block_superset_ranks<P>();

    for (auto& coeff : *coeff_by_rank)
        coeff = field_t::set_zero();
    for (size_t point_rank = 0; point_rank < sydo_owf_terms_per_subvector<P>(); ++point_rank)
    {
        const field_t& h = h_subvector[point_rank];
        for (size_t pos = offsets[point_rank]; pos < offsets[point_rank + 1]; ++pos)
            (*coeff_by_rank)[ranks[pos]] += h;
    }
}

template <typename P, typename field_t>
inline void sydo_owf_ball_stage0_coeffs_for_suffix(
    const field_t* h_subvector, size_t block0_rank,
    std::array<field_t, sydo_owf_block_meta<P, 1>::domain_count>* coeffs)
{
    static_assert(P::elementary_encoding_v == elementary_encoding::hamming_ball);
    static_assert(P::num_var_blocks == 2);
    using meta1 = sydo_owf_block_meta<P, 1>;
    constexpr auto natural_strides = sydo_owf_tuple_strides<P>();

    for (auto& coeff : *coeffs)
        coeff = field_t::set_zero();
    for (size_t point_rank = 0; point_rank < meta1::domain_count; ++point_rank)
    {
        const size_t point_mask = meta1::ball_masks[point_rank];
        const field_t& h = h_subvector[block0_rank * natural_strides[0] +
                                       point_rank * natural_strides[1]];
        sydo_owf_for_ball_superset_ranks<P, 1>(point_mask, [&](size_t monomial_rank) {
            (*coeffs)[monomial_rank] += h;
        });
    }
}

template <typename P, typename QS>
inline void sydo_owf_fold_ball_block1_stage0_prover(
    const sydo_owf_block_cache<P, QS, 1>& cache,
    const std::array<poly_secpar<P::secpar_v>, sydo_owf_block_meta<P, 1>::domain_count>& coeffs,
    std::array<poly_secpar<P::secpar_v>, sydo_owf_block_meta<P, 1>::weight + 1>* out)
{
    static_assert(!QS::is_verifier, "Prover-only hamming-ball stage0 helper.");
    using meta = sydo_owf_block_meta<P, 1>;
    using field_t = poly_secpar<P::secpar_v>;
    static_assert(meta::weight == 2 || meta::weight == 3,
                  "This optimized helper expects block weight 2 or 3.");

    for (auto& coeff : *out)
        coeff = field_t::set_zero();
    std::array<field_t, meta::choose2_count> pair_mix{};
    std::array<field_t, meta::block_size> single_mix{};
    for (auto& coeff : pair_mix)
        coeff = field_t::set_zero();
    for (auto& coeff : single_mix)
        coeff = field_t::set_zero();

    (*out)[meta::weight] += coeffs[meta::ball_rank_by_mask[0]];

    for (size_t i = 0; i < meta::block_size; ++i)
    {
        const size_t mask = size_t{1} << i;
        const field_t& coeff = coeffs[meta::ball_rank_by_mask[mask]];
        single_mix[i] += coeff;
        (*out)[meta::weight] += mul_poly1_polysecpar<P::secpar_v>(cache.a_values[i], coeff);
    }

    for (size_t p = 0; p < meta::choose2_count; ++p)
    {
        const size_t i = meta::choose2_subsets[p][0];
        const size_t j = meta::choose2_subsets[p][1];
        const size_t mask = (size_t{1} << i) | (size_t{1} << j);
        const field_t& coeff = coeffs[meta::ball_rank_by_mask[mask]];
        if constexpr (meta::weight == 2)
            (*out)[0] += mul_polysecpar_reduce<P::secpar_v>(coeff, cache.t2_values[p]);
        else
            pair_mix[p] += coeff;
        single_mix[i] += mul_poly1_polysecpar<P::secpar_v>(cache.a_values[j], coeff);
        single_mix[j] += mul_poly1_polysecpar<P::secpar_v>(cache.a_values[i], coeff);
        (*out)[meta::weight] += mul_poly1_polysecpar<P::secpar_v>(cache.u2_values[p], coeff);
    }

    if constexpr (meta::weight == 3)
    {
        for (size_t t = 0; t < meta::choose3_count; ++t)
        {
            const size_t i = meta::choose3_subsets[t][0];
            const size_t j = meta::choose3_subsets[t][1];
            const size_t k = meta::choose3_subsets[t][2];
            const size_t mask = (size_t{1} << i) | (size_t{1} << j) | (size_t{1} << k);
            const field_t& coeff = coeffs[meta::ball_rank_by_mask[mask]];
            const size_t ij = meta::choose3_pair_ranks[t][0];
            const size_t ik = meta::choose3_pair_ranks[t][1];
            const size_t jk = meta::choose3_pair_ranks[t][2];

            (*out)[0] += mul_polysecpar_reduce<P::secpar_v>(coeff, cache.t3_values[t]);
            pair_mix[jk] += mul_poly1_polysecpar<P::secpar_v>(cache.a_values[i], coeff);
            pair_mix[ik] += mul_poly1_polysecpar<P::secpar_v>(cache.a_values[j], coeff);
            pair_mix[ij] += mul_poly1_polysecpar<P::secpar_v>(cache.a_values[k], coeff);
            single_mix[i] += mul_poly1_polysecpar<P::secpar_v>(cache.u2_values[jk], coeff);
            single_mix[j] += mul_poly1_polysecpar<P::secpar_v>(cache.u2_values[ik], coeff);
            single_mix[k] += mul_poly1_polysecpar<P::secpar_v>(cache.u2_values[ij], coeff);
            (*out)[meta::weight] += mul_poly1_polysecpar<P::secpar_v>(cache.u3_values[t], coeff);
        }
    }

    if constexpr (meta::weight == 3)
    {
        for (size_t p = 0; p < meta::choose2_count; ++p)
            (*out)[1] += mul_polysecpar_reduce<P::secpar_v>(cache.t2_values[p], pair_mix[p]);
    }
    for (size_t i = 0; i < meta::block_size; ++i)
        (*out)[meta::weight - 1] +=
            mul_polysecpar_reduce<P::secpar_v>(cache.c_values[i], single_mix[i]);
}

template <typename P, typename QS>
inline void sydo_owf_fold_ball_block1_stage0_prover_from_h(
    const sydo_owf_block_cache<P, QS, 1>& cache, const poly_secpar<P::secpar_v>* h,
    std::array<poly_secpar<P::secpar_v>, sydo_owf_block_meta<P, 1>::weight + 1>* out)
{
    static_assert(!QS::is_verifier, "Prover-only hamming-ball stage0 helper.");
    using meta = sydo_owf_block_meta<P, 1>;
    using field_t = poly_secpar<P::secpar_v>;
    static_assert(meta::weight == 2 || meta::weight == 3,
                  "This optimized helper expects block weight 2 or 3.");

    std::array<field_t, meta::choose2_count> pair_mix{};
    std::array<field_t, meta::block_size> single_mix{};
    {
        profile::scope timer(profile::OWF_BALL_STAGE0_ZERO);
        for (auto& coeff : *out)
            coeff = field_t::set_zero();
        for (auto& coeff : pair_mix)
            coeff = field_t::set_zero();
        for (auto& coeff : single_mix)
            coeff = field_t::set_zero();
    }

    const field_t& h0 = h[meta::ball_rank_by_mask[0]];
    (*out)[meta::weight] += h0;

    {
        profile::scope timer(profile::OWF_BALL_STAGE0_SINGLE);
        for (size_t i = 0; i < meta::block_size; ++i)
        {
            const field_t coeff = h0 + h[meta::single_ball_rank_by_index[i]];
            single_mix[i] += coeff;
            (*out)[meta::weight] += mul_poly1_polysecpar<P::secpar_v>(cache.a_values[i], coeff);
        }
    }

    {
        profile::scope timer(profile::OWF_BALL_STAGE0_PAIR);
        if constexpr (meta::weight == 2)
        {
            polysecpar_dot_accumulator<P::secpar_v> out0_acc;
            for (size_t p = 0; p < meta::choose2_count; ++p)
            {
                const size_t i = meta::choose2_subsets[p][0];
                const size_t j = meta::choose2_subsets[p][1];
                const field_t coeff = h0 + h[meta::single_ball_rank_by_index[i]] +
                                      h[meta::single_ball_rank_by_index[j]] +
                                      h[meta::pair_ball_ranks[p]];
                out0_acc.add(coeff, cache.t2_values[p]);
                single_mix[i] += mul_poly1_polysecpar<P::secpar_v>(cache.a_values[j], coeff);
                single_mix[j] += mul_poly1_polysecpar<P::secpar_v>(cache.a_values[i], coeff);
                (*out)[meta::weight] +=
                    mul_poly1_polysecpar<P::secpar_v>(cache.u2_values[p], coeff);
            }
            (*out)[0] += out0_acc.reduced();
        }
        else
        {
            for (size_t p = 0; p < meta::choose2_count; ++p)
            {
            const size_t i = meta::choose2_subsets[p][0];
            const size_t j = meta::choose2_subsets[p][1];
                const field_t coeff = h0 + h[meta::single_ball_rank_by_index[i]] +
                                      h[meta::single_ball_rank_by_index[j]] +
                                      h[meta::pair_ball_ranks[p]];
                pair_mix[p] += coeff;
                single_mix[i] += mul_poly1_polysecpar<P::secpar_v>(cache.a_values[j], coeff);
                single_mix[j] += mul_poly1_polysecpar<P::secpar_v>(cache.a_values[i], coeff);
            (*out)[meta::weight] +=
                    mul_poly1_polysecpar<P::secpar_v>(cache.u2_values[p], coeff);
            }
        }
    }

    if constexpr (meta::weight == 3)
    {
        profile::scope timer(profile::OWF_BALL_STAGE0_TRIPLE);
        {
            std::array<field_t, meta::choose3_count> coeffs;
            {
                profile::scope coeff_timer(profile::OWF_BALL_STAGE0_TRIPLE_COEFF);
                for (size_t t = 0; t < meta::choose3_count; ++t)
                {
                    const size_t i = meta::choose3_subsets[t][0];
                    const size_t j = meta::choose3_subsets[t][1];
                    const size_t k = meta::choose3_subsets[t][2];
                    const size_t ij = meta::choose3_pair_ranks[t][0];
                    const size_t ik = meta::choose3_pair_ranks[t][1];
                    const size_t jk = meta::choose3_pair_ranks[t][2];
                    coeffs[t] = h0 + h[meta::single_ball_rank_by_index[i]] +
                                h[meta::single_ball_rank_by_index[j]] +
                                h[meta::single_ball_rank_by_index[k]] +
                                h[meta::pair_ball_ranks[ij]] +
                                h[meta::pair_ball_ranks[ik]] +
                                h[meta::pair_ball_ranks[jk]] +
                                h[meta::triple_ball_ranks[t]];
                }
            }
            {
                profile::scope mul_timer(profile::OWF_BALL_STAGE0_TRIPLE_MUL);
                polysecpar_dot_accumulator<P::secpar_v> out0_acc;
                for (size_t t = 0; t < meta::choose3_count; ++t)
                    out0_acc.add(coeffs[t], cache.t3_values[t]);
                (*out)[0] += out0_acc.reduced();
            }
            {
                profile::scope pair_timer(profile::OWF_BALL_STAGE0_TRIPLE_PAIR_MIX);
                for (size_t t = 0; t < meta::choose3_count; ++t)
                {
                    const size_t i = meta::choose3_subsets[t][0];
                    const size_t j = meta::choose3_subsets[t][1];
                    const size_t k = meta::choose3_subsets[t][2];
                    const size_t ij = meta::choose3_pair_ranks[t][0];
                    const size_t ik = meta::choose3_pair_ranks[t][1];
                    const size_t jk = meta::choose3_pair_ranks[t][2];
                    pair_mix[jk] += mul_poly1_polysecpar<P::secpar_v>(cache.a_values[i], coeffs[t]);
                    pair_mix[ik] += mul_poly1_polysecpar<P::secpar_v>(cache.a_values[j], coeffs[t]);
                    pair_mix[ij] += mul_poly1_polysecpar<P::secpar_v>(cache.a_values[k], coeffs[t]);
                }
            }
            {
                profile::scope single_timer(profile::OWF_BALL_STAGE0_TRIPLE_SINGLE_MIX);
                for (size_t t = 0; t < meta::choose3_count; ++t)
                {
                    const size_t i = meta::choose3_subsets[t][0];
                    const size_t j = meta::choose3_subsets[t][1];
                    const size_t k = meta::choose3_subsets[t][2];
                    const size_t ij = meta::choose3_pair_ranks[t][0];
                    const size_t ik = meta::choose3_pair_ranks[t][1];
                    const size_t jk = meta::choose3_pair_ranks[t][2];
                    single_mix[i] +=
                        mul_poly1_polysecpar<P::secpar_v>(cache.u2_values[jk], coeffs[t]);
                    single_mix[j] +=
                        mul_poly1_polysecpar<P::secpar_v>(cache.u2_values[ik], coeffs[t]);
                    single_mix[k] +=
                        mul_poly1_polysecpar<P::secpar_v>(cache.u2_values[ij], coeffs[t]);
                }
            }
            {
                profile::scope out3_timer(profile::OWF_BALL_STAGE0_TRIPLE_OUT3);
                for (size_t t = 0; t < meta::choose3_count; ++t)
                    (*out)[3] += mul_poly1_polysecpar<P::secpar_v>(cache.u3_values[t], coeffs[t]);
            }
        }
    }

    if constexpr (meta::weight == 3)
    {
        profile::scope timer(profile::OWF_BALL_STAGE0_PAIR_REDUCE);
        polysecpar_dot_accumulator<P::secpar_v> out1_acc;
        for (size_t p = 0; p < meta::choose2_count; ++p)
            out1_acc.add(cache.t2_values[p], pair_mix[p]);
        (*out)[1] += out1_acc.reduced();
    }
    {
        profile::scope timer(profile::OWF_BALL_STAGE0_SINGLE_REDUCE);
        polysecpar_dot_accumulator<P::secpar_v> single_acc;
        for (size_t i = 0; i < meta::block_size; ++i)
            single_acc.add(cache.c_values[i], single_mix[i]);
        (*out)[meta::weight - 1] += single_acc.reduced();
    }
}

template <typename P, typename QS>
inline void sydo_owf_fold_ball_block1_stage0_prover_from_h_batch(
    const sydo_owf_block_cache<P, QS, 1>& cache,
    const poly_secpar<P::secpar_v>* __restrict__ h_subvector,
    std::array<std::array<poly_secpar<P::secpar_v>, sydo_owf_block_meta<P, 1>::weight + 1>,
               sydo_owf_block_meta<P, 0>::domain_count>* __restrict__ out)
{
    static_assert(!QS::is_verifier, "Prover-only hamming-ball stage0 helper.");
    using meta0 = sydo_owf_block_meta<P, 0>;
    using meta1 = sydo_owf_block_meta<P, 1>;
    using field_t = poly_secpar<P::secpar_v>;
    static_assert(meta1::weight == 2 || meta1::weight == 3,
                  "This optimized helper expects block weight 2 or 3.");

    std::array<std::array<field_t, meta0::domain_count>, meta1::choose2_count> pair_mix{};
    std::array<std::array<field_t, meta0::domain_count>, meta1::block_size> single_mix{};
    {
        profile::scope timer(profile::OWF_BALL_STAGE0_ZERO);
        for (size_t r0 = 0; r0 < meta0::domain_count; ++r0)
        {
            for (auto& coeff : (*out)[r0])
                coeff = field_t::set_zero();
        }
        for (auto& row : pair_mix)
            for (auto& coeff : row)
                coeff = field_t::set_zero();
        for (auto& row : single_mix)
            for (auto& coeff : row)
                coeff = field_t::set_zero();
    }

    std::array<const field_t*, meta0::domain_count> h_rows{};
    for (size_t r0 = 0; r0 < meta0::domain_count; ++r0)
        h_rows[r0] = h_subvector + r0 * meta1::domain_count;
    auto h_at = [&](size_t r0, size_t r1) -> const field_t& { return h_rows[r0][r1]; };
    std::array<field_t, meta0::domain_count> h0_values{};
    {
        const size_t h0_rank = meta1::ball_rank_by_mask[0];
        for (size_t r0 = 0; r0 < meta0::domain_count; ++r0)
            h0_values[r0] = h_at(r0, h0_rank);
    }

    {
        profile::scope timer(profile::OWF_BALL_STAGE0_SINGLE);
        #pragma GCC unroll 4
        for (size_t r0 = 0; r0 < meta0::domain_count; ++r0)
            (*out)[r0][meta1::weight] += h0_values[r0];

        for (size_t i = 0; i < meta1::block_size; ++i)
        {
            const size_t rank_i = meta1::single_ball_rank_by_index[i];
            const clmul_block mask_i = detail::poly1_to_bit_mask(cache.a_values[i]);
            #pragma GCC unroll 4
            for (size_t r0 = 0; r0 < meta0::domain_count; ++r0)
            {
                const field_t coeff = h0_values[r0] + h_at(r0, rank_i);
                single_mix[i][r0] += coeff;
                add_mul_poly1_mask_polysecpar<P::secpar_v>((*out)[r0][meta1::weight],
                                                           mask_i, coeff);
            }
        }
    }

    {
        profile::scope timer(profile::OWF_BALL_STAGE0_PAIR);
        if constexpr (meta1::weight == 2)
        {
            std::array<polysecpar_dot_accumulator<P::secpar_v>, meta0::domain_count> out0_acc{};

            for (size_t p = 0; p < meta1::choose2_count; ++p)
            {
                const size_t i = meta1::choose2_subsets[p][0];
                const size_t j = meta1::choose2_subsets[p][1];
                const size_t rank_i = meta1::single_ball_rank_by_index[i];
                const size_t rank_j = meta1::single_ball_rank_by_index[j];
                const size_t rank_ij = meta1::pair_ball_ranks[p];
                const clmul_block mask_i = detail::poly1_to_bit_mask(cache.a_values[i]);
                const clmul_block mask_j = detail::poly1_to_bit_mask(cache.a_values[j]);
                const clmul_block mask_u = detail::poly1_to_bit_mask(cache.u2_values[p]);
                #pragma GCC unroll 4
                for (size_t r0 = 0; r0 < meta0::domain_count; ++r0)
                {
                    const field_t coeff = h0_values[r0] + h_at(r0, rank_i) +
                                          h_at(r0, rank_j) + h_at(r0, rank_ij);
                    out0_acc[r0].add(coeff, cache.t2_values[p]);
                    add_mul_poly1_mask_polysecpar<P::secpar_v>(single_mix[i][r0], mask_j,
                                                               coeff);
                    add_mul_poly1_mask_polysecpar<P::secpar_v>(single_mix[j][r0], mask_i,
                                                               coeff);
                    add_mul_poly1_mask_polysecpar<P::secpar_v>((*out)[r0][meta1::weight],
                                                               mask_u, coeff);
                }
            }
            #pragma GCC unroll 4
            for (size_t r0 = 0; r0 < meta0::domain_count; ++r0)
                (*out)[r0][0] += out0_acc[r0].reduced();
        }
        else
        {
            if constexpr (meta0::domain_count <= 4)
            {
                for (size_t p = 0; p < meta1::choose2_count; ++p)
                {
                    const size_t i = meta1::choose2_subsets[p][0];
                    const size_t j = meta1::choose2_subsets[p][1];
                    const size_t rank_i = meta1::single_ball_rank_by_index[i];
                    const size_t rank_j = meta1::single_ball_rank_by_index[j];
                    const size_t rank_ij = meta1::pair_ball_ranks[p];
                    const clmul_block mask_i = detail::poly1_to_bit_mask(cache.a_values[i]);
                    const clmul_block mask_j = detail::poly1_to_bit_mask(cache.a_values[j]);
                    const clmul_block mask_u = detail::poly1_to_bit_mask(cache.u2_values[p]);

                    const field_t coeff0 =
                        h0_values[0] + h_at(0, rank_i) + h_at(0, rank_j) + h_at(0, rank_ij);
                    pair_mix[p][0] += coeff0;
                    add_mul_poly1_mask_polysecpar<P::secpar_v>(single_mix[i][0], mask_j, coeff0);
                    add_mul_poly1_mask_polysecpar<P::secpar_v>(single_mix[j][0], mask_i, coeff0);
                    add_mul_poly1_mask_polysecpar<P::secpar_v>((*out)[0][meta1::weight], mask_u,
                                                               coeff0);

                    if constexpr (meta0::domain_count > 1)
                    {
                        const field_t coeff1 =
                            h0_values[1] + h_at(1, rank_i) + h_at(1, rank_j) + h_at(1, rank_ij);
                        pair_mix[p][1] += coeff1;
                        add_mul_poly1_mask_polysecpar<P::secpar_v>(single_mix[i][1], mask_j,
                                                                   coeff1);
                        add_mul_poly1_mask_polysecpar<P::secpar_v>(single_mix[j][1], mask_i,
                                                                   coeff1);
                        add_mul_poly1_mask_polysecpar<P::secpar_v>((*out)[1][meta1::weight],
                                                                   mask_u, coeff1);
                    }
                    if constexpr (meta0::domain_count > 2)
                    {
                        const field_t coeff2 =
                            h0_values[2] + h_at(2, rank_i) + h_at(2, rank_j) + h_at(2, rank_ij);
                        pair_mix[p][2] += coeff2;
                        add_mul_poly1_mask_polysecpar<P::secpar_v>(single_mix[i][2], mask_j,
                                                                   coeff2);
                        add_mul_poly1_mask_polysecpar<P::secpar_v>(single_mix[j][2], mask_i,
                                                                   coeff2);
                        add_mul_poly1_mask_polysecpar<P::secpar_v>((*out)[2][meta1::weight],
                                                                   mask_u, coeff2);
                    }
                    if constexpr (meta0::domain_count > 3)
                    {
                        const field_t coeff3 =
                            h0_values[3] + h_at(3, rank_i) + h_at(3, rank_j) + h_at(3, rank_ij);
                        pair_mix[p][3] += coeff3;
                        add_mul_poly1_mask_polysecpar<P::secpar_v>(single_mix[i][3], mask_j,
                                                                   coeff3);
                        add_mul_poly1_mask_polysecpar<P::secpar_v>(single_mix[j][3], mask_i,
                                                                   coeff3);
                        add_mul_poly1_mask_polysecpar<P::secpar_v>((*out)[3][meta1::weight],
                                                                   mask_u, coeff3);
                    }
                }
            }
            else
            {
                for (size_t p = 0; p < meta1::choose2_count; ++p)
                {
                    const size_t i = meta1::choose2_subsets[p][0];
                    const size_t j = meta1::choose2_subsets[p][1];
                    const size_t rank_i = meta1::single_ball_rank_by_index[i];
                    const size_t rank_j = meta1::single_ball_rank_by_index[j];
                    const size_t rank_ij = meta1::pair_ball_ranks[p];
                    const clmul_block mask_i = detail::poly1_to_bit_mask(cache.a_values[i]);
                    const clmul_block mask_j = detail::poly1_to_bit_mask(cache.a_values[j]);
                    const clmul_block mask_u = detail::poly1_to_bit_mask(cache.u2_values[p]);
                    #pragma GCC unroll 4
                    for (size_t r0 = 0; r0 < meta0::domain_count; ++r0)
                    {
                        const field_t coeff = h0_values[r0] + h_at(r0, rank_i) +
                                              h_at(r0, rank_j) + h_at(r0, rank_ij);
                        pair_mix[p][r0] += coeff;
                        add_mul_poly1_mask_polysecpar<P::secpar_v>(single_mix[i][r0], mask_j,
                                                                   coeff);
                        add_mul_poly1_mask_polysecpar<P::secpar_v>(single_mix[j][r0], mask_i,
                                                                   coeff);
                        add_mul_poly1_mask_polysecpar<P::secpar_v>((*out)[r0][meta1::weight],
                                                                   mask_u, coeff);
                    }
                }
            }
        }
    }

    if constexpr (meta1::weight == 3)
    {
        profile::scope timer(profile::OWF_BALL_STAGE0_TRIPLE);
        std::array<std::array<field_t, meta0::domain_count>, meta1::choose3_count> coeffs;
        {
            profile::scope coeff_timer(profile::OWF_BALL_STAGE0_TRIPLE_COEFF);
            for (size_t t = 0; t < meta1::choose3_count; ++t)
            {
                const size_t k = meta1::choose3_subsets[t][2];
                const size_t ij = meta1::choose3_pair_ranks[t][0];
                const size_t ik = meta1::choose3_pair_ranks[t][1];
                const size_t jk = meta1::choose3_pair_ranks[t][2];
                const size_t rank_k = meta1::single_ball_rank_by_index[k];
                const size_t rank_ik = meta1::pair_ball_ranks[ik];
                const size_t rank_jk = meta1::pair_ball_ranks[jk];
                const size_t rank_ijk = meta1::triple_ball_ranks[t];
                #pragma GCC unroll 4
                for (size_t r0 = 0; r0 < meta0::domain_count; ++r0)
                {
                    coeffs[t][r0] = pair_mix[ij][r0] + h_at(r0, rank_k) +
                                    h_at(r0, rank_ik) + h_at(r0, rank_jk) +
                                    h_at(r0, rank_ijk);
                }
            }
        }
        {
            profile::scope mul_timer(profile::OWF_BALL_STAGE0_TRIPLE_MUL);
            std::array<polysecpar_dot_accumulator<P::secpar_v>, meta0::domain_count> out0_acc{};
            for (size_t t = 0; t < meta1::choose3_count; ++t)
            {
                const auto& m = cache.t3_values[t];
                const auto m_sum = polysecpar_rhs_sum<P::secpar_v>(m);
                #pragma GCC unroll 4
                for (size_t r0 = 0; r0 < meta0::domain_count; ++r0)
                    out0_acc[r0].add_with_rhs_sum(coeffs[t][r0], m, m_sum);
            }
            #pragma GCC unroll 4
            for (size_t r0 = 0; r0 < meta0::domain_count; ++r0)
                (*out)[r0][0] += out0_acc[r0].reduced();
        }
        {
            profile::scope pair_timer(profile::OWF_BALL_STAGE0_TRIPLE_PAIR_MIX);
            for (size_t t = 0; t < meta1::choose3_count; ++t)
            {
                const size_t i = meta1::choose3_subsets[t][0];
                const size_t j = meta1::choose3_subsets[t][1];
                const size_t k = meta1::choose3_subsets[t][2];
                const size_t ij = meta1::choose3_pair_ranks[t][0];
                const size_t ik = meta1::choose3_pair_ranks[t][1];
                const size_t jk = meta1::choose3_pair_ranks[t][2];
                const clmul_block mask_i = detail::poly1_to_bit_mask(cache.a_values[i]);
                const clmul_block mask_j = detail::poly1_to_bit_mask(cache.a_values[j]);
                const clmul_block mask_k = detail::poly1_to_bit_mask(cache.a_values[k]);
                #pragma GCC unroll 4
                for (size_t r0 = 0; r0 < meta0::domain_count; ++r0)
                {
                    add_mul_poly1_mask_polysecpar<P::secpar_v>(pair_mix[jk][r0], mask_i,
                                                               coeffs[t][r0]);
                    add_mul_poly1_mask_polysecpar<P::secpar_v>(pair_mix[ik][r0], mask_j,
                                                               coeffs[t][r0]);
                    add_mul_poly1_mask_polysecpar<P::secpar_v>(pair_mix[ij][r0], mask_k,
                                                               coeffs[t][r0]);
                }
            }
        }
        {
            profile::scope single_timer(profile::OWF_BALL_STAGE0_TRIPLE_SINGLE_MIX);
            for (size_t t = 0; t < meta1::choose3_count; ++t)
            {
                const size_t i = meta1::choose3_subsets[t][0];
                const size_t j = meta1::choose3_subsets[t][1];
                const size_t k = meta1::choose3_subsets[t][2];
                const size_t ij = meta1::choose3_pair_ranks[t][0];
                const size_t ik = meta1::choose3_pair_ranks[t][1];
                const size_t jk = meta1::choose3_pair_ranks[t][2];
                const clmul_block mask_ij = detail::poly1_to_bit_mask(cache.u2_values[ij]);
                const clmul_block mask_ik = detail::poly1_to_bit_mask(cache.u2_values[ik]);
                const clmul_block mask_jk = detail::poly1_to_bit_mask(cache.u2_values[jk]);
                #pragma GCC unroll 4
                for (size_t r0 = 0; r0 < meta0::domain_count; ++r0)
                {
                    add_mul_poly1_mask_polysecpar<P::secpar_v>(single_mix[i][r0], mask_jk,
                                                               coeffs[t][r0]);
                    add_mul_poly1_mask_polysecpar<P::secpar_v>(single_mix[j][r0], mask_ik,
                                                               coeffs[t][r0]);
                    add_mul_poly1_mask_polysecpar<P::secpar_v>(single_mix[k][r0], mask_ij,
                                                               coeffs[t][r0]);
                }
            }
        }
        {
            profile::scope out3_timer(profile::OWF_BALL_STAGE0_TRIPLE_OUT3);
            for (size_t t = 0; t < meta1::choose3_count; ++t)
            {
                const clmul_block mask_t = detail::poly1_to_bit_mask(cache.u3_values[t]);
                for (size_t r0 = 0; r0 < meta0::domain_count; ++r0)
                    add_mul_poly1_mask_polysecpar<P::secpar_v>((*out)[r0][3], mask_t,
                                                               coeffs[t][r0]);
            }
        }
    }

    if constexpr (meta1::weight == 3)
    {
        profile::scope timer(profile::OWF_BALL_STAGE0_PAIR_REDUCE);
        if constexpr (meta0::domain_count <= 4)
        {
            polysecpar_dot_accumulator<P::secpar_v> acc0;
            polysecpar_dot_accumulator<P::secpar_v> acc1;
            polysecpar_dot_accumulator<P::secpar_v> acc2;
            polysecpar_dot_accumulator<P::secpar_v> acc3;
            for (size_t p = 0; p < meta1::choose2_count; ++p)
            {
                const auto& m = cache.t2_values[p];
                const auto m_sum = polysecpar_rhs_sum<P::secpar_v>(m);
                acc0.add_with_rhs_sum(pair_mix[p][0], m, m_sum);
                if constexpr (meta0::domain_count > 1)
                    acc1.add_with_rhs_sum(pair_mix[p][1], m, m_sum);
                if constexpr (meta0::domain_count > 2)
                    acc2.add_with_rhs_sum(pair_mix[p][2], m, m_sum);
                if constexpr (meta0::domain_count > 3)
                    acc3.add_with_rhs_sum(pair_mix[p][3], m, m_sum);
            }
            (*out)[0][1] += acc0.reduced();
            if constexpr (meta0::domain_count > 1)
                (*out)[1][1] += acc1.reduced();
            if constexpr (meta0::domain_count > 2)
                (*out)[2][1] += acc2.reduced();
            if constexpr (meta0::domain_count > 3)
                (*out)[3][1] += acc3.reduced();
        }
        else
        {
            std::array<polysecpar_dot_accumulator<P::secpar_v>, meta0::domain_count> out1_acc{};
            for (size_t p = 0; p < meta1::choose2_count; ++p)
            {
                const auto& m = cache.t2_values[p];
                const auto m_sum = polysecpar_rhs_sum<P::secpar_v>(m);
                for (size_t r0 = 0; r0 < meta0::domain_count; ++r0)
                    out1_acc[r0].add_with_rhs_sum(pair_mix[p][r0], m, m_sum);
            }
            for (size_t r0 = 0; r0 < meta0::domain_count; ++r0)
                (*out)[r0][1] += out1_acc[r0].reduced();
        }
    }
    {
        profile::scope timer(profile::OWF_BALL_STAGE0_SINGLE_REDUCE);
        if constexpr (meta0::domain_count <= 4)
        {
            polysecpar_dot_accumulator<P::secpar_v> acc0;
            polysecpar_dot_accumulator<P::secpar_v> acc1;
            polysecpar_dot_accumulator<P::secpar_v> acc2;
            polysecpar_dot_accumulator<P::secpar_v> acc3;
            for (size_t i = 0; i < meta1::block_size; ++i)
            {
                const auto& m = cache.c_values[i];
                const auto m_sum = polysecpar_rhs_sum<P::secpar_v>(m);
                acc0.add_with_rhs_sum(single_mix[i][0], m, m_sum);
                if constexpr (meta0::domain_count > 1)
                    acc1.add_with_rhs_sum(single_mix[i][1], m, m_sum);
                if constexpr (meta0::domain_count > 2)
                    acc2.add_with_rhs_sum(single_mix[i][2], m, m_sum);
                if constexpr (meta0::domain_count > 3)
                    acc3.add_with_rhs_sum(single_mix[i][3], m, m_sum);
            }
            (*out)[0][meta1::weight - 1] += acc0.reduced();
            if constexpr (meta0::domain_count > 1)
                (*out)[1][meta1::weight - 1] += acc1.reduced();
            if constexpr (meta0::domain_count > 2)
                (*out)[2][meta1::weight - 1] += acc2.reduced();
            if constexpr (meta0::domain_count > 3)
                (*out)[3][meta1::weight - 1] += acc3.reduced();
        }
        else
        {
            std::array<polysecpar_dot_accumulator<P::secpar_v>, meta0::domain_count> single_acc{};
            for (size_t i = 0; i < meta1::block_size; ++i)
            {
                const auto& m = cache.c_values[i];
                const auto m_sum = polysecpar_rhs_sum<P::secpar_v>(m);
                for (size_t r0 = 0; r0 < meta0::domain_count; ++r0)
                    single_acc[r0].add_with_rhs_sum(single_mix[i][r0], m, m_sum);
            }
            for (size_t r0 = 0; r0 < meta0::domain_count; ++r0)
                (*out)[r0][meta1::weight - 1] +=
                    single_acc[r0].reduced();
        }
    }
}

template <typename P, typename QS, typename CacheTuple>
inline void sydo_owf_accumulate_ball_direct_prover(
    const CacheTuple& block_caches, size_t subvec_i, const poly_secpar<P::secpar_v>* folded_h_row,
    size_t folded_h_row_len, quicksilver_gfsecpar<QS, QS::max_degree>& folded_constraint)
{
    static_assert(!QS::is_verifier, "Prover-only ball folded IP path.");
    static_assert(P::elementary_encoding_v == elementary_encoding::hamming_ball);
    static_assert(P::num_var_blocks == 2, "Direct hamming-ball folded IP currently supports two blocks.");
    static_assert(QS::term_v == quicksilver_term::leading,
                  "Direct hamming-ball folded IP currently expects leading QS terms.");
    using field_t = poly_secpar<P::secpar_v>;
    using wide_field_t = poly_2secpar<P::secpar_v>;
    using meta0 = sydo_owf_block_meta<P, 0>;
    using meta1 = sydo_owf_block_meta<P, 1>;
    static_assert(sydo_owf_total_weight<P>() <= QS::max_degree);
    static_assert(meta0::weight == 1, "This optimized helper expects block0 weight 1.");

    static thread_local std::array<field_t, sydo_owf_terms_per_subvector<P>()>
        padded_h_subvector;
    const field_t* h_subvector = nullptr;
    {
        profile::scope timer(profile::OWF_PADDED_H_SUBVECTOR);
        h_subvector = sydo_owf_padded_h_subvector<P>(
            folded_h_row, folded_h_row_len, subvec_i, padded_h_subvector.data());
    }

    static thread_local std::array<std::array<field_t, meta1::weight + 1>, meta0::domain_count>
        stage0;
    {
        profile::scope timer(profile::OWF_FOLDED_IP_STAGE0);
        const auto& block1 = std::get<1>(block_caches);
        sydo_owf_fold_ball_block1_stage0_prover_from_h_batch<P, QS>(block1, h_subvector,
                                                                       &stage0);
    }

    {
        profile::scope timer(profile::OWF_FOLDED_IP_REST);
        const auto& block0 = std::get<0>(block_caches);
        for (size_t t = 0; t < meta1::weight; ++t)
            folded_constraint.mac.coeffs[1 + t] += stage0[0][t];
        field_t selected_h = stage0[0][meta1::weight];

        std::array<wide_field_t, meta1::weight + 1> rest_acc{};
        for (auto& acc : rest_acc)
            acc = wide_field_t::set_zero();

        for (size_t i = 0; i < meta0::block_size; ++i)
        {
            const size_t rank = meta0::ball_rank_by_mask[size_t{1} << i];
            for (size_t t = 0; t <= meta1::weight; ++t)
            {
                field_t coeff = stage0[0][t] + stage0[rank][t];
                rest_acc[t] += mul_polysecpar<P::secpar_v>(coeff, block0.c_values[i]);
                if (t < meta1::weight)
                    folded_constraint.mac.coeffs[1 + t] +=
                        mul_poly1_polysecpar<P::secpar_v>(block0.a_values[i], coeff);
                else
                    add_mul_poly1_mask_polysecpar<P::secpar_v>(
                        selected_h, detail::poly1_to_bit_mask(block0.a_values[i]), coeff);
            }
        }
        for (size_t t = 0; t <= meta1::weight; ++t)
            folded_constraint.mac.coeffs[t] += reduce_poly2secpar<P::secpar_v>(rest_acc[t]);

        folded_constraint.value() += selected_h;
    }
}

template <typename P, typename QS, size_t block_idx, size_t cur_degree, size_t coeff_stride,
          size_t row_stride>
inline void sydo_owf_contract_block_ball_sum_batch_add(
    const sydo_owf_block_cache<P, QS, block_idx>& cache,
    const poly_secpar<P::secpar_v>* coeff_base, poly_secpar<P::secpar_v>* out)
{
    static_assert(!QS::is_verifier, "Prover-only ball contraction helper.");
    using meta = sydo_owf_block_meta<P, block_idx>;
    using field_t = poly_secpar<P::secpar_v>;

    for (size_t monomial_idx = 0; monomial_idx < meta::domain_count; ++monomial_idx)
    {
        const size_t monomial_mask = meta::ball_masks[monomial_idx];
        std::array<field_t, cur_degree + 1> coeffs{};
        for (auto& coeff : coeffs)
            coeff = field_t::set_zero();

        for (size_t point_idx = 0; point_idx < meta::domain_count; ++point_idx)
        {
            const size_t point_mask = meta::ball_masks[point_idx];
            if ((point_mask & ~monomial_mask) != 0)
                continue;
            for (size_t t = 0; t <= cur_degree; ++t)
                coeffs[t] += coeff_base[t * row_stride + point_idx * coeff_stride];
        }

        sydo_owf_add_ball_monomial_prover<P, QS, block_idx, cur_degree>(cache, monomial_mask,
                                                                          coeffs, out);
    }
}

template <typename P, typename QS, size_t block_idx, size_t cur_degree, size_t cur_size>
inline void sydo_owf_fold_stage_prover_ct(const sydo_owf_block_cache<P, QS, block_idx>& cache,
                                            const poly_secpar<P::secpar_v>* cur_table,
                                            poly_secpar<P::secpar_v>* next_table)
{
    static_assert(!QS::is_verifier, "Prover-only folded stage.");
    using meta = sydo_owf_block_meta<P, block_idx>;
    using field_t = poly_secpar<P::secpar_v>;

    constexpr size_t next_size = cur_size / meta::domain_count;
    const size_t next_degree = cur_degree + meta::weight;
    static_assert(cur_size == meta::domain_count * next_size, "invalid stage table geometry");

    for (size_t suffix_idx = 0; suffix_idx < next_size; ++suffix_idx)
    {
        std::array<field_t, next_degree + 1> acc{};
        for (size_t d = 0; d <= next_degree; ++d)
            acc[d] = field_t::set_zero();

        const field_t* coeff_base = cur_table + suffix_idx;
        if constexpr (P::elementary_encoding_v == elementary_encoding::hamming_sphere)
        {
            sydo_owf_contract_block_weight_sum_batch_add<P, QS, block_idx, cur_degree, next_size,
                                                           cur_size>(cache, coeff_base,
                                                                      acc.data());
        }
        else
        {
            sydo_owf_contract_block_ball_sum_batch_add<P, QS, block_idx, cur_degree, next_size,
                                                         cur_size>(cache, coeff_base, acc.data());
        }
        for (size_t d = 0; d <= next_degree; ++d)
            next_table[d * next_size + suffix_idx] = acc[d];
    }
}

template <typename P, typename QS, typename CacheTuple>
inline void sydo_owf_fold_stage0_prover(const CacheTuple& block_caches,
                                          const poly_secpar<P::secpar_v>* folded_h_subvector,
                                          poly_secpar<P::secpar_v>* next_table)
{
    static_assert(!QS::is_verifier, "Prover-only folded IP path.");
    constexpr size_t block_idx = P::FOLD_ORDER[0];
    using meta = sydo_owf_block_meta<P, block_idx>;
    using field_t = poly_secpar<P::secpar_v>;
    constexpr size_t stage0_input_size = sydo_owf_stage_input_size<P, 0>();
    constexpr auto stage0_suffix_offsets = sydo_owf_stage0_suffix_base_offsets<P>();
    constexpr auto natural_strides = sydo_owf_tuple_strides<P>();
    constexpr size_t coeff_stride = natural_strides[block_idx];
    constexpr size_t next_size = sydo_owf_stage_output_size<P, 0>();

    static_assert(stage0_input_size == sydo_owf_terms_per_subvector<P>(),
                  "stage 0 input size mismatch");
    static_assert(next_size == sydo_owf_stage_output_size<P, 0>(),
                  "stage 0 output size mismatch");
    static_assert(sydo_owf_stage_prefix_degree<P, 0>() == 0, "stage 0 prefix degree must be 0");

    const auto& cache = std::get<block_idx>(block_caches);
    for (size_t suffix_idx = 0; suffix_idx < next_size; ++suffix_idx)
    {
        std::array<field_t, meta::weight + 1> acc{};
        for (size_t d = 0; d <= meta::weight; ++d)
            acc[d] = field_t::set_zero();

        const field_t* coeff_base = folded_h_subvector + stage0_suffix_offsets[suffix_idx];
        if constexpr (P::elementary_encoding_v == elementary_encoding::hamming_sphere)
        {
            sydo_owf_contract_block_weight_sum_batch_add<P, QS, block_idx, 0, coeff_stride,
                                                           stage0_input_size>(cache, coeff_base,
                                                                              acc.data());
        }
        else
        {
            sydo_owf_contract_block_ball_sum_batch_add<P, QS, block_idx, 0, coeff_stride,
                                                         stage0_input_size>(cache, coeff_base,
                                                                            acc.data());
        }
        for (size_t d = 0; d <= meta::weight; ++d)
            next_table[d * next_size + suffix_idx] = acc[d];
    }
}

template <typename P, typename QS, typename CacheTuple, size_t stage, size_t cur_degree,
          size_t cur_size>
inline void sydo_owf_fold_remaining_stages_prover(const CacheTuple& block_caches,
                                                    poly_secpar<P::secpar_v>*& cur_table,
                                                    poly_secpar<P::secpar_v>*& next_table)
{
    if constexpr (stage < P::num_var_blocks)
    {
        constexpr size_t block_idx = P::FOLD_ORDER[stage];
        using meta = sydo_owf_block_meta<P, block_idx>;
        constexpr size_t next_size = cur_size / meta::domain_count;
        constexpr size_t next_degree = cur_degree + meta::weight;
        static_assert(next_size == sydo_owf_stage_output_size<P, stage>(), "stage size mismatch");
        static_assert(cur_degree == sydo_owf_stage_prefix_degree<P, stage>(),
                      "stage prefix degree mismatch");

        sydo_owf_fold_stage_prover_ct<P, QS, block_idx, cur_degree, cur_size>(
            std::get<block_idx>(block_caches), cur_table, next_table);
        std::swap(cur_table, next_table);

        sydo_owf_fold_remaining_stages_prover<P, QS, CacheTuple, stage + 1, next_degree,
                                                next_size>(block_caches, cur_table, next_table);
    }
}

template <typename P, typename QS, typename CacheTuple>
inline void sydo_owf_accumulate_folded_ip_prover(
    const CacheTuple& block_caches, size_t subvec_i, const poly_secpar<P::secpar_v>* folded_h_row,
    size_t folded_h_row_len, quicksilver_gfsecpar<QS, QS::max_degree>& folded_constraint)
{
    static_assert(!QS::is_verifier, "Prover-only folded IP path.");
    using field_t = poly_secpar<P::secpar_v>;
    constexpr size_t terms_per_subvector = sydo_owf_terms_per_subvector<P>();
    constexpr size_t folded_degree_slots = sydo_owf_total_weight<P>() + 1;
    constexpr size_t first_block = P::FOLD_ORDER[0];
    constexpr size_t first_weight = sydo_owf_block_meta<P, first_block>::weight;
    constexpr size_t size_after_stage0 = sydo_owf_stage_output_size<P, 0>();

    static_assert(folded_degree_slots <= (QS::max_degree + 1),
                  "Folded degree exceeds QS degree slots.");
    SYDO_ASSERT(folded_h_row_len >= P::rsd_n);

    static thread_local std::array<field_t, terms_per_subvector> padded_h_subvector;
    const field_t* h_subvector = nullptr;
    {
        profile::scope timer(profile::OWF_PADDED_H_SUBVECTOR);
        h_subvector = sydo_owf_padded_h_subvector<P>(
            folded_h_row, folded_h_row_len, subvec_i, padded_h_subvector.data());
    }
    static thread_local std::array<field_t, folded_degree_slots * terms_per_subvector> table_a;
    static thread_local std::array<field_t, folded_degree_slots * terms_per_subvector> table_b;
    field_t* cur_table = table_a.data();
    field_t* next_table = table_b.data();

    {
        profile::scope timer(profile::OWF_FOLDED_IP_STAGE0);
        sydo_owf_fold_stage0_prover<P, QS>(block_caches, h_subvector, cur_table);
    }
    {
        profile::scope timer(profile::OWF_FOLDED_IP_REST);
        sydo_owf_fold_remaining_stages_prover<P, QS, CacheTuple, 1, first_weight,
                                               size_after_stage0>(block_caches, cur_table,
                                                                  next_table);
    }

    constexpr size_t final_degree = sydo_owf_total_weight<P>();
    constexpr size_t final_size = sydo_owf_stage_output_size<P, P::num_var_blocks - 1>();
    static_assert(final_size == 1, "final folded table must have one column");
    const size_t shift = QS::max_degree - final_degree;
    if constexpr (P::elementary_encoding_v == elementary_encoding::hamming_ball)
    {
        folded_constraint.value() +=
            sydo_owf_selected_ball_h_value_prover<P, QS>(block_caches, h_subvector);
    }
    {
        profile::scope timer(profile::OWF_FOLDED_CONSTRAINT_ACCUMULATE);
        for (size_t d = 0; d <= final_degree; ++d)
        {
            const size_t coeff_idx =
                (QS::term_v == quicksilver_term::constant) ? (final_degree - d) : (shift + d);
            if constexpr (P::elementary_encoding_v == elementary_encoding::hamming_ball)
            {
                constexpr size_t value_coeff_idx =
                    (QS::term_v == quicksilver_term::constant) ? 0 : QS::max_degree;
                if (coeff_idx == value_coeff_idx)
                    continue;
            }
            folded_constraint.mac.coeffs[coeff_idx] += cur_table[d];
        }
    }
}

template <typename P, typename QS, size_t block_idx>
inline poly_secpar<P::secpar_v> sydo_owf_ball_monomial_eval_verifier(
    const sydo_owf_block_cache<P, QS, block_idx>& cache, size_t mask)
{
    static_assert(QS::is_verifier, "Verifier-only ball monomial helper.");
    using meta = sydo_owf_block_meta<P, block_idx>;
    const size_t weight = sydo_owf_mask_weight(mask);

    if (weight == 0)
        return poly_secpar<P::secpar_v>::from_1(poly1::set_one());

    std::array<size_t, 3> idx{};
    size_t pos = 0;
    for (size_t i = 0; i < meta::block_size; ++i)
    {
        if (((mask >> i) & 1u) != 0)
            idx[pos++] = i;
    }

    if (weight == 1)
        return cache.bits[idx[0]].mac;
    if constexpr (meta::weight >= 2)
    {
        if (weight == 2)
            return cache.pair_products[meta::pair_rank[idx[0]][idx[1]]].mac;
    }
    if constexpr (meta::weight >= 3)
    {
        return cache.triple_products[meta::triple_rank[idx[0]][idx[1]][idx[2]]].mac;
    }
    SYDO_ASSERT(false);
    return poly_secpar<P::secpar_v>::set_zero();
}

template <typename P, typename QS>
inline poly_secpar<P::secpar_v> sydo_owf_fold_ball_block1_stage0_verifier(
    QS* state, const sydo_owf_block_cache<P, QS, 1>& cache,
    const std::array<poly_secpar<P::secpar_v>, sydo_owf_block_meta<P, 1>::domain_count>& coeffs)
{
    static_assert(QS::is_verifier, "Verifier-only hamming-ball stage0 helper.");
    using meta = sydo_owf_block_meta<P, 1>;
    using field_t = poly_secpar<P::secpar_v>;
    static_assert(meta::weight == 2 || meta::weight == 3,
                  "This optimized helper expects block weight 2 or 3.");

    field_t acc = coeffs[meta::ball_rank_by_mask[0]];
    acc = mul_polysecpar_reduce<P::secpar_v>(acc, state->delta_powers[meta::weight - 1]);

    for (size_t i = 0; i < meta::block_size; ++i)
    {
        const size_t mask = size_t{1} << i;
        field_t term = mul_polysecpar_reduce<P::secpar_v>(
            coeffs[meta::ball_rank_by_mask[mask]], cache.bits[i].mac);
        term = mul_polysecpar_reduce<P::secpar_v>(term, state->delta_powers[meta::weight - 2]);
        acc += term;
    }

    for (size_t p = 0; p < meta::choose2_count; ++p)
    {
        const size_t i = meta::choose2_subsets[p][0];
        const size_t j = meta::choose2_subsets[p][1];
        const size_t mask = (size_t{1} << i) | (size_t{1} << j);
        field_t term = mul_polysecpar_reduce<P::secpar_v>(
            coeffs[meta::ball_rank_by_mask[mask]], cache.pair_products[p].mac);
        if constexpr (meta::weight == 3)
            term = mul_polysecpar_reduce<P::secpar_v>(term, state->delta_powers[0]);
        acc += term;
    }

    if constexpr (meta::weight == 3)
    {
        for (size_t t = 0; t < meta::choose3_count; ++t)
        {
            const size_t i = meta::choose3_subsets[t][0];
            const size_t j = meta::choose3_subsets[t][1];
            const size_t k = meta::choose3_subsets[t][2];
            const size_t mask = (size_t{1} << i) | (size_t{1} << j) | (size_t{1} << k);
            acc += mul_polysecpar_reduce<P::secpar_v>(coeffs[meta::ball_rank_by_mask[mask]],
                                                      cache.triple_products[t].mac);
        }
    }
    return acc;
}

template <typename P, typename QS>
inline poly_secpar<P::secpar_v> sydo_owf_fold_ball_block1_stage0_verifier_from_h(
    QS* state, const sydo_owf_block_cache<P, QS, 1>& cache,
    const poly_secpar<P::secpar_v>* h,
    const std::array<poly_secpar<P::secpar_v>, sydo_owf_block_meta<P, 1>::block_size>&
        scaled_bits,
    const std::array<poly_secpar<P::secpar_v>, sydo_owf_block_meta<P, 1>::choose2_count>&
        scaled_pairs)
{
    static_assert(QS::is_verifier, "Verifier-only hamming-ball stage0 helper.");
    using meta = sydo_owf_block_meta<P, 1>;
    using field_t = poly_secpar<P::secpar_v>;
    static_assert(meta::weight == 2 || meta::weight == 3,
                  "This optimized helper expects block weight 2 or 3.");

    const field_t& h0 = h[meta::ball_rank_by_mask[0]];
    polysecpar_dot_accumulator<P::secpar_v> acc;
    acc.add(h0, state->delta_powers[meta::weight - 1]);

    for (size_t i = 0; i < meta::block_size; ++i)
    {
        const field_t coeff = h0 + h[meta::single_ball_rank_by_index[i]];
        acc.add(coeff, scaled_bits[i]);
    }

    for (size_t p = 0; p < meta::choose2_count; ++p)
    {
        const size_t i = meta::choose2_subsets[p][0];
        const size_t j = meta::choose2_subsets[p][1];
        const field_t coeff = h0 + h[meta::single_ball_rank_by_index[i]] +
                              h[meta::single_ball_rank_by_index[j]] +
                              h[meta::pair_ball_ranks[p]];
        if constexpr (meta::weight == 2)
            acc.add(coeff, cache.pair_products[p].mac);
        else
            acc.add(coeff, scaled_pairs[p]);
    }

    if constexpr (meta::weight == 3)
    {
        for (size_t t = 0; t < meta::choose3_count; ++t)
        {
            const size_t i = meta::choose3_subsets[t][0];
            const size_t j = meta::choose3_subsets[t][1];
            const size_t k = meta::choose3_subsets[t][2];
            const size_t ij = meta::choose3_pair_ranks[t][0];
            const size_t ik = meta::choose3_pair_ranks[t][1];
            const size_t jk = meta::choose3_pair_ranks[t][2];
            const field_t coeff = h0 + h[meta::single_ball_rank_by_index[i]] +
                                  h[meta::single_ball_rank_by_index[j]] +
                                  h[meta::single_ball_rank_by_index[k]] +
                                  h[meta::pair_ball_ranks[ij]] +
                                  h[meta::pair_ball_ranks[ik]] +
                                  h[meta::pair_ball_ranks[jk]] +
                                  h[meta::triple_ball_ranks[t]];
            acc.add(coeff, cache.triple_products[t].mac);
        }
    }
    return acc.reduced();
}

template <typename P, typename QS, typename CacheTuple>
inline void sydo_owf_accumulate_ball_direct_verifier(
    QS* state, const CacheTuple& block_caches, size_t subvec_i,
    const poly_secpar<P::secpar_v>* folded_h_row, size_t folded_h_row_len,
    quicksilver_gfsecpar<QS, QS::max_degree>& folded_constraint)
{
    static_assert(QS::is_verifier, "Verifier-only ball folded IP path.");
    static_assert(P::elementary_encoding_v == elementary_encoding::hamming_ball);
    static_assert(P::num_var_blocks == 2, "Direct hamming-ball folded IP currently supports two blocks.");
    static_assert(QS::term_v == quicksilver_term::leading,
                  "Direct hamming-ball folded IP currently expects leading QS terms.");
    using field_t = poly_secpar<P::secpar_v>;
    using wide_field_t = poly_2secpar<P::secpar_v>;
    using meta0 = sydo_owf_block_meta<P, 0>;
    using meta1 = sydo_owf_block_meta<P, 1>;

    static thread_local std::array<field_t, sydo_owf_terms_per_subvector<P>()>
        padded_h_subvector;
    const field_t* h_subvector = sydo_owf_padded_h_subvector<P>(
        folded_h_row, folded_h_row_len, subvec_i, padded_h_subvector.data());

    static thread_local std::array<field_t, meta0::domain_count> stage0;
    {
        profile::scope timer(profile::OWF_FOLDED_IP_STAGE0);
        const auto& block1 = std::get<1>(block_caches);
        std::array<field_t, meta1::block_size> scaled_bits{};
        std::array<field_t, meta1::choose2_count> scaled_pairs{};
        for (size_t i = 0; i < meta1::block_size; ++i)
            scaled_bits[i] = mul_polysecpar_reduce<P::secpar_v>(block1.bits[i].mac,
                                                                 state->delta_powers[meta1::weight - 2]);
        if constexpr (meta1::weight == 3)
        {
            for (size_t p = 0; p < meta1::choose2_count; ++p)
                scaled_pairs[p] = mul_polysecpar_reduce<P::secpar_v>(block1.pair_products[p].mac,
                                                                      state->delta_powers[0]);
        }
        for (size_t r0 = 0; r0 < meta0::domain_count; ++r0)
        {
            const field_t* h = h_subvector + r0 * meta1::domain_count;
            stage0[r0] = sydo_owf_fold_ball_block1_stage0_verifier_from_h<P, QS>(state, block1,
                                                                                   h, scaled_bits,
                                                                                   scaled_pairs);
        }
    }

    {
        profile::scope timer(profile::OWF_FOLDED_IP_REST);
        const auto& block0 = std::get<0>(block_caches);
        wide_field_t rest_acc = mul_polysecpar<P::secpar_v>(stage0[0], state->delta_powers[0]);
        for (size_t i = 0; i < meta0::block_size; ++i)
        {
            const size_t rank = meta0::ball_rank_by_mask[size_t{1} << i];
            const field_t coeff = stage0[0] + stage0[rank];
            rest_acc += mul_polysecpar<P::secpar_v>(coeff, block0.bits[i].mac);
        }
        folded_constraint.mac += reduce_poly2secpar<P::secpar_v>(rest_acc);
    }
}

template <typename P, typename QS, size_t block_idx>
inline poly1 sydo_owf_ball_rank_value_mask_prover(
    const sydo_owf_block_cache<P, QS, block_idx>& cache, size_t rank)
{
    static_assert(!QS::is_verifier, "Prover-only ball rank helper.");
    using meta = sydo_owf_block_meta<P, block_idx>;

    const size_t rank_mask = meta::ball_masks[rank];
    poly1 selected = poly1::set_one();
    for (size_t i = 0; i < meta::block_size; ++i)
    {
        const poly1 bit = cache.a_values[i];
        const poly1 term = ((rank_mask >> i) & 1u) != 0 ? bit : (bit + poly1::set_one());
        selected = mul_poly1(selected, term);
    }
    return selected;
}

template <typename P, typename QS, typename CacheTuple>
inline poly_secpar<P::secpar_v> sydo_owf_selected_ball_h_value_prover(
    const CacheTuple& block_caches, const poly_secpar<P::secpar_v>* h_subvector)
{
    static_assert(!QS::is_verifier, "Prover-only ball value helper.");
    static_assert(P::elementary_encoding_v == elementary_encoding::hamming_ball);
    static_assert(P::num_var_blocks >= 1 && P::num_var_blocks <= 4,
                  "ball value helper supports up to four variable blocks");

    using field_t = poly_secpar<P::secpar_v>;
    constexpr auto natural_strides = sydo_owf_tuple_strides<P>();
    field_t acc = field_t::set_zero();

    if constexpr (P::num_var_blocks == 1)
    {
        using meta0 = sydo_owf_block_meta<P, 0>;
        const auto& block0 = std::get<0>(block_caches);
        for (size_t r0 = 0; r0 < meta0::domain_count; ++r0)
        {
            const poly1 selected =
                sydo_owf_ball_rank_value_mask_prover<P, QS, 0>(block0, r0);
            add_mul_poly1_mask_polysecpar<P::secpar_v>(
                acc, detail::poly1_to_bit_mask(selected), h_subvector[r0 * natural_strides[0]]);
        }
    }
    else if constexpr (P::num_var_blocks == 2)
    {
        using meta0 = sydo_owf_block_meta<P, 0>;
        using meta1 = sydo_owf_block_meta<P, 1>;
        const auto& block0 = std::get<0>(block_caches);
        const auto& block1 = std::get<1>(block_caches);
        std::array<poly1, meta0::domain_count> selected0;
        std::array<poly1, meta1::domain_count> selected1;

        for (size_t r0 = 0; r0 < meta0::domain_count; ++r0)
            selected0[r0] = sydo_owf_ball_rank_value_mask_prover<P, QS, 0>(block0, r0);
        for (size_t r1 = 0; r1 < meta1::domain_count; ++r1)
            selected1[r1] = sydo_owf_ball_rank_value_mask_prover<P, QS, 1>(block1, r1);

        for (size_t r0 = 0; r0 < meta0::domain_count; ++r0)
        {
            const size_t base0 = r0 * natural_strides[0];
            for (size_t r1 = 0; r1 < meta1::domain_count; ++r1)
            {
                const poly1 selected = mul_poly1(selected0[r0], selected1[r1]);
                add_mul_poly1_mask_polysecpar<P::secpar_v>(
                    acc, detail::poly1_to_bit_mask(selected),
                    h_subvector[base0 + r1 * natural_strides[1]]);
            }
        }
    }
    else if constexpr (P::num_var_blocks == 3)
    {
        using meta0 = sydo_owf_block_meta<P, 0>;
        using meta1 = sydo_owf_block_meta<P, 1>;
        using meta2 = sydo_owf_block_meta<P, 2>;
        const auto& block0 = std::get<0>(block_caches);
        const auto& block1 = std::get<1>(block_caches);
        const auto& block2 = std::get<2>(block_caches);
        std::array<poly1, meta0::domain_count> selected0;
        std::array<poly1, meta1::domain_count> selected1;
        std::array<poly1, meta2::domain_count> selected2;

        for (size_t r0 = 0; r0 < meta0::domain_count; ++r0)
            selected0[r0] = sydo_owf_ball_rank_value_mask_prover<P, QS, 0>(block0, r0);
        for (size_t r1 = 0; r1 < meta1::domain_count; ++r1)
            selected1[r1] = sydo_owf_ball_rank_value_mask_prover<P, QS, 1>(block1, r1);
        for (size_t r2 = 0; r2 < meta2::domain_count; ++r2)
            selected2[r2] = sydo_owf_ball_rank_value_mask_prover<P, QS, 2>(block2, r2);

        for (size_t r0 = 0; r0 < meta0::domain_count; ++r0)
        {
            const size_t base0 = r0 * natural_strides[0];
            for (size_t r1 = 0; r1 < meta1::domain_count; ++r1)
            {
                const poly1 selected01 = mul_poly1(selected0[r0], selected1[r1]);
                const size_t base01 = base0 + r1 * natural_strides[1];
                for (size_t r2 = 0; r2 < meta2::domain_count; ++r2)
                {
                    const poly1 selected = mul_poly1(selected01, selected2[r2]);
                    add_mul_poly1_mask_polysecpar<P::secpar_v>(
                        acc, detail::poly1_to_bit_mask(selected),
                        h_subvector[base01 + r2 * natural_strides[2]]);
                }
            }
        }
    }
    else if constexpr (P::num_var_blocks == 4)
    {
        using meta0 = sydo_owf_block_meta<P, 0>;
        using meta1 = sydo_owf_block_meta<P, 1>;
        using meta2 = sydo_owf_block_meta<P, 2>;
        using meta3 = sydo_owf_block_meta<P, 3>;
        const auto& block0 = std::get<0>(block_caches);
        const auto& block1 = std::get<1>(block_caches);
        const auto& block2 = std::get<2>(block_caches);
        const auto& block3 = std::get<3>(block_caches);
        std::array<poly1, meta0::domain_count> selected0;
        std::array<poly1, meta1::domain_count> selected1;
        std::array<poly1, meta2::domain_count> selected2;
        std::array<poly1, meta3::domain_count> selected3;

        for (size_t r0 = 0; r0 < meta0::domain_count; ++r0)
            selected0[r0] = sydo_owf_ball_rank_value_mask_prover<P, QS, 0>(block0, r0);
        for (size_t r1 = 0; r1 < meta1::domain_count; ++r1)
            selected1[r1] = sydo_owf_ball_rank_value_mask_prover<P, QS, 1>(block1, r1);
        for (size_t r2 = 0; r2 < meta2::domain_count; ++r2)
            selected2[r2] = sydo_owf_ball_rank_value_mask_prover<P, QS, 2>(block2, r2);
        for (size_t r3 = 0; r3 < meta3::domain_count; ++r3)
            selected3[r3] = sydo_owf_ball_rank_value_mask_prover<P, QS, 3>(block3, r3);

        for (size_t r0 = 0; r0 < meta0::domain_count; ++r0)
        {
            const size_t base0 = r0 * natural_strides[0];
            for (size_t r1 = 0; r1 < meta1::domain_count; ++r1)
            {
                const poly1 selected01 = mul_poly1(selected0[r0], selected1[r1]);
                const size_t base01 = base0 + r1 * natural_strides[1];
                for (size_t r2 = 0; r2 < meta2::domain_count; ++r2)
                {
                    const poly1 selected012 = mul_poly1(selected01, selected2[r2]);
                    const size_t base012 = base01 + r2 * natural_strides[2];
                    for (size_t r3 = 0; r3 < meta3::domain_count; ++r3)
                    {
                        const poly1 selected = mul_poly1(selected012, selected3[r3]);
                        add_mul_poly1_mask_polysecpar<P::secpar_v>(
                            acc, detail::poly1_to_bit_mask(selected),
                            h_subvector[base012 + r3 * natural_strides[3]]);
                    }
                }
            }
        }
    }

    return acc;
}

template <typename P, typename QS, size_t block_idx, size_t coeff_stride>
inline poly_secpar<P::secpar_v> sydo_owf_contract_block_ball_eval_verifier(
    const sydo_owf_block_cache<P, QS, block_idx>& cache,
    const poly_secpar<P::secpar_v>* coeff_base)
{
    static_assert(QS::is_verifier, "Verifier-only ball contraction helper.");
    using meta = sydo_owf_block_meta<P, block_idx>;
    using field_t = poly_secpar<P::secpar_v>;

    field_t acc = field_t::set_zero();
    for (size_t monomial_idx = 0; monomial_idx < meta::domain_count; ++monomial_idx)
    {
        const size_t monomial_mask = meta::ball_masks[monomial_idx];
        field_t coeff = field_t::set_zero();
        for (size_t point_idx = 0; point_idx < meta::domain_count; ++point_idx)
        {
            const size_t point_mask = meta::ball_masks[point_idx];
            if ((point_mask & ~monomial_mask) == 0)
                coeff += coeff_base[point_idx * coeff_stride];
        }

        acc += mul_polysecpar_reduce<P::secpar_v>(
            coeff, sydo_owf_ball_monomial_eval_verifier<P, QS, block_idx>(cache,
                                                                            monomial_mask));
    }
    return acc;
}

template <typename P, typename QS, typename CacheTuple>
inline void sydo_owf_fold_stage0_verifier(const CacheTuple& block_caches,
                                            const poly_secpar<P::secpar_v>* folded_h_subvector,
                                            poly_secpar<P::secpar_v>* next_table)
{
    static_assert(QS::is_verifier, "Verifier-only folded IP path.");
    using field_t = poly_secpar<P::secpar_v>;
    constexpr size_t block_idx = P::FOLD_ORDER[0];
    using meta = sydo_owf_block_meta<P, block_idx>;
    constexpr auto stage0_suffix_offsets = sydo_owf_stage0_suffix_base_offsets<P>();
    constexpr auto natural_strides = sydo_owf_tuple_strides<P>();
    constexpr size_t coeff_stride = natural_strides[block_idx];
    constexpr size_t next_size = sydo_owf_stage_output_size<P, 0>();

    const auto& cache = std::get<block_idx>(block_caches);
    for (size_t suffix_idx = 0; suffix_idx < next_size; ++suffix_idx)
    {
        field_t acc = field_t::set_zero();
        const field_t* coeff_base = folded_h_subvector + stage0_suffix_offsets[suffix_idx];
        if constexpr (P::elementary_encoding_v == elementary_encoding::hamming_sphere)
        {
            for (size_t subset_idx = 0; subset_idx < meta::choose_w_count; ++subset_idx)
            {
                acc += mul_polysecpar_reduce<P::secpar_v>(
                    coeff_base[subset_idx * coeff_stride],
                    sydo_owf_choose_w_eval_verifier<P, QS, block_idx>(cache, subset_idx));
            }
        }
        else
        {
            acc = sydo_owf_contract_block_ball_eval_verifier<P, QS, block_idx, coeff_stride>(
                cache, coeff_base);
        }
        next_table[suffix_idx] = acc;
    }
}

template <typename P, typename QS, typename CacheTuple, size_t stage, size_t cur_size>
inline void sydo_owf_fold_remaining_stages_verifier(const CacheTuple& block_caches,
                                                      poly_secpar<P::secpar_v>*& cur_table,
                                                      poly_secpar<P::secpar_v>*& next_table)
{
    if constexpr (stage < P::num_var_blocks)
    {
        using field_t = poly_secpar<P::secpar_v>;
        constexpr size_t block_idx = P::FOLD_ORDER[stage];
        using meta = sydo_owf_block_meta<P, block_idx>;
        constexpr size_t next_size = cur_size / meta::domain_count;
        static_assert(next_size == sydo_owf_stage_output_size<P, stage>(), "stage size mismatch");
        const auto& cache = std::get<block_idx>(block_caches);

        for (size_t suffix_idx = 0; suffix_idx < next_size; ++suffix_idx)
        {
            field_t acc = field_t::set_zero();
            const field_t* coeff_base = cur_table + suffix_idx;
            if constexpr (P::elementary_encoding_v == elementary_encoding::hamming_sphere)
            {
                for (size_t subset_idx = 0; subset_idx < meta::choose_w_count; ++subset_idx)
                {
                    acc += mul_polysecpar_reduce<P::secpar_v>(
                        coeff_base[subset_idx * next_size],
                        sydo_owf_choose_w_eval_verifier<P, QS, block_idx>(cache, subset_idx));
                }
            }
            else
            {
                acc = sydo_owf_contract_block_ball_eval_verifier<P, QS, block_idx, next_size>(
                    cache, coeff_base);
            }
            next_table[suffix_idx] = acc;
        }

        std::swap(cur_table, next_table);
        sydo_owf_fold_remaining_stages_verifier<P, QS, CacheTuple, stage + 1, next_size>(
            block_caches, cur_table, next_table);
    }
}

template <typename P, typename QS, typename CacheTuple>
inline void sydo_owf_accumulate_folded_ip_verifier(
    QS* state, const CacheTuple& block_caches, size_t subvec_i,
    const poly_secpar<P::secpar_v>* folded_h_row, size_t folded_h_row_len,
    quicksilver_gfsecpar<QS, QS::max_degree>& folded_constraint)
{
    static_assert(QS::is_verifier, "Verifier-only folded IP path.");
    using field_t = poly_secpar<P::secpar_v>;
    constexpr size_t terms_per_subvector = sydo_owf_terms_per_subvector<P>();
    constexpr size_t final_degree = sydo_owf_total_weight<P>();
    constexpr size_t size_after_stage0 = sydo_owf_stage_output_size<P, 0>();
    constexpr size_t final_size = sydo_owf_stage_output_size<P, P::num_var_blocks - 1>();
    constexpr size_t shift = QS::max_degree - final_degree;
    static_assert(final_size == 1, "final folded table must have one column");
    static_assert(final_degree <= QS::max_degree, "folded degree exceeds QS max degree");

    static thread_local std::array<field_t, terms_per_subvector> padded_h_subvector;
    const field_t* h_subvector = sydo_owf_padded_h_subvector<P>(
        folded_h_row, folded_h_row_len, subvec_i, padded_h_subvector.data());
    static thread_local std::array<field_t, terms_per_subvector> table_a;
    static thread_local std::array<field_t, terms_per_subvector> table_b;
    field_t* cur_table = table_a.data();
    field_t* next_table = table_b.data();

    {
        profile::scope timer(profile::OWF_FOLDED_IP_STAGE0);
        sydo_owf_fold_stage0_verifier<P, QS>(block_caches, h_subvector, cur_table);
    }
    {
        profile::scope timer(profile::OWF_FOLDED_IP_REST);
        sydo_owf_fold_remaining_stages_verifier<P, QS, CacheTuple, 1, size_after_stage0>(
            block_caches, cur_table, next_table);
    }

    field_t scaled = cur_table[0];
    if constexpr (QS::term_v == quicksilver_term::leading && shift > 0)
        scaled = mul_polysecpar_reduce<P::secpar_v>(scaled, state->delta_powers[shift - 1]);
    folded_constraint.mac += scaled;
}

template <typename P, typename QS, typename CacheTuple>
inline void
sydo_owf_accumulate_folded_ip(QS* state, const CacheTuple& block_caches, size_t subvec_i,
                                const poly_secpar<P::secpar_v>* folded_h_row,
                                size_t folded_h_row_len,
                                quicksilver_gfsecpar<QS, QS::max_degree>& folded_constraint)
{
    if constexpr (P::elementary_encoding_v == elementary_encoding::hamming_ball)
    {
        if constexpr (QS::is_verifier)
        {
            sydo_owf_accumulate_ball_direct_verifier<P, QS>(state, block_caches, subvec_i,
                                                              folded_h_row, folded_h_row_len,
                                                              folded_constraint);
        }
        else
        {
            (void)state;
            sydo_owf_accumulate_ball_direct_prover<P, QS>(
                block_caches, subvec_i, folded_h_row, folded_h_row_len, folded_constraint);
        }
    }
    else if constexpr (QS::is_verifier)
    {
        sydo_owf_accumulate_folded_ip_verifier<P, QS>(state, block_caches, subvec_i, folded_h_row,
                                                        folded_h_row_len, folded_constraint);
    }
    else
    {
        (void)state;
        sydo_owf_accumulate_folded_ip_prover<P, QS>(block_caches, subvec_i, folded_h_row,
                                                      folded_h_row_len, folded_constraint);
    }
}

template <typename P, typename QS>
static void add_subvector_constraints_and_folded_ip(
    QS* constraint_state, QS* folded_state, size_t subvec_i, size_t subvector_offset_bits,
    const poly_secpar<P::secpar_v>* folded_h_row, size_t folded_h_row_len,
    quicksilver_gfsecpar<QS, QS::max_degree>& folded_constraint)
{
    profile::scope subvector_timer(profile::OWF_SUBVECTOR_TOTAL);
    static_assert(sydo_owf_params_in_supported_range<P>(),
                  "Supported parameters: 2 <= B <= 4, 1 <= w_b <= 3, n_b >= 2 w_b.");
    static_assert(P::witness_extended_subvector_bits == sydo_owf_sum_block_sizes<P>(),
                  "Extended subvector layout must match per-block bit sizes.");
    static_assert(P::rsd_n % P::rsd_w == 0, "rsd_n must be divisible by rsd_w.");
    static_assert((P::rsd_n / P::rsd_w) <= sydo_owf_terms_per_subvector<P>(),
                  "Actual elementary-vector segment length must fit in the block domain.");
    static_assert(sydo_owf_total_weight<P>() <= QS::max_degree,
                  "Folded monomial degree exceeds QS degree.");

    using block_cache_tuple = sydo_owf_block_cache_tuple<P, QS>;
    using block_index_seq = std::make_index_sequence<P::num_var_blocks>;

    SYDO_ASSERT(folded_h_row_len >= P::rsd_n);

    static thread_local block_cache_tuple block_caches;
    {
        profile::scope timer(profile::OWF_LOAD_BLOCK_CACHES);
        sydo_owf_load_all_block_caches<P, QS>(constraint_state, subvector_offset_bits,
                                              &block_caches, block_index_seq{});
    }
    {
        profile::scope timer(profile::OWF_EMIT_BLOCK_CONSTRAINTS);
        sydo_owf_emit_all_block_constraints<P, QS>(constraint_state, block_caches,
                                                   block_index_seq{});
    }
    sydo_owf_accumulate_folded_ip<P, QS>(folded_state, block_caches, subvec_i, folded_h_row,
                                         folded_h_row_len, folded_constraint);
}

template <typename P, bool verifier>
void sydo_owf_constraints(
    quicksilver_state<P::secpar_v, verifier, P::OWF_CONSTS::QS_DEGREE, P::quicksilver_term_v>*
        state,
    const sydo_public_key<P>* pk, uint8_t* folded_poly_coeffs_out)
{
    using QS =
        quicksilver_state<P::secpar_v, verifier, P::OWF_CONSTS::QS_DEGREE, P::quicksilver_term_v>;
    using field_t = poly_secpar<P::secpar_v>;
    constexpr size_t explicit_constraints = P::rsd_w * P::membership_constraints_per_subvector;
    constexpr size_t padding_constraints =
        P::OWF_CONSTS::OWF_NUM_CONSTRAINTS - explicit_constraints;
    static_assert(explicit_constraints <= P::OWF_CONSTS::OWF_NUM_CONSTRAINTS,
                  "Explicit constraints exceed declared OWF_NUM_CONSTRAINTS.");
    static_assert(padding_constraints == P::y_vector_len,
                  "Unexpected OWF padding count.");

    static thread_local field_t folded_h_row[P::rsd_n];
    {
        profile::scope timer(profile::OWF_FOLD_H_ROWS);
        sydo_detail::sydo_fold_expanded_h_rows_field<P, verifier>(folded_h_row, P::rsd_n,
                                                                  pk->seed_pk, state);
    }
    field_t folded_y;
    {
        profile::scope timer(profile::OWF_FOLD_PUBLIC_Y);
        folded_y = sydo_detail::sydo_fold_public_y<P, verifier>(pk, state);
    }

    const uint64_t rest_copy_start = profile::now_ns();
    QS rest_state = *state;
    profile::add_time(profile::OWF_REST_STATE_COPY, profile::now_ns() - rest_copy_start);

    auto folded_constraint = quicksilver_gfsecpar<QS, QS::max_degree>(0, &rest_state);
    if constexpr (!verifier)
    {
        for (size_t subvec_i = 0; subvec_i < P::rsd_w; ++subvec_i)
        {
            const size_t subvector_offset_bits = subvec_i * P::witness_extended_subvector_bits;
            add_subvector_constraints_and_folded_ip<P>(
                state, &rest_state, subvec_i, subvector_offset_bits, folded_h_row, P::rsd_n,
                folded_constraint);
        }
    }
    else
    {
        for (size_t subvec_i = 0; subvec_i < P::rsd_w; ++subvec_i)
        {
            const size_t subvector_offset_bits = subvec_i * P::witness_extended_subvector_bits;
            add_subvector_constraints_and_folded_ip<P>(state, &rest_state, subvec_i,
                                                       subvector_offset_bits, folded_h_row,
                                                       P::rsd_n, folded_constraint);
        }
    }
    {
        profile::scope final_timer(profile::OWF_FINAL_FOLDED_CONSTRAINT);
        folded_constraint += quicksilver_gfsecpar<QS, 0>(folded_y, &rest_state);
        if constexpr (!QS::is_verifier)
        {
            constexpr size_t value_coeff_idx =
                (QS::term_v == quicksilver_term::constant) ? 0 : QS::max_degree;
            SYDO_ASSERT(folded_constraint.mac.coeffs[value_coeff_idx] == field_t::set_zero());
        }

        if (folded_poly_coeffs_out != nullptr)
        {
            constexpr size_t coeff_bytes = sydo_secpar_to_bytes(P::secpar_v);
            if constexpr (!QS::is_verifier)
            {
                for (size_t i = 0; i < QS::max_degree; ++i)
                {
                    const size_t coeff_idx =
                        (QS::term_v == quicksilver_term::constant) ? (i + 1) : i;
                    folded_constraint.mac.coeffs[coeff_idx].store1(
                        folded_poly_coeffs_out + i * coeff_bytes);
                }
            }
            else
            {
                auto folded_eval = folded_constraint.mac;
                if constexpr (QS::term_v == quicksilver_term::constant)
                    folded_eval = mul_polysecpar_reduce<P::secpar_v>(
                        folded_eval, state->high_coeff_eval_scale);
                folded_eval.store1(folded_poly_coeffs_out);
                if constexpr (QS::max_degree > 1)
                {
                    memset(folded_poly_coeffs_out + coeff_bytes, 0,
                           (QS::max_degree - 1) * coeff_bytes);
                }
            }
        }
    }

    if constexpr (padding_constraints > 0)
    {
        profile::scope padding_timer(profile::OWF_PADDING_CONSTRAINTS);
        const auto seed = quicksilver_gfsecpar<QS>(state->get_witness_bit(0));
        const auto same = quicksilver_gfsecpar<QS, QS::max_degree>(seed);
        const auto zero_like = same + same;
        for (size_t i = 0; i < padding_constraints; ++i)
            state->add_constraint(zero_like);
    }

}

} // namespace sydo

#endif
