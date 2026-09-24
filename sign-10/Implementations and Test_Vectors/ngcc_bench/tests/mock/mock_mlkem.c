/*
 * mock_mlkem.c – Simplified ML-KEM-512 mock for ngcc_bench verification.
 *
 * Parameter sizes match NIST ML-KEM-512:
 *   pk = 800 bytes,  sk = 1632 bytes,  ct = 768 bytes,  ss = 32 bytes
 *
 * Internal algorithm (NOT cryptographically secure):
 *   keygen:  pk = random(800), sk = random(32) || pk   (sk_seed at offset 0)
 *   enc:     ss = random(32)
 *             ct = XOR-expand(ss, pk_hash) padded to 768 bytes
 *             (pk_hash = simple rolling hash of pk → 32 bytes)
 *   dec:     recover ss by reversing the XOR with pk embedded in sk
 *
 * Build:  cc -shared -fPIC -O2 mock_mlkem.c -o libmock_mlkem.so
 *
 * Also provides stub implementations of hash/sig/kex so that
 * --test all works without missing symbols.
 */

#include <string.h>
#include <stdlib.h>

/* ================================================================
 *  Portable random fill – getrandom(2) with /dev/urandom fallback
 * ================================================================ */
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/random.h>

static int fill_random(unsigned char *buf, unsigned long long len) {
    unsigned long long off = 0;
    while (off < len) {
        ssize_t got = getrandom(buf + off, (size_t)(len - off), 0);
        if (got > 0) { off += (unsigned long long)got; continue; }
        if (got < 0 && errno == EINTR) continue;
        break;
    }
    if (off == len) return 0;
    /* fallback */
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0) return -1;
    while (off < len) {
        ssize_t got = read(fd, buf + off, (size_t)(len - off));
        if (got > 0) { off += (unsigned long long)got; continue; }
        if (got < 0 && errno == EINTR) continue;
        close(fd); return -1;
    }
    close(fd);
    return 0;
}

/* ================================================================
 *  ML-KEM-512 parameter sizes
 * ================================================================ */
#define MLKEM_PK_BYTES  800
#define MLKEM_SK_BYTES  1632   /* 32-byte seed + 800-byte pk + 800-byte extra */
#define MLKEM_CT_BYTES  768
#define MLKEM_SS_BYTES  32

#define MLKEM_SEED_BYTES 32

/* ================================================================
 *  Helpers
 * ================================================================ */

/* Simple deterministic hash: pk (800 B) → 32-byte digest.
 * Used to make ct derivation depend on pk so dec can check consistency. */
static void pk_hash(const unsigned char *pk, unsigned long long pk_len,
                    unsigned char out[MLKEM_SS_BYTES]) {
    unsigned long long i;
    memset(out, 0, MLKEM_SS_BYTES);
    for (i = 0; i < pk_len; ++i) {
        out[i % MLKEM_SS_BYTES] ^= pk[i];
        out[i % MLKEM_SS_BYTES] += (unsigned char)(i & 0xFF);
    }
}

/* XOR buf with a repeating key of key_len. */
static void xor_buf(unsigned char *buf, unsigned long long buf_len,
                    const unsigned char *key, unsigned long long key_len) {
    unsigned long long i;
    for (i = 0; i < buf_len; ++i) {
        buf[i] ^= key[i % key_len];
    }
}

/* ================================================================
 *  KEM API
 * ================================================================ */

unsigned long long kem_get_pk_len_bytes(void) { return MLKEM_PK_BYTES; }
unsigned long long kem_get_sk_len_bytes(void) { return MLKEM_SK_BYTES; }
unsigned long long kem_get_ss_len_bytes(void) { return MLKEM_SS_BYTES; }
unsigned long long kem_get_ct_len_bytes(void) { return MLKEM_CT_BYTES; }

/*
 * keygen: sk = seed(32) || pk(800) || filler(800)
 *         pk = random 800 bytes
 */
