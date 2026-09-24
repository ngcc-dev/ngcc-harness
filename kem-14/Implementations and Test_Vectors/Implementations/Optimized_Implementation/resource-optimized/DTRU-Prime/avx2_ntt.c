#include "avx2_ntt.h"
#include "radix_ntt_n1087.h"

#define q_vec      _mm256_set1_epi32(Q_N1087)
#define qinv_vec   _mm256_set1_epi32(QINV)
#define mask_vec   _mm256_set1_epi32(0xffffff)

__m256i qinvzeta_vec;
__m256i zetas_vec;
static __m256i t;

void butterfly(__m256i *l, __m256i *r, __m256i qinvzeta_0, __m256i qinvzeta_1, __m256i zetas_0, __m256i zetas_1){
    __m256i h = *r;
    __m256i l_val = *l;
    
    // vpmuldq		%ymm\zl0,%ymm\h,%ymm13 # he*zeta_beta*qinv 
    __m256i ymm13 = _mm256_mul_epi32(qinvzeta_0, h);
    // vmovshdup	%ymm\h,%ymm12 # ho 
    __m256i ymm12 = _mm256_castps_si256(_mm256_movehdup_ps(_mm256_castsi256_ps(h)));
    // vpmuldq		%ymm\zl1,%ymm12,%ymm14 # ho*zeta_beta*qinv 
    __m256i ymm14 = _mm256_mul_epi32(qinvzeta_1, ymm12);
    // vpmuldq		%ymm\zh0,%ymm\h,%ymm\h # he*zeta_beta 
    h = _mm256_mul_epi32(zetas_0, h);
    // vpmuldq		%ymm\zh1,%ymm12,%ymm12 # ho*zeta_beta 
    ymm12 = _mm256_mul_epi32(zetas_1, ymm12);
    // vpmuldq	    %ymm0,%ymm13,%ymm13 # (he*zeta_beta*qinv mod beta) * q 
    ymm13 = _mm256_mul_epi32(q_vec, ymm13);
    // vpmuldq	    %ymm0,%ymm14,%ymm14 # (ho*zeta_beta*qinv mod beta) * q 合起来是m*q 
    ymm14 = _mm256_mul_epi32(q_vec, ymm14);
    // vmovshdup	%ymm\h,%ymm\h # he*zeta_beta / beta 
    h = _mm256_castps_si256(_mm256_movehdup_ps(_mm256_castsi256_ps(h)));
    // vpblendd	$0xAA,%ymm12,%ymm\h,%ymm\h # h*zeta_beta / beta  c1 
    h = _mm256_blend_epi32(h, ymm12, 0xAA);
    // vpsubd		%ymm\h,%ymm\l,%ymm12 # l - c1 
    ymm12 = _mm256_sub_epi32(l_val, h);
    // vpaddd		%ymm\h,%ymm\l,%ymm\l # l + c1 
    l_val = _mm256_add_epi32(l_val, h);
    // vmovshdup	%ymm13,%ymm13 # (he*zeta_beta*qinv mod beta) * q 
    ymm13 = _mm256_castps_si256(_mm256_movehdup_ps(_mm256_castsi256_ps(ymm13)));
    // vpblendd	$0xAA,%ymm14,%ymm13,%ymm13 # h*zeta_beta mod beta /beta t1 
    ymm13 = _mm256_blend_epi32(ymm13, ymm14, 0xAA);
    // vpaddd		%ymm13,%ymm12,%ymm\h # l - (c1-t1)   (c1-t1)就是h*zeta约减后的值 
    *r = _mm256_add_epi32(ymm12, ymm13);
    // vpsubd		%ymm13,%ymm\l,%ymm\l # l + (c1-t1) 
    *l = _mm256_sub_epi32(l_val, ymm13);
}

