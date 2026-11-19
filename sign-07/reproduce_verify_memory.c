/* sign-07: CS verifier memory defects.
 *
 * LEAK build (-DLEAK -Wl,--wrap=malloc,--wrap=free): CS_Verify allocates
 * buf = malloc(len + HBYTES) (cs.c:157) and frees it only before the final
 * "return true" (cs.c:209); the four "return false" exits leak it. The
 * driver counts live blocks and bytes across 100 accepted verifications, 100
 * verifications of the signature against a different message (challenge
 * mismatch, cs.c:200) and 100 verifications of a signature whose z2 rANS
 * length field is corrupted (sigDecode failure, cs.c:163). It also writes
 * pk, a valid signature and its length to argv[1].
 *
 * ASAN build: sig_verify passes sn without its length (SIG_AlgorithmInstance.c
 * sig_verify calls CS_Verify(pk, m, m_len_bytes, sn)). The signature from
 * argv[1] is copied into a heap buffer of exactly L bytes and verified with
 * declared length L. argv[2] "full": L = SIGNATUREBYTES (control). "short":
 * L = one byte less than the bytes the decoder consumes, i.e. the offset of
 * the last rANS field (SIGNATUREBYTES - POLYZ2_PACKEDBYTES) plus its 2-byte
 * length plus that length; the rest of the fixed-size signature is padding
 * that verification never reads.
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

unsigned long long sig_get_pk_len_bytes(void), sig_get_sk_len_bytes(void), sig_get_sn_len_bytes(void);
int sig_keygen(unsigned char *, unsigned long long *, unsigned char *, unsigned long long *);
int sig_sign(unsigned char *, unsigned long long, unsigned char *, unsigned long long,
             unsigned char *, unsigned long long *);
int sig_verify(unsigned char *, unsigned long long, unsigned char *, unsigned long long,
               unsigned char *, unsigned long long);

#ifdef LEAK
void *__real_malloc(size_t);
void __real_free(void *);
static long live; static long long live_bytes;
static void *ptrs[4096]; static size_t sizes[4096];
void *__wrap_malloc(size_t n)
{
    void *p = __real_malloc(n);
    if (p) { live++; live_bytes += n; for (int i = 0; i < 4096; i++) if (!ptrs[i]) { ptrs[i] = p; sizes[i] = n; break; } }
    return p;
}
void __wrap_free(void *p)
{
    if (p) for (int i = 0; i < 4096; i++) if (ptrs[i] == p) { live--; live_bytes -= sizes[i]; ptrs[i] = 0; break; }
    __real_free(p);
}
#endif

int main(int argc, char **argv)
{
    unsigned long long PL = sig_get_pk_len_bytes(), SL = sig_get_sk_len_bytes(), NL = sig_get_sn_len_bytes(), a, b;
    unsigned char *pk = malloc(PL), *sk = malloc(SL), *sig = malloc(NL);
    unsigned char m[28], m2[28];
    for (int i = 0; i < 28; i++) { m[i] = (unsigned char)i; m2[i] = (unsigned char)(i ^ 0x5a); }
    if (argc < 2) return 2;
#ifdef LEAK
    if (sig_keygen(pk, &a, sk, &b) || sig_sign(sk, SL, m, sizeof m, sig, &b)) return 3;
    unsigned long long siglen = b;
    /* copy with the last rANS length field (z2) corrupted: sigDecode fails */
    unsigned char *bad = malloc(NL); memcpy(bad, sig, NL);
    bad[NL - POLYZ2 + 0] ^= 0xff; bad[NL - POLYZ2 + 1] ^= 0x7f;
    long b0 = live; long long y0 = live_bytes; int acc = 0, rej1 = 0, rej2 = 0;
    for (int i = 0; i < 100; i++) acc += sig_verify(pk, PL, sig, siglen, m, sizeof m) == 0;
    long b1 = live; long long y1 = live_bytes;
    for (int i = 0; i < 100; i++) rej1 += sig_verify(pk, PL, sig, siglen, m2, sizeof m2) != 0;
    long b2 = live; long long y2 = live_bytes;
    for (int i = 0; i < 100; i++) rej2 += sig_verify(pk, PL, bad, siglen, m, sizeof m) != 0;
    long b3 = live; long long y3 = live_bytes;
    printf("accepted=%d leaked_blocks=%ld leaked_bytes=%lld; challenge_reject=%d leaked_blocks=%ld leaked_bytes=%lld; "
           "decode_reject=%d leaked_blocks=%ld leaked_bytes=%lld\n",
           acc, b1 - b0, y1 - y0, rej1, b2 - b1, y2 - y1, rej2, b3 - b2, y3 - y2);
    FILE *f = fopen(argv[1], "wb");
    if (!f || fwrite(pk, 1, PL, f) != PL || fwrite(sig, 1, NL, f) != NL) return 4;
    fclose(f);
#else
    FILE *f = fopen(argv[1], "rb");
    if (!f || fread(pk, 1, PL, f) != PL || fread(sig, 1, NL, f) != NL) return 4;
    fclose(f);
    unsigned long long used = NL - POLYZ2 + 2 + (sig[NL - POLYZ2] | sig[NL - POLYZ2 + 1] << 8);
    unsigned long long L = (argc > 2 && !strcmp(argv[2], "short")) ? used - 1 : NL;
    if (L > NL) return 2;
    unsigned char *t = malloc(L ? L : 1);
    memcpy(t, sig, L);
    int rc = sig_verify(pk, PL, t, L, m, sizeof m);
    printf("siglen=%llu used=%llu declared=%llu verify rc=%d\n", NL, used, L, rc);
    free(t);
#endif
    return 0;
}
