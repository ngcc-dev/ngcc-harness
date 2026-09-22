/*
 * ngcc_attack: reproducers for the defects reported in the NGCC Round 1
 * candidate reference implementations.
 *
 * Each subcommand loads a candidate shared library built by this repository
 * (see api/README.md) through the uniform ABI and demonstrates one specific
 * defect. Every check prints a single verdict line:
 *
 *     ATTACK <check> <instance> CONFIRMED|NOT-CONFIRMED <detail>
 *
 * and exits 0 when the defect is confirmed, 1 when it is not, 2 on usage or
 * load errors. "Confirmed" means the reported behaviour was observed; for the
 * control runs documented in tools/README.md a NOT-CONFIRMED result is the
 * expected and desired outcome.
 *
 * Nothing here modifies a submission. The libraries are built from the
 * candidates' own sources by the per-candidate Makefiles.
 *
 * Build:  make -C tools
 * Run:    tools/reproduce.sh          (all findings)
 *         tools/ngcc_attack <check> <library> [args]
 */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <signal.h>
#include <setjmp.h>
#include <unistd.h>

#include "link_common.h"
#include "link_hash.h"
#include "link_kem.h"
#include "link_sig.h"

/* Some candidate verifiers crash on malformed input (a reported defect in its
 * own right). Rather than aborting the sweep we trap the fault, count it, and
 * continue. Jumping out of a fault handler is not strictly portable, but it is
 * reliable on Linux for this purpose and keeps one bad input from hiding the
 * rest of the result. */
static sigjmp_buf g_fault;
static volatile sig_atomic_t g_armed;
static unsigned long long g_crashes;

static void fault_handler(int sig)
{
    (void)sig;
    if (g_armed) { g_armed = 0; siglongjmp(g_fault, 1); }
    _exit(2);
}

static void arm_faults(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_handler = fault_handler;
    sa.sa_flags = SA_NODEFER;
    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGBUS,  &sa, NULL);
    sigaction(SIGILL,  &sa, NULL);
    sigaction(SIGFPE,  &sa, NULL);
    sigaction(SIGABRT, &sa, NULL);
}

/* Run vf(...) and return its result, or -99 if it faulted. */
#define GUARDED(expr, crashed_val) ({                    \
    int _r;                                              \
    g_armed = 1;                                         \
    if (sigsetjmp(g_fault, 1) == 0) { _r = (expr); }     \
    else { _r = (crashed_val); g_crashes++; }            \
    g_armed = 0; _r; })

static void *g_lib;
static const ngcc_meta_t *g_meta;

static void *sym(const char *n)
{
    void *p = dlsym(g_lib, n);
    if (!p) { fprintf(stderr, "missing symbol %s\n", n); exit(2); }
    return p;
}

static void load(const char *path)
{
    char buf[4096];
    if (!strchr(path, '/')) { snprintf(buf, sizeof buf, "./%s", path); path = buf; }
    g_lib = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (!g_lib) { fprintf(stderr, "dlopen: %s\n", dlerror()); exit(2); }
    const ngcc_meta_t *(*mf)(void) = sym("ngcc_meta");
    g_meta = mf();
    if (!g_meta || g_meta->magic != NGCC_META_MAGIC) {
        fprintf(stderr, "bad metadata\n"); exit(2);
    }
}

static int verdict(const char *check, int confirmed, const char *fmt, ...)
{
    va_list ap;
    printf("ATTACK %-22s %-24s %s ", check, g_meta->instance,
           confirmed ? "CONFIRMED" : "NOT-CONFIRMED");
    va_start(ap, fmt); vprintf(fmt, ap); va_end(ap);
    printf("\n");
    return confirmed ? 0 : 1;
}

static void seed_lib(unsigned char tag)
{
    int (*s)(const unsigned char *, unsigned long long) = sym("ngcc_seed");
    unsigned char seed[64];
    for (int i = 0; i < 64; i++) seed[i] = (unsigned char)(tag + 7 * i);
    if (s(seed, sizeof seed) != 0) { fprintf(stderr, "ngcc_seed failed\n"); exit(2); }
}

/* Seed schedule used by the repository-wide security audit.  Keep it
 * separate from the older attack-driver schedule so published deterministic
 * witnesses can be replayed byte-for-byte. */
static void seed_lib_audit(unsigned char tag)
{
    int (*s)(const unsigned char *, unsigned long long) = sym("ngcc_seed");
    unsigned char seed[64];
    for (int i = 0; i < 64; i++) seed[i] = (unsigned char)(tag + 29 * i);
    if (s(seed, sizeof seed) != 0) { fprintf(stderr, "ngcc_seed failed\n"); exit(2); }
}

/* ---------------------------------------------------------------- hashes */

