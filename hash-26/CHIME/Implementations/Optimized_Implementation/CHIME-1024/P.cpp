#include "P.hpp"

/*
 * Holds the runtime-expanded 256-bit round-constant table.
 * 保存运行时展开后的 256 位轮常量表。
 */
__m256i ARC[20];

/*
 * Initializes the global round-constant table from the 64-bit constants.
 * Inputs:
 *     none
 * Outputs:
 *     ARC: populated in place
 *
 * 根据 64 位常量初始化全局轮常量表。
 * 输入：
 *     无
 * 输出：
 *     ARC：原地填充完成
 */
void init_arc() {
    for (int i = 0; i < 20; i++) {
        ARC[i] = _mm256_set1_epi64x((long long)ARC64[i]);
    }
}

/*
 * Executes the 15-round permutation schedule used by CHIME-1024.
 * Inputs:
 *     s: state to permute
 * Outputs:
 *     s: updated state after 15 rounds
 *
 * 执行 CHIME-1024 使用的 15 轮置换调度。
 * 输入：
 *     s：待置换状态
 * 输出：
 *     s：15 轮后的更新状态
 */
void P_15(S &s) {
    rp(s, 0);   RP_fwd(s);
    rp(s, 1);   RP_fwd(s);
    rp(s, 2);   RP_fwd(s);
    rp(s, 3);   RP_rev(s);
    rp(s, 4);   RP_rev(s);
    rp(s, 5);   RP_rev(s);
    rp(s, 6);   RP_fwd(s);
    rp(s, 7);   RP_fwd(s);
    rp(s, 8);   RP_fwd(s);
    rp(s, 9);   RP_rev(s);
    rp(s, 10);  RP_rev(s);
    rp(s, 11);  RP_rev(s);
    rp(s, 12);  RP_fwd(s);
    rp(s, 13);  RP_fwd(s);
    rp(s, 14);  RP_fwd(s);
}

/*
 * Executes the 20-round permutation schedule used by CHIME-1024.
 * Inputs:
 *     s: state to permute
 * Outputs:
 *     s: updated state after 20 rounds
 *
 * 执行 CHIME-1024 使用的 20 轮置换调度。
 * 输入：
 *     s：待置换状态
 * 输出：
 *     s：20 轮后的更新状态
 */
void P_20(S &s) {
    rp(s, 0);   RP_fwd(s);
    rp(s, 1);   RP_fwd(s);
    rp(s, 2);   RP_fwd(s);
    rp(s, 3);   RP_rev(s);
    rp(s, 4);   RP_rev(s);
    rp(s, 5);   RP_rev(s);
    rp(s, 6);   RP_fwd(s);
    rp(s, 7);   RP_fwd(s);
    rp(s, 8);   RP_fwd(s);
    rp(s, 9);   RP_rev(s);
    rp(s, 10);  RP_rev(s);
    rp(s, 11);  RP_rev(s);
    rp(s, 12);  RP_fwd(s);
    rp(s, 13);  RP_fwd(s);
    rp(s, 14);  RP_fwd(s);
    rp(s, 15);  RP_rev(s);
    rp(s, 16);  RP_rev(s);
    rp(s, 17);  RP_rev(s);
    rp(s, 18);  RP_fwd(s);
    rp(s, 19);  RP_fwd(s);
}