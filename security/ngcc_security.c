/*
 * Low-hanging-fruit security checks for NGCC shared libraries.
 *
 * One invocation performs one check.  The Python runner starts each check in
 * a fresh, timeout-limited process, so malformed-input crashes do not prevent
 * later candidates from being tested.
 *
 * Output (one tab-separated line):
 *   SECURITY <id> <instance> <check> <PASS|FINDING|ERROR|SKIP> <detail>
 */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "link_common.h"
#include "link_sig.h"
#include "link_kem.h"
#include "link_kex.h"
#include "link_hash.h"

#define SEED_BYTES 64
#define GUARD_BYTES 64
#define MAX_SAMPLE_BITS 32

static void *lib;
static const ngcc_meta_t *meta;
static int (*seed_fn)(const unsigned char *, unsigned long long);
static int (*random_fn)(unsigned char *, unsigned long long);

typedef struct {
    unsigned char *raw;
    unsigned char *p;
    size_t n;
} buf_t;

static buf_t buf_new(size_t n)
{
    buf_t b = {0};
    b.raw = calloc(n + 2 * GUARD_BYTES + 1, 1);
    if (!b.raw) return b;
    memset(b.raw, 0xa5, GUARD_BYTES);
    memset(b.raw + GUARD_BYTES + n, 0xa5, GUARD_BYTES);
    b.p = b.raw + GUARD_BYTES;
    b.n = n;
    return b;
}

static void buf_free(buf_t *b) { free(b->raw); memset(b, 0, sizeof *b); }

static int guards_ok(const buf_t *b)
{
    for (size_t i = 0; i < GUARD_BYTES; i++)
        if (b->raw[i] != 0xa5 || b->raw[GUARD_BYTES + b->n + i] != 0xa5) return 0;
    return 1;
}

static void fill_seed(unsigned char seed[SEED_BYTES], unsigned char tag)
{
    for (size_t i = 0; i < SEED_BYTES; i++) seed[i] = (unsigned char)(tag + 29 * i);
}

static int reseed(unsigned char tag)
{
    unsigned char seed[SEED_BYTES];
    fill_seed(seed, tag);
    return seed_fn(seed, sizeof seed);
}

static void *sym(const char *name)
{
    dlerror();
    void *p = dlsym(lib, name);
    return dlerror() ? NULL : p;
}

static void report(const char *check, const char *status, const char *detail)
{
    printf("SECURITY\t%s\t%s\t%s\t%s\t%s\n", meta ? meta->id : "?",
           meta ? meta->instance : "?", check, status, detail ? detail : "");
}

static int same(const unsigned char *a, size_t an, const unsigned char *b, size_t bn)
{
    return an == bn && !memcmp(a, b, an);
}

static uint64_t fingerprint(const unsigned char *p, size_t n, uint64_t h)
{
    for (size_t i = 0; i < n; i++) { h ^= p[i]; h *= UINT64_C(1099511628211); }
    return h ^ (uint64_t)n;
}

static unsigned char fresh_seed_tag(void)
{
    const char *s = getenv("NGCC_SECURITY_FRESH_TAG");
    return (unsigned char)(s && *s ? strtoul(s, NULL, 0) : 0x71);
}

static size_t sample_bit(size_t total_bits, int sample)
{
    if (total_bits <= 1) return 0;
    return (size_t)(((uint64_t)sample * (total_bits - 1)) / (MAX_SAMPLE_BITS - 1));
}

static int sig_setup(ngcc_sig_api_t *a)
{
#define GET(f, n) do { a->f = (void *)sym(n); if (!a->f) return 0; } while (0)
    GET(get_pk_len_bytes, "sig_get_pk_len_bytes"); GET(get_sk_len_bytes, "sig_get_sk_len_bytes");
    GET(get_sn_len_bytes, "sig_get_sn_len_bytes"); GET(keygen, "sig_keygen");
    GET(sign, "sig_sign"); GET(verify, "sig_verify");
#undef GET
    return 1;
}

static int sig_material(ngcc_sig_api_t *a, buf_t *pk, size_t *pkn, buf_t *sk,
                        size_t *skn, buf_t *sn, size_t *snn, const unsigned char *msg, size_t mn)
{
    const ngcc_meta_sig_t *m = (const ngcc_meta_sig_t *)meta;
    *pk = buf_new(m->pk_len); *sk = buf_new(m->sk_len); *sn = buf_new(m->sn_len);
    unsigned long long pl = m->pk_len, sl = m->sk_len, nl = m->sn_len;
    if (!pk->raw || !sk->raw || !sn->raw || reseed(0x11) || a->keygen(pk->p, &pl, sk->p, &sl) ||
        a->sign(sk->p, sl, (unsigned char *)msg, mn, sn->p, &nl)) return 0;
    *pkn = pl; *skn = sl; *snn = nl;
    return guards_ok(pk) && guards_ok(sk) && guards_ok(sn) && nl <= sn->n;
}

