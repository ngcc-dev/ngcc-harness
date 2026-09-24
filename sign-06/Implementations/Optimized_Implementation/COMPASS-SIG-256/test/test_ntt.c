#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../params.h"
#include "../reduce.h"
//#include "../ntt_avx.h"
#include "../consts.h"  // 必须包含这个头文件以获取 cdecl 宏
#include "../poly.h"
// ==========================================
// 1. 声明 AVX2 汇编函数接口
// ==========================================
extern void cdecl(ntt_avx)(int32_t a[N], const qdata_t *qd);
extern void cdecl(invntt_avx)(int32_t a[N], const qdata_t *qd);
// Pointwise 乘法接收 4 个参数：结果 c, 输入 a, 输入 b, 常量表 qd
extern void cdecl(pointwise_avx)(int32_t c[N], const int32_t a[N], const int32_t b[N], const qdata_t *qd);

// ==========================================
// 2. 纯净的 C 语言参考版逻辑 (重命名以防冲突)
// ==========================================
#if Q == 2081281
static const int32_t ref_zetas[256] = {
    -796688, 601032, 584045, -327734, -887302, -490228, -429181, 629140,
    15867, 277172, -1032567, -275828, 759342, 471482, -503358, 209076,
    -776887, 472088, -109937, -712219, -173831, 272608, 448628, -542721,
    -464828, 626878, 678923, -619423, -275787, -961950, -905903, 249158,
    -316123, 636139, 1034702, -840783, -72858, -370002, 45172, -95397,
    478263, 189158, -390792, -825093, -691900, 112686, 208961, -196871,
    -160014, 374142, 210960, -78396, 51146, -296274, -659997, -41355,
    408984, -576765, 694087, -966966, -65561, 299855, 20880, 631542,
    698470, -829014, 827349, 423834, -463518, 911709, -296056, 968577,
    536332, 837306, -266081, 694022, -335759, -763974, 25061, 951178,
    167044, -370835, -581481, -185607, 355725, -894027, -341385, 71813,
    -277035, -1014051, 702923, -898269, 641628, 510254, -348790, 137903,
    967283, -868649, -76382, 903754, -653755, 629486, -464800, 127739,
    520108, 159993, -164153, 346438, -820594, -647140, -170937, 863715,
    -650805, 222315, 421839, -125152, -992176, -996082, 543734, 155712,
    673613, 529741, -552816, -744993, 326270, -77443, -590446, 335677,
    -784411, -982723, 984131, 875355, 661976, 219032, 890368, 988454,
    541468, -629313, 477514, 533120, -262325, -850221, -442449, 775664,
    -676268, 44841, -37031, -900558, -750824, 954783, 776198, -305732,
    271774, -271239, -408171, 875869, 593677, 714773, 244952, -380371,
    -706727, 919572, 594600, 688266, 615346, 883949, 118052, 625739,
    840192, 494712, -684947, 968305, -27329, -65258, 605809, 600873,
    925356, -1013012, -604230, 760844, 616526, -527688, -776061, -724186,
    497878, -637976, -905034, -450016, 986610, -427018, -379455, 373646,
    -675186, -660450, 282526, -462763, 125363, 5386, -205702, 748182,
    -805697, 420048, 360472, -1001039, -571165, -335296, -447961, 25231,
    -948451, -346392, 721616, 587553, 923013, -945725, -555643, -77530,
    831662, 768993, -341948, -967316, 701701, 221436, 994858, -282807,
    918225, -925477, -427040, 743537, 118249, -581785, 313130, -726068,
    206827, -956373, -1025385, -306200, -711796, -517801, -638948, -118198,
    312837, -483143, 753184, 384505, -222128, 252553, -279348, -1035567,
    74083, -134199, -647080, 642988, -956402, 87144, -32003, 450051
};
#endif