int kem_keygen(unsigned char *pk, unsigned long long *pk_len_bytes,
               unsigned char *sk, unsigned long long *sk_len_bytes) {
    unsigned char seed[MLKEM_SEED_BYTES];

    if (fill_random(pk, MLKEM_PK_BYTES) != 0) return -1;
    if (fill_random(seed, MLKEM_SEED_BYTES) != 0) return -1;

    /* sk layout: [seed 32B][pk 800B][filler 800B] */
    memcpy(sk, seed, MLKEM_SEED_BYTES);
    memcpy(sk + MLKEM_SEED_BYTES, pk, MLKEM_PK_BYTES);
    /* fill the remaining space with deterministic data derived from seed */
    {
        unsigned long long filler_len = MLKEM_SK_BYTES - MLKEM_SEED_BYTES - MLKEM_PK_BYTES;
        unsigned long long i;
        for (i = 0; i < filler_len; ++i) {
            sk[MLKEM_SEED_BYTES + MLKEM_PK_BYTES + i] = seed[i % MLKEM_SEED_BYTES] ^ (unsigned char)i;
        }
    }

    *pk_len_bytes = MLKEM_PK_BYTES;
    *sk_len_bytes = MLKEM_SK_BYTES;
    return 0;
}

/*
 * enc(pk) → (ss, ct)
 *
 *   1. Generate random ss (32 bytes)
 *   2. Compute pk_digest = pk_hash(pk)
 *   3. ct = ss || zeros..., then XOR with repeating pk_digest,
 *      then XOR with repeating pk (making ct depend on pk)
 */
int kem_enc(unsigned char *pk, unsigned long long pk_len_bytes,
            unsigned char *ss, unsigned long long *ss_len_bytes,
            unsigned char *ct, unsigned long long *ct_len_bytes) {
    unsigned char pk_digest[MLKEM_SS_BYTES];

    if (pk_len_bytes < MLKEM_PK_BYTES) return -1;

    /* random shared secret */
    if (fill_random(ss, MLKEM_SS_BYTES) != 0) return -1;

    /* build ct: start with ss padded to ct length */
    memset(ct, 0, MLKEM_CT_BYTES);
    memcpy(ct, ss, MLKEM_SS_BYTES);

    /* first mix: XOR with pk_digest */
    pk_hash(pk, MLKEM_PK_BYTES, pk_digest);
    xor_buf(ct, MLKEM_CT_BYTES, pk_digest, MLKEM_SS_BYTES);

    /* second mix: XOR with pk itself (makes ct fully depend on pk) */
    xor_buf(ct, MLKEM_CT_BYTES, pk, MLKEM_PK_BYTES);

    *ss_len_bytes = MLKEM_SS_BYTES;
    *ct_len_bytes = MLKEM_CT_BYTES;
    return 0;
}

/*
 * dec(sk, ct) → ss
 *
 *   1. Extract pk from sk (offset 32)
 *   2. Reverse the XOR operations done in enc
 *   3. Extract first 32 bytes as ss
 */
int kem_dec(unsigned char *sk, unsigned long long sk_len_bytes,
            unsigned char *ct, unsigned long long ct_len_bytes,
            unsigned char *ss, unsigned long long *ss_len_bytes) {
    unsigned char pk_digest[MLKEM_SS_BYTES];
    unsigned char buf[MLKEM_CT_BYTES];
    const unsigned char *pk;

    if (sk_len_bytes < MLKEM_SK_BYTES || ct_len_bytes < MLKEM_CT_BYTES) return -1;

    /* extract pk from sk */
    pk = sk + MLKEM_SEED_BYTES;

    /* copy ct and reverse the enc operations */
    memcpy(buf, ct, MLKEM_CT_BYTES);

    /* reverse second mix: XOR with pk */
    xor_buf(buf, MLKEM_CT_BYTES, pk, MLKEM_PK_BYTES);

    /* reverse first mix: XOR with pk_digest */
    pk_hash(pk, MLKEM_PK_BYTES, pk_digest);
    xor_buf(buf, MLKEM_CT_BYTES, pk_digest, MLKEM_SS_BYTES);

    /* first 32 bytes = ss */
    memcpy(ss, buf, MLKEM_SS_BYTES);
    *ss_len_bytes = MLKEM_SS_BYTES;
    return 0;
}