static void check_sig(const char *check)
{
    ngcc_sig_api_t a;
    if (!sig_setup(&a)) { report(check, "ERROR", "missing signature API symbol"); return; }
    static const unsigned char msg[] = "NGCC low hanging fruit signature message";
    const size_t mn = sizeof msg - 1;
    buf_t pk = {0}, sk = {0}, sn = {0}; size_t pkn = 0, skn = 0, snn = 0;
    if (!strcmp(check, "sig-drng-consumption")) {
        const ngcc_meta_sig_t *m = (const ngcc_meta_sig_t *)meta;
        buf_t p=buf_new(m->pk_len),s=buf_new(m->sk_len);unsigned char control[32],after[32];
        unsigned long long pn=m->pk_len,sn_=m->sk_len;
        int r=reseed(0x35)||random_fn(control,sizeof control*8)||reseed(0x35)||a.keygen(p.p,&pn,s.p,&sn_)||random_fn(after,sizeof after*8);
        if(r)report(check,"ERROR","key generation/DRNG probe failed");
        else report(check,!memcmp(control,after,sizeof control)?"FINDING":"PASS",!memcmp(control,after,sizeof control)?"key generation did not advance the harness DRNG":"key generation consumed the harness DRNG");
        buf_free(&p);buf_free(&s);return;
    }
    if (!strcmp(check, "sig-seed-keygen")) {
        const ngcc_meta_sig_t *m = (const ngcc_meta_sig_t *)meta;
        buf_t p1 = buf_new(m->pk_len), s1 = buf_new(m->sk_len), p2 = buf_new(m->pk_len), s2 = buf_new(m->sk_len),
              p3 = buf_new(m->pk_len), s3 = buf_new(m->sk_len);
        unsigned long long p1n=m->pk_len,s1n=m->sk_len,p2n=m->pk_len,s2n=m->sk_len,p3n=m->pk_len,s3n=m->sk_len;
        int r = reseed(0x31) || a.keygen(p1.p,&p1n,s1.p,&s1n) || reseed(0x31) || a.keygen(p2.p,&p2n,s2.p,&s2n) ||
                reseed(0x32) || a.keygen(p3.p,&p3n,s3.p,&s3n);
        if (r) report(check,"ERROR","key generation failed");
        else if (!same(p1.p,p1n,p2.p,p2n) || !same(s1.p,s1n,s2.p,s2n)) report(check,"FINDING","same seed produced different key pair");
        else if (same(p1.p,p1n,p3.p,p3n) && same(s1.p,s1n,s3.p,s3n)) report(check,"FINDING","different seeds produced identical key pair");
        else report(check,"PASS","key generation is reproducible and seed-sensitive");
        buf_free(&p1);buf_free(&s1);buf_free(&p2);buf_free(&s2);buf_free(&p3);buf_free(&s3); return;
    }
    if (!strcmp(check, "sig-fresh-keygen")) {
        const ngcc_meta_sig_t *m = (const ngcc_meta_sig_t *)meta;
        buf_t p=buf_new(m->pk_len),s=buf_new(m->sk_len);unsigned long long pn=m->pk_len,sn_=m->sk_len;
        int r=!p.raw||!s.raw||reseed(fresh_seed_tag())||a.keygen(p.p,&pn,s.p,&sn_);
        if(r||!guards_ok(&p)||!guards_ok(&s))report(check,"ERROR","fresh-process key generation failed or overflowed");
        else {char d[160];uint64_t p1=fingerprint(p.p,pn,UINT64_C(1469598103934665603)),p2=fingerprint(p.p,pn,UINT64_C(7809847782465536322));uint64_t s1=fingerprint(s.p,sn_,UINT64_C(1469598103934665603)),s2=fingerprint(s.p,sn_,UINT64_C(7809847782465536322));snprintf(d,sizeof d,"pk=%016llx%016llx sk=%016llx%016llx",(unsigned long long)p1,(unsigned long long)p2,(unsigned long long)s1,(unsigned long long)s2);report(check,"PASS",d);}
        buf_free(&p);buf_free(&s);return;
    }
    if (!sig_material(&a,&pk,&pkn,&sk,&skn,&sn,&snn,msg,mn)) { report(check,"ERROR","keygen/sign setup failed or overflowed"); goto out; }
    if (a.verify(pk.p,pkn,sn.p,snn,(unsigned char *)msg,mn)) { report(check,"ERROR","valid signature rejected"); goto out; }
    if (!strcmp(check,"sig-message-flip")) {
        unsigned char m2[sizeof msg]; memcpy(m2,msg,mn); m2[0]^=0x80;
        int accepted = a.verify(pk.p,pkn,sn.p,snn,m2,mn)==0;
        report(check, accepted?"FINDING":"PASS", accepted?"modified message accepted":"modified message rejected");
    } else if (!strcmp(check,"sig-signature-flip")) {
        int accepted=0; size_t at=0, bits=snn*8, samples=bits<MAX_SAMPLE_BITS?bits:MAX_SAMPLE_BITS;
        for (size_t i=0;i<samples;i++) { size_t bit=sample_bit(bits,(int)i); sn.p[bit/8]^=(unsigned char)(1u<<(bit%8));
            int r=a.verify(pk.p,pkn,sn.p,snn,(unsigned char *)msg,mn); sn.p[bit/8]^=(unsigned char)(1u<<(bit%8)); if(!r){accepted=1;at=bit;break;} }
        char d[192]; if(accepted)snprintf(d,sizeof d,"modified signature accepted at sampled bit %zu (byte %zu mask 0x%02x of %zu-byte signature)",at,at/8,1u<<(at%8),snn);else snprintf(d,sizeof d,"%zu sampled signature-bit changes rejected",samples);
        report(check,accepted?"FINDING":"PASS",d);
    } else if (!strcmp(check,"sig-truncate")) {
        if (!snn) report(check,"SKIP","zero-length signature");
        else { int accepted=a.verify(pk.p,pkn,sn.p,snn-1,(unsigned char *)msg,mn)==0;
               report(check,accepted?"FINDING":"PASS",accepted?"declared shorter signature length was ignored while the full backing buffer remained available":"declared shorter signature length was rejected"); }
    } else if (!strcmp(check,"sig-append")) {
        buf_t extended=buf_new(snn+1); memcpy(extended.p,sn.p,snn); extended.p[snn]=0;
        int accepted=a.verify(pk.p,pkn,extended.p,snn+1,(unsigned char *)msg,mn)==0;
        report(check,accepted?"FINDING":"PASS",accepted?"signature with appended byte accepted":"signature with appended byte rejected");
        buf_free(&extended);
    } else if (!strcmp(check,"sig-zero")) {
        memset(sn.p,0,snn);
        int accepted=a.verify(pk.p,pkn,sn.p,snn,(unsigned char *)msg,mn)==0;
        report(check,accepted?"FINDING":"PASS",accepted?"all-zero signature accepted":"all-zero signature rejected");
    } else report(check,"SKIP","unknown signature check");
out: buf_free(&pk); buf_free(&sk); buf_free(&sn);
}