/* Eijen: for every byte-aligned M, H(M) == H(M || 0000000).
 * Byte-aligned input uses the specification's LSB-first 0x01 delimiter,
 * while partial-byte API input is MSB-first and also produces 0x01 after
 * seven explicit zero bits. */
static int hash_collide_zeropad(void)
{
    ngcc_hash_fn h = sym("CryptHash");
    const ngcc_meta_hash_t *m = (const ngcc_meta_hash_t *)g_meta;
    size_t dn = (size_t)m->digest_len;
    unsigned char msg[64], *d1 = calloc(dn, 1), *d2 = calloc(dn, 1);
    int hits = 0, tried = 0;
    unsigned long long first = 0;
    for (int bytes = 0; bytes <= 8; bytes++) {
        memset(msg, 0, sizeof msg);
        for (int i = 0; i < bytes; i++) msg[i] = (unsigned char)(0x5A + 7 * i);
        unsigned long long b1 = (unsigned long long)bytes * 8;
        if (h(m->digest_bits, msg, b1, d1) || h(m->digest_bits, msg, b1 + 7, d2)) {
            fprintf(stderr, "CryptHash error\n"); exit(2);
        }
        tried++;
        if (!memcmp(d1, d2, dn)) { if (!hits) first = b1; hits++; }
    }
    free(d1); free(d2);
    return verdict("hash-collide-zeropad", hits > 0,
        "%d/%d byte-aligned lengths collide with their 7-zero-bit extension%s",
        hits, tried,
        hits ? (first == 0 ? " (incl. the empty string vs 0x00/7 bits)" : "") : "");
}

/* MasterCube: pad10*1 places both padding bits in the same position when
 * |M| mod r == r-1, so M and M with its final 1 bit removed collide. */
static int hash_collide_rate(unsigned long long bits)
{
    ngcc_hash_fn h = sym("CryptHash");
    const ngcc_meta_hash_t *m = (const ngcc_meta_hash_t *)g_meta;
    size_t dn = (size_t)m->digest_len, nb = (size_t)((bits + 7) / 8);
    unsigned char *msg = calloc(nb + 2, 1), *d1 = calloc(dn, 1), *d2 = calloc(dn, 1);
    for (size_t i = 0; i < nb; i++) msg[i] = (unsigned char)(0x3C + 11 * i);
    size_t bi = (size_t)bits - 1;
    msg[bi / 8] |= (unsigned char)(0x80u >> (bi % 8));   /* final bit = 1 */
    h(m->digest_bits, msg, bits, d1);
    h(m->digest_bits, msg, bits - 1, d2);
    int same = !memcmp(d1, d2, dn);
    free(msg); free(d1); free(d2);
    return verdict("hash-collide-rate", same,
        "H(M,%llu bits) %s H(M,%llu bits)", bits, same ? "==" : "!=", bits - 1);
}

/* Megascon / Mozi: no domain separation, so the shorter digest is a byte-exact
 * prefix of the longer one. Takes two libraries. */
static int hash_prefix(const char *pathb)
{
    ngcc_hash_fn ha = sym("CryptHash");
    const ngcc_meta_hash_t *ma = (const ngcc_meta_hash_t *)g_meta;
    void *lb = dlopen(pathb[0] == '/' || strchr(pathb, '/') ? pathb : pathb, RTLD_NOW | RTLD_LOCAL);
    if (!lb) { fprintf(stderr, "dlopen b: %s\n", dlerror()); exit(2); }
    const ngcc_meta_t *(*mfb)(void) = dlsym(lb, "ngcc_meta");
    ngcc_hash_fn hb = dlsym(lb, "CryptHash");
    if (!mfb || !hb) { fprintf(stderr, "second library lacks the ABI\n"); exit(2); }
    const ngcc_meta_hash_t *mb = (const ngcc_meta_hash_t *)mfb();
    size_t da = (size_t)ma->digest_len, db = (size_t)mb->digest_len;
    size_t mn = da < db ? da : db;
    unsigned char msg[256], *A = calloc(da, 1), *B = calloc(db, 1);
    int pref = 0, tot = 0;
    for (unsigned long long bits = 0; bits <= 1016; bits += 127) {
        for (size_t i = 0; i < sizeof msg; i++)
            msg[i] = (unsigned char)(0x11 + 13 * i + bits);
        ha(ma->digest_bits, msg, bits, A);
        hb(mb->digest_bits, msg, bits, B);
        tot++;
        if (!memcmp(A, B, mn)) pref++;
    }
    free(A); free(B);
    return verdict("hash-prefix", pref == tot,
        "%s is a byte-exact prefix of %s in %d/%d messages",
        ma->h.instance, mb->h.instance, pref, tot);
}

/* ------------------------------------------------------------------ KEMs */

/* Cheetah / Loong: the rejection mask is not normalised to all-ones, so a
 * modified ciphertext returns a bitwise mixture that retains the low 7 bits
 * of every byte of the valid shared secret. */
