#include <stdint.h>
#include <string.h>
#include <immintrin.h>
#include "ntt_avx2.h"
#include "ntt_avx2_params.h"

int mul_mod(int16_t *a, int16_t *b, int16_t *c)
{
    int i;

    __m256i tmp_a, tmp_b, tmp_c1, tmp_c0, tmp_x;
    __m256i tmp_QINV = _mm256_set1_epi16(INVERSE_Q); //RL_KEM_Q^{-1} mod 2^16, for montgomery
    __m256i tmp_Q = _mm256_set1_epi16(RL_KEM_Q);

    for (i = 0; i < RL_KEM_N; i += 16)
    {
        //load
        tmp_a = _mm256_load_si256((__m256i *)(a + i));
        tmp_b = _mm256_load_si256((__m256i *)(b + i));
        //c=a*b
        tmp_c0 = _mm256_mullo_epi16(tmp_a, tmp_b);
        tmp_c1 = _mm256_mulhi_epi16(tmp_a, tmp_b);
        //montgomery mod RL_KEM_Q
        tmp_x = _mm256_mullo_epi16(tmp_c0, tmp_QINV);
        tmp_x = _mm256_mulhi_epi16(tmp_x, tmp_Q);
        tmp_c1 = _mm256_sub_epi16(tmp_c1, tmp_x);
        //if c1<0 then +RL_KEM_Q
        tmp_c1 = _mm256_add_epi16(tmp_c1, _mm256_and_si256(_mm256_srai_epi16(tmp_c1, 15), tmp_Q));
        //store
        _mm256_store_si256((__m256i *)(c + i), tmp_c1);
    }

    return 0;
}


inline int ntt_core(__m256i *U, __m256i *V, __m256i *S,__m256i *A1,__m256i *A2)  //对蝴蝶变换输入U求模
{
    __m256i tmp_c0, tmp_c1, tmp_x;

    __m256i tmp_Q = _mm256_set1_epi16(RL_KEM_Q);
    __m256i tmp_QINV = _mm256_set1_epi16(INVERSE_Q); //RL_KEM_Q^{-1} mod 2^16
    __m256i tmp_B = _mm256_set1_epi16(B_Q); //RL_KEM_Q^{-1} mod 2^16
    
    //V*S  系数*本原单位根
    tmp_c0 = _mm256_mullo_epi16(*V, *S);
    tmp_c1 = _mm256_mulhi_epi16(*V, *S);
    
    //对乘积利用Montgomery求模，并map到(0,q)
    //V*S montgomery mod RL_KEM_Q
    tmp_x  = _mm256_mullo_epi16(tmp_c0, tmp_QINV);
    tmp_x  = _mm256_mulhi_epi16(tmp_x, tmp_Q);
    *V     = _mm256_sub_epi16(tmp_c1, tmp_x);

    //Mont(U) 
    tmp_c0 = _mm256_mullo_epi16(*U, tmp_B);
    tmp_c1 = _mm256_mulhi_epi16(*U, tmp_B);
        // Montgomery mod RL_KEM_Q
    tmp_x  = _mm256_mullo_epi16(tmp_c0, tmp_QINV);
    tmp_x  = _mm256_mulhi_epi16(tmp_x, tmp_Q);
    *U    = _mm256_sub_epi16(tmp_c1, tmp_x);
    
    //(U mod RL_KEM_Q) + V 
    *A1    = _mm256_add_epi16(*U, *V);
    
    //(U mod RL_KEM_Q) - V
    *A2   = _mm256_sub_epi16(*U, *V);

    return 0;
}
inline int ntt_core_lazymod(__m256i *U, __m256i *V, __m256i *S,__m256i *A1,__m256i *A2)
{
     __m256i tmp_c0, tmp_c1, tmp_x;

    __m256i tmp_Q = _mm256_set1_epi16(RL_KEM_Q);
    __m256i tmp_QINV = _mm256_set1_epi16(INVERSE_Q); //RL_KEM_Q^{-1} mod 2^16
    //V*S 系数*本原单位根
    tmp_c0 = _mm256_mullo_epi16(*V, *S);
    tmp_c1 = _mm256_mulhi_epi16(*V, *S);
    
    //对乘积利用Montgomery求模，并map到(0,RL_KEM_Q)
    //V*S montgomery map full mod RL_KEM_Q
    tmp_x  = _mm256_mullo_epi16(tmp_c0, tmp_QINV);
    tmp_x  = _mm256_mulhi_epi16(tmp_x, tmp_Q);
    *V     = _mm256_sub_epi16(tmp_c1, tmp_x);
    //if c1<0 then +RL_KEM_Q
  //  *V     = _mm256_add_epi16(*V, _mm256_and_si256(_mm256_srai_epi16(*V, 15), tmp_Q));

    //U + V - RL_KEM_Q
    *A1    = _mm256_add_epi16(*U, *V);
   // *A1    = _mm256_sub_epi16(*A1, tmp_Q); //去掉出错
    
    //U - V
    *A2   = _mm256_sub_epi16(*U, *V);

    return 0;

    
}