static int kem_setup(ngcc_kem_api_t *a)
{
#define GET(f, n) do { a->f = (void *)sym(n); if (!a->f) return 0; } while (0)
    GET(get_pk_len_bytes,"kem_get_pk_len_bytes");GET(get_sk_len_bytes,"kem_get_sk_len_bytes");
    GET(get_ss_len_bytes,"kem_get_ss_len_bytes");GET(get_ct_len_bytes,"kem_get_ct_len_bytes");
    GET(keygen,"kem_keygen");GET(enc,"kem_enc");GET(dec,"kem_dec");
#undef GET
    return 1;
}

typedef struct { buf_t pk,sk,ct,ss; size_t pkn,skn,ctn,ssn; } kem_mat_t;
static void kem_free(kem_mat_t *x){buf_free(&x->pk);buf_free(&x->sk);buf_free(&x->ct);buf_free(&x->ss);}
static int kem_material(ngcc_kem_api_t *a, kem_mat_t *x, unsigned char keyseed, unsigned char encseed)
{
    const ngcc_meta_kem_t *m=(const ngcc_meta_kem_t *)meta;
    x->pk=buf_new(m->pk_len);x->sk=buf_new(m->sk_len);x->ct=buf_new(m->ct_len);x->ss=buf_new(m->ss_len);
    unsigned long long pn=m->pk_len,kn=m->sk_len,cn=m->ct_len,sn=m->ss_len;
    if(!x->pk.raw||!x->sk.raw||!x->ct.raw||!x->ss.raw||reseed(keyseed)||a->keygen(x->pk.p,&pn,x->sk.p,&kn)||
       reseed(encseed)||a->enc(x->pk.p,pn,x->ss.p,&sn,x->ct.p,&cn)) return 0;
    x->pkn=pn;x->skn=kn;x->ctn=cn;x->ssn=sn;
    return guards_ok(&x->pk)&&guards_ok(&x->sk)&&guards_ok(&x->ct)&&guards_ok(&x->ss)&&
           pn<=x->pk.n&&kn<=x->sk.n&&cn<=x->ct.n&&sn<=x->ss.n;
}

static int kem_dec_same(ngcc_kem_api_t *a, const kem_mat_t *x, const unsigned char *ct, size_t ctn,
                        const unsigned char *sk, size_t skn, int *ret)
{
    const ngcc_meta_kem_t *m=(const ngcc_meta_kem_t *)meta; buf_t out=buf_new(m->ss_len);
    unsigned long long on=m->ss_len; *ret=a->dec((unsigned char *)sk,skn,(unsigned char *)ct,ctn,out.p,&on);
    int eq=guards_ok(&out)&&same(out.p,on,x->ss.p,x->ssn); buf_free(&out); return eq;
}