static int kem_reject_mask(void)
{
    const ngcc_meta_kem_t *m = (const ngcc_meta_kem_t *)g_meta;
    ngcc_kem_keygen_fn kg = sym("kem_keygen");
    ngcc_kem_enc_fn en = sym("kem_enc");
    ngcc_kem_dec_fn de = sym("kem_dec");
    seed_lib(0x41);
    unsigned char *pk = calloc(m->pk_len,1), *sk = calloc(m->sk_len,1),
                  *ct = calloc(m->ct_len,1), *ss = calloc(m->ss_len,1), *ss2 = calloc(m->ss_len,1);
    unsigned long long pl=m->pk_len, sl=m->sk_len, cl=m->ct_len, l=m->ss_len, l2;
    if (kg(pk,&pl,sk,&sl) || en(pk,pl,ss,&l,ct,&cl)) { fprintf(stderr,"keygen/enc failed\n"); exit(2); }
    int best = 0; unsigned bestb = 0;
    for (int byte = 0; byte < 2; byte++) {
        ct[byte] ^= 0x80; l2 = m->ss_len;
        int r = de(sk,sl,ct,cl,ss2,&l2);
        ct[byte] ^= 0x80;
        if (r) continue;
        unsigned low7 = 0, tot = 0;
        for (unsigned long long i = 0; i < l; i++)
            for (int b = 0; b < 7; b++) { tot++; if (((ss[i]>>b)&1) == ((ss2[i]>>b)&1)) low7++; }
        if (low7 == tot && tot) { best = 1; bestb = (unsigned)byte; break; }
    }
    free(pk); free(sk); free(ct); free(ss); free(ss2);
    return verdict("kem-reject-mask", best,
        best ? "modified ciphertext byte %u: all low-7 bits of the valid secret retained"
             : "no tested byte retained the valid secret%.0u", bestb);
}

/* Aigis-Enc+: the implicit-rejection branch is dead code, so most single-bit
 * ciphertext changes still yield the original shared secret. */
static int kem_ct_flip(void)
{
    const ngcc_meta_kem_t *m = (const ngcc_meta_kem_t *)g_meta;
    ngcc_kem_keygen_fn kg = sym("kem_keygen");
    ngcc_kem_enc_fn en = sym("kem_enc");
    ngcc_kem_dec_fn de = sym("kem_dec");
    seed_lib(0x41);
    unsigned char *pk = calloc(m->pk_len,1), *sk = calloc(m->sk_len,1),
                  *ct = calloc(m->ct_len,1), *ss = calloc(m->ss_len,1), *ss2 = calloc(m->ss_len,1);
    unsigned long long pl=m->pk_len, sl=m->sk_len, cl=m->ct_len, l=m->ss_len, l2;
    if (kg(pk,&pl,sk,&sl) || en(pk,pl,ss,&l,ct,&cl)) { fprintf(stderr,"keygen/enc failed\n"); exit(2); }
    unsigned long long same = 0, total = cl * 8;
    for (unsigned long long b = 0; b < total; b++) {
        ct[b/8] ^= (unsigned char)(1u << (b%8)); l2 = m->ss_len;
        int r = de(sk,sl,ct,cl,ss2,&l2);
        ct[b/8] ^= (unsigned char)(1u << (b%8));
        if (r == 0 && l2 == l && !memcmp(ss, ss2, (size_t)l)) same++;
    }
    free(pk); free(sk); free(ct); free(ss); free(ss2);
    return verdict("kem-ct-flip", same > 0,
        "%llu/%llu single-bit ciphertext flips return the ORIGINAL shared secret",
        same, total);
}

/* BRA: rbc_qpoly_left_div2 passes an unchecked degree into rbc_qpoly_mul2,
 * which writes past the q-polynomial coefficient array. At the harness
 * default -O2 an all-zero ciphertext returns; the stable witness is one
 * flipped secret-key byte, then decapsulation of the honest ciphertext. */
static int kem_decoder_fault(void)
{
    if (g_meta->type != NGCC_TYPE_KEM) { fprintf(stderr, "not a KEM\n"); exit(2); }
    const ngcc_meta_kem_t *m = (const ngcc_meta_kem_t *)g_meta;
    ngcc_kem_keygen_fn kg = sym("kem_keygen");
    ngcc_kem_enc_fn en = sym("kem_enc");
    ngcc_kem_dec_fn de = sym("kem_dec");
    seed_lib(0x42);
    unsigned char *pk = calloc(m->pk_len,1), *sk = calloc(m->sk_len,1),
                  *ct = calloc(m->ct_len,1), *ss = calloc(m->ss_len,1), *ss2 = calloc(m->ss_len,1);
    unsigned long long pl=m->pk_len, sl=m->sk_len, cl=m->ct_len, l=m->ss_len, l2=m->ss_len;
    if (kg(pk,&pl,sk,&sl) || en(pk,pl,ss,&l,ct,&cl)) { fprintf(stderr,"keygen/enc failed\n"); exit(2); }
    int honest = de(sk,sl,ct,cl,ss2,&l2);
    if (honest || l2 != l || memcmp(ss, ss2, (size_t)l)) {
        fprintf(stderr, "honest decapsulation failed\n"); exit(2);
    }
    sk[0] ^= 0xff;
    l2 = m->ss_len;
    int r = GUARDED(de(sk,sl,ct,cl,ss2,&l2), -99);
    free(pk); free(sk); free(ct); free(ss); free(ss2);
    if (r == -99)
        return verdict("kem-decoder-fault", 1,
            "flipping secret-key byte 0 faults decapsulation of the honest ciphertext");
    return verdict("kem-decoder-fault", 0,
        "flipping secret-key byte 0: kem_dec returned %d", r);
}