int NTT_AVX2(int16_t *a)
{
    int i, j, s;

    __m256i tmp_U, tmp_V, tmp_A1, tmp_A2, tmp_S;

    // __m256i tmp_shuffle16_pre = _mm256_set_epi8(0x0f, 0x0e, 0x0b, 0x0a, 0x07, 0x06, 0x03, 0x02, 0x0d, 0x0c, 0x09, 0x08, 0x05, 0x04, 0x01, 0x00,
    //                                     0x0f, 0x0e, 0x0b, 0x0a, 0x07, 0x06, 0x03, 0x02, 0x0d, 0x0c, 0x09, 0x08, 0x05, 0x04, 0x01, 0x00);

    // m：使用不同本原单位根的个数 （蝴蝶变换的组数）
    //level 1  step=N/2=256  m=1
    //load root 本原单位根存到 tmp_S
    tmp_S = _mm256_set1_epi16(f[1]);
    for(j=0;j<256;j+=16) //因为每个系数16bit存储，申请256bit的空间，所以一次可以计算16个系数乘法
    {
        //load  把系数保存到tmp_U tmp_V ，step=N/2
        tmp_U = _mm256_load_si256((__m256i *)(a +j));  //f[m+ (s++)]
        tmp_V = _mm256_load_si256((__m256i *)(a  + 256 +j)); //第一层 step=N/2
        //
        //执行CT蝴蝶变换
        // V*S%RL_KEM_Q, (U+V)%RL_KEM_Q, (U-V)%RL_KEM_Q
        ntt_core_lazymod(&tmp_U,&tmp_V,&tmp_S,&tmp_A1,&tmp_A2);
        //store  把蝴蝶变换的结果保存到系数向量中
        _mm256_store_si256((__m256i *)(a +j), tmp_A1);
        _mm256_store_si256((__m256i *)(a  + 256 +j), tmp_A2);
    }
    
    //level 2   step=N/4=128  m=2
    s=0;
    for (i = 0; i < RL_KEM_N; i += 256) //i=0,256  i=start index  循环m=2次
    {
        //load root
        tmp_S = _mm256_set1_epi16(f[2 +(s++)]);  //f[m+ (s++)]： f[2], f[3]
        for(j=0;j<128;j+=16) // j=0~step
            //i+j:每个蝴蝶变换的第一个输入， i+j+step：第二个输入 两个输入之间间隔一个step的长度
        {
            //load
            tmp_U = _mm256_load_si256((__m256i *)(a + i+j));
            tmp_V = _mm256_load_si256((__m256i *)(a + i + 128+j));
            // V*S%RL_KEM_Q, (U+V)%RL_KEM_Q, (U-V)%RL_KEM_Q
            ntt_core_lazymod(&tmp_U,&tmp_V,&tmp_S,&tmp_A1,&tmp_A2);
            //store
            _mm256_store_si256((__m256i *)(a + i+j), tmp_A1);
            _mm256_store_si256((__m256i *)(a + i + 128+j), tmp_A2);
        }
    }

    //level 3  step=N/8=64  m=4
    s=0;
    for (i = 0; i < RL_KEM_N; i += 128)  //  循环m=4次 i=start index
    {
        //load root
        tmp_S = _mm256_set1_epi16(f[4 +(s++)]);   //f[m+ (s++)]：f[4],f[5],f[6],f[7]
        for(j=0;j<64;j+=16) //j=0~step i+j: first input  i+j+step: second input
        {
            //load
            tmp_U = _mm256_load_si256((__m256i *)(a + i+j));
            tmp_V = _mm256_load_si256((__m256i *)(a + i + 64+j));
            // V*S%RL_KEM_Q, (U+V)%RL_KEM_Q, (U-V)%RL_KEM_Q
            ntt_core_lazymod(&tmp_U,&tmp_V,&tmp_S,&tmp_A1,&tmp_A2);
            //store
            _mm256_store_si256((__m256i *)(a + i+j), tmp_A1);
            _mm256_store_si256((__m256i *)(a + i + 64 +j), tmp_A2);
        }
    }

    //level 4 step=N/16=32 m=8
    s=0;
    for (i = 0; i < RL_KEM_N; i += 64)  //循环8次 i=start index
    {
        //load root
        tmp_S = _mm256_set1_epi16(f[8 +(s++)]); //f[8 + (s++)]
        for(j=0;j<32;j+=16)  //j=0~step
        {
            //load
            tmp_U = _mm256_load_si256((__m256i *)(a + i+j));
            tmp_V = _mm256_load_si256((__m256i *)(a + i + 32+j));
            // V*S%RL_KEM_Q, (U+V)%RL_KEM_Q, (U-V)%RL_KEM_Q
            // ntt_core(&tmp_U,&tmp_V,&tmp_S,&tmp_A1,&tmp_A2);
            ntt_core_lazymod(&tmp_U,&tmp_V,&tmp_S,&tmp_A1,&tmp_A2);
            //store
            _mm256_store_si256((__m256i *)(a + i+j), tmp_A1);
            _mm256_store_si256((__m256i *)(a + i + 32+j), tmp_A2);
        }
    }

    //level 5  step=N/32=16 m=16
    s=0;
    for (i = 0; i < RL_KEM_N; i += 32) //循环32次
    {
        //load
        tmp_S = _mm256_set1_epi16(f[16 +(s++)]);
        //j=0
        tmp_U = _mm256_load_si256((__m256i *)(a + i));
        tmp_V = _mm256_load_si256((__m256i *)(a + i + 16));
        // V*S%RL_KEM_Q, (U+V)%RL_KEM_Q, (U-V)%RL_KEM_Q
       
        ntt_core_lazymod(&tmp_U,&tmp_V,&tmp_S,&tmp_A1,&tmp_A2);
        //store
        _mm256_store_si256((__m256i *)(a + i), tmp_A1);
        _mm256_store_si256((__m256i *)(a + i + 16), tmp_A2);
    }
    
    
    //level 6  step=N/64=8 m=32  step不够一次并行计算 permute处理  2个参与
    s = 0;
    for (i = 0; i < RL_KEM_N; i += 32)
    {
        //S=f[m+i];
        tmp_S = _mm256_set_epi16(f[32 + s + 1], f[32 + s + 1], f[32 + s + 1], f[32 + s + 1], f[32 + s + 1], f[32 + s + 1], f[32 + s + 1], f[32 + s + 1],
                                 f[32 + s], f[32 + s], f[32 + s], f[32 + s], f[32 + s], f[32 + s], f[32 + s], f[32 + s]);  //256长 16个item，每个长16bit
        s += 2;
        //load
        tmp_A1 = _mm256_load_si256((__m256i *)(a + i));
        tmp_A2 = _mm256_load_si256((__m256i *)(a + i + 16));
        //permute
        tmp_U = _mm256_permute2x128_si256(tmp_A1, tmp_A2, 0x20);  //A1低128bit + A2低128bit
        tmp_V = _mm256_permute2x128_si256(tmp_A1, tmp_A2, 0x31);  //A1高128bit + A2高128bit
        // _mm256_permute2x128_si256
       // 功能：由两个256bit的数，选择其中的一部分构成一个新的256bit数。

        //V*S%RL_KEM_Q, (U+V)%RL_KEM_Q, (U-V)%RL_KEM_Q
        ntt_core_lazymod(&tmp_U,&tmp_V,&tmp_S,&tmp_A1,&tmp_A2);
        //store
        _mm256_store_si256((__m256i *)(a + i), tmp_A1);
        _mm256_store_si256((__m256i *)(a + i + 16), tmp_A2);
    }


    //level 7  step=N/128=4  m=64  step不够一次并行计算，permute处理 4个参与
    s = 0;
    for (i = 0; i < RL_KEM_N; i += 32)
    {
        //S=f[m+i];
        tmp_S = _mm256_set_epi16(f[64 + s + 3], f[64 + s + 3], f[64 + s + 3], f[64 + s + 3], f[64 + s + 1], f[64 + s + 1], f[64 + s + 1], f[64 + s + 1],
                                 f[64 + s + 2], f[64 + s + 2], f[64 + s + 2], f[64 + s + 2], f[64 + s], f[64 + s], f[64 + s], f[64 + s]);
        s += 4;
        //load
        tmp_A1 = _mm256_load_si256((__m256i *)(a + i));
        tmp_A2 = _mm256_load_si256((__m256i *)(a + i + 16));
        //permute
        tmp_A1 = _mm256_permute4x64_epi64(tmp_A1, 0xd8);
        tmp_A2 = _mm256_permute4x64_epi64(tmp_A2, 0xd8);
        tmp_U = _mm256_permute2x128_si256(tmp_A1, tmp_A2, 0x20);
        tmp_V = _mm256_permute2x128_si256(tmp_A1, tmp_A2, 0x31);
        //V*S%RL_KEM_Q, (U+V)%RL_KEM_Q, (U-V)%RL_KEM_Q
        ntt_core_lazymod(&tmp_U,&tmp_V,&tmp_S,&tmp_A1,&tmp_A2);
        //store
        _mm256_store_si256((__m256i *)(a + i), tmp_A1);
        _mm256_store_si256((__m256i *)(a + i + 16), tmp_A2);
    }

    // //level 8  step=N/256=2 m=128  8-permute
    // s = 0;
    // for (i = 0; i < RL_KEM_N; i += 32)
    // {
    //     //S=f[m+i];
    //     tmp_S = _mm256_set_epi16(f[128 + s + 7], f[128 + s + 7], f[128 + s + 3], f[128 + s + 3], f[128 + s + 5], f[128 + s + 5], f[128 + s + 1], f[128 + s + 1],
    //                              f[128 + s + 6], f[128 + s + 6], f[128 + s + 2], f[128 + s + 2], f[128 + s + 4], f[128 + s + 4], f[128 + s], f[128 + s]);
    //     s += 8;
    //     //load
    //     tmp_A1 = _mm256_load_si256((__m256i *)(a + i));
    //     tmp_A2 = _mm256_load_si256((__m256i *)(a + i + 16));
    //     //permute
    //     tmp_A1 = _mm256_shuffle_epi32(tmp_A1, 0xd8);
    //     tmp_A1 = _mm256_permute4x64_epi64(tmp_A1, 0xd8);

    //     tmp_A2 = _mm256_shuffle_epi32(tmp_A2, 0xd8);
    //     tmp_A2 = _mm256_permute4x64_epi64(tmp_A2, 0xd8);

    //     tmp_U = _mm256_permute2x128_si256(tmp_A1, tmp_A2, 0x20);
    //     tmp_V = _mm256_permute2x128_si256(tmp_A1, tmp_A2, 0x31);
    //     //V*S
    //     //V*S%RL_KEM_Q, (U+V)%RL_KEM_Q, (U-V)%RL_KEM_Q
    //     // ntt_core(&tmp_U,&tmp_V,&tmp_S,&tmp_A1,&tmp_A2);
    //     ntt_core_lazymod(&tmp_U,&tmp_V,&tmp_S,&tmp_A1,&tmp_A2);
    //     //store
    //     _mm256_store_si256((__m256i *)(a + i), tmp_A1);
    //     _mm256_store_si256((__m256i *)(a + i + 16), tmp_A2);
    // }

    // //level 9  step=N/512=1 m=256   16-permute
    // s = 0;
    // for (i = 0; i < RL_KEM_N; i += 32)
    // {
    //     //S=f[m+i];
    //     tmp_S = _mm256_set_epi16(f[256 + s + 15], f[256 + s + 7], f[256 + s + 11], f[256 + s + 3], f[256 + s + 13], f[256 + s + 5], f[256 + s + 9], f[256 + s + 1],
    //                              f[256 + s + 14], f[256 + s + 6], f[256 + s + 10], f[256 + s + 2], f[256 + s + 12], f[256 + s + 4], f[256 + s + 8], f[256 + s]);
    //     s += 16;
    //     //load
    //     tmp_A1 = _mm256_load_si256((__m256i *)(a + i));
    //     tmp_A2 = _mm256_load_si256((__m256i *)(a + i + 16));
    //     //permute
    //     tmp_A1 = _mm256_shuffle_epi8(tmp_A1, tmp_shuffle16_pre);
    //     tmp_A1 = _mm256_permute4x64_epi64(tmp_A1, 0xd8);

    //     tmp_A2 = _mm256_shuffle_epi8(tmp_A2, tmp_shuffle16_pre);
    //     tmp_A2 = _mm256_permute4x64_epi64(tmp_A2, 0xd8);

    //     tmp_U = _mm256_permute2x128_si256(tmp_A1, tmp_A2, 0x20);
    //     tmp_V = _mm256_permute2x128_si256(tmp_A1, tmp_A2, 0x31);
    //     //V*S%RL_KEM_Q, (U+V)%RL_KEM_Q, (U-V)%RL_KEM_Q
    //     ntt_core(&tmp_U,&tmp_V,&tmp_S,&tmp_A1,&tmp_A2);
    //     //store
    //     _mm256_store_si256((__m256i *)(a + i), tmp_A1);
    //     _mm256_store_si256((__m256i *)(a + i + 16), tmp_A2);
    // }

    return 0;
}


