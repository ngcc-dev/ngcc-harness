/*
 * namespace.h - central symbol namespacing for SHUTTLE.
 *
 * Every public SHUTTLE symbol is renamed to SHUTTLE_NAMESPACE(x) =
 * shuttle<set>_<ref|avx2|avx512>_x (see config.h) so the three parameter
 * sets
 * -- and the three backends -- can coexist in one link unit without symbol
 * clashes.  All the renames live here (instead of being scattered through
 * the per-module headers) so they can be managed and toggled in one place.
 *
 * Toggle: define DISABLE_NAMESPACE=1 (e.g. -DDISABLE_NAMESPACE=1) to keep
 * the plain, un-mangled names -- convenient for debugging, indexing, and
 * single-library builds.  Leave it UNDEFINED for the normal namespaced
 * build; do NOT set its value in source.
 *
 * config.h includes this at its end, AFTER defining SHUTTLE_NAMESPACE, so
 * the renames are visible everywhere config.h reaches (i.e. the whole
 * tree, via params.h).
 *
 * The file is organized into groups by module: the top-level (NGCC sig_* +
 * NIST crypto_sign_* + unified xof*) entry points and the STRUCTURAL
 * reduce/poly/packing renames, then the NTT, poly/packing, sampler, IRS,
 * rounding, rANS, and sign groups, each adding its own public symbols
 * below. Vendored NTT asm symbols are namespaced per config independently
 * (the .S files emit <qset>-suffixed labels).
 */
#ifndef SHUTTLE_NAMESPACE_H
#define SHUTTLE_NAMESPACE_H

#if !DISABLE_NAMESPACE

/* ---- NGCC fixed interface (SIG_AlgorithmInstance.h) ---- */
/* NOTE: the NGCC sig_* prototypes are FIXED (verbatim contract) and are
 * NOT mangled here -- KAT_SIG.c links against the unmangled
 * sig_keygen/sig_sign/ sig_verify/sig_get_*_len_bytes by name.  They are
 * listed here only as a reminder that the NGCC adapter is the ONE set of
 * public symbols that stays un-namespaced (one AlgorithmInstance per built
 * binary). */

/* ---- top-level NIST/SUPERCOP API (api.h, sign.c) ---- */
#    define crypto_sign_keypair SHUTTLE_NAMESPACE(keypair)
#    define crypto_sign_signature SHUTTLE_NAMESPACE(signature)
#    define crypto_sign_verify SHUTTLE_NAMESPACE(verify)
#    define crypto_sign SHUTTLE_NAMESPACE(sign)
#    define crypto_sign_open SHUTTLE_NAMESPACE(open)
/* internal xi/rnd-driven entries + keygen-attempt diagnostic */
#    define crypto_sign_keypair_xi SHUTTLE_NAMESPACE(keypair_xi)
#    define crypto_sign_signature_rnd SHUTTLE_NAMESPACE(signature_rnd)
#    define shuttle_last_keygen_attempts \
        SHUTTLE_NAMESPACE(last_keygen_attempts)

/* ---- unified XOF wrappers (symmetric.{c,h}, xof.h) ---- */
#    define xof128_init SHUTTLE_NAMESPACE(xof128_init)
#    define xof128_squeeze SHUTTLE_NAMESPACE(xof128_squeeze)
#    define xof256_init SHUTTLE_NAMESPACE(xof256_init)
#    define xof256_squeeze SHUTTLE_NAMESPACE(xof256_squeeze)
/* ---- AVX2/AVX512 lane-batched XOF variants (symmetric_avx{2,512}.c) ----
 */