/* ------------------------------------------------------------ signatures */

/* Aigis-Sig+ / CS: non-canonical trailing encoding bytes, so a distinct
 * signature verifies for the same message (SUF-CMA). Sweeps every bit. */
static int sig_malleable(void)
{
    const ngcc_meta_sig_t *m = (const ngcc_meta_sig_t *)g_meta;
    ngcc_sig_keygen_fn kg = sym("sig_keygen");
    ngcc_sig_sign_fn sg = sym("sig_sign");
    ngcc_sig_verify_fn vf = sym("sig_verify");
    seed_lib(0x11);
    static const unsigned char original[] = "NGCC reproducer message";
    unsigned char msg[sizeof original];
    memcpy(msg, original, sizeof original);
    unsigned long long mn = sizeof msg - 1;
    size_t sn_cap = (size_t)m->sn_len + (size_t)mn + 64;
    unsigned char *pk = calloc(m->pk_len,1), *sk = calloc(m->sk_len,1), *sn = calloc(sn_cap,1);
    unsigned long long pl=m->pk_len, sl=m->sk_len, nl=m->sn_len;
    if (kg(pk,&pl,sk,&sl) || sg(sk,sl,msg,mn,sn,&nl)) { fprintf(stderr,"keygen/sign failed\n"); exit(2); }
    if (vf(pk,pl,sn,nl,msg,mn)) { fprintf(stderr,"valid signature rejected\n"); exit(2); }
    memcpy(msg, original, sizeof original);
    unsigned long long acc = 0, total = m->sn_len * 8, firstbyte = 0;
    for (unsigned long long b = 0; b < total; b++) {
        sn[b/8] ^= (unsigned char)(1u << (b%8));
        memcpy(msg, original, sizeof original);
        int r = GUARDED(vf(pk,pl,sn,nl,msg,mn), -99);
        sn[b/8] ^= (unsigned char)(1u << (b%8));
        if (r == 0) { if (!acc) firstbyte = b/8; acc++; }
    }
    free(pk); free(sk); free(sn);
    if (g_crashes)
        return verdict("sig-malleable", acc > 0,
            "%llu/%llu signature-bit flips still verify (first at byte %llu of %llu); "
            "%llu flips CRASHED the verifier",
            acc, total, firstbyte, nl, g_crashes);
    return verdict("sig-malleable", acc > 0,
        "%llu/%llu signature-bit flips still verify (first at byte %llu of %llu)",
        acc, total, firstbyte, nl);
}

/* MORNING-ATLAS: the fixed-size hint encoding contains unused coefficient
 * slots, but unpack_sig stops at the final cumulative count and never checks
 * that the remaining slots have their canonical zero encoding.  Flip a bit
 * in the last unused slot for each submitted parameter set. */