/* ================================================================
 *  Hash stub (required for --test all)
 * ================================================================ */

int CryptHash(int digest_len_bits,
              const unsigned char *msg,
              unsigned long long msg_len_bits,
              unsigned char *digest) {
    unsigned long long msg_len = msg_len_bits / 8ULL;
    unsigned long long digest_len = (unsigned long long)digest_len_bits / 8ULL;
    unsigned long long i;
    unsigned char acc = (unsigned char)digest_len_bits;

    if (digest_len == 0) digest_len = 1;
    for (i = 0; i < digest_len; ++i) digest[i] = acc;
    for (i = 0; i < msg_len; ++i) {
        digest[i % digest_len] ^= msg[i];
        acc = (unsigned char)((acc * 31u + msg[i]) & 0xFFu);
        digest[(i + 1u) % digest_len] ^= acc;
    }
    return 0;
}

/* ================================================================
 *  Signature stubs (required for --test all)
 * ================================================================ */

unsigned long long sig_get_pk_len_bytes(void) { return 32; }
unsigned long long sig_get_sk_len_bytes(void) { return 64; }
unsigned long long sig_get_sn_len_bytes(void) { return 64; }

int sig_keygen(unsigned char *pk, unsigned long long *pk_len_bytes,
               unsigned char *sk, unsigned long long *sk_len_bytes) {
    if (fill_random(pk, 32) != 0) return -1;
    if (fill_random(sk, 32) != 0) return -1;
    memcpy(sk + 32, pk, 32);   /* sk = sk_seed || pk */
    *pk_len_bytes = 32;
    *sk_len_bytes = 64;
    return 0;
}

int sig_sign(unsigned char *sk, unsigned long long sk_len_bytes,
             unsigned char *m,  unsigned long long m_len_bytes,
             unsigned char *sn, unsigned long long *sn_len_bytes) {
    unsigned long long i;
    (void)sk_len_bytes;
    memset(sn, 0, 64);
    for (i = 0; i < m_len_bytes && i < 64; ++i)
        sn[i] = m[i] ^ sk[i % 32];
    for (; i < 64; ++i)
        sn[i] = sk[i % 32];
    *sn_len_bytes = 64;
    return 0;
}

int sig_verify(unsigned char *pk, unsigned long long pk_len_bytes,
               unsigned char *sn, unsigned long long sn_len_bytes,
               unsigned char *m,  unsigned long long m_len_bytes) {
    /* always accept (stub) */
    (void)pk; (void)pk_len_bytes;
    (void)sn; (void)sn_len_bytes;
    (void)m;  (void)m_len_bytes;
    return 0;
}

/* ================================================================
 *  KEX stubs (required for --test all)
 * ================================================================ */

unsigned long long kex_get_passes_num(void)        { return 3; }
unsigned long long kex_get_pk_len_bytes(void)      { return 32; }
unsigned long long kex_get_sk_len_bytes(void)      { return 32; }
unsigned long long kex_get_sta_len_bytes(void)     { return 64; }
unsigned long long kex_get_stb_len_bytes(void)     { return 64; }
unsigned long long kex_get_ss_len_bytes(void)      { return 32; }
unsigned long long kex_get_total_msg_len_bytes(void) { return 96; }

int kex_init_a(unsigned char *pka, unsigned long long *pka_len_bytes,
               unsigned char *ska, unsigned long long *ska_len_bytes,
               unsigned char *sta, unsigned long long *sta_len_bytes) {
    if (fill_random(pka, 32) != 0) return -1;
    if (fill_random(ska, 32) != 0) return -1;
    memset(sta, 0, 64);
    memcpy(sta, pka, 32);
    *pka_len_bytes = 32; *ska_len_bytes = 32; *sta_len_bytes = 64;
    return 0;
}