static void check_kem(const char *check)
{
    ngcc_kem_api_t a; if(!kem_setup(&a)){report(check,"ERROR","missing KEM API symbol");return;}
    if(!strcmp(check,"kem-keygen-drng-consumption")) {
        const ngcc_meta_kem_t *m=(const ngcc_meta_kem_t *)meta;buf_t p=buf_new(m->pk_len),s=buf_new(m->sk_len);unsigned char control[32],after[32];unsigned long long pn=m->pk_len,sn=m->sk_len;
        int r=reseed(0x45)||random_fn(control,sizeof control*8)||reseed(0x45)||a.keygen(p.p,&pn,s.p,&sn)||random_fn(after,sizeof after*8);
        if(r)report(check,"ERROR","key generation/DRNG probe failed");else report(check,!memcmp(control,after,sizeof control)?"FINDING":"PASS",!memcmp(control,after,sizeof control)?"key generation did not advance the harness DRNG":"key generation consumed the harness DRNG");buf_free(&p);buf_free(&s);return;
    }
    if(!strcmp(check,"kem-enc-drng-consumption")) {
        const ngcc_meta_kem_t *m=(const ngcc_meta_kem_t *)meta;kem_mat_t x={0};
        x.pk=buf_new(m->pk_len);x.sk=buf_new(m->sk_len);x.ct=buf_new(m->ct_len);x.ss=buf_new(m->ss_len);unsigned long long pn=m->pk_len,kn=m->sk_len,cn=m->ct_len,sn=m->ss_len;unsigned char control[32],after[32];
        int r=reseed(0x46)||a.keygen(x.pk.p,&pn,x.sk.p,&kn)||reseed(0x47)||random_fn(control,sizeof control*8)||reseed(0x47)||a.enc(x.pk.p,pn,x.ss.p,&sn,x.ct.p,&cn)||random_fn(after,sizeof after*8);
        if(r)report(check,"ERROR","encapsulation/DRNG probe failed");else report(check,!memcmp(control,after,sizeof control)?"FINDING":"PASS",!memcmp(control,after,sizeof control)?"encapsulation did not advance the harness DRNG":"encapsulation consumed the harness DRNG");kem_free(&x);return;
    }
    if(!strcmp(check,"kem-seed-keygen")) {
        kem_mat_t x={0},y={0},z={0}; int ok=kem_material(&a,&x,0x41,0x51)&&kem_material(&a,&y,0x41,0x51)&&kem_material(&a,&z,0x42,0x51);
        if(!ok) report(check,"ERROR","KEM setup failed");
        else if(!same(x.pk.p,x.pkn,y.pk.p,y.pkn)||!same(x.sk.p,x.skn,y.sk.p,y.skn)) report(check,"FINDING","same seed produced different key pair");
        else if(same(x.pk.p,x.pkn,z.pk.p,z.pkn)&&same(x.sk.p,x.skn,z.sk.p,z.skn)) report(check,"FINDING","different seeds produced identical key pair");
        else report(check,"PASS","key generation is reproducible and seed-sensitive");
        kem_free(&x);kem_free(&y);kem_free(&z);return;
    }
    if(!strcmp(check,"kem-fresh-keygen")) {
        const ngcc_meta_kem_t *m=(const ngcc_meta_kem_t *)meta;buf_t p=buf_new(m->pk_len),s=buf_new(m->sk_len);unsigned long long pn=m->pk_len,sn=m->sk_len;
        int rr=!p.raw||!s.raw||reseed(fresh_seed_tag())||a.keygen(p.p,&pn,s.p,&sn);
        if(rr||!guards_ok(&p)||!guards_ok(&s))report(check,"ERROR","fresh-process key generation failed or overflowed");
        else {char d[160];uint64_t p1=fingerprint(p.p,pn,UINT64_C(1469598103934665603)),p2=fingerprint(p.p,pn,UINT64_C(7809847782465536322));uint64_t s1=fingerprint(s.p,sn,UINT64_C(1469598103934665603)),s2=fingerprint(s.p,sn,UINT64_C(7809847782465536322));snprintf(d,sizeof d,"pk=%016llx%016llx sk=%016llx%016llx",(unsigned long long)p1,(unsigned long long)p2,(unsigned long long)s1,(unsigned long long)s2);report(check,"PASS",d);}
        buf_free(&p);buf_free(&s);return;
    }
    if(!strcmp(check,"kem-seed-encapsulation")) {
        kem_mat_t x={0},y={0}; int ok=kem_material(&a,&x,0x41,0x51)&&kem_material(&a,&y,0x41,0x52);
        if(!ok) report(check,"ERROR","KEM setup failed");
        else if(same(x.ct.p,x.ctn,y.ct.p,y.ctn)&&same(x.ss.p,x.ssn,y.ss.p,y.ssn)) report(check,"FINDING","different encapsulation seeds produced identical ciphertext and secret");
        else report(check,"PASS","encapsulation is seed-sensitive");
        kem_free(&x);kem_free(&y);return;
    }
    kem_mat_t x={0}; if(!kem_material(&a,&x,0x41,0x51)){report(check,"ERROR","KEM setup failed or overflowed");return;}
    int r=0;
    if(!strcmp(check,"kem-roundtrip")) {int eq=kem_dec_same(&a,&x,x.ct.p,x.ctn,x.sk.p,x.skn,&r);report(check,eq&&r==0?"PASS":"FINDING",eq&&r==0?"encapsulation/decapsulation consistency":"valid ciphertext failed or produced a different shared secret");}
    else if(!strcmp(check,"kem-ciphertext-flip")) {
        int accepted=0;size_t at=0,bits=x.ctn*8,samples=bits<MAX_SAMPLE_BITS?bits:MAX_SAMPLE_BITS;
        for(size_t i=0;i<samples;i++){size_t bit=sample_bit(bits,(int)i);x.ct.p[bit/8]^=(unsigned char)(1u<<(bit%8));int eq=kem_dec_same(&a,&x,x.ct.p,x.ctn,x.sk.p,x.skn,&r);x.ct.p[bit/8]^=(unsigned char)(1u<<(bit%8));if(eq&&r==0){accepted=1;at=bit;break;}}
        char d[192];if(accepted)snprintf(d,sizeof d,"modified ciphertext retained original shared secret at sampled bit %zu (byte %zu mask 0x%02x of %zu-byte ciphertext)",at,at/8,1u<<(at%8),x.ctn);else snprintf(d,sizeof d,"%zu sampled ciphertext-bit changes changed/rejected the shared secret",samples);report(check,accepted?"FINDING":"PASS",d);
    } else if(!strcmp(check,"kem-reject-mask")) {
        /* A nonzero compare accumulator must be normalized to an all-ones
         * selection mask.  Otherwise an invalid ciphertext can retain a
         * predictable subset of the valid encapsulated key.  High-bit byte
         * flips cheaply expose the common `mask = -difference` defect. */
        const ngcc_meta_kem_t *m=(const ngcc_meta_kem_t *)meta;buf_t changed=buf_new(x.ctn),out=buf_new(m->ss_len);
        int leaked=0;size_t at=0,equal_high=0;unsigned long long on=0;
        if(!changed.raw||!out.raw){report(check,"ERROR","mutation/output allocation failed");buf_free(&changed);buf_free(&out);kem_free(&x);return;}
        for(size_t i=0;i<x.ctn&&!leaked;i++){
            memcpy(changed.p,x.ct.p,x.ctn);changed.p[i]^=0x80;memset(out.p,0,out.n);on=m->ss_len;
            if(a.dec(x.sk.p,x.skn,changed.p,x.ctn,out.p,&on)==0&&guards_ok(&out)&&on<=out.n&&on==x.ssn&&!same(out.p,on,x.ss.p,x.ssn)){
                int low_equal=1;equal_high=0;
                for(size_t j=0;j<on;j++){if(((out.p[j]^x.ss.p[j])&0x7f)!=0)low_equal=0;if(((out.p[j]^x.ss.p[j])&0x80)==0)equal_high++;}
                if(low_equal){leaked=1;at=i;}
            }
        }
        char d[224];if(leaked)snprintf(d,sizeof d,"ciphertext byte %zu xor 0x80 retained all seven low bits of every shared-secret byte (%zu/%zu high bits also matched)",at,equal_high,x.ssn);else snprintf(d,sizeof d,"%zu high-bit ciphertext-byte mutations did not retain the tested seven-bit key pattern",x.ctn);report(check,leaked?"FINDING":"PASS",d);buf_free(&changed);buf_free(&out);
    } else if(!strcmp(check,"kem-truncate")) {
        if(!x.ctn)report(check,"SKIP","zero-length ciphertext");else {int eq=kem_dec_same(&a,&x,x.ct.p,x.ctn-1,x.sk.p,x.skn,&r),unsafe=eq&&r==0;report(check,unsafe?"FINDING":"PASS",unsafe?"declared shorter ciphertext length was ignored while the full backing buffer remained available; original shared secret returned successfully":"declared shorter ciphertext length was rejected or changed the shared secret");}
    } else if(!strcmp(check,"kem-append")) {buf_t extended=buf_new(x.ctn+1);memcpy(extended.p,x.ct.p,x.ctn);extended.p[x.ctn]=0;int eq=kem_dec_same(&a,&x,extended.p,x.ctn+1,x.sk.p,x.skn,&r),unsafe=eq&&r==0;report(check,unsafe?"FINDING":"PASS",unsafe?"extended ciphertext returned original shared secret successfully":"extended ciphertext was rejected or changed the shared secret");buf_free(&extended);
    } else if(!strcmp(check,"kem-zero")) {buf_t z=buf_new(x.ctn);int eq=kem_dec_same(&a,&x,z.p,x.ctn,x.sk.p,x.skn,&r),unsafe=eq&&r==0;report(check,unsafe?"FINDING":"PASS",unsafe?"all-zero ciphertext returned original shared secret successfully":"all-zero ciphertext was rejected or changed the shared secret");buf_free(&z);
    } else if(!strcmp(check,"kem-wrong-secret-key")) {kem_mat_t y={0};if(!kem_material(&a,&y,0x42,0x52))report(check,"ERROR","second key generation failed");else {int eq=kem_dec_same(&a,&x,x.ct.p,x.ctn,y.sk.p,y.skn,&r),unsafe=eq&&r==0;report(check,unsafe?"FINDING":"PASS",unsafe?"different secret key returned original shared secret successfully":"different secret key was rejected or did not recover original shared secret");}kem_free(&y);
    } else report(check,"SKIP","unknown KEM check"); kem_free(&x);
}