void ref_ntt(int32_t a[N]) {
    unsigned int len, start, j, k = 0;
    int32_t zeta, t;
    for(len = (N>>1); len > 0; len >>= 1) {
        for(start = 0; start < N; start = j + len) {
            zeta = ref_zetas[++k];
            for(j = start; j < start + len; ++j) {
                t = montgomery_reduce((int64_t)zeta * a[j + len]);
                a[j + len] = a[j] - t;
                a[j] = a[j] + t;
            }
        }
    }
}

void ref_invntt_tomont(int32_t a[N]) {
    unsigned int start, len, j, k = N;
    int32_t t, zeta;
    const int32_t f = 537178; // mont^2/256 for Q=2081281
    for(len = 1; len < N; len <<= 1) {
        for(start = 0; start < N; start = j + len) {
            zeta = -ref_zetas[--k];
            for(j = start; j < start + len; ++j) {
                t = a[j];
                a[j] = t + a[j + len];
                a[j + len] = t - a[j + len];
                a[j + len] = montgomery_reduce((int64_t)zeta * a[j + len]);
            }
        }
    }
    for(j = 0; j < N; ++j) {
        a[j] = montgomery_reduce((int64_t)f * a[j]);
    }
}

void ref_pointwise(int32_t c[N], const int32_t a[N], const int32_t b[N]) {
    for(int i = 0; i < N; ++i)
        c[i] = montgomery_reduce((int64_t)a[i] * b[i]);
}

// ==========================================
// 3. 测试框架核心
// ==========================================
extern void cdecl(nttunpack_avx)(int32_t a[N]);

int main() {
    poly p_a_ref, p_b_ref, p_c_ref;
    poly p_a_avx, p_b_avx, p_c_avx;

    srand(time(NULL));
    printf("========================================\n");
    printf("  AVX2 真实业务级算术正确性验证 (Q=%d, N=%d)\n", Q, N);
    printf("========================================\n");

    int n_tests = 10000;
    int err = 0;

    for (int trial = 0; trial < n_tests; trial++) {
        // 1. 生成线性随机数据
        for (int i = 0; i < N; i++) {
            int32_t val_a = (rand() % (4 * Q)) - (2 * Q);
            int32_t val_b = (rand() % (4 * Q)) - (2 * Q);
            p_a_ref.coeffs[i] = val_a; p_a_avx.coeffs[i] = val_a;
            p_b_ref.coeffs[i] = val_b; p_b_avx.coeffs[i] = val_b;
        }

        // 2. REF 侧参考计算 (如果你有重命名的 ref 函数，请用 ref 函数)
        ref_ntt(p_a_ref.coeffs);
        ref_ntt(p_b_ref.coeffs);
        ref_pointwise(p_c_ref.coeffs, p_a_ref.coeffs, p_b_ref.coeffs);
        ref_invntt_tomont(p_c_ref.coeffs);

        // 3. AVX 侧实际业务函数计算
        // 这里的 poly_ntt, poly_pointwise_montgomery, poly_invntt_tomont 
        // 会在内部自动处理所有汇编需要的 unpack 和格式转换逻辑
        poly_ntt(&p_a_avx);
        poly_ntt(&p_b_avx);
        poly_pointwise_montgomery(&p_c_avx, &p_a_avx, &p_b_avx);
        poly_invntt_tomont(&p_c_avx);

        // 4. 比较
        for (int i = 0; i < N; i++) {
            if (reduce32(p_c_ref.coeffs[i]) != reduce32(p_c_avx.coeffs[i])) {
                err++;
                if(err == 1) printf("[业务级流水线错误] Trial %d, Index %d: Ref=%d, AVX=%d\n", trial, i, reduce32(p_c_ref.coeffs[i]), reduce32(p_c_avx.coeffs[i]));
            }
        }
    }

    if (err == 0) printf("  [✅] 真实业务级算术 100%% 匹配！\n");
    else printf("  [❌] 失败，共 %d 个错误\n", err);

    return 0;
}
