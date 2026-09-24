// verify_kat.cpp - verify GALAS KAT records with the optimized verifier.
#include "faest.hpp"
#include "faest_keys.hpp"
#include "hash.hpp"
#include "owf_proof.hpp"
#include "parameters.hpp"
#include "quicksilver.hpp"
#include "small_vole.hpp"
#include "transpose_secpar.hpp"
#include "util.hpp"
#include "vole_check.hpp"
#include "vole_commit.hpp"

#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

using namespace faest;

static int hex_nibble(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static bool parse_hex(std::vector<uint8_t>& out, size_t n, const char* s)
{
    out.assign(n, 0);
    for (size_t i = 0; i < n; ++i)
    {
        int hi = hex_nibble(s[2 * i]);
        int lo = hex_nibble(s[2 * i + 1]);
        if (hi < 0 || lo < 0) return false;
        out[i] = (uint8_t)((hi << 4) | lo);
    }
    return true;
}

template <typename P>
static const char* verify_record_stage(const std::vector<uint8_t>& pk_packed_vec,
                                       const std::vector<uint8_t>& msg,
                                       const std::vector<uint8_t>& sig_vec)
{
    using CP = P::CONSTS;
    using OC = P::OWF_CONSTS;
    constexpr auto S = P::secpar_v;

    if (pk_packed_vec.size() != FAEST_PUBLIC_KEY_BYTES<P>)
        return "pk_len";
    if (sig_vec.size() != FAEST_SIGNATURE_BYTES<P>)
        return "sig_len";

    const uint8_t* signature = sig_vec.data();
    const uint8_t* pk_packed = pk_packed_vec.data();

    hash_state hasher;

    block_2secpar<S> mu;
    hasher.init(S);
    hasher.update(pk_packed, FAEST_PUBLIC_KEY_BYTES<P>);
    hasher.update(msg.data(), msg.size());
    hasher.update_byte(8 + 0);
    hasher.finalize(&mu, sizeof(mu));

    const uint8_t* vole_check_proof = signature + CP::VOLE_COMMIT_SIZE;
    const uint8_t* correction = vole_check_proof + CP::VOLE_CHECK::PROOF_BYTES;
    const uint8_t* qs_proof = correction + OC::WITNESS_BITS / 8;
    const uint8_t* veccom_open_start = qs_proof + CP::QS::PROOF_BYTES;
    const uint8_t* delta = veccom_open_start + P::bavc_t::OPEN_SIZE;
    const uint8_t* iv_pre_ptr = delta + sizeof(block_secpar<S>);
    typename P::tree_prg_t::iv_t iv;
    const uint8_t* counter = iv_pre_ptr + sizeof(iv);

    for (size_t i = P::secpar_bits; i-- > P::delta_bits_v;)
        if ((delta[i / 8] >> (i % 8)) & 1)
            return "grinding";

    hasher.init(S);
    hasher.update(iv_pre_ptr, sizeof(iv));
    hasher.update_byte(4);
    hasher.finalize(reinterpret_cast<uint8_t*>(&iv), sizeof(iv));

    std::array<uint8_t, P::delta_bits_v> delta_bytes;
    expand_bits_to_bytes(delta_bytes.data(), P::delta_bits_v, delta);

    vole_block* q = reinterpret_cast<vole_block*>(
        aligned_alloc(alignof(vole_block), P::secpar_bits * CP::VOLE_COL_BLOCKS * sizeof(vole_block)));
    uint8_t vole_commit_check[CP::VOLE_COMMIT_CHECK_SIZE];

    if (!vole_reconstruct<P>(iv, q, delta_bytes.data(), signature, veccom_open_start,
                             vole_commit_check))
    {
        free(q);
        return "vole_reconstruct";
    }

    std::array<uint8_t, CP::VOLE_CHECK::CHALLENGE_BYTES> chal1;
    hasher.init(S);
    hasher.update(&mu, sizeof(mu));
    hasher.update(vole_commit_check, CP::VOLE_COMMIT_CHECK_SIZE);
    hasher.update(signature, CP::VOLE_COMMIT_SIZE);
    hasher.update(&iv, sizeof(iv));
    hasher.update_byte(8 + 1);
    hasher.finalize(chal1.data(), sizeof(chal1));

    std::array<uint8_t, CP::QS::CHALLENGE_BYTES> chal2;
    hasher.init(S);
    hasher.update(chal1.data(), sizeof(chal1));
    hasher.update(vole_check_proof, CP::VOLE_CHECK::PROOF_BYTES);
    vole_check_receiver<P>(q, delta_bytes.data(), chal1.data(), vole_check_proof, hasher);
    hasher.update(correction, OC::WITNESS_BITS / 8);
    hasher.update_byte(8 + 2);
    hasher.finalize(chal2.data(), sizeof(chal2));

    std::array<vole_block, CP::WITNESS_BLOCKS> correction_blocks;
    memcpy(&correction_blocks, correction, OC::WITNESS_BITS / 8);
    memset(reinterpret_cast<uint8_t*>(correction_blocks.data()) + OC::WITNESS_BITS / 8, 0,
           sizeof(correction_blocks) - OC::WITNESS_BITS / 8);
    vole_receiver_apply_correction<P>(CP::WITNESS_BLOCKS, P::delta_bits_v,
                                      correction_blocks.data(), q, delta_bytes.data());

    block_secpar<S>* macs = reinterpret_cast<block_secpar<S>*>(
        aligned_alloc(alignof(block_secpar<S>), CP::VOLE_ROWS_PADDED * sizeof(block_secpar<S>)));
    transpose_secpar<S>(q, macs, CP::VOLE_COL_STRIDE, CP::QUICKSILVER_ROWS_PADDED);
    free(q);

    block_secpar<S> delta_block;
    memcpy(&delta_block, delta, sizeof(delta_block));

    public_key<P> pk;
    faest_unpack_public_key(&pk, pk_packed);

    quicksilver_state<S, true, OC::QS_DEGREE> qs(macs, OC::OWF_NUM_CONSTRAINTS, delta_block,
                                                 chal2.data());
    owf_constraints(&qs, &pk);

    std::array<uint8_t, CP::QS::CHECK_BYTES> qs_check;
    qs.verify(OC::WITNESS_BITS, qs_proof, qs_check.data());
    free(macs);

    block_secpar<S> delta_check;
    hasher.init(S);
    hasher.update(chal2.data(), sizeof(chal2));
    hasher.update(qs_check.data(), CP::QS::CHECK_BYTES);
    hasher.update(qs_proof, CP::QS::PROOF_BYTES);
    if constexpr (P::use_grinding)
        hasher.update(counter, P::grinding_counter_size);
    hasher.update_byte(8 + 3);
    hasher.finalize(&delta_check, sizeof(delta_check));

    if (memcmp(delta, &delta_check, sizeof(delta_check)) != 0)
        return "delta_hash";
    return "ok";
}

template <typename P>
static bool verify_record(const std::vector<uint8_t>& pk, const std::vector<uint8_t>& msg,
                          const std::vector<uint8_t>& sig, const char** stage)
{
    *stage = verify_record_stage<P>(pk, msg, sig);
    return !strcmp(*stage, "ok");
}

template <typename P>
static int verify_stream()
{
    char line[1 << 20];
    std::vector<uint8_t> pk, msg, sig;
    size_t pk_len = 0, msg_len = 0, sig_len = 0;
    bool have_pk = false, have_msg = false, have_sig = false;
    unsigned ok = 0, bad = 0, total = 0;

    auto flush = [&]() {
        if (!(have_pk && have_msg && have_sig)) return;
        ++total;
        const char* stage = nullptr;
        if (verify_record<P>(pk, msg, sig, &stage)) ++ok;
        else {
            if (bad < 5)
                printf("record %u failed at %s\n", total - 1, stage ? stage : "unknown");
            ++bad;
        }
        have_pk = have_msg = have_sig = false;
    };

    while (fgets(line, sizeof(line), stdin))
    {
        line[strcspn(line, "\r\n")] = 0;
        if (!strncmp(line, "Count = ", 8))
        {
            flush();
        }
        else if (!strncmp(line, "PK_Len = ", 9))
        {
            pk_len = strtoull(line + 9, nullptr, 10);
        }
        else if (!strncmp(line, "PK = ", 5))
        {
            have_pk = parse_hex(pk, pk_len, line + 5);
        }
        else if (!strncmp(line, "M_Len = ", 8))
        {
            msg_len = strtoull(line + 8, nullptr, 10);
        }
        else if (!strncmp(line, "M = ", 4))
        {
            have_msg = parse_hex(msg, msg_len, line + 4);
        }
        else if (!strncmp(line, "Sn_Len = ", 9))
        {
            sig_len = strtoull(line + 9, nullptr, 10);
        }
        else if (!strncmp(line, "Sn = ", 5))
        {
            have_sig = parse_hex(sig, sig_len, line + 5);
        }
    }
    flush();

    printf("opt KAT verify: %u records, %u ok, %u bad\n", total, ok, bad);
    return bad == 0 ? 0 : 1;
}

int main(int argc, char** argv)
{
    const char* inst = argc > 1 ? argv[1] : "Galas-256S";
    if (!strcmp(inst, "Galas-160S"))
        return verify_stream<galas::galas_ngcc_160s>();
    if (!strcmp(inst, "Galas-160F"))
        return verify_stream<galas::galas_ngcc_160f>();
    if (!strcmp(inst, "Galas-256S"))
        return verify_stream<galas::galas_ngcc_256s>();
    if (!strcmp(inst, "Galas-256F"))
        return verify_stream<galas::galas_ngcc_256f>();
    if (!strcmp(inst, "Galas-384S"))
        return verify_stream<galas::galas_ngcc_384s>();
    if (!strcmp(inst, "Galas-384F"))
        return verify_stream<galas::galas_ngcc_384f>();
    if (!strcmp(inst, "Galas-512S"))
        return verify_stream<galas::galas_ngcc_512s>();
    if (!strcmp(inst, "Galas-512F"))
        return verify_stream<galas::galas_ngcc_512f>();
    fprintf(stderr, "unknown instance: %s\n", inst);
    return 2;
}
