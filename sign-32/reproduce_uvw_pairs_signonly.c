/* Sign-only harness for parameter sets whose key generation is too slow
   for a minutes-scale experiment.  It draws seed and nonces exactly as
   sig_keygen does (but skips the H1-invertibility retry, which does not
   affect the signing distribution: sign uses H1^T, never H1^{-1}) and calls
   the frozen sig_sign.  It records the signer's full weight-w vector fe,
   which equals the publicly recomputable (H(m,r) - e R^T || e) whenever the
   key pair is valid (Alg. 7/8 correctness, spec eq. (1)-(4)).
   usage: uvw_signonly KEYSEEDHEX WORKERSEED N out.vecs out.truth */
#include "SIG_AlgorithmInstance.c"
#include <stdint.h>
DRNG_ctx drng_algorithm;
extern vf3_e *uvw_last_fe;
int main(int argc, char **argv) {
    uvw_param P = uvw_param_from_level(
#if SEED_LEN_BYTES == 64
        3
#else
        LEVEL
#endif
    );
    unsigned char ks[64] = {0}; strncpy((char*)ks, argv[1], 63);
    init_random_number(uvw_drng, ks, 48);
    unsigned char sk[SEED_LEN_BYTES + 8];
    get_random_number(uvw_drng, sk, SEED_LEN_BYTES * 8);
    for (int i = 0; i < 4; i++) { uint16_t nn; get_random_number(uvw_drng, (unsigned char*)&nn, 16); sk[SEED_LEN_BYTES+2*i] = nn >> 8; sk[SEED_LEN_BYTES+2*i+1] = nn & 0xff; }
    if (argv[1][0] == '@') { FILE *fk = fopen(argv[1] + 1, "rb"); if (!fk || fread(sk, 1, sizeof sk, fk) != sizeof sk) { fprintf(stderr, "bad sk file\n"); return 1; } fclose(fk); }
    unsigned char ws[64] = {0}; strncpy((char*)ws, argv[2], 63);
    init_random_number(uvw_drng, ws, 48);
    int N = atoi(argv[3]);
    unsigned long long snl = sn_serialized_size(&P);
    unsigned char *sn = malloc(snl); unsigned long long sl;
    FILE *fo = fopen(argv[4], "wb"); uint8_t *v = malloc(P.n);
    for (int j = 0; j < N; j++) {
        char m[64]; int ml = snprintf(m, sizeof m, "sign-32-3 %s %d", argv[2], j);
        if (sig_sign(sk, sizeof sk, (unsigned char*)m, ml, sn, &sl)) return 1;
        size_t wt = vf3_hamming_weight(uvw_last_fe);
        if (wt != P.w) fprintf(stderr, "weight %zu != %zu\n", wt, (size_t)P.w);
        for (size_t i = 0; i < P.n; i++) v[i] = vf3_get_element(i, uvw_last_fe);
        fwrite(v, 1, P.n, fo); fflush(fo);
    }
    fclose(fo);
    uint16_t nd = ((uint16_t)sk[SEED_LEN_BYTES + 6] << 8) | sk[SEED_LEN_BYTES + 7];
    dp_e *d = dp_alloc(P.n); generate_dp_matrix(d, sk, nd);
    FILE *ft = fopen(argv[5], "wb");
    for (size_t i = 0; i < P.n; i++) { uint32_t p = d->p[i]; uint8_t dv = vf3_get_element(i, d->d); fwrite(&p, 4, 1, ft); fwrite(&dv, 1, 1, ft); }
    fclose(ft);
    return 0;
}
