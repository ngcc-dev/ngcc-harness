/* Measures the exact decryption noise Y_i (value 2Y+m = centered(2(v'-us))) of honest
   Cheetah encapsulations using the submitted reference code (KEM_Cheetah.c included verbatim;
   noise_fn.h is indcpa_decrypt with the final mod-2 step replaced by a copy-out). */
#include "KEM_Cheetah.c"
#include "noise_fn.h"
#include <stdlib.h>
#include <math.h>
DRNG_ctx drng_algorithm;
int main(int argc, char **argv){
    long iters = atol(argv[1]); unsigned char seed[48]={0}; seed[0]=argc>2?atoi(argv[2]):1;
    init_random_number(&drng_algorithm, seed, 48);
    static unsigned char pk[PUBLICKEY_BYTES], sk[SECRETKEY_BYTES], ct[CIPHERTEXT_BYTES], ss[64], ss2[64];
    unsigned long long pkl, skl, ssl, ctl;
    kem_keygen(pk,&pkl,sk,&skl);
    double s2=0; long cnt=0, hist[8]={0}, fails=0; int thr[8]={600,800,1000,1100,1200,1300,1400,1500};
    for(long it=0; it<iters; it++){
        kem_enc(pk,pkl,ss,&ssl,ct,&ctl);
        int16_t val[SHARED_KEY_BYTES*8]; noise_vals(ct, sk, val);
        for(int i=0;i<SHARED_KEY_BYTES*8;i++){ int y = (val[i] - (val[i]&1))/2;
            s2 += (double)y*y; cnt++; for(int j=0;j<8;j++) if (abs(y)>=thr[j]) hist[j]++; }
        if (it < 2000) { kem_dec(sk,skl,ct,ctl,ss2,&ssl); if (memcmp(ss,ss2,SHARED_KEY_BYTES)) fails++; }
    }
    printf("%s: %ld encaps, %ld coefs, sigma=%.2f, decaps mismatches in first 2000: %ld\n", ALGORITHM_INSTANCE, iters, cnt, sqrt(s2/cnt), fails);
    for(int j=0;j<8;j++) printf("  P(|Y|>=%d) = %ld / %ld = %.3e\n", thr[j], hist[j], cnt, (double)hist[j]/cnt);
}
