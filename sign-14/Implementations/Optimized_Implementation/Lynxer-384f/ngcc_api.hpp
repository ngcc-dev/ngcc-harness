#ifndef LYNX_NGCC_API_HPP
#define LYNX_NGCC_API_HPP

#include "drng.h"
#include "voleith.hpp"
#include "voleith_keys.hpp"

#include <cstddef>
#include <cstdint>

extern "C" {
extern DRNG_ctx drng_algorithm;
}

namespace sig
{

template <typename P> struct ngcc_sig_scheme
{
    constexpr static unsigned long long sk_bytes = SIG_SECRET_KEY_BYTES<P>;
    constexpr static unsigned long long pk_bytes = SIG_PUBLIC_KEY_BYTES<P>;
    constexpr static unsigned long long sn_bytes = SIG_SIGNATURE_BYTES<P>;

    static unsigned long long sig_get_pk_len_bytes() { return pk_bytes; }
    static unsigned long long sig_get_sk_len_bytes() { return sk_bytes; }
    static unsigned long long sig_get_sn_len_bytes() { return sn_bytes; }

    static int sig_keygen(unsigned char* pk, unsigned long long* pk_len_bytes, unsigned char* sk,
                          unsigned long long* sk_len_bytes)
    {
        if (!pk || !pk_len_bytes || !sk || !sk_len_bytes)
            return -1;

        constexpr auto input_size = SIG_IV_BYTES<P>;
        unsigned char* sk_key = sk + input_size;

        get_random_number(&drng_algorithm, sk_key, P::secpar_bits);
        get_random_number(&drng_algorithm, sk, input_size * 8);

        // Compute public key: pk = IV || OWF(key, IV)
        memcpy(pk, sk, input_size);
        sig_pubkey<P>(pk, sk);

        *pk_len_bytes = pk_bytes;
        *sk_len_bytes = sk_bytes;
        return 0;
    }

    static int sig_sign(unsigned char* sk, unsigned long long sk_len_bytes, unsigned char* m,
                        unsigned long long m_len_bytes, unsigned char* sn,
                        unsigned long long* sn_len_bytes)
    {
        if (!sk || !sn || !sn_len_bytes || (!m && m_len_bytes))
            return -1;
        if (sk_len_bytes != sk_bytes)
            return -1;

        uint8_t rho[P::secpar_bytes];
        get_random_number(&drng_algorithm, rho, P::secpar_bits);
        if (!voleith_sign<P>(sn, m, static_cast<size_t>(m_len_bytes), sk, rho, sizeof(rho)))
            return -1;

        *sn_len_bytes = sn_bytes;
        return 0;
    }

    static int sig_verify(unsigned char* pk, unsigned long long pk_len_bytes, unsigned char* sn,
                          unsigned long long sn_len_bytes, unsigned char* m,
                          unsigned long long m_len_bytes)
    {
        if (!pk || !sn || (!m && m_len_bytes))
            return -1;
        if (pk_len_bytes != pk_bytes || sn_len_bytes != sn_bytes)
            return -1;

        return voleith_verify<P>(sn, m, static_cast<size_t>(m_len_bytes), pk) ? 0 : -1;
    }
};

} // namespace sig

#endif
