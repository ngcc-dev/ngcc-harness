#include "sig_core.c"
#include "SIG_AlgorithmInstance.h"

DRNG_ctx drng_algorithm;
void sig_unpack_public_key(const unsigned char pk[PK_LEN_BYTES], unsigned char H_seed[SEED_BYTES], unsigned long y_bits[M_K + 12]);
int sig_deserialize_signature(const unsigned char *sn, unsigned long long sn_len_bytes, RawGF2EX *e_eval_commit, RawGF2EX *v_eval_commit, unsigned char root_hash[total_groups][HASH_DIGEST_LENGTH], RawGF2EX p_poly_interactive[SIGMA_REPETITIONS], RawGF2EX rv_out_interactive[SIGMA_REPETITIONS], RawGF2EX G_coeffs_raw[6], RawGF2EX w_hat_prime_raw[N_PRIME_LEN], unsigned char commitment_open_hashes[MERKLE_MAX_PROOF_HASHES][HASH_DIGEST_LENGTH], size_t *commitment_open_hash_count, RawGF2EX e_hat_encoded_commitment_open_output[opennum]);

static unsigned long Hb[embded_num][M], yb[embded_num], eb[M];
static RawGF2EX Hh[ROWS][COLS];
static uint16_t yh[ROWS];
static VerifierWorkspace vws;
static unsigned char sn[MAX_SIGNATURE_BYTES+64];
static RawGF2EX ee,ve,pp[SIGMA_REPETITIONS],rv[SIGMA_REPETITIONS],G[6],W[N_PRIME_LEN],eo[opennum];
static unsigned char rh[total_groups][HASH_DIGEST_LENGTH],oh[MERKLE_MAX_PROOF_HASHES][HASH_DIGEST_LENGTH];

static void putraw(FILE *f,const RawGF2EX *a) {
    for (int k=0;k<RAW_POLY_LEN;k++) fprintf(f,"%04x",a->coeffs[k]);
}

int main(int argc,char **argv) {
    if (argc != 4 && argc != 6) {
        fprintf(stderr,"usage: %s seed_offset public_output truth_output [worker_id max_signatures]\n",argv[0]);
        return 2;
    }
    int natural = argc == 6;
    unsigned long maximum = natural ? strtoul(argv[5],NULL,10) : 1;
    unsigned char seed[48];
    for(int i=0;i<48;i++) seed[i]=(unsigned char)(i*13+atoi(argv[1]));
    init_random_number(&drng_algorithm,seed,48);
    unsigned char pk[PK_LEN_BYTES],sk[SK_LEN_BYTES];
    unsigned long long pl,sl,snl;
    if(sig_keygen(pk,&pl,sk,&sl)) return 3;
    if(natural) {
        unsigned worker = (unsigned)strtoul(argv[4],NULL,10);
        for(int i=0;i<48;i++) seed[i]=(unsigned char)(i*13+worker*29+91);
        init_random_number(&drng_algorithm,seed,48);
    }
    unsigned char msg[80]="zeta-zero leak test";
    size_t msglen=sizeof("zeta-zero leak test");

    /* Only pk, message, and signature are used to construct these equations. */
    unsigned char Hs[SEED_BYTES]; sig_unpack_public_key(pk,Hs,yb);
    random_H(Hb,Hs); embed_H_raw(Hh,Hb); embed_y_raw(yh,yb);
    unsigned char fsin[FS_COMMITMENT_INPUT_BYTES];
    memset(fsin,0,HASH_DIGEST_LENGTH); memcpy(fsin+HASH_DIGEST_LENGTH,pk,PK_LEN_BYTES);
    RawGF2EX z,D,vc1,vc2;
    uint16_t observed_zeta=0;
    unsigned long attempts=0;
    int found=0;
    for(unsigned long trial=0;trial<maximum;trial++) {
        if(natural) {
            int n=snprintf((char *)msg,sizeof msg,"zeta-zero leak test %s %lu",argv[4],trial);
            if(n<0 || (size_t)n>=sizeof msg) return 2;
            msglen=(size_t)n+1;
        }
        if(sig_sign(sk,sl,msg,msglen,sn,&snl)) {
            if(natural) continue;
            return 4;
        }
        attempts++;
        size_t ohc=0;
        if(sig_deserialize_signature(sn,snl,&ee,&ve,rh,pp,rv,G,W,oh,&ohc,eo)) return 6;
        sig_init_verifier_workspace(&vws);
        if(!Verifier(&vws,msg,msglen,fsin,Hh,yh,&ee,&ve,rh,pp,rv,G,W,oh,ohc,eo,&z,&D,&vc1,&vc2)) return 7;
        hash_to_rand_PolyRes(vws.tau_vec_raw,&observed_zeta,vws.fs_value_polyres);
        if(!natural || observed_zeta==0) { found=1; break; }
    }
    if(natural && !found) {
        fprintf(stderr,"worker %s: no zero challenge in %lu signatures\n",argv[4],attempts);
        return 77;
    }
    int accepted=sig_verify(pk,pl,sn,snl,msg,msglen);
    printf("signature verifier return = %d (zero means accepted)\n",accepted);
    if(accepted) return 5;
    printf("derived challenge zeta = %u after %lu signatures\n",(unsigned)observed_zeta,attempts);
    RawGF2EX f[7]; get_polydef_cached(f);
    FILE *o=fopen(argv[2],"w"); if(!o) return 8;
    fprintf(o,"%d %d %d %d\n",COLS,RAW_POLY_LEN,embded_num,M);
    for(int i=0;i<COLS;i++) for(int a=0;a<6;a++) {
        RawGF2EX e; memset(&e,0,sizeof e); e.coeffs[a]=1;
        RawGF2EX_Product pr; RawGF2EX De,v,g[6];
        mul_raw(&pr,&D,&e); rem_raw(&De,&pr);
        add_raw(&v,&W[i],&De);
        get_gi_coeffs_from_formula_Raw(g,&e,&v,f);
        for(int j=0;j<6;j++) {
            RawGF2EX t; mul_raw(&pr,&vws.tau_vec_raw[i],&g[j]);
            rem_raw(&t,&pr); putraw(o,&t);
        }
        fputc('\n',o);
    }
    for(int j=0;j<6;j++) putraw(o,&G[j]); fputc('\n',o);
    for(int r=0;r<embded_num;r++) {
        for(int c=0;c<M;c++) fputc('0'+(int)Hb[r][c],o);
        fprintf(o," %lu\n",yb[r]);
    }
    fclose(o);
    /* This separate file is never supplied to the solver. */
    unsigned char es[SEED_BYTES]; memcpy(es,sk+SEED_BYTES,SEED_BYTES); random_e(eb,es);
    o=fopen(argv[3],"w"); if(!o) return 9;
    for(int c=0;c<M;c++) fputc('0'+(int)eb[c],o);
    fputc('\n',o); fclose(o);
    return 0;
}
