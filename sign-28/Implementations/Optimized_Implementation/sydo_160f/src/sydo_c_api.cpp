#include "sydo_c_api.h"

#include "keys.hpp"
#include "keygen.hpp"
#include "parameters.hpp"
#include "sydo.hpp"
#include "verify.hpp"

#include <cstddef>

using sydo_params = sydo::sydo_160_f;

#if defined(__GNUC__) || defined(__clang__)
#define SYDO_API_HOT_FLATTEN __attribute__((noinline, hot, flatten))
#else
#define SYDO_API_HOT_FLATTEN
#endif

extern "C" unsigned long long sydo_pk_capacity_c(void)
{
    return sydo::SYDO_PACKED_PUBLIC_KEY_BYTES<sydo_params>;
}

extern "C" unsigned long long sydo_sk_capacity_c(void)
{
    return sydo::SYDO_PACKED_SECRET_KEY_BYTES<sydo_params>;
}

extern "C" unsigned long long sydo_sn_capacity_c(void)
{
    return sydo::SYDO_SIGNATURE_BYTES<sydo_params>;
}

extern "C" unsigned long long sydo_keygen_random_seed_bytes_c(void)
{
    return sydo::SYDO_KEYGEN_RANDOM_SEED_BYTES<sydo_params>;
}

extern "C" unsigned long long sydo_sign_random_seed_bytes_c(void)
{
    return sydo_params::secpar_bytes;
}

extern "C" int sydo_keygen_c(
    unsigned char *pk, unsigned long long *pk_len_bytes,
    unsigned char *sk, unsigned long long *sk_len_bytes,
    const unsigned char *random_seed, unsigned long long random_seed_len)
{
    if (!pk || !pk_len_bytes || !sk || !sk_len_bytes)
        return -1;
    if (!random_seed || random_seed_len != sydo_keygen_random_seed_bytes_c())
        return -2;

    *pk_len_bytes = sydo_pk_capacity_c();
    *sk_len_bytes = sydo_sk_capacity_c();
    return sydo::sydo_keygen<sydo_params>(
               pk, sk, random_seed, static_cast<std::size_t>(random_seed_len))
               ? 0
               : -1;
}

extern "C" SYDO_API_HOT_FLATTEN int sydo_sign_c(
    unsigned char *sk, unsigned long long sk_len_bytes,
    unsigned char *m, unsigned long long m_len_bytes,
    unsigned char *sn, unsigned long long *sn_len_bytes,
    const unsigned char *random_seed, unsigned long long random_seed_len)
{
    if (!sk || !m || !sn || !sn_len_bytes)
        return -1;
    if (sk_len_bytes != sydo_sk_capacity_c())
        return -2;
    if (!random_seed || random_seed_len != sydo_sign_random_seed_bytes_c())
        return -3;

    *sn_len_bytes = sydo_sn_capacity_c();
    return sydo::sydo_sign<sydo_params>(
               sn, m, m_len_bytes, sk, random_seed, static_cast<std::size_t>(random_seed_len))
               ? 0
               : -1;
}

extern "C" SYDO_API_HOT_FLATTEN int sydo_verify_c(
    unsigned char *pk, unsigned long long pk_len_bytes,
    unsigned char *sn, unsigned long long sn_len_bytes,
    unsigned char *m, unsigned long long m_len_bytes)
{
    if (!pk || !sn || !m)
        return -2;
    if (pk_len_bytes != sydo_pk_capacity_c() || sn_len_bytes != sydo_sn_capacity_c())
        return -2;

    return sydo::sydo_verify<sydo_params>(sn, m, m_len_bytes, pk) ? 0 : -1;
}

#undef SYDO_API_HOT_FLATTEN