void invbutterfly(__m256i *l, __m256i *r, __m256i qinvzeta_0, __m256i qinvzeta_1, __m256i zetas_0, __m256i zetas_1){
    __m256i h = *r;
    __m256i l_val = *l;
    __m256i l_copy = l_val; 
    
    // vpaddd		%ymm\h,%ymm3,%ymm\l # l = l + h 
    l_val = _mm256_add_epi32(l_copy, h);
    *l = l_val;
    
    // vpsubd		%ymm\h,%ymm3,%ymm\h # h = l - h 
    h = _mm256_sub_epi32(l_copy, h);
    // vpmuldq		%ymm\zl0,%ymm\h,%ymm13 # he*zeta_inv_beta*qinv 
    __m256i ymm13 = _mm256_mul_epi32(qinvzeta_0, h);
    // vmovshdup	%ymm\h,%ymm12 # ho 
    __m256i ymm12 = _mm256_castps_si256(_mm256_movehdup_ps(_mm256_castsi256_ps(h)));
    // vpmuldq		%ymm\zl1,%ymm12,%ymm14 # ho*zeta_inv_beta*qinv 
    __m256i ymm14 = _mm256_mul_epi32(qinvzeta_1, ymm12);
    // vpmuldq		%ymm\zh0,%ymm\h,%ymm\h # he*zeta_inv_beta 
    h = _mm256_mul_epi32(zetas_0, h);
    // vpmuldq		%ymm\zh1,%ymm12,%ymm12 # ho*zeta_inv_beta 
    ymm12 = _mm256_mul_epi32(zetas_1, ymm12);
    // vpmuldq	    %ymm0,%ymm13,%ymm13 # (he*zeta_inv_beta*qinv mod beta) * q 
    ymm13 = _mm256_mul_epi32(q_vec, ymm13);
    // vpmuldq	    %ymm0,%ymm14,%ymm14 # (ho*zeta_inv_beta*qinv mod beta) * q 合起来是m*q 
    ymm14 = _mm256_mul_epi32(q_vec, ymm14);
    // vmovshdup	%ymm\h,%ymm\h # he*zeta_inv_beta / beta 
    h = _mm256_castps_si256(_mm256_movehdup_ps(_mm256_castsi256_ps(h)));
    // vpblendd	$0xAA,%ymm12,%ymm\h,%ymm\h # h*zeta_inv_beta / beta  a1 
    h = _mm256_blend_epi32(h, ymm12, 0xAA);
    // vmovshdup	%ymm13,%ymm13 # (he*zeta_inv_beta*qinv mod beta) * q 
    ymm13 = _mm256_castps_si256(_mm256_movehdup_ps(_mm256_castsi256_ps(ymm13)));
    // vpblendd	$0xAA,%ymm14,%ymm13,%ymm13 # h*zeta_inv_beta mod beta /beta t1 
    ymm13 = _mm256_blend_epi32(ymm13, ymm14, 0xAA);
    // vpsubd		%ymm13,%ymm\h,%ymm\h # h = a1-t1  
    *r = _mm256_sub_epi32(h, ymm13);
}

__m256i fqmul_avx2(__m256i a, __m256i blqinv, __m256i bhqinv, __m256i bl, __m256i bh) {
    
    // vpmuldq %ymm\blqinv, %ymm\a, %ymm13
    __m256i ymm13 = _mm256_mul_epi32(blqinv, a);
    
    // vmovshdup %ymm\a, %ymm12
    __m256i ymm12 = _mm256_castps_si256(_mm256_movehdup_ps(_mm256_castsi256_ps(a)));
    
    // vpmuldq %ymm\bhqinv, %ymm12, %ymm14
    __m256i ymm14 = _mm256_mul_epi32(bhqinv, ymm12);
    
    // vpmuldq %ymm\bl, %ymm\a, %ymm\a
    a = _mm256_mul_epi32(bl, a);
    
    // vpmuldq %ymm\bh, %ymm12, %ymm12
    ymm12 = _mm256_mul_epi32(bh, ymm12);
    
    // vpmuldq %ymm0, %ymm13, %ymm13
    ymm13 = _mm256_mul_epi32(q_vec, ymm13);
    
    // vpmuldq %ymm0, %ymm14, %ymm14
    ymm14 = _mm256_mul_epi32(q_vec, ymm14);
    
    // vmovshdup %ymm\a, %ymm\a
    a = _mm256_castps_si256(_mm256_movehdup_ps(_mm256_castsi256_ps(a)));
    
    // vpblendd $0xAA, %ymm12, %ymm\a, %ymm\a
    a = _mm256_blend_epi32(a, ymm12, 0xAA);
    
    // vmovshdup %ymm13, %ymm13
    ymm13 = _mm256_castps_si256(_mm256_movehdup_ps(_mm256_castsi256_ps(ymm13)));
    
    // vpblendd $0xAA, %ymm14, %ymm13, %ymm13
    ymm13 = _mm256_blend_epi32(ymm13, ymm14, 0xAA);
    
    // vpsubd %ymm13, %ymm\a, %ymm\a
    a = _mm256_sub_epi32(a, ymm13);
    
    return a;
}