//********************************Lazy INTT************************************//

int lazyincom_mul_mod_avx2(int16_t *a, int16_t *b, int16_t *c)  //  c=a*b
{
    int i;

    int16_t __attribute__((aligned(64))) r0[(RL_KEM_N / 4)],r1[(RL_KEM_N / 4)],a0[(RL_KEM_N / 4)],a1[(RL_KEM_N / 4)],b0[(RL_KEM_N / 4)],b1[(RL_KEM_N / 4)];
    int16_t __attribute__((aligned(64))) r2[(RL_KEM_N / 4)],r3[(RL_KEM_N / 4)],a2[(RL_KEM_N / 4)],a3[(RL_KEM_N / 4)],b2[(RL_KEM_N / 4)],b3[(RL_KEM_N / 4)];

    for(i=0;i<RL_KEM_N;i+=4)
    {
        a0[i>>2]=a[i];
        a1[i>>2]=a[i+1];
        a2[i>>2]=a[i+2];
        a3[i>>2]=a[i+3];

        b0[i>>2]=b[i];
        b1[i>>2]=b[i+1];
        b2[i>>2]=b[i+2];
        b3[i>>2]=b[i+3];
    }

    __m256i tmp_r0,tmp_r1,tmp_a0,tmp_a1,tmp_b0,tmp_b1,tmp_s;
    __m256i tmp_r2,tmp_r3,tmp_a2,tmp_a3,tmp_b2,tmp_b3;
    __m256i tmp_c0, tmp_c1, tmp_x;
    __m256i tmp_m0, tmp_m1, tmp_m2, tmp_m3;
    __m256i tmp_QINV = _mm256_set1_epi16(INVERSE_Q); //RL_KEM_Q^{-1} mod 2^16, for montgomery
    __m256i tmp_Q = _mm256_set1_epi16(RL_KEM_Q);

    for (i = 0; i < (RL_KEM_N / 4); i += 16)
    {
        // load
        tmp_a0 = _mm256_load_si256((__m256i *)(a0+i));
        tmp_a1 = _mm256_load_si256((__m256i *)(a1+i));
        tmp_a2 = _mm256_load_si256((__m256i *)(a2+i));
        tmp_a3 = _mm256_load_si256((__m256i *)(a3+i));

        tmp_b0 = _mm256_load_si256((__m256i *)(b0+i));
        tmp_b1 = _mm256_load_si256((__m256i *)(b1+i));
        tmp_b2 = _mm256_load_si256((__m256i *)(b2+i));
        tmp_b3 = _mm256_load_si256((__m256i *)(b3+i));

        tmp_s = _mm256_load_si256((__m256i *)(poly_basemul_f2+i));

        // first res : a0b0 + (a1b3 + a2b2 + a3b1)*s
        // m_1 = Mont(a1 * b3)
        tmp_c0 = _mm256_mullo_epi16(tmp_a1, tmp_b3);
        tmp_c1 = _mm256_mulhi_epi16(tmp_a1, tmp_b3);
        tmp_x  = _mm256_mullo_epi16(tmp_c0, tmp_QINV);
        tmp_x  = _mm256_mulhi_epi16(tmp_x, tmp_Q);
        tmp_m1     = _mm256_sub_epi16(tmp_c1, tmp_x);
        // m_2 = Mont(a2 * b2)
        tmp_c0 = _mm256_mullo_epi16(tmp_a2, tmp_b2);
        tmp_c1 = _mm256_mulhi_epi16(tmp_a2, tmp_b2);
        tmp_x  = _mm256_mullo_epi16(tmp_c0, tmp_QINV);
        tmp_x  = _mm256_mulhi_epi16(tmp_x, tmp_Q);
        tmp_m2     = _mm256_sub_epi16(tmp_c1, tmp_x);
        // m_3 = Mont(a3 * b1)
        tmp_c0 = _mm256_mullo_epi16(tmp_a3, tmp_b1);
        tmp_c1 = _mm256_mulhi_epi16(tmp_a3, tmp_b1);
        tmp_x  = _mm256_mullo_epi16(tmp_c0, tmp_QINV);
        tmp_x  = _mm256_mulhi_epi16(tmp_x, tmp_Q);
        tmp_m3     = _mm256_sub_epi16(tmp_c1, tmp_x);

        // m1 = m1+m2+m3
        tmp_m1     = _mm256_add_epi16(tmp_m1, tmp_m2);
        tmp_m1     = _mm256_add_epi16(tmp_m1, tmp_m3);
        // m1 = Mont(m1 * s)
        tmp_c0 = _mm256_mullo_epi16(tmp_m1, tmp_s);
        tmp_c1 = _mm256_mulhi_epi16(tmp_m1, tmp_s);
        tmp_x  = _mm256_mullo_epi16(tmp_c0, tmp_QINV);
        tmp_x  = _mm256_mulhi_epi16(tmp_x, tmp_Q);
        tmp_m1 = _mm256_sub_epi16(tmp_c1, tmp_x);

        // m_0 = Mont(a0 * b0)
        tmp_c0 = _mm256_mullo_epi16(tmp_a0, tmp_b0);
        tmp_c1 = _mm256_mulhi_epi16(tmp_a0, tmp_b0);
        tmp_x  = _mm256_mullo_epi16(tmp_c0, tmp_QINV);
        tmp_x  = _mm256_mulhi_epi16(tmp_x, tmp_Q);
        tmp_m0     = _mm256_sub_epi16(tmp_c1, tmp_x);

        // r0 = m0 + m1
        tmp_r0     = _mm256_add_epi16(tmp_m0, tmp_m1);
        


        // second res  : a0b1 + a1b0 + (a2b3 + a3b2)*s
        // m_0 = Mont(a0 * b1)
        tmp_c0 = _mm256_mullo_epi16(tmp_a0, tmp_b1);
        tmp_c1 = _mm256_mulhi_epi16(tmp_a0, tmp_b1);
        tmp_x  = _mm256_mullo_epi16(tmp_c0, tmp_QINV);
        tmp_x  = _mm256_mulhi_epi16(tmp_x, tmp_Q);
        tmp_m0     = _mm256_sub_epi16(tmp_c1, tmp_x);
        // m_1 = Mont(a1 * b0)
        tmp_c0 = _mm256_mullo_epi16(tmp_a1, tmp_b0);
        tmp_c1 = _mm256_mulhi_epi16(tmp_a1, tmp_b0);
        tmp_x  = _mm256_mullo_epi16(tmp_c0, tmp_QINV);
        tmp_x  = _mm256_mulhi_epi16(tmp_x, tmp_Q);
        tmp_m1     = _mm256_sub_epi16(tmp_c1, tmp_x);

        // m_2 = Mont(a2 * b3)
        tmp_c0 = _mm256_mullo_epi16(tmp_a2, tmp_b3);
        tmp_c1 = _mm256_mulhi_epi16(tmp_a2, tmp_b3);
        tmp_x  = _mm256_mullo_epi16(tmp_c0, tmp_QINV);
        tmp_x  = _mm256_mulhi_epi16(tmp_x, tmp_Q);
        tmp_m2     = _mm256_sub_epi16(tmp_c1, tmp_x);
        // m_3 = Mont(a3 * b2)
        tmp_c0 = _mm256_mullo_epi16(tmp_a3, tmp_b2);
        tmp_c1 = _mm256_mulhi_epi16(tmp_a3, tmp_b2);
        tmp_x  = _mm256_mullo_epi16(tmp_c0, tmp_QINV);
        tmp_x  = _mm256_mulhi_epi16(tmp_x, tmp_Q);
        tmp_m3     = _mm256_sub_epi16(tmp_c1, tmp_x);

        // m2 = m2+m3
        tmp_m2     = _mm256_add_epi16(tmp_m2, tmp_m3);
        // m2 = Mont(m2 * s)
        tmp_c0 = _mm256_mullo_epi16(tmp_m2, tmp_s);
        tmp_c1 = _mm256_mulhi_epi16(tmp_m2, tmp_s);
        tmp_x  = _mm256_mullo_epi16(tmp_c0, tmp_QINV);
        tmp_x  = _mm256_mulhi_epi16(tmp_x, tmp_Q);
        tmp_m2 = _mm256_sub_epi16(tmp_c1, tmp_x);

        // r1 = m0 + m1 +m2
        tmp_r1     = _mm256_add_epi16(tmp_m0, tmp_m1);
        tmp_r1     = _mm256_add_epi16(tmp_r1, tmp_m2);



        // third res  : a0b2 + a1b1 + a2b0 + a3b3*s
        // m_0 = Mont(a0 * b2)
        tmp_c0 = _mm256_mullo_epi16(tmp_a0, tmp_b2);
        tmp_c1 = _mm256_mulhi_epi16(tmp_a0, tmp_b2);
        tmp_x  = _mm256_mullo_epi16(tmp_c0, tmp_QINV);
        tmp_x  = _mm256_mulhi_epi16(tmp_x, tmp_Q);
        tmp_m0     = _mm256_sub_epi16(tmp_c1, tmp_x);
        // m_1 = Mont(a1 * b1)
        tmp_c0 = _mm256_mullo_epi16(tmp_a1, tmp_b1);
        tmp_c1 = _mm256_mulhi_epi16(tmp_a1, tmp_b1);
        tmp_x  = _mm256_mullo_epi16(tmp_c0, tmp_QINV);
        tmp_x  = _mm256_mulhi_epi16(tmp_x, tmp_Q);
        tmp_m1     = _mm256_sub_epi16(tmp_c1, tmp_x);
        // m_2 = Mont(a2 * b0)
        tmp_c0 = _mm256_mullo_epi16(tmp_a2, tmp_b0);
        tmp_c1 = _mm256_mulhi_epi16(tmp_a2, tmp_b0);
        tmp_x  = _mm256_mullo_epi16(tmp_c0, tmp_QINV);
        tmp_x  = _mm256_mulhi_epi16(tmp_x, tmp_Q);
        tmp_m2     = _mm256_sub_epi16(tmp_c1, tmp_x);

        // m_3 = Mont(a3 * b3)
        tmp_c0 = _mm256_mullo_epi16(tmp_a3, tmp_b3);
        tmp_c1 = _mm256_mulhi_epi16(tmp_a3, tmp_b3);
        tmp_x  = _mm256_mullo_epi16(tmp_c0, tmp_QINV);
        tmp_x  = _mm256_mulhi_epi16(tmp_x, tmp_Q);
        tmp_m3     = _mm256_sub_epi16(tmp_c1, tmp_x);
        // m3 = Mont(m3 * s)
        tmp_c0 = _mm256_mullo_epi16(tmp_m3, tmp_s);
        tmp_c1 = _mm256_mulhi_epi16(tmp_m3, tmp_s);
        tmp_x  = _mm256_mullo_epi16(tmp_c0, tmp_QINV);
        tmp_x  = _mm256_mulhi_epi16(tmp_x, tmp_Q);
        tmp_m3 = _mm256_sub_epi16(tmp_c1, tmp_x);

        // m0 = m0+m1+m2
        tmp_m0     = _mm256_add_epi16(tmp_m0, tmp_m1);
        tmp_m0     = _mm256_add_epi16(tmp_m0, tmp_m2);
        
        // r2 = m0 + m3
        tmp_r2     = _mm256_add_epi16(tmp_m0, tmp_m3);


        // fourth res  : a0b3 + a1b2 + a2b1 + a3b0
        // m_0 = Mont(a0 * b3)
        tmp_c0 = _mm256_mullo_epi16(tmp_a0, tmp_b3);
        tmp_c1 = _mm256_mulhi_epi16(tmp_a0, tmp_b3);
        tmp_x  = _mm256_mullo_epi16(tmp_c0, tmp_QINV);
        tmp_x  = _mm256_mulhi_epi16(tmp_x, tmp_Q);
        tmp_m0     = _mm256_sub_epi16(tmp_c1, tmp_x);
        // m_1 = Mont(a1 * b2)
        tmp_c0 = _mm256_mullo_epi16(tmp_a1, tmp_b2);
        tmp_c1 = _mm256_mulhi_epi16(tmp_a1, tmp_b2);
        tmp_x  = _mm256_mullo_epi16(tmp_c0, tmp_QINV);
        tmp_x  = _mm256_mulhi_epi16(tmp_x, tmp_Q);
        tmp_m1     = _mm256_sub_epi16(tmp_c1, tmp_x);
        // m_2 = Mont(a2 * b1)
        tmp_c0 = _mm256_mullo_epi16(tmp_a2, tmp_b1);
        tmp_c1 = _mm256_mulhi_epi16(tmp_a2, tmp_b1);
        tmp_x  = _mm256_mullo_epi16(tmp_c0, tmp_QINV);
        tmp_x  = _mm256_mulhi_epi16(tmp_x, tmp_Q);
        tmp_m2     = _mm256_sub_epi16(tmp_c1, tmp_x);
        // m_3 = Mont(a3 * b0)
        tmp_c0 = _mm256_mullo_epi16(tmp_a3, tmp_b0);
        tmp_c1 = _mm256_mulhi_epi16(tmp_a3, tmp_b0);
        tmp_x  = _mm256_mullo_epi16(tmp_c0, tmp_QINV);
        tmp_x  = _mm256_mulhi_epi16(tmp_x, tmp_Q);
        tmp_m3     = _mm256_sub_epi16(tmp_c1, tmp_x);

        //r3 =  m0+m1+m2+m3
        tmp_m0     = _mm256_add_epi16(tmp_m0, tmp_m1);
        tmp_m2     = _mm256_add_epi16(tmp_m2, tmp_m3);
        tmp_r3     = _mm256_add_epi16(tmp_m0, tmp_m2);


        //store
        _mm256_store_si256((__m256i *)(r0+i), tmp_r0);
        _mm256_store_si256((__m256i *)(r1+i), tmp_r1);
        _mm256_store_si256((__m256i *)(r2+i), tmp_r2);
        _mm256_store_si256((__m256i *)(r3+i), tmp_r3);

    }
    for(i=0;i<RL_KEM_N;i+=4)
    {
        c[i]=r0[i>>2];
        c[i+1]=r1[i>>2];
        c[i+2]=r2[i>>2];
        c[i+3]=r3[i>>2];
    }

    return 0;
}