#    define xof128_avx2_init SHUTTLE_NAMESPACE(xof128_avx2_init)
#    define xof128_avx2_squeeze SHUTTLE_NAMESPACE(xof128_avx2_squeeze)
#    define xof256_avx2_init SHUTTLE_NAMESPACE(xof256_avx2_init)
#    define xof256_avx2_squeeze SHUTTLE_NAMESPACE(xof256_avx2_squeeze)
#    define xof128_avx512_init SHUTTLE_NAMESPACE(xof128_avx512_init)
#    define xof128_avx512_squeeze SHUTTLE_NAMESPACE(xof128_avx512_squeeze)
#    define xof256_avx512_init SHUTTLE_NAMESPACE(xof256_avx512_init)
#    define xof256_avx512_squeeze SHUTTLE_NAMESPACE(xof256_avx512_squeeze)

/* ---- reduce.h ---- */
#    define reduce32 SHUTTLE_NAMESPACE(reduce32)
#    define caddq SHUTTLE_NAMESPACE(caddq)
#    define caddq2 SHUTTLE_NAMESPACE(caddq2)
#    define freeze SHUTTLE_NAMESPACE(freeze)
#    define reduce_mod_2q SHUTTLE_NAMESPACE(reduce_mod_2q)

/* ---- poly.h (scheme-domain arithmetic helpers + unpack_pk_bn) ---- */
#    define poly_reduce SHUTTLE_NAMESPACE(poly_reduce)
#    define poly_caddq SHUTTLE_NAMESPACE(poly_caddq)
#    define poly_freeze SHUTTLE_NAMESPACE(poly_freeze)
#    define poly_add SHUTTLE_NAMESPACE(poly_add)
#    define poly_sub SHUTTLE_NAMESPACE(poly_sub)
#    define poly16_add SHUTTLE_NAMESPACE(poly16_add)
#    define poly_ntt SHUTTLE_NAMESPACE(poly_ntt)
#    define poly_invntt_tomont SHUTTLE_NAMESPACE(poly_invntt_tomont)
#    define poly_pointwise_montgomery \
        SHUTTLE_NAMESPACE(poly_pointwise_montgomery)
#    define poly_sqnorm SHUTTLE_NAMESPACE(poly_sqnorm)
#    define unpack_pk_bn SHUTTLE_NAMESPACE(unpack_pk_bn)

/* ---- packing.h ---- */
#    define integer_to_bytes SHUTTLE_NAMESPACE(integer_to_bytes)
#    define bytes_to_integer SHUTTLE_NAMESPACE(bytes_to_integer)
#    define poly_to_bytes SHUTTLE_NAMESPACE(poly_to_bytes)
#    define bytes_to_poly SHUTTLE_NAMESPACE(bytes_to_poly)
#    define ct_range_reject SHUTTLE_NAMESPACE(ct_range_reject)
#    define pack_pk SHUTTLE_NAMESPACE(pack_pk)
#    define unpack_pk SHUTTLE_NAMESPACE(unpack_pk)
#    define pack_sk SHUTTLE_NAMESPACE(pack_sk)
#    define unpack_sk SHUTTLE_NAMESPACE(unpack_sk)
#    define pack_com SHUTTLE_NAMESPACE(pack_com)
#    define unpack_com SHUTTLE_NAMESPACE(unpack_com)
/* pack_sig{,_raw}/unpack_sig{,_raw} belong to the rANS / sigEncode module;
 * declared here so the namespace group is complete. */
#    define pack_sig_raw SHUTTLE_NAMESPACE(pack_sig_raw)
#    define unpack_sig_raw SHUTTLE_NAMESPACE(unpack_sig_raw)
#    define pack_sig SHUTTLE_NAMESPACE(pack_sig)
#    define unpack_sig SHUTTLE_NAMESPACE(unpack_sig)

/* ---- ntt group ----
 * The shim entry points poly_ntt / poly_invntt_tomont /
 * poly_pointwise_montgomery are already renamed in the poly.h block above.
 * Here we add the two canonical-order shim helpers.  The vendored
 * NTT asm/consts symbols (the per-config s256_ / s512_ / s1024_ family)
 * are namespaced INDEPENDENTLY by the generators (the .S cannot see C
 * macros) and are deliberately NOT routed through SHUTTLE_NAMESPACE --
 * their s<n>_ prefix already deconflicts the three configs across one link
 * unit. */