void butterfly3(__m256i *a0, __m256i *a1, __m256i *a2, __m256i qinvzeta_0, __m256i qinvzeta_1, __m256i zetas_0, __m256i zetas_1){
    __m256i z0 = *a1;
    __m256i z1 = *a2;

     __m256i d  = _mm256_sub_epi32(z0, z1);
    __m256i z2 = fqmul_avx2(d, qinvzeta_0, qinvzeta_1, zetas_0, zetas_1);

    *a2 = _mm256_add_epi32(_mm256_add_epi32(*a0, z0), z1);
    *a1 = _mm256_sub_epi32(_mm256_sub_epi32(*a0, z0), z2);
    *a0 = _mm256_add_epi32(_mm256_sub_epi32(*a0, z1), z2);
}

void invbutterfly3(__m256i *a0, __m256i *a1, __m256i *a2, __m256i qinvzeta_0, __m256i qinvzeta_1, __m256i zetas_0, __m256i zetas_1){
    __m256i z0 = *a0;
    __m256i z1 = *a1;

    __m256i d  = _mm256_sub_epi32(z0, z1);
    __m256i z2 = fqmul_avx2(d, qinvzeta_0, qinvzeta_1, zetas_0, zetas_1);

    __m256i t0 = _mm256_add_epi32(_mm256_add_epi32(*a2, z0), z1);
    __m256i t1 = _mm256_add_epi32(_mm256_sub_epi32(*a2, z1), z2);
    __m256i t2 = _mm256_sub_epi32(_mm256_sub_epi32(*a2, z0), z2);

    __m256i scl_blqinv = _mm256_set1_epi32(tree_inv_qinv[766]);
    __m256i scl_bl     = _mm256_set1_epi32(tree_inv[766]);
    __m256i scl_bhqinv = _mm256_castps_si256(_mm256_movehdup_ps(_mm256_castsi256_ps(scl_blqinv)));
    __m256i scl_bh     = _mm256_castps_si256(_mm256_movehdup_ps(_mm256_castsi256_ps(scl_bl)));

    *a0 = fqmul_avx2(t0, scl_blqinv, scl_bhqinv, scl_bl, scl_bh);
    *a1 = fqmul_avx2(t1, scl_blqinv, scl_bhqinv, scl_bl, scl_bh);
    *a2 = fqmul_avx2(t2, scl_blqinv, scl_bhqinv, scl_bl, scl_bh);
}

static inline __m256i pseudomersenne_reduce_avx2(__m256i a) {
    __m256i t0 = _mm256_and_si256(a, mask_vec);
    __m256i t1 = _mm256_srai_epi32(a, 24);
    __m256i t1l = _mm256_slli_epi32(t1, 8);
    __m256i t = _mm256_add_epi32(t1l, t0);
    t = _mm256_sub_epi32(t, t1);
    return _mm256_sub_epi32(t, q_vec);
}