// inline int intt_first_level(__m256i *U, __m256i *V, __m256i *S,__m256i *A1,__m256i *A2)
// {
//     __m256i tmp_c0, tmp_c1, tmp_x;

//     __m256i tmp_Q = _mm256_set1_epi16(RL_KEM_Q);
//     __m256i tmp_QINV = _mm256_set1_epi16(INVERSE_Q); //RL_KEM_Q^{-1} mod 2^16
//     //A1=(U+V)-RL_KEM_Q
//     *A1 = _mm256_add_epi16(*U, *V);
//     *A1 = _mm256_sub_epi16(*A1, tmp_Q);
//     //V=(U-V)
//     *A2 = _mm256_sub_epi16(*U, *V);
//     //V*S
//     tmp_c0 = _mm256_mullo_epi16(*A2, *S);
//     tmp_c1 = _mm256_mulhi_epi16(*A2, *S);
//     //mod RL_KEM_Q
//     //montgomery mod RL_KEM_Q
//     tmp_x  = _mm256_mullo_epi16(tmp_c0, tmp_QINV);
//     tmp_x  = _mm256_mulhi_epi16(tmp_x, tmp_Q);
//     *A2    = _mm256_sub_epi16(tmp_c1, tmp_x);

//     return 0;
// }

inline int intt_core_lazymod(__m256i *U, __m256i *V, __m256i *S,__m256i *A1,__m256i *A2)
{
    __m256i tmp_c0, tmp_c1, tmp_x;

    __m256i tmp_Q = _mm256_set1_epi16(RL_KEM_Q);
    __m256i tmp_QINV = _mm256_set1_epi16(INVERSE_Q); //RL_KEM_Q^{-1} mod 2^16
    //A1=U+V
    *A1 = _mm256_add_epi16(*U, *V);

    //V=(U-V)
    *A2 = _mm256_sub_epi16(*U, *V);
    //V*S
    tmp_c0 = _mm256_mullo_epi16(*A2, *S);
    tmp_c1 = _mm256_mulhi_epi16(*A2, *S);
    //mod RL_KEM_Q
    //montgomery mod RL_KEM_Q
    tmp_x  = _mm256_mullo_epi16(tmp_c0, tmp_QINV);
    tmp_x  = _mm256_mulhi_epi16(tmp_x, tmp_Q);
    *A2    = _mm256_sub_epi16(tmp_c1, tmp_x);

    return 0;
}

