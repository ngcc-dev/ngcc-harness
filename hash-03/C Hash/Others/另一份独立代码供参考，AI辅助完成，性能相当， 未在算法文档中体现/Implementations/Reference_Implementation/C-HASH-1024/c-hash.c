/*
 * C-Hash-1024 Reference Implementation 
 *
 * Architecture: C-Engine-FF (permutation with FeedForward, P_FF(x)=P(x) XOR x)
 * + unified VIL/FIL mode (identical structure to C-Hash-512).
 *
 * The VIL and FIL structures are identical to C-Hash-512;
 * only the C-Engine variant (P vs P_FF), lane ID, IV, and output
 * length differ.
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "c-hash.h"

/* -- Constants -- */
#define NB          184
#define BB          92
#define N_BITS      1472
#define BLOCK_BITS  736

static const uint8_t SBOX[16]={0x1,0x4,0x0,0xC,0x3,0x2,0x5,0xB,0xA,0x8,0x6,0xF,0x7,0x9,0xD,0xE};
static const uint32_t RC[28]={
    0x243F6A88U,0x85A308D3U,0x13198A2EU,0x03707344U,0xA4093822U,0x299F31D0U,
    0x082EFA98U,0xEC4E6C89U,0x452821E6U,0x38D01377U,0xBE5466CFU,0x34E90C6CU,
    0xC0AC29B7U,0xC97C50DDU,0x3F84D5B5U,0xB5470917U,0x9216D5D9U,0x8979FB1BU,
    0xD1310BA6U,0x98DFB5ACU,0x2FFD72DBU,0xD01ADFB7U,0xB8E1AFEDU,0x6A267E96U,
    0xBA7C9045U,0xF12C7F99U,0x24A19947U,0xB3916CF7U
};
static const uint8_t SC[6]={0xC,0x9,0x0,0x8,0xB,0x2};
static const int P1S[4]={15,7,23,16};

/* ================================================================
   C-Engine: 1536-bit permutation (28 rounds, SPS structure)
   ================================================================ */
static void parse_to_matrix(const uint8_t *in,uint8_t st[6][64])
{
    for(int i=0;i<6;i++)for(int j=0;j<64;j+=2){
        uint8_t b=in[32*i+(j>>1)];st[i][j]=b>>4;st[i][j+1]=b&0xF;
    }
}
static void serialize_matrix(const uint8_t st[6][64],uint8_t *out)
{
    for(int i=0;i<6;i++)for(int k=0;k<32;k++)
        out[32*i+k]=(st[i][k<<1]<<4)|st[i][(k<<1)+1];
}
static void s_layer(uint8_t st[6][64])
{
    for(int j=0;j<64;j++){
        uint8_t b0=SBOX[st[0][j]],b1=SBOX[st[1][j]],b2=SBOX[st[2][j]];
        uint8_t b3=SBOX[st[3][j]],b4=SBOX[st[4][j]],b5=SBOX[st[5][j]];
        uint8_t c0=(b0^b2^b3)^SC[0],c1=(b0^b1^b4)^SC[1],c2=(b1^b2^b5)^SC[2];
        uint8_t c3=(b0^b1^b2^b3^b4)^SC[3],c4=(b0^b1^b2^b4^b5)^SC[4],c5=(b0^b1^b2^b3^b5)^SC[5];
        st[0][j]=SBOX[c0];st[1][j]=SBOX[c1];st[2][j]=SBOX[c2];
        st[3][j]=SBOX[c3];st[4][j]=SBOX[c4];st[5][j]=SBOX[c5];
    }
}
static void m_layer(uint8_t st[6][64])
{
    for(int row=0;row<6;row++){
        uint8_t L[32],R[32];memcpy(L,st[row],32);memcpy(R,st[row]+32,32);
        for(int lr=0;lr<4;lr++){
            int sh=P1S[lr];uint8_t S[32];
            for(int j=0;j<32;j++)S[j]=L[(j+sh)&31]^R[(j+sh)&31];
            for(int j=0;j<32;j++)L[j]^=S[j];
            for(int j=0;j<32;j++)S[j]=R[j]^S[j];
            for(int j=0;j<32;j++)R[j]=S[(j-1+32)&31];
        }
        memcpy(st[row],L,32);memcpy(st[row]+32,R,32);
    }
}
static void k_layer(uint8_t st[6][64],int r)
{
    uint32_t c=RC[r];
    st[0][0]^=(c>>28)&0xF;st[0][1]^=(c>>24)&0xF;st[0][2]^=(c>>20)&0xF;st[0][3]^=(c>>16)&0xF;
    st[0][4]^=(c>>12)&0xF;st[0][5]^=(c>>8)&0xF;st[0][6]^=(c>>4)&0xF;st[0][7]^=c&0xF;
}
static void c_engine(const uint8_t *in,uint8_t *out,int nr)
{
    uint8_t st[6][64];parse_to_matrix(in,st);
    for(int r=0;r<nr;r++){s_layer(st);m_layer(st);k_layer(st,r);}
    serialize_matrix(st,out);
}