int kex_init_b(unsigned char *pkb, unsigned long long *pkb_len_bytes,
               unsigned char *skb, unsigned long long *skb_len_bytes,
               unsigned char *stb, unsigned long long *stb_len_bytes) {
    if (fill_random(pkb, 32) != 0) return -1;
    if (fill_random(skb, 32) != 0) return -1;
    memset(stb, 0, 64);
    memcpy(stb, pkb, 32);
    *pkb_len_bytes = 32; *skb_len_bytes = 32; *stb_len_bytes = 64;
    return 0;
}

int kex_generate_pass1_msg_a(unsigned char *ska, unsigned long long ska_len_bytes,
                             unsigned char *pkb, unsigned long long pkb_len_bytes,
                             unsigned char *sta, unsigned long long *sta_len_bytes,
                             unsigned char *m1,  unsigned long long *m1_len_bytes) {
    unsigned long long i;
    (void)ska_len_bytes; (void)pkb_len_bytes;
    for (i = 0; i < 32; ++i) m1[i] = ska[i] ^ pkb[i];
    memcpy(sta + 32, m1, 32);
    *sta_len_bytes = 64; *m1_len_bytes = 32;
    return 0;
}

int kex_generate_pass2_msg_b(unsigned char *skb, unsigned long long skb_len_bytes,
                             unsigned char *pka, unsigned long long pka_len_bytes,
                             unsigned char *m1,  unsigned long long m1_len_bytes,
                             unsigned char *stb, unsigned long long *stb_len_bytes,
                             unsigned char *m2,  unsigned long long *m2_len_bytes) {
    unsigned long long i;
    (void)skb_len_bytes; (void)pka_len_bytes; (void)m1_len_bytes;
    for (i = 0; i < 32; ++i) m2[i] = skb[i] ^ pka[i] ^ m1[i];
    memcpy(stb + 32, m2, 32);
    *stb_len_bytes = 64; *m2_len_bytes = 32;
    return 0;
}

int kex_generate_pass3_msg_a(unsigned char *ska, unsigned long long ska_len_bytes,
                             unsigned char *pkb, unsigned long long pkb_len_bytes,
                             unsigned char *m2,  unsigned long long m2_len_bytes,
                             unsigned char *sta, unsigned long long *sta_len_bytes,
                             unsigned char *m3,  unsigned long long *m3_len_bytes) {
    unsigned long long i;
    (void)ska_len_bytes; (void)pkb_len_bytes; (void)m2_len_bytes; (void)sta;
    for (i = 0; i < 32; ++i) m3[i] = ska[i] ^ pkb[i] ^ m2[i];
    *sta_len_bytes = 64; *m3_len_bytes = 32;
    return 0;
}

int kex_derive_ss_a(unsigned char *ska, unsigned long long ska_len_bytes,
                    unsigned char *pkb, unsigned long long pkb_len_bytes,
                    unsigned char *mb,  unsigned long long mb_len_bytes,
                    unsigned char *sta, unsigned long long sta_len_bytes,
                    unsigned char *ssa, unsigned long long *ssa_len_bytes) {
    unsigned long long i;
    (void)ska_len_bytes; (void)pkb_len_bytes; (void)mb_len_bytes; (void)sta_len_bytes;
    for (i = 0; i < 32; ++i) ssa[i] = ska[i] ^ pkb[i] ^ mb[i % 32] ^ sta[i];
    *ssa_len_bytes = 32;
    return 0;
}

int kex_derive_ss_b(unsigned char *skb, unsigned long long skb_len_bytes,
                    unsigned char *pka, unsigned long long pka_len_bytes,
                    unsigned char *ma,  unsigned long long ma_len_bytes,
                    unsigned char *stb, unsigned long long stb_len_bytes,
                    unsigned char *ssb, unsigned long long *ssb_len_bytes) {
    unsigned long long i;
    (void)skb_len_bytes; (void)pka_len_bytes; (void)ma_len_bytes; (void)stb_len_bytes;
    for (i = 0; i < 32; ++i) ssb[i] = skb[i] ^ pka[i] ^ ma[i % 32] ^ stb[i];
    *ssb_len_bytes = 32;
    return 0;
}