inline int intt_core_extramod(__m256i *U, __m256i *V, __m256i *S,__m256i *A1,__m256i *A2)
{
    __m256i tmp_c0, tmp_c1, tmp_x;

    __m256i tmp_Q = _mm256_set1_epi16(RL_KEM_Q);
    __m256i tmp_QINV = _mm256_set1_epi16(INVERSE_Q); //RL_KEM_Q^{-1} mod 2^16
    __m256i tmp_B = _mm256_set1_epi16(B_Q); //RL_KEM_Q^{-1} mod 2^16
    //A1=Mont((U+V)* (beta mod q))
        //A1=Mont(U+V)
    *A1 = _mm256_add_epi16(*U, *V);
        //(U+V)*beta
    tmp_c0 = _mm256_mullo_epi16(*A1, tmp_B);
    tmp_c1 = _mm256_mulhi_epi16(*A1, tmp_B);
        // Montgomery mod RL_KEM_Q
    tmp_x  = _mm256_mullo_epi16(tmp_c0, tmp_QINV);
    tmp_x  = _mm256_mulhi_epi16(tmp_x, tmp_Q);
    *A1    = _mm256_sub_epi16(tmp_c1, tmp_x);

    //V=(U-V)
    *A2 = _mm256_sub_epi16(*U, *V);
    //V*S
    tmp_c0 = _mm256_mullo_epi16(*A2, *S);
    tmp_c1 = _mm256_mulhi_epi16(*A2, *S);
    //mod RL_KEM_Q
    //montgomery mod RL_KEM_Q
    tmp_x  = _mm256_mullo_epi16(tmp_c0, tmp_QINV);
    tmp_x  = _mm256_mulhi_epi16(tmp_x, tmp_Q);
    *A2    = _mm256_sub_epi16(tmp_c1, tmp_x);

    return 0;
}
inline int intt_core_finalextra(__m256i *U, __m256i *V, __m256i *S,__m256i *A1,__m256i *A2)
{
    __m256i tmp_c0, tmp_c1, tmp_x;

    __m256i tmp_Q = _mm256_set1_epi16(RL_KEM_Q);
    __m256i tmp_QINV = _mm256_set1_epi16(INVERSE_Q); //RL_KEM_Q^{-1} mod 2^16
    __m256i tmp_b = _mm256_set1_epi16(INVERSE_N_BETA2);//N^-1*2^16*2*16 mod RL_KEM_Q
    //A1=U+V
    *A1 = _mm256_add_epi16(*U, *V);
    //c=A1*b
    tmp_c0 = _mm256_mullo_epi16(*A1, tmp_b);
    tmp_c1 = _mm256_mulhi_epi16(*A1, tmp_b);
    //montgomery mod RL_KEM_Q
    tmp_x = _mm256_mullo_epi16(tmp_c0, tmp_QINV);
    tmp_x = _mm256_mulhi_epi16(tmp_x, tmp_Q);
    *A1   = _mm256_sub_epi16(tmp_c1, tmp_x);
    //if c1<0 then +RL_KEM_Q
    *A1 = _mm256_add_epi16(*A1, _mm256_and_si256(_mm256_srai_epi16(*A1, 15), tmp_Q));

    //V=(U-V)
    *A2 = _mm256_sub_epi16(*U, *V);
     //c=A2*S
    tmp_c0 = _mm256_mullo_epi16(*A2,  *S);
    tmp_c1 = _mm256_mulhi_epi16(*A2,  *S);
    //montgomery mod RL_KEM_Q
    tmp_x = _mm256_mullo_epi16(tmp_c0, tmp_QINV);
    tmp_x = _mm256_mulhi_epi16(tmp_x, tmp_Q);
    *A2   = _mm256_sub_epi16(tmp_c1, tmp_x);
    
    //if c1<0 then +RL_KEM_Q
    *A2    = _mm256_add_epi16(*A2, _mm256_and_si256(_mm256_srai_epi16(*A2, 15), tmp_Q));

    return 0;
}
int INTT_AVX2_Lazy(int16_t *a)
{
    int i, j, s;

    __m256i tmp_U, tmp_V, tmp_A1, tmp_A2, tmp_S;
    // __m256i tmp_shuffle16_pos = _mm256_set_epi8(0x0f, 0x0e, 0x07, 0x06, 0x0d, 0x0c, 0x05, 0x04, 0x0b, 0x0a, 0x03, 0x02, 0x09, 0x08, 0x01, 0x00,
    //                                     0x0f, 0x0e, 0x07, 0x06, 0x0d, 0x0c, 0x05, 0x04, 0x0b, 0x0a, 0x03, 0x02, 0x09, 0x08, 0x01, 0x00);

    // //level 9  step=N/512=1  m=256  16-permute
    // s = 0;
    // for (i = 0; i < RL_KEM_N; i += 32)
    //     //循环体内容与NTT过程相反
    // {
    //     //S=f[m+i];
    //     tmp_S = _mm256_set_epi16(fn[256 + s + 15], fn[256 + s + 7], fn[256 + s + 11], fn[256 + s + 3], fn[256 + s + 13], fn[256 + s + 5], fn[256 + s + 9], fn[256 + s + 1],
    //                              fn[256 + s + 14], fn[256 + s + 6], fn[256 + s + 10], fn[256 + s + 2], fn[256 + s + 12], fn[256 + s + 4], fn[256 + s + 8], fn[256 + s]);
    //     s += 16;
    //     //load
    //     tmp_U = _mm256_load_si256((__m256i *)(a + i));
    //     tmp_V = _mm256_load_si256((__m256i *)(a + i + 16));
    //     //  A1=(U+V-RL_KEM_Q)%RL_KEM_Q, A2=MontFull((U-V)*S)
    //     intt_first_level(&tmp_U,&tmp_V,&tmp_S,&tmp_A1,&tmp_A2);

    //     //permute
    //     tmp_U = _mm256_permute2x128_si256(tmp_A1, tmp_A2, 0x20);
    //     tmp_V = _mm256_permute2x128_si256(tmp_A1, tmp_A2, 0x31);

    //     tmp_U = _mm256_permute4x64_epi64(tmp_U, 0xd8);
    //     tmp_U = _mm256_shuffle_epi8(tmp_U, tmp_shuffle16_pos);

    //     tmp_V = _mm256_permute4x64_epi64(tmp_V, 0xd8);
    //     tmp_V = _mm256_shuffle_epi8(tmp_V, tmp_shuffle16_pos);
    //     //store
    //     _mm256_store_si256((__m256i *)(a + i), tmp_U);
    //     _mm256_store_si256((__m256i *)(a + i + 16), tmp_V);
    // }

    // //level 8  step=N/256=2  m=128  8-permute
    // s = 0;
    // for (i = 0; i < RL_KEM_N; i += 32)
    // {
    //     //S=f[m+i];
    //     tmp_S = _mm256_set_epi16(fn[128 + s + 7], fn[128 + s + 7], fn[128 + s + 3], fn[128 + s + 3], fn[128 + s + 5], fn[128 + s + 5], fn[128 + s + 1], fn[128 + s + 1],
    //                              fn[128 + s + 6], fn[128 + s + 6], fn[128 + s + 2], fn[128 + s + 2], fn[128 + s + 4], fn[128 + s + 4], fn[128 + s], fn[128 + s]);
    //     s += 8;
    //     //load
    //     tmp_U = _mm256_load_si256((__m256i *)(a + i));
    //     tmp_V = _mm256_load_si256((__m256i *)(a + i + 16));
    //     //V*S%RL_KEM_Q, (U+V)%RL_KEM_Q, (U-V)%RL_KEM_Q
    //     intt_core_extramod(&tmp_U,&tmp_V,&tmp_S,&tmp_A1,&tmp_A2);
    //     //permute
    //     tmp_U = _mm256_permute2x128_si256(tmp_A1, tmp_A2, 0x20);
    //     tmp_V = _mm256_permute2x128_si256(tmp_A1, tmp_A2, 0x31);

    //     tmp_U = _mm256_permute4x64_epi64(tmp_U, 0xd8);
    //     tmp_U = _mm256_shuffle_epi32(tmp_U, 0xd8);

    //     tmp_V = _mm256_permute4x64_epi64(tmp_V, 0xd8);
    //     tmp_V = _mm256_shuffle_epi32(tmp_V, 0xd8);
    //     //store
    //     _mm256_store_si256((__m256i *)(a + i), tmp_U);
    //     _mm256_store_si256((__m256i *)(a + i + 16), tmp_V);
    // }

    //level 7  step=N/128=4  m=64  4-permute
    s = 0;
    for (i = 0; i < RL_KEM_N; i += 32)
    {
        //S=f[m+i];
        tmp_S = _mm256_set_epi16(fn[64 + s + 3], fn[64 + s + 3], fn[64 + s + 3], fn[64 + s + 3], fn[64 + s + 1], fn[64 + s + 1], fn[64 + s + 1], fn[64 + s + 1],
                                 fn[64 + s + 2], fn[64 + s + 2], fn[64 + s + 2], fn[64 + s + 2], fn[64 + s], fn[64 + s], fn[64 + s], fn[64 + s]);
        s += 4;
        //load
        tmp_U = _mm256_load_si256((__m256i *)(a + i));
        tmp_V = _mm256_load_si256((__m256i *)(a + i + 16));
        //V*S%RL_KEM_Q, (U+V)%RL_KEM_Q, (U-V)%RL_KEM_Q
        intt_core_lazymod(&tmp_U,&tmp_V,&tmp_S,&tmp_A1,&tmp_A2);

        //permute
        tmp_U = _mm256_permute2x128_si256(tmp_A1, tmp_A2, 0x20);
        tmp_V = _mm256_permute2x128_si256(tmp_A1, tmp_A2, 0x31);
        tmp_U = _mm256_permute4x64_epi64(tmp_U, 0xd8);
        tmp_V = _mm256_permute4x64_epi64(tmp_V, 0xd8);
        //store  
        _mm256_store_si256((__m256i *)(a + i), tmp_U);
        _mm256_store_si256((__m256i *)(a + i + 16), tmp_V);
    }

    //level 6   step=N/64=8  m=32  2-permute
    s = 0;
    for (i = 0; i < RL_KEM_N; i += 32)
    {
        //S=f[m+i];
        tmp_S = _mm256_set_epi16(fn[32 + s + 1], fn[32 + s + 1], fn[32 + s + 1], fn[32 + s + 1], fn[32 + s + 1], fn[32 + s + 1], fn[32 + s + 1], fn[32 + s + 1],
                                 fn[32 + s], fn[32 + s], fn[32 + s], fn[32 + s], fn[32 + s], fn[32 + s], fn[32 + s], fn[32 + s]);
        s += 2;
        //load
        tmp_U = _mm256_load_si256((__m256i *)(a + i));
        tmp_V = _mm256_load_si256((__m256i *)(a + i + 16));
       
        //V*S%RL_KEM_Q, (U+V)%RL_KEM_Q, (U-V)%RL_KEM_Q
        intt_core_lazymod(&tmp_U,&tmp_V,&tmp_S,&tmp_A1,&tmp_A2);
        //permute
        tmp_U = _mm256_permute2x128_si256(tmp_A1, tmp_A2, 0x20);
        tmp_V = _mm256_permute2x128_si256(tmp_A1, tmp_A2, 0x31);
        //store
        _mm256_store_si256((__m256i *)(a + i), tmp_U);
        _mm256_store_si256((__m256i *)(a + i + 16), tmp_V);
    }
    //level 5  step=N/32=16   m=16  
    s = 0;
    for (i = 0; i < RL_KEM_N; i += 32)
    {
        //S=f[m+i];
        tmp_S = _mm256_set1_epi16(fn[16 + s++]);
        //load
        tmp_U = _mm256_load_si256((__m256i *)(a + i));
        tmp_V = _mm256_load_si256((__m256i *)(a + i + 16));
       
        //V*S%RL_KEM_Q, (U+V)%RL_KEM_Q, (U-V)%RL_KEM_Q
        intt_core_lazymod(&tmp_U,&tmp_V,&tmp_S,&tmp_A1,&tmp_A2);
       
        //store
        _mm256_store_si256((__m256i *)(a + i), tmp_A1);
        _mm256_store_si256((__m256i *)(a + i + 16), tmp_A2);
    }

    //level 4  step=N/16=32   m=8
    s=0;
    for (i = 0; i < RL_KEM_N; i += 64)
    {
        //load root
        tmp_S = _mm256_set1_epi16(fn[8 +(s++)]);
        //load block 1
        tmp_U = _mm256_load_si256((__m256i *)(a + i));
        tmp_V = _mm256_load_si256((__m256i *)(a + i + 32));
        //V*S%RL_KEM_Q, (U+V)%RL_KEM_Q, (U-V)%RL_KEM_Q
        intt_core_lazymod(&tmp_U,&tmp_V,&tmp_S,&tmp_A1,&tmp_A2);
        //store
        _mm256_store_si256((__m256i *)(a + i), tmp_A1);
        _mm256_store_si256((__m256i *)(a + i + 32), tmp_A2);

         //load block 2
        tmp_U = _mm256_load_si256((__m256i *)(a + i+16));
        tmp_V = _mm256_load_si256((__m256i *)(a + i + 48));
        //V*S%RL_KEM_Q, (U+V)%RL_KEM_Q, (U-V)%RL_KEM_Q
        intt_core_lazymod(&tmp_U,&tmp_V,&tmp_S,&tmp_A1,&tmp_A2);
        //store
        _mm256_store_si256((__m256i *)(a + i+16), tmp_A1);
        _mm256_store_si256((__m256i *)(a + i + 48), tmp_A2);
    }

    //level 3  step=N/8=64  m=4
    s=0;
    for (i = 0; i < RL_KEM_N; i += 128)
    {
        //load root
        tmp_S = _mm256_set1_epi16(fn[4 +(s++)]);
        for(j=0;j<64;j+=16)
        {
            //load
            tmp_U = _mm256_load_si256((__m256i *)(a + i+j));
            tmp_V = _mm256_load_si256((__m256i *)(a + i + 64+j));
            //V*S%RL_KEM_Q, (U+V)%RL_KEM_Q, (U-V)%RL_KEM_Q
            intt_core_lazymod(&tmp_U,&tmp_V,&tmp_S,&tmp_A1,&tmp_A2);
            //store
            _mm256_store_si256((__m256i *)(a + i+j), tmp_A1);
            _mm256_store_si256((__m256i *)(a + i + 64+j), tmp_A2);
        }
    }

     //level 2  step=N/4=128  m=2
    s=0;
    for (i = 0; i < RL_KEM_N; i += 256)
    {
        //load root
        tmp_S = _mm256_set1_epi16(fn[2 +(s++)]);
        for(j=0;j<128;j+=16)
        {
            //load
            tmp_U = _mm256_load_si256((__m256i *)(a + i+j));
            tmp_V = _mm256_load_si256((__m256i *)(a + i + 128+j));
            //V*S%RL_KEM_Q, (U+V)%RL_KEM_Q, (U-V)%RL_KEM_Q
            intt_core_lazymod(&tmp_U,&tmp_V,&tmp_S,&tmp_A1,&tmp_A2);
            //store
            _mm256_store_si256((__m256i *)(a + i+j), tmp_A1);
            _mm256_store_si256((__m256i *)(a + i + 128+j), tmp_A2);
        }
    }

    //level 1 step=N/2=256  m=1
    tmp_S = _mm256_set1_epi16(Normal);
    //fn[1]*INVERSE_N_BETA%RL_KEM_Q, 
    //point-mul and inverse-n-mul use montgomery mod, so two 2^16 factor are embeded in INVERSE_N,
    //however, for the final level, the right half inverse-n-mul is combined with *fn[1]
    for(j=0;j<256;j+=16)
    {
        //load
        tmp_U = _mm256_load_si256((__m256i *)(a +j));
        tmp_V = _mm256_load_si256((__m256i *)(a  + 256 +j));
        //V*S%RL_KEM_Q, (U+V)%RL_KEM_Q, (U-V)%RL_KEM_Q
        intt_core_finalextra(&tmp_U,&tmp_V,&tmp_S,&tmp_A1,&tmp_A2);
        //store
        _mm256_store_si256((__m256i *)(a +j), tmp_A1);
        _mm256_store_si256((__m256i *)(a  + 256 +j), tmp_A2);
    }
    
    // *1/N %RL_KEM_Q
    //intt_post(a);

    return 0;
}