static int sig_hint_padding(void)
{
    const ngcc_meta_sig_t *m = (const ngcc_meta_sig_t *)g_meta;
    ngcc_sig_keygen_fn kg = sym("sig_keygen");
    ngcc_sig_sign_fn sg = sym("sig_sign");
    ngcc_sig_verify_fn vf = sym("sig_verify");
    unsigned long long offset;

    if (!strcmp(g_meta->instance, "lwrdsa128")) offset = 2047;
    else if (!strcmp(g_meta->instance, "lwrdsa192")) offset = 3327;
    else if (!strcmp(g_meta->instance, "lwrdsa256")) offset = 4607;
    else if (!strcmp(g_meta->instance, "lwrdsa512")) offset = 9999;
    else { fprintf(stderr, "sig-hint-padding needs a MORNING-ATLAS library\n"); exit(2); }
    if (offset >= m->sn_len) { fprintf(stderr, "unexpected signature layout\n"); exit(2); }

    static const unsigned char original[] = "NGCC reproducer message";
    unsigned char msg[sizeof original];
    memcpy(msg, original, sizeof original);
    unsigned long long mn = sizeof msg - 1;
    size_t sn_cap = (size_t)m->sn_len + (size_t)mn + 64;
    unsigned char *pk = calloc(m->pk_len, 1), *sk = calloc(m->sk_len, 1),
                  *sn = calloc(sn_cap, 1);
    unsigned long long pl = m->pk_len, sl = m->sk_len, nl = m->sn_len;
    seed_lib(0x11);
    if (kg(pk, &pl, sk, &sl) || sg(sk, sl, msg, mn, sn, &nl)) {
        fprintf(stderr, "keygen/sign failed\n"); exit(2);
    }
    if (vf(pk, pl, sn, nl, msg, mn)) {
        fprintf(stderr, "valid signature rejected\n"); exit(2);
    }

    /* Verification incorrectly overwrites the caller's message from the
     * absent signature tail, so restore the same message before the attack. */
    memcpy(msg, original, sizeof original);
    sn[offset] ^= 0x80;
    int changed = GUARDED(vf(pk, pl, sn, nl, msg, mn), -99);
    sn[offset] ^= 0x80;
    free(pk); free(sk); free(sn);
    return verdict("sig-hint-padding", changed == 0,
        "bit 7 of ignored hint byte %llu: modified signature %s for the same message",
        offset, changed == 0 ? "ACCEPTED" : (changed == -99 ? "CRASHED" : "rejected"));
}

/* FlexTree-160f: the verifier decodes the PORS portion without checking an
 * unused padding bit.  Mutating the known padding position therefore creates
 * a distinct signature for the same message without an exhaustive bit scan. */
static int sig_pors_padding(void)
{
    const ngcc_meta_sig_t *m = (const ngcc_meta_sig_t *)g_meta;
    ngcc_sig_keygen_fn kg = sym("sig_keygen");
    ngcc_sig_sign_fn sg = sym("sig_sign");
    ngcc_sig_verify_fn vf = sym("sig_verify");
    const unsigned long long offset = 3613;

    if (strcmp(g_meta->instance, "Flextree-160f")) {
        fprintf(stderr, "sig-pors-padding needs the Flextree-160f library\n"); exit(2);
    }
    if (offset >= m->sn_len) { fprintf(stderr, "unexpected signature layout\n"); exit(2); }

    /* Keep the original audit's deterministic transcript so the selected
     * PORS instance has the same unused-node boundary as its published bit
     * 28911 witness. */
    static const unsigned char original[] = "NGCC low hanging fruit signature message";
    unsigned char msg[sizeof original];
    memcpy(msg, original, sizeof original);
    unsigned long long mn = sizeof msg - 1;
    size_t sn_cap = (size_t)m->sn_len + (size_t)mn + 64;
    unsigned char *pk = calloc(m->pk_len, 1), *sk = calloc(m->sk_len, 1),
                  *sn = calloc(sn_cap, 1);
    unsigned long long pl = m->pk_len, sl = m->sk_len, nl = m->sn_len;
    seed_lib_audit(0x11);
    if (kg(pk, &pl, sk, &sl) || sg(sk, sl, msg, mn, sn, &nl)) {
        fprintf(stderr, "keygen/sign failed\n"); exit(2);
    }
    if (vf(pk, pl, sn, nl, msg, mn)) {
        fprintf(stderr, "valid signature rejected\n"); exit(2);
    }

    memcpy(msg, original, sizeof original);
    sn[offset] ^= 0x80;
    int changed = GUARDED(vf(pk, pl, sn, nl, msg, mn), -99);
    sn[offset] ^= 0x80;
    free(pk); free(sk); free(sn);
    return verdict("sig-pors-padding", changed == 0,
        "bit 7 of unchecked PORS byte %llu: modified signature %s for the same message",
        offset, changed == 0 ? "ACCEPTED" : (changed == -99 ? "CRASHED" : "rejected"));
}

/* UVW: sig_verify discards its result and returns success unconditionally. */
static int sig_accept_all(void)
{
    const ngcc_meta_sig_t *m = (const ngcc_meta_sig_t *)g_meta;
    ngcc_sig_keygen_fn kg = sym("sig_keygen");
    ngcc_sig_sign_fn sg = sym("sig_sign");
    ngcc_sig_verify_fn vf = sym("sig_verify");
    seed_lib(0x11);
    unsigned char msg[] = "NGCC reproducer message";
    unsigned long long mn = sizeof msg - 1;
    unsigned char *pk = calloc(m->pk_len,1), *sk = calloc(m->sk_len,1), *sn = calloc(m->sn_len,1);
    unsigned long long pl=m->pk_len, sl=m->sk_len, nl=m->sn_len;
    if (kg(pk,&pl,sk,&sl) || sg(sk,sl,msg,mn,sn,&nl)) { fprintf(stderr,"keygen/sign failed\n"); exit(2); }
    msg[0] ^= 0x80;
    int mr = GUARDED(vf(pk,pl,sn,nl,msg,mn), -99);
    msg[0] ^= 0x80;
    memset(sn, 0, (size_t)nl);
    int zr = GUARDED(vf(pk,pl,sn,nl,msg,mn), -99);
    free(pk); free(sk); free(sn);
    const char *ms = mr == 0 ? "ACCEPTED" : (mr == -99 ? "CRASHED the verifier" : "rejected");
    const char *zs = zr == 0 ? "ACCEPTED" : (zr == -99 ? "CRASHED the verifier" : "rejected");
    return verdict("sig-accept-all", mr == 0 || zr == 0,
        "modified message %s, all-zero signature %s", ms, zs);
}