static inline void ntt_stage_biglen(__m256i* f, int L, int* k){
    int stride = L / 8;
    int Vecs = N_N1087 / 8;
    if (stride <= 0) return;
    for (int i = 0; i < Vecs / (2 * stride); i++) {
        qinvzeta_vec = _mm256_set1_epi32(tree_qinv[*k]);
        zetas_vec    = _mm256_set1_epi32(tree[(*k)++]);
        for (int j = 0; j < stride; j++) {
            butterfly(&f[j + i * 2 * stride], &f[j + stride + i * 2 * stride], qinvzeta_vec, qinvzeta_vec, zetas_vec, zetas_vec);
        }
    }
}
static inline void invntt_stage_biglen(__m256i* f, int L, int* k){
    int stride = L / 8;
    int Vecs = N_N1087 / 8;
    if (stride <= 0) return;
    for (int i = 0; i < Vecs / (2 * stride); i++) {
        qinvzeta_vec = _mm256_set1_epi32(tree_inv_qinv[*k]);
        zetas_vec    = _mm256_set1_epi32(tree_inv[(*k)++]);
        for (int j = 0; j < stride; j++) {
            invbutterfly(&f[j + i * 2 * stride], &f[j + stride + i * 2 * stride], qinvzeta_vec, qinvzeta_vec, zetas_vec, zetas_vec);
        }
    }
}
//ntt
void ntt_avx2_intrinsic(__m256i* f){
    int k = 0;

    //level0:间隔768
    {
        int stride = 768/8;
        int Vecs = N_N1087/8;
        for (int i = 0; i < Vecs/(3*stride); i++){
            qinvzeta_vec = _mm256_set1_epi32(tree_qinv[k]);
            zetas_vec    = _mm256_set1_epi32(tree[k++]);
            for (int j = 0; j < stride; j++){
                butterfly3(&f[j + i*3*stride], &f[j + i*3*stride + stride], &f[j + i*3*stride + 2*stride], qinvzeta_vec, qinvzeta_vec, zetas_vec, zetas_vec);
            }
        }
    }

    //level1:间隔384
    ntt_stage_biglen(f, 384, &k);

    //level2:间隔192
    ntt_stage_biglen(f, 192, &k);

    //level3:间隔96
    ntt_stage_biglen(f, 96, &k);

    for (int i=0;i<N_N1087/8;i++){
        f[i] = pseudomersenne_reduce_avx2(f[i]);
    }

    //level4:间隔48
    ntt_stage_biglen(f, 48, &k);

    //level5:间隔24
    ntt_stage_biglen(f, 24, &k);
    
    /**
    input:
    f[0] = a00, a01, a02, a03, a04, a05, a06, a07;
    f[1] = a08, a09, a10, a11, a12, a13, a14, a15;
    f[2] = a16, a17, a18, a19, a20, a21, a22, a23;
    f[3] = a24, a25, a26, a27, a28, a29, a30, a31;
    f[4] = a32, a33, a34, a35, a36, a37, a38, a39;
    f[5] = a40, a41, a42, a43, a44, a45, a46, a47;
    output:
    f[0] = a00, a01, a02, a03, a24, a25, a26, a27;
    f[1] = a08, a09, a10, a11, a32, a33, a34, a35;
    f[2] = a16, a17, a18, a19, a40, a41, a42, a43;
    f[3] = a04, a05, a06, a07, a28, a29, a30, a31;
    f[4] = a12, a13, a14, a15, a36, a37, a38, a39;
    f[5] = a20, a21, a22, a23, a44, a45, a46, a47;
    
    */
    for (int i = 0; i < N_N1087/(24*2); i++) {
        for (int j = 0;j < 3; j++){
            shuffle8(f[j+i*2*3], f[j+i*2*3+3]);
        }
    }
    //level6:间隔12,分32批处理,每一批处理6个寄存器48个系数
    for (int i = 0; i < N_N1087/(6*8); i++) {
        qinvzeta_vec = _mm256_set_epi32(
             tree_qinv[k+1],   tree_qinv[k+1],   tree_qinv[k+1],   tree_qinv[k+1],   tree_qinv[k],   tree_qinv[k],   tree_qinv[k],   tree_qinv[k]
        );
        zetas_vec = _mm256_set_epi32(
            tree[k+1], tree[k+1], tree[k+1], tree[k+1], tree[k], tree[k], tree[k], tree[k]
        );
        butterfly(&f[0+i*6], &f[4+i*6], qinvzeta_vec, qinvzeta_vec, zetas_vec, zetas_vec);
        butterfly(&f[3+i*6], &f[2+i*6], qinvzeta_vec, qinvzeta_vec, zetas_vec, zetas_vec);
        butterfly(&f[1+i*6], &f[5+i*6], qinvzeta_vec, qinvzeta_vec, zetas_vec, zetas_vec);

        k += 2;
    }

    /**
    input:
    f[0] = a00, a01, a02, a03, a24, a25, a26, a27;
    f[1] = a08, a09, a10, a11, a32, a33, a34, a35;
    f[2] = a16, a17, a18, a19, a40, a41, a42, a43;
    f[3] = a04, a05, a06, a07, a28, a29, a30, a31;
    f[4] = a12, a13, a14, a15, a36, a37, a38, a39;
    f[5] = a20, a21, a22, a23, a44, a45, a46, a47;
    output:
    f[0] = a00, a01, a12, a13, a24, a25, a36, a37;
    f[1] = a08, a09, a20, a21, a32, a33, a44, a45;
    f[2] = a06, a07, a18, a19, a30, a31, a42, a43;
    f[3] = a04, a05, a16, a17, a28, a29, a40, a41;
    f[4] = a02, a03, a14, a15, a26, a27, a38, a39;
    f[5] = a10, a11, a22, a23, a34, a35, a46, a47;
    
    */
    for (int i = 0; i < N_N1087/(24*2); i++) {
        shuffle4(f[0+i*2*3], f[4+i*2*3]);
        shuffle4(f[3+i*2*3], f[2+i*2*3]);
        shuffle4(f[1+i*2*3], f[5+i*2*3]);
    }
    //level7:间隔6,分32批处理,每一批处理6个寄存器48个系数
    for (int i = 0; i < N_N1087/(6*8); i++) {
        qinvzeta_vec = _mm256_set_epi32(
             tree_qinv[k+3], tree_qinv[k+3], tree_qinv[k+2], tree_qinv[k+2], tree_qinv[k+1], tree_qinv[k+1], tree_qinv[k],   tree_qinv[k]
        );
        zetas_vec = _mm256_set_epi32(
            tree[k+3], tree[k+3], tree[k+2], tree[k+2], tree[k+1], tree[k+1], tree[k],   tree[k]
        );
        butterfly(&f[0+i*6], &f[2+i*6], qinvzeta_vec, qinvzeta_vec, zetas_vec, zetas_vec);
        butterfly(&f[4+i*6], &f[1+i*6], qinvzeta_vec, qinvzeta_vec, zetas_vec, zetas_vec);
        butterfly(&f[3+i*6], &f[5+i*6], qinvzeta_vec, qinvzeta_vec, zetas_vec, zetas_vec);

        k += 4;
    }

    /**
    input:
    f[0] = a00, a01, a12, a13, a24, a25, a36, a37;
    f[1] = a08, a09, a20, a21, a32, a33, a44, a45;
    f[2] = a06, a07, a18, a19, a30, a31, a42, a43;
    f[3] = a04, a05, a16, a17, a28, a29, a40, a41;
    f[4] = a02, a03, a14, a15, a26, a27, a38, a39;
    f[5] = a10, a11, a22, a23, a34, a35, a46, a47;
    output:
    f[0] = a00, a06, a12, a18, a24, a30, a36, a42;
    f[1] = a03, a09, a15, a21, a27, a33, a39, a45;
    f[2] = a01, a07, a13, a19, a25, a31, a37, a43;
    f[3] = a04, a10, a16, a22, a28, a34, a40, a46;
    f[4] = a02, a08, a14, a20, a26, a32, a38, a44;
    f[5] = a05, a11, a17, a23, a29, a35, a41, a47;
    
    */
    for (int i = 0; i < N_N1087/(24*2); i++) {
        shuffle2(f[0+i*2*3], f[2+i*2*3]);
        shuffle2(f[4+i*2*3], f[1+i*2*3]);
        shuffle2(f[3+i*2*3], f[5+i*2*3]);
    }
    //level8:间隔3,分32批处理,每一批处理6个寄存器48个系数
    for (int i = 0; i < N_N1087/(6*8); i++) {
        qinvzeta_vec = _mm256_set_epi32(
             tree_qinv[k+7], tree_qinv[k+6], tree_qinv[k+5], tree_qinv[k+4], tree_qinv[k+3], tree_qinv[k+2], tree_qinv[k+1], tree_qinv[k]
        );
        zetas_vec = _mm256_set_epi32(
            tree[k+7], tree[k+6], tree[k+5], tree[k+4], tree[k+3], tree[k+2], tree[k+1], tree[k]
        );
        __m256i qinvzeta_vec_1 = _mm256_castps_si256(_mm256_movehdup_ps(_mm256_castsi256_ps(qinvzeta_vec)));
        __m256i zetas_vec_1 = _mm256_castps_si256(_mm256_movehdup_ps(_mm256_castsi256_ps(zetas_vec)));
        
        butterfly(&f[0+i*6], &f[1+i*6], qinvzeta_vec, qinvzeta_vec_1, zetas_vec, zetas_vec_1);
        butterfly(&f[2+i*6], &f[3+i*6], qinvzeta_vec, qinvzeta_vec_1, zetas_vec, zetas_vec_1);
        butterfly(&f[4+i*6], &f[5+i*6], qinvzeta_vec, qinvzeta_vec_1, zetas_vec, zetas_vec_1);

        k += 8;
    }
}