int poly_mul_ntt_avx2_lazy(const uint16_t *a, const uint16_t *s, uint16_t *b)
{
    int16_t __attribute__((aligned(64))) a_buf[RL_KEM_N], s_buf[RL_KEM_N], b_buf[RL_KEM_N];

    memcpy(a_buf, a, RL_KEM_N * sizeof(a_buf[0]));
    memcpy(s_buf, s, RL_KEM_N * sizeof(s_buf[0]));

    //NTT form
    NTT_AVX2(a_buf);
    NTT_AVX2(s_buf);

    //point mul
    lazyincom_mul_mod_avx2(a_buf, s_buf, b_buf);

    //INTT form
    INTT_AVX2_Lazy(b_buf);
    memcpy(b, b_buf, RL_KEM_N * (sizeof(b_buf[0])));

    return 0;
}


static inline __m256i avx2_montgomery_reduce_i16(__m256i a)
{
    const __m256i factor = _mm256_set1_epi16(B_Q);
    const __m256i qinv = _mm256_set1_epi16(INVERSE_Q);
    const __m256i q = _mm256_set1_epi16(RL_KEM_Q);
    __m256i lo = _mm256_mullo_epi16(a, factor);
    __m256i hi = _mm256_mulhi_epi16(a, factor);
    __m256i x = _mm256_mullo_epi16(lo, qinv);
    x = _mm256_mulhi_epi16(x, q);
    __m256i r = _mm256_sub_epi16(hi, x);
    r = _mm256_add_epi16(r, _mm256_and_si256(_mm256_srai_epi16(r, 15), q));
    __m256i y = _mm256_sub_epi16(r, q);
    r = _mm256_add_epi16(y, _mm256_and_si256(_mm256_srai_epi16(y, 15), q));
    return r;
}