/* SQIsign2D2 Level2-eff uncompressed: with NDEBUG, verifier decisions for a
 * malformed signature depend on data left in its stack frame by an earlier
 * call.  Fill the part of the stack reused by the verifier without relying on
 * optimisation-sensitive dead stores. */
__attribute__((noinline))
static void scrub_stack(void)
{
    volatile unsigned char scratch[1024 * 1024];
    for (size_t i = 0; i < sizeof scratch; i++) scratch[i] = 0;
    __asm__ volatile ("" : : "r"(&scratch[0]) : "memory");
}

static int sig_uninit_verdict(void)
{
    const ngcc_meta_sig_t *m = (const ngcc_meta_sig_t *)g_meta;
    ngcc_sig_keygen_fn kg = sym("sig_keygen");
    ngcc_sig_sign_fn sg = sym("sig_sign");
    ngcc_sig_verify_fn vf = sym("sig_verify");
    static const unsigned char text[] = "NGCC low hanging fruit signature message";
    unsigned char msg[sizeof text];
    memcpy(msg, text, sizeof text);
    unsigned long long mn = sizeof text - 1;
    unsigned char *pk = calloc(m->pk_len, 1), *sk = calloc(m->sk_len, 1),
                  *good = calloc(m->sn_len, 1), *zero = calloc(m->sn_len, 1);
    unsigned long long pl = m->pk_len, sl = m->sk_len, nl = m->sn_len;
    seed_lib(0x11);
    if (kg(pk, &pl, sk, &sl) || sg(sk, sl, msg, mn, good, &nl)) {
        fprintf(stderr, "keygen/sign failed\n"); exit(2);
    }

    int vr = vf(pk, pl, good, nl, msg, mn);
    int primed = vf(pk, pl, zero, nl, msg, mn);
    vr |= vf(pk, pl, good, nl, msg, mn);
    scrub_stack();
    int scrubbed = vf(pk, pl, zero, nl, msg, mn);
    free(pk); free(sk); free(good); free(zero);
    if (vr) { fprintf(stderr, "valid signature rejected\n"); exit(2); }
    return verdict("sig-uninit-verdict", primed != scrubbed,
        "identical all-zero signature: primed stack %s, scrubbed stack %s",
        primed == 0 ? "ACCEPTED" : "rejected",
        scrubbed == 0 ? "ACCEPTED" : "rejected");
}

/* ------------------------------------------------------- key generation */

/* Galas / VDOO / HEP-QC: key generation ignores the seeded DRNG. Two different
 * seeds in one process must give different keys. */
static int keygen_determinism(void)
{
    ngcc_sig_keygen_fn skg = NULL; ngcc_kem_keygen_fn kkg = NULL;
    unsigned long long pkl, skl;
    if (g_meta->type == NGCC_TYPE_SIG) {
        const ngcc_meta_sig_t *m = (const ngcc_meta_sig_t *)g_meta;
        skg = sym("sig_keygen"); pkl = m->pk_len; skl = m->sk_len;
    } else if (g_meta->type == NGCC_TYPE_KEM) {
        const ngcc_meta_kem_t *m = (const ngcc_meta_kem_t *)g_meta;
        kkg = sym("kem_keygen"); pkl = m->pk_len; skl = m->sk_len;
    } else { fprintf(stderr, "keygen-determinism needs a sig or kem library\n"); exit(2); }

    unsigned char *p1 = calloc(pkl,1), *s1 = calloc(skl,1),
                  *p2 = calloc(pkl,1), *s2 = calloc(skl,1);
    unsigned long long a=pkl,b=skl,a2=pkl,b2=skl;
    seed_lib(0x01); if (skg ? skg(p1,&a,s1,&b) : kkg(p1,&a,s1,&b)) { fprintf(stderr,"keygen failed\n"); exit(2); }
    seed_lib(0x99); if (skg ? skg(p2,&a2,s2,&b2) : kkg(p2,&a2,s2,&b2)) { fprintf(stderr,"keygen failed\n"); exit(2); }
    int pk_same = (a==a2) && !memcmp(p1,p2,(size_t)a);
    int sk_same = (b==b2) && !memcmp(s1,s2,(size_t)b);
    free(p1); free(s1); free(p2); free(s2);
    return verdict("keygen-determinism", pk_same && sk_same,
        "two different seeds: public key %s, secret key %s",
        pk_same ? "IDENTICAL" : "differ", sk_same ? "IDENTICAL" : "differ");
}