#    define poly_ntt_canonical SHUTTLE_NAMESPACE(poly_ntt_canonical)
#    define poly_ntt_import SHUTTLE_NAMESPACE(poly_ntt_import)
/* ---- sampler group ----
 * BaseSampler external-linkage symbols (sampler.c).  The
 * ApproxExp/ApproxLog entry points are `static inline` in
 * approx_{exp,log}.h, so they are file-local and need no namespacing
 * (their caller-facing aliases are macros). */
#    define cdt_scan96 SHUTTLE_NAMESPACE(cdt_scan96)
#    define sampler_sigma2 SHUTTLE_NAMESPACE(sampler_sigma2)
#    define noise_magnitude_batch SHUTTLE_NAMESPACE(noise_magnitude_batch)
/* ---- sampler-wiring group (polyvec.c + sampler.c) ---- */
#    define expand_seeds SHUTTLE_NAMESPACE(expand_seeds)
#    define expand_signing_seeds SHUTTLE_NAMESPACE(expand_signing_seeds)
#    define expand_a SHUTTLE_NAMESPACE(expand_a)
#    define expand_s SHUTTLE_NAMESPACE(expand_s)
#    define sample_c SHUTTLE_NAMESPACE(sample_c)
#    define sample_y SHUTTLE_NAMESPACE(sample_y)
#    define gauss_stream_init SHUTTLE_NAMESPACE(gauss_stream_init)
#    define gs_ensure SHUTTLE_NAMESPACE(gs_ensure)
#    define gauss_stream_chunk SHUTTLE_NAMESPACE(gauss_stream_chunk)
#    define gauss_finalize SHUTTLE_NAMESPACE(gauss_finalize)
/* ---- irs / sampler_u group ---- */
#    define sampler_u SHUTTLE_NAMESPACE(sampler_u)
#    define sampler_u_decode SHUTTLE_NAMESPACE(sampler_u_decode)
#    define sampler_u_x2 SHUTTLE_NAMESPACE(sampler_u_x2)
#    define reject_sample SHUTTLE_NAMESPACE(reject_sample)

/* ---- rounding group ---- */
#    define lsb_coeff SHUTTLE_NAMESPACE(lsb_coeff)
#    define lift_to_mod2q_coeff SHUTTLE_NAMESPACE(lift_to_mod2q_coeff)
#    define poly_lift_to_mod2q SHUTTLE_NAMESPACE(poly_lift_to_mod2q)
#    define compress_y SHUTTLE_NAMESPACE(compress_y)
#    define stretch_s SHUTTLE_NAMESPACE(stretch_s)
#    define roundB_update_s2 SHUTTLE_NAMESPACE(roundB_update_s2)
#    define mat_mul_2q SHUTTLE_NAMESPACE(mat_mul_2q)
#    define mat_mul_z1_2q SHUTTLE_NAMESPACE(mat_mul_z1_2q)
#    define highbits_reduced SHUTTLE_NAMESPACE(highbits_reduced)
#    define hbvalue SHUTTLE_NAMESPACE(hbvalue)
#    define make_hint SHUTTLE_NAMESPACE(make_hint)
#    define use_hint SHUTTLE_NAMESPACE(use_hint)
#    define recon_comY0p SHUTTLE_NAMESPACE(recon_comY0p)
#    define poly_array_sqnorm SHUTTLE_NAMESPACE(poly_array_sqnorm)
#    define keygen_norm_ok SHUTTLE_NAMESPACE(keygen_norm_ok)
#    define response_norm_ok SHUTTLE_NAMESPACE(response_norm_ok)

/* ---- rans group (engine entry points; pack_sig{,_raw}
 * already declared in the packing group above) ---- */
#    define shuttle_rans_encode SHUTTLE_NAMESPACE(rans_encode)
#    define shuttle_rans_decode SHUTTLE_NAMESPACE(rans_decode)

#endif /* !DISABLE_NAMESPACE */
#endif /* SHUTTLE_NAMESPACE_H */