static int hash_once(ngcc_hash_fn h, int dbits, const unsigned char *m, size_t bits, unsigned char *out)
{ return h(dbits,m,bits,out); }

static void check_hash(const char *check)
{
    ngcc_hash_fn h=(void *)sym("CryptHash");const ngcc_meta_hash_t *m=(const ngcc_meta_hash_t *)meta;
    if(!h){report(check,"ERROR","missing CryptHash symbol");return;} size_t dn=m->digest_len;
    buf_t d1=buf_new(dn),d2=buf_new(dn),d3=buf_new(dn);unsigned char msg[130],copy[130];
    for(size_t i=0;i<sizeof msg;i++)msg[i]=(unsigned char)(3+17*i);
    memcpy(copy,msg,sizeof msg);
    if(!strcmp(check,"hash-determinism")) {int r=hash_once(h,m->digest_bits,msg,777,d1.p)||hash_once(h,m->digest_bits,msg,777,d2.p);report(check,r?"ERROR":same(d1.p,dn,d2.p,dn)?"PASS":"FINDING",r?"CryptHash returned an error":same(d1.p,dn,d2.p,dn)?"repeated hash is deterministic":"same input produced different digests");}
    else if(!strcmp(check,"hash-state-reset")) {int r=hash_once(h,m->digest_bits,msg,777,d1.p)||hash_once(h,m->digest_bits,msg+1,513,d2.p)||hash_once(h,m->digest_bits,msg,777,d3.p);report(check,r?"ERROR":same(d1.p,dn,d3.p,dn)?"PASS":"FINDING",r?"CryptHash returned an error":same(d1.p,dn,d3.p,dn)?"intervening call did not leak state":"digest depends on prior call");}
    else if(!strcmp(check,"hash-input-immutable")) {int r=hash_once(h,m->digest_bits,msg,777,d1.p);report(check,r?"ERROR":memcmp(msg,copy,sizeof msg)?"FINDING":"PASS",r?"CryptHash returned an error":memcmp(msg,copy,sizeof msg)?"CryptHash modified its input":"input remained unchanged");}
    else if(!strcmp(check,"hash-zero-padding")) {int finding=0;size_t lens[]={0,1,7,8,9,63,64,65,127,128,129,255,256,257,447,511,512,513,703,777,959};char detail[128]="length encoding separates tested zero extensions";
        /* Exercise extensions by one through eight zero bits.  The wider
         * range catches byte-boundary errors such as H(M)=H(M||0^7), while
         * 447/703/959 cover the common r-1 boundaries of the submitted
         * 448/704/960-bit-rate designs. */
        for(size_t i=0;i<sizeof lens/sizeof lens[0]&&!finding;i++)for(size_t z=1;z<=8;z++){size_t b=lens[i];memcpy(copy,msg,sizeof msg);for(size_t bit=b;bit<b+z;bit++)copy[bit/8]&=(unsigned char)~(1u<<(7-(bit%8)));if(hash_once(h,m->digest_bits,msg,b,d1.p)||hash_once(h,m->digest_bits,copy,b+z,d2.p)){report(check,"ERROR","CryptHash returned an error");goto done;}if(same(d1.p,dn,d2.p,dn)){finding=1;snprintf(detail,sizeof detail,"collision between a %zu-bit message and its %zu-zero-bit extension",b,z);break;}}
        /* A broken 10*1 implementation can merge both delimiter bits when
         * exactly one rate bit remains.  Compare an r-1-bit string ending in
         * one with the same string shortened by that bit. */
        {size_t edges[]={447,703,959};for(size_t i=0;i<sizeof edges/sizeof edges[0]&&!finding;i++){size_t b=edges[i];memcpy(copy,msg,sizeof msg);copy[(b-1)/8]|=(unsigned char)(1u<<(7-((b-1)%8)));if(hash_once(h,m->digest_bits,copy,b,d1.p)||hash_once(h,m->digest_bits,copy,b-1,d2.p)){report(check,"ERROR","CryptHash returned an error");goto done;}if(same(d1.p,dn,d2.p,dn)){finding=1;snprintf(detail,sizeof detail,"collision after deleting the final one bit at %zu/%zu bits",b,b-1);}}}
        report(check,finding?"FINDING":"PASS",detail);
    } else if(!strcmp(check,"hash-unused-bits")) {int finding=0;char detail[128]="unused low bits do not affect tested partial-byte messages";
        for(size_t b=1;b<32;b++)if(b%8){memcpy(copy,msg,sizeof msg);unsigned unused=8-(b%8);copy[b/8]^=(unsigned char)((1u<<unused)-1u);if(hash_once(h,m->digest_bits,msg,b,d1.p)||hash_once(h,m->digest_bits,copy,b,d2.p)){report(check,"ERROR","CryptHash returned an error");goto done;}if(!same(d1.p,dn,d2.p,dn)){finding=1;snprintf(detail,sizeof detail,"digest depends on bits outside declared %zu-bit input",b);break;}}
        report(check,finding?"FINDING":"PASS",detail);
    } else if(!strcmp(check,"hash-bit-flip")) {int finding=0;size_t at=0;hash_once(h,m->digest_bits,msg,1024,d1.p);for(int i=0;i<MAX_SAMPLE_BITS;i++){size_t bit=sample_bit(1024,i);msg[bit/8]^=(unsigned char)(1u<<(7-(bit%8)));hash_once(h,m->digest_bits,msg,1024,d2.p);msg[bit/8]^=(unsigned char)(1u<<(7-(bit%8)));if(same(d1.p,dn,d2.p,dn)){finding=1;at=bit;break;}}char detail[128];if(finding)snprintf(detail,sizeof detail,"sampled input-bit change at bit %zu left digest unchanged",at);else snprintf(detail,sizeof detail,"%d sampled input-bit changes altered the digest",MAX_SAMPLE_BITS);report(check,finding?"FINDING":"PASS",detail);
    } else report(check,"SKIP","unknown hash check");
done: if(!guards_ok(&d1)||!guards_ok(&d2)||!guards_ok(&d3))report(check,"FINDING","digest output overflowed advertised length");buf_free(&d1);buf_free(&d2);buf_free(&d3);
}