/* HEP-QC: the seed is ignored but an internal PRNG advances within a process,
 * so the defect shows as an identical FIRST key across fresh processes.
 * Prints a digest of the first public key; tools/reproduce.sh compares runs. */
static int keygen_fresh(const char *seedarg)
{
    unsigned char tag = (unsigned char)strtoul(seedarg, NULL, 0);
    unsigned long long pkl, skl;
    ngcc_sig_keygen_fn skg = NULL; ngcc_kem_keygen_fn kkg = NULL;
    if (g_meta->type == NGCC_TYPE_SIG) {
        const ngcc_meta_sig_t *m = (const ngcc_meta_sig_t *)g_meta;
        skg = sym("sig_keygen"); pkl = m->pk_len; skl = m->sk_len;
    } else {
        const ngcc_meta_kem_t *m = (const ngcc_meta_kem_t *)g_meta;
        kkg = sym("kem_keygen"); pkl = m->pk_len; skl = m->sk_len;
    }
    unsigned char *pk = calloc(pkl,1), *sk = calloc(skl,1);
    unsigned long long a=pkl,b=skl;
    seed_lib(tag);
    if (skg ? skg(pk,&a,sk,&b) : kkg(pk,&a,sk,&b)) { fprintf(stderr,"keygen failed\n"); exit(2); }
    uint64_t h = 1469598103934665603ULL;
    for (unsigned long long i = 0; i < a; i++) { h ^= pk[i]; h *= 1099511628211ULL; }
    printf("FRESHKEY %s seed=0x%02x pk-fnv1a=%016llx\n",
           g_meta->instance, tag, (unsigned long long)h);
    free(pk); free(sk);
    return 0;
}

/* HEP-QC: emit digests of the first key, ciphertext and shared secret from a
 * fresh process.  tools/reproduce.sh invokes this twice with different API
 * seeds; the candidate ignores both seeds and repeats all three objects. */
static int kem_enc_fresh(const char *seedarg)
{
    if (g_meta->type != NGCC_TYPE_KEM) {
        fprintf(stderr, "kem-enc-fresh needs a KEM library\n"); exit(2);
    }
    const ngcc_meta_kem_t *m = (const ngcc_meta_kem_t *)g_meta;
    ngcc_kem_keygen_fn kg = sym("kem_keygen");
    ngcc_kem_enc_fn en = sym("kem_enc");
    unsigned char tag = (unsigned char)strtoul(seedarg, NULL, 0);
    unsigned char *pk = calloc(m->pk_len, 1), *sk = calloc(m->sk_len, 1),
                  *ct = calloc(m->ct_len, 1), *ss = calloc(m->ss_len, 1);
    unsigned long long pl=m->pk_len, sl=m->sk_len, cl=m->ct_len, ssl=m->ss_len;
    seed_lib(tag);
    if (kg(pk,&pl,sk,&sl) || en(pk,pl,ss,&ssl,ct,&cl)) {
        fprintf(stderr, "keygen/enc failed\n"); exit(2);
    }
    uint64_t hp=1469598103934665603ULL, hc=hp, hs=hp;
    for (unsigned long long i=0; i<pl; i++) { hp ^= pk[i]; hp *= 1099511628211ULL; }
    for (unsigned long long i=0; i<cl; i++) { hc ^= ct[i]; hc *= 1099511628211ULL; }
    for (unsigned long long i=0; i<ssl; i++) { hs ^= ss[i]; hs *= 1099511628211ULL; }
    printf("FRESHENC %s seed=0x%02x pk=%016llx ct=%016llx ss=%016llx\n",
           g_meta->instance, tag, (unsigned long long)hp,
           (unsigned long long)hc, (unsigned long long)hs);
    free(pk); free(sk); free(ct); free(ss);
    return 0;
}

/* VDOO: key generation and signing consume a private global generator that
 * ignores the API seed.  Across fresh processes, different messages therefore
 * receive the same 16-byte signature salt (and the source uses the continued
 * stream for its vinegar and diagonal randomness). */