void invntt_avx2_intrinsic(__m256i* f){
    int k = 0;
    /**
    input:
    f[0] = a00, a06, a12, a18, a24, a30, a36, a42;
    f[1] = a03, a09, a15, a21, a27, a33, a39, a45;
    f[2] = a01, a07, a13, a19, a25, a31, a37, a43;
    f[3] = a04, a10, a16, a22, a28, a34, a40, a46;
    f[4] = a02, a08, a14, a20, a26, a32, a38, a44;
    f[5] = a05, a11, a17, a23, a29, a35, a41, a47;   
    
    */
    //level8:间隔3,分32批处理,每一批处理6个寄存器48个系数
    for (int i = 0; i < N_N1087/(6*8); i++) {
        qinvzeta_vec = _mm256_set_epi32(
             tree_inv_qinv[k+7], tree_inv_qinv[k+6], tree_inv_qinv[k+5], tree_inv_qinv[k+4], tree_inv_qinv[k+3], tree_inv_qinv[k+2], tree_inv_qinv[k+1], tree_inv_qinv[k]
        );
        zetas_vec = _mm256_set_epi32(
            tree_inv[k+7], tree_inv[k+6], tree_inv[k+5], tree_inv[k+4], tree_inv[k+3], tree_inv[k+2], tree_inv[k+1], tree_inv[k]
        );
        __m256i qinvzeta_vec_1 = _mm256_castps_si256(_mm256_movehdup_ps(_mm256_castsi256_ps(qinvzeta_vec)));
        __m256i zetas_vec_1 = _mm256_castps_si256(_mm256_movehdup_ps(_mm256_castsi256_ps(zetas_vec)));
        
        invbutterfly(&f[0+i*6], &f[1+i*6], qinvzeta_vec, qinvzeta_vec_1, zetas_vec, zetas_vec_1);
        invbutterfly(&f[2+i*6], &f[3+i*6], qinvzeta_vec, qinvzeta_vec_1, zetas_vec, zetas_vec_1);
        invbutterfly(&f[4+i*6], &f[5+i*6], qinvzeta_vec, qinvzeta_vec_1, zetas_vec, zetas_vec_1);

        k += 8;
    }
    for (int i = 0; i < N_N1087/(24*2); i++) {
        shuffle2(f[0+i*2*3], f[2+i*2*3]);
        shuffle2(f[4+i*2*3], f[1+i*2*3]);
        shuffle2(f[3+i*2*3], f[5+i*2*3]);
    }
    
    //level7:间隔6,分32批处理,每一批处理6个寄存器48个系数
    for (int i = 0; i < N_N1087/(6*8); i++) {
        qinvzeta_vec = _mm256_set_epi32(
             tree_inv_qinv[k+3], tree_inv_qinv[k+3], tree_inv_qinv[k+2], tree_inv_qinv[k+2], tree_inv_qinv[k+1], tree_inv_qinv[k+1], tree_inv_qinv[k],   tree_inv_qinv[k]
        );
        zetas_vec = _mm256_set_epi32(
            tree_inv[k+3], tree_inv[k+3], tree_inv[k+2], tree_inv[k+2], tree_inv[k+1], tree_inv[k+1], tree_inv[k],   tree_inv[k]
        );
        invbutterfly(&f[0+i*6], &f[2+i*6], qinvzeta_vec, qinvzeta_vec, zetas_vec, zetas_vec);
        invbutterfly(&f[4+i*6], &f[1+i*6], qinvzeta_vec, qinvzeta_vec, zetas_vec, zetas_vec);
        invbutterfly(&f[3+i*6], &f[5+i*6], qinvzeta_vec, qinvzeta_vec, zetas_vec, zetas_vec);

        k += 4;
    }
    for (int i = 0; i < N_N1087/(24*2); i++) {
        shuffle4(f[0+i*2*3], f[4+i*2*3]);
        shuffle4(f[3+i*2*3], f[2+i*2*3]);
        shuffle4(f[1+i*2*3], f[5+i*2*3]);
    }
    
    //level6:间隔12,分32批处理,每一批处理6个寄存器48个系数
    for (int i = 0; i < N_N1087/(6*8); i++) {
        qinvzeta_vec = _mm256_set_epi32(
             tree_inv_qinv[k+1],   tree_inv_qinv[k+1],   tree_inv_qinv[k+1],   tree_inv_qinv[k+1],   tree_inv_qinv[k],   tree_inv_qinv[k],   tree_inv_qinv[k],   tree_inv_qinv[k]
        );
        zetas_vec = _mm256_set_epi32(
            tree_inv[k+1], tree_inv[k+1], tree_inv[k+1], tree_inv[k+1], tree_inv[k], tree_inv[k], tree_inv[k], tree_inv[k]
        );
        invbutterfly(&f[0+i*6], &f[4+i*6], qinvzeta_vec, qinvzeta_vec, zetas_vec, zetas_vec);
        invbutterfly(&f[3+i*6], &f[2+i*6], qinvzeta_vec, qinvzeta_vec, zetas_vec, zetas_vec);
        invbutterfly(&f[1+i*6], &f[5+i*6], qinvzeta_vec, qinvzeta_vec, zetas_vec, zetas_vec);

        k += 2;
    }
    for (int i = 0; i < N_N1087/(24*2); i++) {
        for (int j = 0;j < 3; j++){
            shuffle8(f[j+i*2*3], f[j+i*2*3+3]);
        }
    }

    //level5:间隔24
    invntt_stage_biglen(f, 24, &k);

    //level4:间隔48
    invntt_stage_biglen(f, 48, &k);

    //level3:间隔96
    invntt_stage_biglen(f, 96, &k);

    for (int i=0;i<N_N1087/8;i++){
        f[i] = pseudomersenne_reduce_avx2(f[i]);
    }

    //level2:间隔192
    invntt_stage_biglen(f, 192, &k);

    //level1:间隔384
    invntt_stage_biglen(f, 384, &k);

    //level0:间隔768
    {
        int stride = 768/8;
        int Vecs = N_N1087/8;
        for (int i = 0; i < Vecs/(3*stride); i++){
            qinvzeta_vec = _mm256_set1_epi32(tree_inv_qinv[k]);
            zetas_vec    = _mm256_set1_epi32(tree_inv[k++]);
            for (int j = 0; j < stride; j++){
                invbutterfly3(&f[j + i*3*stride], &f[j + i*3*stride + stride], &f[j + i*3*stride + 2*stride], qinvzeta_vec, qinvzeta_vec, zetas_vec, zetas_vec);
            }
        }
    }
}