#define SEC_KEX_MAX_PASSES 16
typedef struct {
    ngcc_kex_init_fn init_a, init_b;
    ngcc_kex_pass1_fn pass1;
    ngcc_kex_passn_fn pass[SEC_KEX_MAX_PASSES + 1];
    ngcc_kex_derive_fn derive_a, derive_b;
} sec_kex_api_t;

static int kex_api(sec_kex_api_t *a)
{
    memset(a,0,sizeof *a);a->init_a=(void *)sym("kex_init_a");a->init_b=(void *)sym("kex_init_b");
    a->pass1=(void *)sym("kex_generate_pass1_msg_a");a->derive_a=(void *)sym("kex_derive_ss_a");a->derive_b=(void *)sym("kex_derive_ss_b");
    for(int k=2;k<=SEC_KEX_MAX_PASSES;k++){char n[64];snprintf(n,sizeof n,"kex_generate_pass%d_msg_%c",k,(k&1)?'a':'b');a->pass[k]=(void *)sym(n);}
    return a->init_a&&a->init_b&&a->pass1&&a->derive_a&&a->derive_b;
}

/* Returns 0 for a completed exchange, 1 for protocol/API failure, 2 for an
 * unsupported/missing pass.  mutate_pass is changed in transit after the
 * sender creates it and before the peer consumes it. */
