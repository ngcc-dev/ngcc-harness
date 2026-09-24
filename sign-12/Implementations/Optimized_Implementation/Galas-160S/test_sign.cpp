// test_sign.cpp - full keygen/sign/verify test for GALAS optimized impl.
#include "faest.hpp"
#include "faest_keys.hpp"
#include "parameters.hpp"
#include "hash.hpp"

#include <array>
#include <cstdio>
#include <cstring>

using namespace faest;

template <typename P>
static bool run_instance(const char* name)
{
    constexpr size_t SK_SIZE = FAEST_SECRET_KEY_BYTES<P>;
    constexpr size_t PK_SIZE = FAEST_PUBLIC_KEY_BYTES<P>;
    constexpr size_t SIG_SIZE = FAEST_SIGNATURE_BYTES<P>;

    printf("[%s] sizes: sk=%zu pk=%zu sig=%zu\n", name, SK_SIZE, PK_SIZE, SIG_SIZE);
    fflush(stdout);

    std::array<uint8_t, SK_SIZE> sk_packed;
    std::array<uint8_t, PK_SIZE> pk_packed;

    secret_key<P> sk_unpacked;
    bool kg_ok = false;
    for (int attempt = 0; attempt < 10000 && !kg_ok; ++attempt)
    {
        hash_state hk;
        hk.init(P::secpar_v);
        hk.update(name, strlen(name));
        hk.update(&attempt, sizeof(attempt));
        hk.finalize(sk_packed.data(), SK_SIZE);
        sk_packed[0] |= 0x03;
        kg_ok = faest_unpack_sk_and_get_pubkey<P>(
            pk_packed.data(), sk_packed.data(), &sk_unpacked);
    }
    if (!kg_ok)
    {
        printf("[%s] keygen: failed after 10000 attempts\n", name);
        fflush(stdout);
        return false;
    }

    std::array<uint8_t, SIG_SIZE> sig;
    const char* msg = "hello galas optimized";
    size_t msg_len = strlen(msg);

    bool sign_ok = faest_sign<P>(
        sig.data(), (const uint8_t*)msg, msg_len, sk_packed.data(), nullptr, 0);
    printf("[%s] sign: %s\n", name, sign_ok ? "OK" : "FAIL");
    fflush(stdout);
    if (!sign_ok)
        return false;

    bool verify_ok = faest_verify<P>(
        sig.data(), (const uint8_t*)msg, msg_len, pk_packed.data());
    printf("[%s] verify: %s\n", name, verify_ok ? "ACCEPT" : "REJECT");
    fflush(stdout);

    sig[0] ^= 1;
    bool verify_bad = faest_verify<P>(
        sig.data(), (const uint8_t*)msg, msg_len, pk_packed.data());
    printf("[%s] verify(tampered): %s\n", name, verify_bad ? "ACCEPT(BUG)" : "REJECT");
    fflush(stdout);

    return verify_ok && !verify_bad;
}

static bool should_run(int argc, char** argv, const char* name)
{
    if (argc <= 1 || strcmp(argv[1], "all") == 0)
        return true;
    return strcmp(argv[1], name) == 0;
}

template <typename P>
static bool maybe_run_instance(int argc, char** argv, const char* name)
{
    if (!should_run(argc, argv, name))
        return true;
    return run_instance<P>(name);
}

int main(int argc, char** argv)
{
    bool pass = true;
    pass &= maybe_run_instance<galas::galas_ngcc_160s>(argc, argv, "Galas-160S");
    pass &= maybe_run_instance<galas::galas_ngcc_160f>(argc, argv, "Galas-160F");
    pass &= maybe_run_instance<galas::galas_ngcc_256s>(argc, argv, "Galas-256S");
    pass &= maybe_run_instance<galas::galas_ngcc_256f>(argc, argv, "Galas-256F");
    pass &= maybe_run_instance<galas::galas_ngcc_384s>(argc, argv, "Galas-384S");
    pass &= maybe_run_instance<galas::galas_ngcc_384f>(argc, argv, "Galas-384F");
    pass &= maybe_run_instance<galas::galas_ngcc_512s>(argc, argv, "Galas-512S");
    pass &= maybe_run_instance<galas::galas_ngcc_512f>(argc, argv, "Galas-512F");

    printf("\n%s\n", pass ? "OPT SIGN/VERIFY PASS" : "OPT SIGN/VERIFY FAIL");
    return pass ? 0 : 1;
}
