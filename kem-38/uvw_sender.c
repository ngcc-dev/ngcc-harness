/* Sender side with the frozen reference code: fresh key pair + nct encapsulations.
 * usage: uvw_sender seedbyte nct dir
 * writes dir/pk.bin, dir/sk.bin (scoring only), dir/truth.txt (D map, scoring only),
 * dir/ct<t>.bin, dir/ss<t>.bin (sender's shared secret, scoring only).
 * Also runs the legitimate kem_dec on each ct as a sanity check of the reference code. */
#include "KEM_AlgorithmInstance.c"
DRNG_ctx drng_algorithm;
static void wr(const char *dir, const char *name, const void *p, size_t len) {
    char path[512]; snprintf(path, sizeof path, "%s/%s", dir, name);
    FILE *f = fopen(path, "wb"); fwrite(p, 1, len, f); fclose(f);
}
int main(int argc, char **argv) {
    if (argc < 4) return 2;
    int nct = atoi(argv[2]); const char *dir = argv[3];
    unsigned char seed[48];
    for (int i = 0; i < 48; ++i) seed[i] = (unsigned char)(i * 53 + 7);
    seed[0] ^= (unsigned char)atoi(argv[1]); seed[1] ^= 0x5a;
    init_random_number(&drng_algorithm, seed, sizeof seed);
    unsigned long long pl = kem_get_pk_len_bytes(), sl = kem_get_sk_len_bytes(), cl = kem_get_ct_len_bytes(), ssl = kem_get_ss_len_bytes();
    unsigned char *pk = malloc(pl), *sk = malloc(sl), *ct = malloc(cl), ss[64], ss2[64];
    if (kem_keygen(pk, &pl, sk, &sl) != 0) return 3;
    wr(dir, "pk.bin", pk, kem_get_pk_len_bytes()); wr(dir, "sk.bin", sk, kem_get_sk_len_bytes());
    private_key_t *s = malloc(sizeof *s); expand_D(sk, sk[PRIVATE_KEY_SEED_BYTES + 2], s);
    char path[512]; snprintf(path, sizeof path, "%s/truth.txt", dir); FILE *f = fopen(path, "w");
    for (int c = 0; c < UVW_N; ++c) fprintf(f, "%d %d %d\n", c, s->D_perm[c], s->D_coeffs[c]);
    fclose(f);
    int legit_ok = 0;
    for (int t = 0; t < nct; ++t) {
        unsigned long long cl2 = cl, ssl2 = ssl;
        if (kem_enc(pk, pl, ss, &ssl2, ct, &cl2) != 0) return 4;
        char nm[64];
        snprintf(nm, sizeof nm, "ct%d.bin", t); wr(dir, nm, ct, cl);
        snprintf(nm, sizeof nm, "ss%d.bin", t); wr(dir, nm, ss, 64);
        unsigned long long l3 = 64; memset(ss2, 0, 64);
        int rc = kem_dec(sk, sl, ct, cl, ss2, &l3);
        if (rc == 0 && !memcmp(ss, ss2, 64)) legit_ok++;
    }
    fprintf(stderr, "kg_enc seed=%s: pk %llu B, ct %llu B, %d ciphertexts, legitimate kem_dec matched %d/%d\n", argv[1], pl, cl, nct, legit_ok, nct);
    return 0;
}
