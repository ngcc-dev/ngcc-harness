/* Key generation through the frozen sig_keygen (fast but output-identical
   matrix inverse).  usage: keygen_fast SEED48FILE pk.out sk.out */
#include "SIG_fast.c"
DRNG_ctx drng_algorithm;
int main(int argc, char **argv) {
    unsigned char s[48]; FILE *f = fopen(argv[1], "rb"); if (!f || fread(s, 1, 48, f) != 48) return 2; fclose(f);
    init_random_number(uvw_drng, s, 48);
    unsigned long long pl = sig_get_pk_len_bytes(), sl = sig_get_sk_len_bytes();
    unsigned char *pk = malloc(pl), *sk = malloc(sl);
    if (sig_keygen(pk, &pl, sk, &sl)) { fprintf(stderr, "keygen failed\n"); return 1; }
    f = fopen(argv[2], "wb"); fwrite(pk, 1, pl, f); fclose(f);
    f = fopen(argv[3], "wb"); fwrite(sk, 1, sl, f); fclose(f);
    fprintf(stderr, "pk=%llu sk=%llu\n", pl, sl); return 0;
}