static int sig_random_fresh(const char *seedarg, const char *msgarg)
{
    if (g_meta->type != NGCC_TYPE_SIG) {
        fprintf(stderr, "sig-random-fresh needs a signature library\n"); exit(2);
    }
    const ngcc_meta_sig_t *m = (const ngcc_meta_sig_t *)g_meta;
    ngcc_sig_keygen_fn kg = sym("sig_keygen");
    ngcc_sig_sign_fn sg = sym("sig_sign");
    unsigned char tag = (unsigned char)strtoul(seedarg, NULL, 0);
    unsigned char msgtag = (unsigned char)strtoul(msgarg, NULL, 0);
    unsigned char msg[32];
    for (size_t i=0; i<sizeof msg; i++) msg[i] = (unsigned char)(msgtag + 13*i);
    unsigned char *pk = calloc(m->pk_len, 1), *sk = calloc(m->sk_len, 1),
                  *sn = calloc(m->sn_len + sizeof msg + 64, 1);
    unsigned long long pl=m->pk_len, sl=m->sk_len, nl=m->sn_len;
    seed_lib(tag);
    if (kg(pk,&pl,sk,&sl) || sg(sk,sl,msg,sizeof msg,sn,&nl) || nl < 16) {
        fprintf(stderr, "keygen/sign failed\n"); exit(2);
    }
    uint64_t hp=1469598103934665603ULL, ht=hp;
    for (unsigned long long i=0; i<pl; i++) { hp ^= pk[i]; hp *= 1099511628211ULL; }
    for (unsigned long long i=nl-16; i<nl; i++) { ht ^= sn[i]; ht *= 1099511628211ULL; }
    printf("FRESHSIGN %s seed=0x%02x msg=0x%02x pk=%016llx tail16=%016llx\n",
           g_meta->instance, tag, msgtag,
           (unsigned long long)hp, (unsigned long long)ht);
    free(pk); free(sk); free(sn);
    return 0;
}

/* ------------------------------------------------------------------ main */

static void usage(void)
{
    fprintf(stderr,
      "usage: ngcc_attack <check> <library> [arg]\n\n"
      "  hash-collide-zeropad <lib>            H(M) == H(M||0000000)            (Eijen)\n"
      "  hash-collide-rate    <lib> <bits>     pad10*1 boundary collision       (MasterCube)\n"
      "  hash-prefix          <libA> <libB>    short digest prefixes the long   (Megascon, Mozi)\n"
      "  kem-reject-mask      <lib>            rejection mask leaks the secret  (Cheetah, Loong)\n"
      "  kem-ct-flip          <lib>            dead implicit rejection          (Aigis-Enc+)\n"
      "  kem-decoder-fault    <lib>            q-polynomial division fault      (BRA)\n"
      "  sig-malleable        <lib>            SUF-CMA malleability             (Aigis-Sig+, CS)\n"
      "  sig-hint-padding     <lib>            ignored hint encoding            (MORNING-ATLAS)\n"
      "  sig-pors-padding     <lib>            unchecked PORS padding            (FlexTree)\n"
      "  sig-accept-all       <lib>            verifier accepts anything        (UVW)\n"
      "  sig-uninit-verdict   <lib>            verdict depends on stale stack   (SQIsign2D2)\n"
      "  keygen-determinism   <lib>            seed ignored, same key           (Galas, VDOO)\n"
      "  keygen-fresh         <lib> <seed>     per-process key digest           (HEP-QC, VDOO)\n"
      "  kem-enc-fresh        <lib> <seed>     per-process encapsulation digest (HEP-QC)\n"
      "  sig-random-fresh     <lib> <seed> <m> repeated signing salt            (VDOO)\n");
    exit(2);
}

int main(int argc, char **argv)
{
    if (argc < 3) usage();
    const char *check = argv[1];
    arm_faults();
    load(argv[2]);
    if (!strcmp(check, "hash-collide-zeropad")) return hash_collide_zeropad();
    if (!strcmp(check, "hash-collide-rate")) {
        if (argc < 4) usage();
        return hash_collide_rate(strtoull(argv[3], NULL, 0));
    }
    if (!strcmp(check, "hash-prefix")) { if (argc < 4) usage(); return hash_prefix(argv[3]); }
    if (!strcmp(check, "kem-reject-mask"))    return kem_reject_mask();
    if (!strcmp(check, "kem-ct-flip"))        return kem_ct_flip();
    if (!strcmp(check, "kem-decoder-fault"))  return kem_decoder_fault();
    if (!strcmp(check, "sig-malleable"))      return sig_malleable();
    if (!strcmp(check, "sig-hint-padding"))   return sig_hint_padding();
    if (!strcmp(check, "sig-pors-padding"))   return sig_pors_padding();
    if (!strcmp(check, "sig-accept-all"))     return sig_accept_all();
    if (!strcmp(check, "sig-uninit-verdict")) return sig_uninit_verdict();
    if (!strcmp(check, "keygen-determinism")) return keygen_determinism();
    if (!strcmp(check, "keygen-fresh"))       { if (argc < 4) usage(); return keygen_fresh(argv[3]); }
    if (!strcmp(check, "kem-enc-fresh"))      { if (argc < 4) usage(); return kem_enc_fresh(argv[3]); }
    if (!strcmp(check, "sig-random-fresh"))   { if (argc < 5) usage(); return sig_random_fresh(argv[3], argv[4]); }
    usage();
    return 2;
}