static inline __m256i avx2_montgomery_mul_i16(__m256i a, __m256i b)
{
    const __m256i qinv = _mm256_set1_epi16(INVERSE_Q);
    const __m256i q = _mm256_set1_epi16(RL_KEM_Q);
    __m256i lo = _mm256_mullo_epi16(a, b);
    __m256i hi = _mm256_mulhi_epi16(a, b);
    __m256i x = _mm256_mullo_epi16(lo, qinv);
    x = _mm256_mulhi_epi16(x, q);
    return _mm256_sub_epi16(hi, x);
}

static inline void avx2_basemul_product_block(__m256i *vr, const __m256i *va, const __m256i *vb, __m256i s, int base)
{
    for (int k = 0; k < base; k++) {
        __m256i lo = _mm256_setzero_si256();
        __m256i hi = _mm256_setzero_si256();

        for (int j = 0; j <= k; j++) {
            lo = _mm256_add_epi16(lo, avx2_montgomery_mul_i16(va[j], vb[k - j]));
        }
        for (int j = k + 1; j < base; j++) {
            hi = _mm256_add_epi16(hi, avx2_montgomery_mul_i16(va[j], vb[k + base - j]));
        }

        if (k + 1 < base) {
            hi = avx2_montgomery_mul_i16(hi, s);
            lo = _mm256_add_epi16(lo, hi);
        }
        vr[k] = lo;
    }
}