/* C-Engine with FeedForward: P_FF(x) = P(x) XOR x */
static void c_engine_ff_internal(const uint8_t *in,uint8_t *out,int nr)
{
    uint8_t st[6][64];parse_to_matrix(in,st);
    for(int r=0;r<nr;r++){s_layer(st);m_layer(st);k_layer(st,r);}
    /* Serialize with XOR (FF): 5 full rows (160B) + last 24B */
    for(int i=0;i<5;i++)for(int k=0;k<32;k++){
        int idx=32*i+k;out[idx]=((st[i][k<<1]<<4)|st[i][(k<<1)+1])^in[idx];
    }
    for(int k=0;k<24;k++){
        int idx=160+k;out[idx]=((st[5][k<<1]<<4)|st[5][(k<<1)+1])^in[idx];
    }
}

/* API: C-Engine pure permutation P(x) */
int c_engine_permute(const unsigned char *in,unsigned char *out)
{
    c_engine(in,out,28);return 0;
}
/* API: C-Engine permutation with FeedForward P_FF(x) = P(x) XOR x */
int c_engine_permute_ff(const unsigned char *in,unsigned char *out)
{
    c_engine_ff_internal(in,out,28);return 0;
}

/* ================================================================
   C-Hash-1024 Parameters
   ================================================================ */
#define HASHLEN     1024
#define LANE_VIL    0x81
#define LANE_FIL    0x82
#define LANE_LDRH   0x83

static void build_iv(uint8_t iv[BB])
{
    memset(iv,0,BB);
    iv[91]=(uint8_t)((HASHLEN>> 0)&0xFF);iv[90]=(uint8_t)((HASHLEN>> 8)&0xFF);
    iv[89]=(uint8_t)((HASHLEN>>16)&0xFF);iv[88]=(uint8_t)((HASHLEN>>24)&0xFF);
}
static uint64_t build_counter(uint8_t laneid,uint64_t payload)
{
    return ((uint64_t)laneid<<56)|(payload&0x00FFFFFFFFFFFFFFULL);
}

/* Set 8-byte counter into buf[184..191] (big-endian) */
static void set_counter(uint8_t buf[192],uint64_t ctr)
{
    buf[184]=(uint8_t)(ctr>>56);buf[185]=(uint8_t)(ctr>>48);
    buf[186]=(uint8_t)(ctr>>40);buf[187]=(uint8_t)(ctr>>32);
    buf[188]=(uint8_t)(ctr>>24);buf[189]=(uint8_t)(ctr>>16);
    buf[190]=(uint8_t)(ctr>>8); buf[191]=(uint8_t)ctr;
}

/* Leftmost-bit extraction */
static void lmb(uint32_t b,const uint8_t *x,uint8_t *out,uint32_t ob)
{
    uint32_t n=(b+7)>>3;if(n>ob)n=ob;memcpy(out,x,n);
    if((b&7)&&n)out[n-1]&=(uint8_t)(0xFF<<(8-(b&7)));
    if(n<ob)memset(out+n,0,ob-n);
}