static int kex_exchange(const ngcc_meta_kex_t *m, sec_kex_api_t *a, int mutate_pass,
                        int mutate_last_bit, unsigned char *outa, size_t *outan,
                        unsigned char *outb, size_t *outbn, size_t *mutated_len)
{
    unsigned long long need=m->passes<3?3:m->passes;if(need>SEC_KEX_MAX_PASSES)return 2;
    for(unsigned long long k=2;k<=need;k++)if(!a->pass[k])return 2;
    buf_t pka=buf_new(m->pk_len),ska=buf_new(m->sk_len),pkb=buf_new(m->pk_len),skb=buf_new(m->sk_len),
          sta=buf_new(m->sta_len),stb=buf_new(m->stb_len),ssa=buf_new(m->ss_len),ssb=buf_new(m->ss_len);
    buf_t messages[SEC_KEX_MAX_PASSES+1];memset(messages,0,sizeof messages);
    for(unsigned long long k=1;k<=need;k++)messages[k]=buf_new(m->total_msg_len);
    unsigned long long pkan=m->pk_len,skan=m->sk_len,pkbn=m->pk_len,skbn=m->sk_len,stan=m->sta_len,stbn=m->stb_len,
        ssan=m->ss_len,ssbn=m->ss_len,mlen[SEC_KEX_MAX_PASSES+1];memset(mlen,0,sizeof mlen);
    unsigned char *ma=NULL,*mb=NULL;unsigned long long man=0,mbn=0;int rc=1,r;
    if(reseed(0x6a)||(r=a->init_a(pka.p,&pkan,ska.p,&skan,sta.p,&stan))<0||(r=a->init_b(pkb.p,&pkbn,skb.p,&skbn,stb.p,&stbn))<0)goto out;
    for(unsigned long long k=1;k<=need;k++){
        if(k==1)r=a->pass1(ska.p,skan,pkb.p,pkbn,sta.p,&stan,messages[k].p,&mlen[k]);
        else if(k&1)r=a->pass[k](ska.p,skan,pkb.p,pkbn,messages[k-1].p,mlen[k-1],sta.p,&stan,messages[k].p,&mlen[k]);
        else r=a->pass[k](skb.p,skbn,pka.p,pkan,messages[k-1].p,mlen[k-1],stb.p,&stbn,messages[k].p,&mlen[k]);
        if(r<0||mlen[k]>m->total_msg_len)goto out;
        if((int)k==mutate_pass&&mlen[k]){size_t bit=mutate_last_bit?(size_t)mlen[k]*8-1:0;messages[k].p[bit/8]^=(unsigned char)(1u<<(bit%8));if(mutated_len)*mutated_len=(size_t)mlen[k];}
        if(k&1){ma=messages[k].p;man=mlen[k];}else{mb=messages[k].p;mbn=mlen[k];}
        if(r==1)break;
    }
    r=a->derive_a(ska.p,skan,pkb.p,pkbn,mb,mbn,sta.p,stan,ssa.p,&ssan);if(r<0)goto out;
    r=a->derive_b(skb.p,skbn,pka.p,pkan,ma,man,stb.p,stbn,ssb.p,&ssbn);if(r<0)goto out;
    if(ssan>m->ss_len||ssbn>m->ss_len)goto out;
    memcpy(outa,ssa.p,ssan);memcpy(outb,ssb.p,ssbn);*outan=ssan;*outbn=ssbn;rc=0;
out:
    buf_free(&pka);buf_free(&ska);buf_free(&pkb);buf_free(&skb);buf_free(&sta);buf_free(&stb);buf_free(&ssa);buf_free(&ssb);
    for(unsigned long long k=1;k<=need&&k<=SEC_KEX_MAX_PASSES;k++)buf_free(&messages[k]);
    return rc;
}