void basemul3x3_avx2_intrinsic(__m256i* c, const __m256i* a, const __m256i* b) {
    /**
    input:
    f[0] = a00, a06, a12, a18, a24, a30, a36, a42;
    f[1] = a03, a09, a15, a21, a27, a33, a39, a45;
    f[2] = a01, a07, a13, a19, a25, a31, a37, a43;
    f[3] = a04, a10, a16, a22, a28, a34, a40, a46;
    f[4] = a02, a08, a14, a20, a26, a32, a38, a44;
    f[5] = a05, a11, a17, a23, a29, a35, a41, a47;   
    
    */
    int k = 382;
    for (int i = 0; i < N_N1087/(6*8); i++) {
        // Load zetas
        qinvzeta_vec = _mm256_set_epi32(
             tree_qinv[k+7], tree_qinv[k+6], tree_qinv[k+5], tree_qinv[k+4], tree_qinv[k+3], tree_qinv[k+2], tree_qinv[k+1], tree_qinv[k]
        );
        zetas_vec = _mm256_set_epi32(
            tree[k+7], tree[k+6], tree[k+5], tree[k+4], tree[k+3], tree[k+2], tree[k+1], tree[k]
        );

        k += 8;
        
        // Prepare zeta params for fqmul
        // In assembly: fqmul 3,7,8,10,9 # a2*b2*zeta
        // a=a2*b2, blqinv=qinvzeta_vec, bhqinv=qinvzeta_vec>>32, bl=zeta, bh=zeta_dup
        __m256i z_blqinv = qinvzeta_vec;
        __m256i z_bhqinv = _mm256_srli_epi64(qinvzeta_vec, 32);
        __m256i z_bl = zetas_vec;
        __m256i z_bh = _mm256_castps_si256(_mm256_movehdup_ps(_mm256_castsi256_ps(zetas_vec)));

        // Block 1: Positive
        {
            __m256i a0 = a[0]; __m256i a1 = a[2]; __m256i a2 = a[4];
            __m256i b0 = b[0]; __m256i b1 = b[2]; __m256i b2 = b[4];
            
            // Precompute b params
            // b0
            __m256i b0_qinv = _mm256_mullo_epi32(b0, qinv_vec);
            __m256i b0_h_qinv = _mm256_srli_epi64(b0_qinv, 32);
            __m256i b0_h = _mm256_castps_si256(_mm256_movehdup_ps(_mm256_castsi256_ps(b0)));
            
            // b1
            __m256i b1_qinv = _mm256_mullo_epi32(b1, qinv_vec);
            __m256i b1_h_qinv = _mm256_srli_epi64(b1_qinv, 32);
            __m256i b1_h = _mm256_castps_si256(_mm256_movehdup_ps(_mm256_castsi256_ps(b1)));
            
            // b2
            __m256i b2_qinv = _mm256_mullo_epi32(b2, qinv_vec);
            __m256i b2_h_qinv = _mm256_srli_epi64(b2_qinv, 32);
            __m256i b2_h = _mm256_castps_si256(_mm256_movehdup_ps(_mm256_castsi256_ps(b2)));
            
            // c2 = a0*b2 + a2*b0 + a1*b1
            __m256i c2 = fqmul_avx2(a0, b2_qinv, b2_h_qinv, b2, b2_h);
            c2 = _mm256_add_epi32(c2, fqmul_avx2(a2, b0_qinv, b0_h_qinv, b0, b0_h));
            c2 = _mm256_add_epi32(c2, fqmul_avx2(a1, b1_qinv, b1_h_qinv, b1, b1_h));
            c[4] = c2;
            
            // c1 = a0*b1 + a1*b0 + a2*b2*zeta
            __m256i c1 = fqmul_avx2(a0, b1_qinv, b1_h_qinv, b1, b1_h);
            // a2*b2
            __m256i t = fqmul_avx2(a2, b2_qinv, b2_h_qinv, b2, b2_h);
            // * zeta
            t = fqmul_avx2(t, z_blqinv, z_bhqinv, z_bl, z_bh);
            
            c1 = _mm256_add_epi32(c1, t); // Note: Assembly order: a0*b1 + a2*b2*zeta + a1*b0 (Lines 95-96)
            c1 = _mm256_add_epi32(c1, fqmul_avx2(a1, b0_qinv, b0_h_qinv, b0, b0_h));
            c[2] = c1;
            
            // c0 = a0*b0 + (a1*b2 + a2*b1)*zeta
            __m256i c0 = fqmul_avx2(a0, b0_qinv, b0_h_qinv, b0, b0_h);
            __m256i t_sum = _mm256_add_epi32(fqmul_avx2(a1, b2_qinv, b2_h_qinv, b2, b2_h), 
                                             fqmul_avx2(a2, b1_qinv, b1_h_qinv, b1, b1_h));
            c0 = _mm256_add_epi32(c0, fqmul_avx2(t_sum, z_blqinv, z_bhqinv, z_bl, z_bh));
            c[0] = c0;
            
        }
        
        // Block 2: Negative
        {
             __m256i a0 = a[1]; __m256i a1 = a[3]; __m256i a2 = a[5];
            __m256i b0 = b[1]; __m256i b1 = b[3]; __m256i b2 = b[5];
            
            // Precompute b params (Same as above, maybe macro?)
             // b0
            __m256i b0_qinv = _mm256_mullo_epi32(b0, qinv_vec);
            __m256i b0_h_qinv = _mm256_srli_epi64(b0_qinv, 32);
            __m256i b0_h = _mm256_castps_si256(_mm256_movehdup_ps(_mm256_castsi256_ps(b0)));
            
            // b1
            __m256i b1_qinv = _mm256_mullo_epi32(b1, qinv_vec);
            __m256i b1_h_qinv = _mm256_srli_epi64(b1_qinv, 32);
            __m256i b1_h = _mm256_castps_si256(_mm256_movehdup_ps(_mm256_castsi256_ps(b1)));
            
            // b2
            __m256i b2_qinv = _mm256_mullo_epi32(b2, qinv_vec);
            __m256i b2_h_qinv = _mm256_srli_epi64(b2_qinv, 32);
            __m256i b2_h = _mm256_castps_si256(_mm256_movehdup_ps(_mm256_castsi256_ps(b2)));
            
            // c2 = same
            __m256i c2 = fqmul_avx2(a0, b2_qinv, b2_h_qinv, b2, b2_h);
            c2 = _mm256_add_epi32(c2, fqmul_avx2(a2, b0_qinv, b0_h_qinv, b0, b0_h));
            c2 = _mm256_add_epi32(c2, fqmul_avx2(a1, b1_qinv, b1_h_qinv, b1, b1_h));
            c[5] = c2;
            
            // c1 = a0*b1 + a1*b0 - a2*b2*zeta
             __m256i c1 = fqmul_avx2(a0, b1_qinv, b1_h_qinv, b1, b1_h);
            __m256i t = fqmul_avx2(a2, b2_qinv, b2_h_qinv, b2, b2_h);
            t = fqmul_avx2(t, z_blqinv, z_bhqinv, z_bl, z_bh);
            
            c1 = _mm256_sub_epi32(c1, t); // SUB
            c1 = _mm256_add_epi32(c1, fqmul_avx2(a1, b0_qinv, b0_h_qinv, b0, b0_h));
            c[3] = c1;
            
             // c0 = a0*b0 - (a1*b2 + a2*b1)*zeta
            __m256i c0 = fqmul_avx2(a0, b0_qinv, b0_h_qinv, b0, b0_h);
            __m256i t_sum = _mm256_add_epi32(fqmul_avx2(a1, b2_qinv, b2_h_qinv, b2, b2_h), 
                                             fqmul_avx2(a2, b1_qinv, b1_h_qinv, b1, b1_h));
            c0 = _mm256_sub_epi32(c0, fqmul_avx2(t_sum, z_blqinv, z_bhqinv, z_bl, z_bh)); // SUB
            c[1] = c0;
            
            a += 6; b += 6; c += 6;
        }
    }
}

void poly_radix_ntt_n1087_q1_intrinsic(poly *c, const poly *a, const poly *b){
    nttpoly_n1087 ntta, nttb, nttc;

    poly_extend(&ntta, a);
    poly_extend(&nttb, b);

    ntt_avx2_intrinsic(ntta.vec);
    ntt_avx2_intrinsic(nttb.vec);
    basemul3x3_avx2_intrinsic(nttc.vec, ntta.vec, nttb.vec);
    invntt_avx2_intrinsic(nttc.vec);

    poly_extract(c, &nttc);
}