/* Bit-length padding: M || 1 || 0^k */
static size_t pad_message_bits(const uint8_t *msg,unsigned long long mlen_bits,uint8_t **out,size_t *ol)
{
    size_t rem=(size_t)((mlen_bits+1)%BLOCK_BITS);
    size_t pb=rem?(BLOCK_BITS-rem):0;
    size_t tb=(size_t)(mlen_bits+1+pb);
    size_t nb=tb/BLOCK_BITS;
    size_t tbc=(tb+7)>>3;
    *out=(uint8_t*)malloc(tbc);
    if(!*out){*ol=0;return 0;}
    size_t full_bytes=(size_t)(mlen_bits>>3);
    size_t rem_bits=(size_t)(mlen_bits&7);
    memcpy(*out,msg,full_bytes);
    if(rem_bits){(*out)[full_bytes]=msg[full_bytes];(*out)[full_bytes]&=(uint8_t)(0xFF<<(8-(int)rem_bits));}
    else(*out)[full_bytes]=0;
    (*out)[full_bytes]|=(uint8_t)(1U<<(7-(int)rem_bits));
    for(size_t i=full_bytes+1;i<tbc;i++)(*out)[i]=0;
    *ol=tbc;return nb;
}

/* ================================================================
   LDRH: Short Message Domain (mlen_bits < 1472)
   y = P_FF(x||ctr1) XOR P_FF(x||ctr2)
   ================================================================ */
static void ldrh_bits(const uint8_t *msg,unsigned long long mlen_bits,uint8_t *d)
{
    uint8_t x[NB];memset(x,0,NB);
    size_t fb=(size_t)(mlen_bits>>3);
    size_t rb=(size_t)(mlen_bits&7);
    memcpy(x,msg,fb);
    if(rb)x[fb]=(msg[fb]&(uint8_t)(0xFF<<(8-(int)rb)));
    x[fb]|=(uint8_t)(1U<<(7-(int)rb));

    uint8_t in[192],y1[192],y2[192];
    memset(in,0,192);memcpy(in,x,NB);set_counter(in,build_counter(LANE_LDRH,1));
    c_engine_permute_ff(in,y1);
    memset(in,0,192);memcpy(in,x,NB);set_counter(in,build_counter(LANE_LDRH,2));
    c_engine_permute_ff(in,y2);

    for(int i=0;i<NB;i++)y1[i]^=y2[i];
    lmb(HASHLEN,y1,d,HASHLEN>>3);
}

/* ================================================================
   VIL + FIL: Variable Input Length + Finalization
   Unified structure shared with C-Hash-512.
   ================================================================ */
static int vil_fil_bits(const uint8_t *msg,unsigned long long mlen_bits,uint8_t *d)
{
    uint8_t iv[BB],h1[BB],h2[BB];
    build_iv(iv);

    /* State initialization */
    {
        uint8_t in[192],o[192];
        memset(in,0,192);memcpy(in,iv,BB);set_counter(in,build_counter(LANE_VIL,0));
        c_engine_permute_ff(in,o);
        memcpy(h1,o,BB);memcpy(h2,o+BB,BB);
        for(size_t i=0;i<BB;i++)h2[i]^=iv[i];
    }

    /* Message padding */
    uint8_t *pd=NULL;size_t pl=0;
    size_t nb=pad_message_bits(msg,mlen_bits,&pd,&pl);
    if(!pd)return-1;

    /* VIL compression loop */
    for(size_t bi=0;bi<nb;bi++){
        const uint8_t *mi=pd+bi*BB;
        uint8_t in[192],o[192];
        for(size_t i=0;i<BB;i++)in[i]=h1[i]^mi[i];
        memcpy(in+BB,h2,BB);
        set_counter(in,build_counter(LANE_VIL,(uint64_t)(bi+1)));
        c_engine_permute_ff(in,o);
        memcpy(h1,o,BB);memcpy(h2,o+BB,BB);
        for(size_t i=0;i<BB;i++)h2[i]^=mi[i];
    }
    free(pd);

    /* FIL finalization */
    {
        uint8_t in[192],o[192];
        memcpy(in,h1,BB);memcpy(in+BB,h2,BB);
        set_counter(in,build_counter(LANE_FIL,1));
        c_engine_permute_ff(in,o);
        lmb(HASHLEN,o,d,HASHLEN>>3);
    }
    return 0;
}

/* ================================================================
   Public API
   ================================================================ */
int c_hash_1024_bits(const unsigned char *msg,unsigned long long mlen_bits,unsigned char *digest)
{
    if(mlen_bits<(unsigned long long)N_BITS){ldrh_bits(msg,mlen_bits,digest);return 0;}
    return vil_fil_bits(msg,mlen_bits,digest);
}
int c_hash_1024(const unsigned char *msg,size_t msg_len,unsigned char *digest)
{
    return c_hash_1024_bits(msg,(unsigned long long)msg_len<<3,digest);
}