int incom_mul_acc_avx2(int16_t *acc, const int16_t *a, const int16_t *b)
{
    enum { BASE = 4, BLOCKS = RL_KEM_N / BASE };
    int16_t __attribute__((aligned(64))) a_part[BASE][BLOCKS];
    int16_t __attribute__((aligned(64))) b_part[BASE][BLOCKS];
    int16_t __attribute__((aligned(64))) acc_part[BASE][BLOCKS];

    for (int i = 0; i < RL_KEM_N; i += BASE) {
        int j = i / BASE;
        for (int k = 0; k < BASE; k++) {
            a_part[k][j] = a[i + k];
            b_part[k][j] = b[i + k];
            acc_part[k][j] = acc[i + k];
        }
    }

    for (int i = 0; i < BLOCKS; i += 16) {
        __m256i va[BASE];
        __m256i vb[BASE];
        __m256i vr[BASE];
        __m256i s = _mm256_load_si256((const __m256i *)(const void *)(poly_basemul_f2 + i));

        for (int k = 0; k < BASE; k++) {
            va[k] = _mm256_load_si256((const __m256i *)(const void *)(a_part[k] + i));
            vb[k] = _mm256_load_si256((const __m256i *)(const void *)(b_part[k] + i));
        }

        avx2_basemul_product_block(vr, va, vb, s, BASE);

        for (int k = 0; k < BASE; k++) {
            __m256i vacc = _mm256_load_si256((const __m256i *)(const void *)(acc_part[k] + i));
            vacc = _mm256_add_epi16(vacc, vr[k]);
            vacc = avx2_montgomery_reduce_i16(vacc);
            _mm256_store_si256((__m256i *)(void *)(acc_part[k] + i), vacc);
        }
    }

    for (int i = 0; i < RL_KEM_N; i += BASE) {
        int j = i / BASE;
        for (int k = 0; k < BASE; k++) {
            acc[i + k] = acc_part[k][j];
        }
    }
    return 0;
}

int incom_mul_add2_avx2(int16_t *out,
                        const int16_t *a0, const int16_t *b0,
                        const int16_t *a1, const int16_t *b1)
{
    enum { BASE = 4, BLOCKS = RL_KEM_N / BASE };
    int16_t __attribute__((aligned(64))) a0_part[BASE][BLOCKS];
    int16_t __attribute__((aligned(64))) b0_part[BASE][BLOCKS];
    int16_t __attribute__((aligned(64))) a1_part[BASE][BLOCKS];
    int16_t __attribute__((aligned(64))) b1_part[BASE][BLOCKS];
    int16_t __attribute__((aligned(64))) out_part[BASE][BLOCKS];

    for (int i = 0; i < RL_KEM_N; i += BASE) {
        int j = i / BASE;
        for (int k = 0; k < BASE; k++) {
            a0_part[k][j] = a0[i + k];
            b0_part[k][j] = b0[i + k];
            a1_part[k][j] = a1[i + k];
            b1_part[k][j] = b1[i + k];
        }
    }

    for (int i = 0; i < BLOCKS; i += 16) {
        __m256i va0[BASE];
        __m256i vb0[BASE];
        __m256i va1[BASE];
        __m256i vb1[BASE];
        __m256i vr0[BASE];
        __m256i vr1[BASE];
        __m256i s = _mm256_load_si256((const __m256i *)(const void *)(poly_basemul_f2 + i));

        for (int k = 0; k < BASE; k++) {
            va0[k] = _mm256_load_si256((const __m256i *)(const void *)(a0_part[k] + i));
            vb0[k] = _mm256_load_si256((const __m256i *)(const void *)(b0_part[k] + i));
            va1[k] = _mm256_load_si256((const __m256i *)(const void *)(a1_part[k] + i));
            vb1[k] = _mm256_load_si256((const __m256i *)(const void *)(b1_part[k] + i));
        }

        avx2_basemul_product_block(vr0, va0, vb0, s, BASE);
        avx2_basemul_product_block(vr1, va1, vb1, s, BASE);

        for (int k = 0; k < BASE; k++) {
            __m256i sum = avx2_montgomery_reduce_i16(vr0[k]);
            sum = _mm256_add_epi16(sum, vr1[k]);
            sum = avx2_montgomery_reduce_i16(sum);
            _mm256_store_si256((__m256i *)(void *)(out_part[k] + i), sum);
        }
    }

    for (int i = 0; i < RL_KEM_N; i += BASE) {
        int j = i / BASE;
        for (int k = 0; k < BASE; k++) {
            out[i + k] = out_part[k][j];
        }
    }
    return 0;
}