static void check_kex(const char *check)
{
    const ngcc_meta_kex_t *m=(const ngcc_meta_kex_t *)meta;
    ngcc_kex_init_fn init=(void *)sym("kex_init_a"); if(!init){report(check,"ERROR","missing kex_init_a");return;}
    if(!strcmp(check,"kex-fresh-init")) {
        buf_t p=buf_new(m->pk_len),s=buf_new(m->sk_len),t=buf_new(m->sta_len);unsigned long long pn=m->pk_len,sn=m->sk_len,tn=m->sta_len;
        int r=!p.raw||!s.raw||!t.raw||reseed(fresh_seed_tag())||init(p.p,&pn,s.p,&sn,t.p,&tn)<0;
        if(r||!guards_ok(&p)||!guards_ok(&s)||!guards_ok(&t))report(check,"ERROR","fresh-process initiator setup failed or overflowed");
        else if(pn==0&&sn==0&&tn==0)report(check,"SKIP","initiator setup returned deferred zero-length material");
        else {char d[160];uint64_t a=fingerprint(p.p,pn,UINT64_C(1469598103934665603)),b=fingerprint(s.p,sn,a),c=fingerprint(t.p,tn,b);uint64_t x=fingerprint(p.p,pn,UINT64_C(7809847782465536322)),y=fingerprint(s.p,sn,x),z=fingerprint(t.p,tn,y);snprintf(d,sizeof d,"init=%016llx%016llx",(unsigned long long)c,(unsigned long long)z);report(check,"PASS",d);}
        buf_free(&p);buf_free(&s);buf_free(&t);return;
    }
    if(!strcmp(check,"kex-init-drng-consumption")) {
        buf_t p=buf_new(m->pk_len),s=buf_new(m->sk_len),t=buf_new(m->sta_len);unsigned long long pn=m->pk_len,sn=m->sk_len,tn=m->sta_len;unsigned char control[32],after[32];
        int r=reseed(0x65)||random_fn(control,sizeof control*8)||reseed(0x65)||init(p.p,&pn,s.p,&sn,t.p,&tn)<0||random_fn(after,sizeof after*8);
        if(r)report(check,"ERROR","initiator setup/DRNG probe failed");
        else if(pn==0&&sn==0&&tn==0)report(check,"SKIP","initiator setup returned deferred zero-length material");
        else report(check,!memcmp(control,after,sizeof control)?"FINDING":"PASS",!memcmp(control,after,sizeof control)?"initiator setup did not advance the harness DRNG":"initiator setup consumed the harness DRNG");
        buf_free(&p);buf_free(&s);buf_free(&t);return;
    }
    if(!strcmp(check,"kex-transcript-flip")) {
        if(m->passes==0){report(check,"SKIP","non-interactive scheme has no transcript message");return;}
        sec_kex_api_t a;if(!kex_api(&a)){report(check,"ERROR","missing KEX transcript symbol");return;}
        unsigned char *basea=calloc(m->ss_len,1),*baseb=calloc(m->ss_len,1),*muta=calloc(m->ss_len,1),*mutb=calloc(m->ss_len,1);
        size_t ban=0,bbn=0,man=0,mbn=0,mlen=0;int rc=kex_exchange(m,&a,0,0,basea,&ban,baseb,&bbn,NULL);
        if(rc||!same(basea,ban,baseb,bbn)){report(check,"ERROR","baseline exchange failed or parties disagreed");goto kex_flip_out;}
        int accepted=0,which=0,last=0,changed=0;
        for(int k=1;k<=(int)m->passes&&!accepted;k++)for(int edge=0;edge<2&&!accepted;edge++){
            man=mbn=0;mlen=0;rc=kex_exchange(m,&a,k,edge,muta,&man,mutb,&mbn,&mlen);
            if(!rc&&mlen>0&&same(muta,man,mutb,mbn)){accepted=1;which=k;last=edge;changed=!same(basea,ban,muta,man);}
        }
        if(accepted){char d[224];size_t bit=last?mlen*8-1:0;snprintf(d,sizeof d,"pass %d %s-bit mutation (byte %zu mask 0x%02x of %zu-byte message) still completed with matching %s shared secret",which,last?"last":"first",bit/8,1u<<(bit%8),mlen,changed?"changed":"original");report(check,"FINDING",d);}
        else report(check,"PASS","tested first/last-bit transcript mutations failed or prevented agreement");
kex_flip_out: free(basea);free(baseb);free(muta);free(mutb);return;
    }
    buf_t p1=buf_new(m->pk_len),s1=buf_new(m->sk_len),t1=buf_new(m->sta_len),p2=buf_new(m->pk_len),s2=buf_new(m->sk_len),t2=buf_new(m->sta_len),p3=buf_new(m->pk_len),s3=buf_new(m->sk_len),t3=buf_new(m->sta_len);
    unsigned long long p1n=m->pk_len,s1n=m->sk_len,t1n=m->sta_len,p2n=m->pk_len,s2n=m->sk_len,t2n=m->sta_len,p3n=m->pk_len,s3n=m->sk_len,t3n=m->sta_len;
    int r=reseed(0x61)||init(p1.p,&p1n,s1.p,&s1n,t1.p,&t1n)<0||reseed(0x61)||init(p2.p,&p2n,s2.p,&s2n,t2.p,&t2n)<0||reseed(0x62)||init(p3.p,&p3n,s3.p,&s3n,t3.p,&t3n)<0;
    if(strcmp(check,"kex-seed-init"))report(check,"SKIP","unknown KEX check");
    else if(r)report(check,"ERROR","kex_init_a failed");
    else if(p1n==0&&s1n==0&&t1n==0&&p2n==0&&s2n==0&&t2n==0&&p3n==0&&s3n==0&&t3n==0)report(check,"SKIP","initiator setup returned deferred zero-length material");
    else if(!same(p1.p,p1n,p2.p,p2n)||!same(s1.p,s1n,s2.p,s2n)||!same(t1.p,t1n,t2.p,t2n))report(check,"FINDING","same seed produced different initiator material");
    else if(same(p1.p,p1n,p3.p,p3n)&&same(s1.p,s1n,s3.p,s3n)&&same(t1.p,t1n,t3.p,t3n))report(check,"FINDING","different seeds produced identical initiator material");
    else report(check,"PASS","initiator setup is reproducible and seed-sensitive");
    buf_free(&p1);buf_free(&s1);buf_free(&t1);buf_free(&p2);buf_free(&s2);buf_free(&t2);buf_free(&p3);buf_free(&s3);buf_free(&t3);
}

int main(int argc, char **argv)
{
    if(argc!=3){fprintf(stderr,"usage: %s LIB CHECK\n",argv[0]);return 2;}
    lib=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);if(!lib){fprintf(stderr,"dlopen: %s\n",dlerror());return 2;}
    const ngcc_meta_t *(*mf)(void)=(void *)sym("ngcc_meta");seed_fn=(void *)sym("ngcc_seed");random_fn=(void *)sym("ngcc_random");
    if(!mf||!seed_fn||!random_fn){fprintf(stderr,"missing ngcc_meta/ngcc_seed/ngcc_random\n");return 2;}meta=mf();
    if(!meta||meta->magic!=NGCC_META_MAGIC){fprintf(stderr,"invalid metadata\n");return 2;}
    if(meta->type==NGCC_TYPE_SIG)check_sig(argv[2]);else if(meta->type==NGCC_TYPE_KEM)check_kem(argv[2]);
    else if(meta->type==NGCC_TYPE_HASH)check_hash(argv[2]);else if(meta->type==NGCC_TYPE_KEX)check_kex(argv[2]);
    else {report(argv[2],"SKIP","unknown scheme type");}
    dlclose(lib);return 0;
}
