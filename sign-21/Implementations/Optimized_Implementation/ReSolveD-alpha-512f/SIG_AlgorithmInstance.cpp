#include "SIG_AlgorithmInstance.h"
#include "ngcc_api.hpp"

using resolved_alpha_params = sig::resolved_alpha::resolved_alpha_512_f;
using resolved_alpha_scheme = sig::ngcc_sig_scheme<resolved_alpha_params>;

static_assert(resolved_alpha_scheme::sk_bytes == 128);
static_assert(resolved_alpha_scheme::pk_bytes == 385);
static_assert(resolved_alpha_scheme::sn_bytes == 72611);

extern "C" unsigned long long sig_get_pk_len_bytes(void)
{
    return resolved_alpha_scheme::sig_get_pk_len_bytes();
}

extern "C" unsigned long long sig_get_sk_len_bytes(void)
{
    return resolved_alpha_scheme::sig_get_sk_len_bytes();
}

extern "C" unsigned long long sig_get_sn_len_bytes(void)
{
    return resolved_alpha_scheme::sig_get_sn_len_bytes();
}

extern "C" int sig_keygen(unsigned char* pk, unsigned long long* pk_len_bytes, unsigned char* sk,
                          unsigned long long* sk_len_bytes)
{
    return resolved_alpha_scheme::sig_keygen(pk, pk_len_bytes, sk, sk_len_bytes);
}

extern "C" int sig_sign(unsigned char* sk, unsigned long long sk_len_bytes, unsigned char* m,
                        unsigned long long m_len_bytes, unsigned char* sn,
                        unsigned long long* sn_len_bytes)
{
    return resolved_alpha_scheme::sig_sign(sk, sk_len_bytes, m, m_len_bytes, sn, sn_len_bytes);
}

extern "C" int sig_verify(unsigned char* pk, unsigned long long pk_len_bytes, unsigned char* sn,
                          unsigned long long sn_len_bytes, unsigned char* m,
                          unsigned long long m_len_bytes)
{
    return resolved_alpha_scheme::sig_verify(pk, pk_len_bytes, sn, sn_len_bytes, m, m_len_bytes);
}
